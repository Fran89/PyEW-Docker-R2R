/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2pktman.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.4  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.3  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.2  2000/05/09 23:59:17  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:48:29  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2pktman.c: K2 packet manangement and Serial Data Stream (SDS) functions */
/*                                                                           */
/*    1/6/99 -- [ET]  File started                                           */
/*                                                                           */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>       /* Earthworm include file (for "logit()" fn) */
#include "glbvars.h"         /* K2EW global variables */
#include "k2misc.h"
#include "k2pktdef.h"        /* K2 packet definitions and types */
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */
#include "k2pktio.h"         /* K2 packet I/O functions */
#include "byteswap.h"        /* integer byte-swap macros */
#include "k2crc.h"           /* K2 CRC calculation macro */
#include "k2pktman.h"        /* header file for this module */

#define K2PM_WAITFOR_MS   5000         /* response data timeout (ms) */
#define K2PM_RACK_TOUTMS  5000         /* receive ACK packet timeout */
#define K2PM_RSDS_TOUTMS  5000         /* receive SDS packets timeout */
#define K2PM_SHORT_FLUSH  250
#define K2PM_WAITPKT_LIM  100          /* limit for 'k2pm_recv_waitpkt()' */
#define K2PM_PKT_SRCNUM   42           /* arbitrary packet source # for file */
#define K2PM_STATUS_BITS ((unsigned char)0x07)  
                                       /* status byte bits for 'data type' */
#define K2PM_STATUS_COMP ((unsigned char)0x01)
                                       /* value for difference-compressed */

static struct PACKET_HDR g_k2pm_hdrblk;          /* received header buffer */
static unsigned char g_k2pm_buff[PACKET_MAX_SIZE-9]; /* received data buffer */

#if K2PM_ERRDEBUG_FLG               /* if error simulating debug enabled */
int gdebug_stmcrc_errflg = 0;       /* set TRUE to simulate CRC error */
#endif


/**************************************************************************
 * k2pm_stop_streaming:  sends command to stop Serial Data Stream (SDS)   *
 *      packets (and possibly sends command to stop acquisition)          *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *         redo     - number of times to retry socket connection          *
 *                    after a timeout                                     *
 *      return value:  returns K2R_POSITIVE on stopping streaming or if   *
 *                       streaming was already stopped.                   *
 *                     K2R_NO_ERROR if no fatal errors occured (command   *
 *                       may not have been carried out by K2; can't tell) *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2pm_stop_streaming(unsigned char *pseqnum, int redo)
{
  int rc;

  /* send stop-streaming command */
  if ((rc = k2pm_send_strctrl(K2SCC_STOP_STREAM,pseqnum,redo)) != K2R_POSITIVE)
    return rc;          /* command failed; give up */
  
#if K2PM_STMACQOFF_FLG       /* having ACQ off can make K2 start streaming */
  /*  without being asked; so don't */
  /* stop-streaming command sent OK; send acquisition-stop command */
  if ( (rc = k2p_send_packet((unsigned char)PKC_ACQSTOP, *pseqnum,
                             K2PM_PKT_SRCNUM, (unsigned short)0,NULL,redo)) 
       == K2ERR_NO_ERROR)
  {    /* send OK; wait for expected 'PKR_ACK' packet (ignore msg packets) */
    rc = k2pm_recv_waitpkt((unsigned char)PKR_ACK, *pseqnum, K2PM_PKT_SRCNUM,
                           &g_k2pm_hdrblk, g_k2pm_buff,
                           (unsigned char)PKC_MSG, gcfg_commtimeout_itvl, 
                           redo);
  }
  ++(*pseqnum);       /* increment sequence number */
#endif  

/* try to flush any trailing stream bytes */
  if (k2c_flush_recv(K2PM_SHORT_FLUSH) != K2R_NO_ERROR)
    return K2R_ERROR;
  
  return rc;                 /* return code */
}


/**************************************************************************
 * k2pm_start_streaming:  sends command to reset Serial Data Stream       *
 *      (SDS), sends command to start acquisition and then sends          *
 *      command to start Serial Data Stream (SDS) packets                 *
 *         *pseqnum - address of sequence number for sent command packet  *
 *                    which will be incremented                           *
 *         redo     - number of times to retry socket connection          *
 *                    after a timeout                                     *
 *      return value:  returns K2R_POSITIVE if streaming started          *
 *                     K2R_NO_ERROR if no fatal errors occured (command   *
 *                       may not have been carried out by K2; can't tell) *
 *                     K2R_ERROR on fatal errors; K2R_TIMEOUT on timeout  *
 **************************************************************************/

int k2pm_start_streaming(unsigned char *pseqnum, int redo)
{
  int rc;

  /* send stop-acquisition before start-acquisition command */
  if ( (rc = k2p_send_packet((unsigned char)PKC_ACQSTOP, *pseqnum,
                             K2PM_PKT_SRCNUM, (unsigned short)0,
                             NULL,redo)) == K2R_NO_ERROR)
  {      /* send OK; wait for expected 'PKR_ACK' packet (ignore msg packets) */
    rc = k2pm_recv_waitpkt((unsigned char)PKR_ACK, *pseqnum, K2PM_PKT_SRCNUM,
                           &g_k2pm_hdrblk, g_k2pm_buff,
                           (unsigned char)PKC_MSG, gcfg_commtimeout_itvl, 
                           redo);
    /* check for error codes not caused by "random" data coming in */
  }
  ++(*pseqnum);       /* increment sequence number */

  /* if previous command OK then send stream-reset command */
  if (rc == K2R_POSITIVE &&
      (rc = k2pm_send_strctrl(K2SCC_RESET_SEQNUM, pseqnum, 
                              redo)) == K2R_POSITIVE)
  {      /* stream-reset command sent OK; send start-acqusition command */
    if ( (rc = k2p_send_packet((unsigned char)PKC_ACQSTART, *pseqnum, 
                               K2PM_PKT_SRCNUM, (unsigned short)0,
                               NULL,redo)) == K2R_NO_ERROR)
    {    /* send OK; wait for expected 'PKR_ACK' packet (ignore msg packets) */
      rc = k2pm_recv_waitpkt((unsigned char)PKR_ACK, *pseqnum, K2PM_PKT_SRCNUM,
                             &g_k2pm_hdrblk, g_k2pm_buff,
                             (unsigned char)PKC_MSG, gcfg_commtimeout_itvl, 
                             redo);
    }
    ++(*pseqnum);       /* increment sequence number */
    if (rc == K2R_POSITIVE)
    {    /* start-acquisition command sent OK; send start-streaming command */
      rc = k2pm_send_strctrl(K2SCC_START_STREAM, pseqnum, redo);
    }
  }
  return rc;            /* return code */
}


/**************************************************************************
 * k2pm_send_strctrl:  sends stream control command packet and receives   *
 *      and interprets response                                           *
 *         cmd       - stream control command byte (one of 'K2SCC_...'    *
 *                     defines)                                           *
 *         *pseqnum  - address of sequence number for sent command packet *
 *                     which will be incremented                          *
 *         redo      - number of times to retry socket connection         *
 *                     after a timeout                                    *
 *      return value:  returns K2R_POSITIVE if specified packet recv'd;   *
 *                     K2R_NO_ERROR if something recv'd but not specified *
 *                       packet;                                          *
 *                     K2R_ERROR on error, or K2R_TIMEOUT on timeout      *
 **************************************************************************/

int k2pm_send_strctrl(unsigned char cmd, unsigned char *pseqnum, int redo)
{
  int rc;

  /* check receive and flush if any data waiting */
  if ( (rc = k2c_flush_recv(0)) != K2R_NO_ERROR)
    return rc;

  /* send stream control command packet */
  if ( (rc = k2p_send_packet((unsigned char)PKC_STRCTRL, *pseqnum,
                             (unsigned short)K2PM_PKT_SRCNUM, 
                             1, &cmd,redo)) == K2R_NO_ERROR)
  { /* send OK; wait for expected 'PKR_ACK' packet (don't ignore packets) */

    rc = k2pm_recv_waitpkt((unsigned char)PKR_ACK, *pseqnum, 
                           (unsigned char)K2PM_PKT_SRCNUM, &g_k2pm_hdrblk, 
                           g_k2pm_buff, K2PM_IGNR_NONE, gcfg_commtimeout_itvl, 
                           redo);
  }
  ++(*pseqnum);         /* increment sequence number */
  return rc;            /* return code */
}


/**************************************************************************
 * k2pm_req_stmblk:  sends a command packet to request the retransmission *
 *      of the specified serial stream data packet                        *
 *         stmnum  - stream number of requested data packet               *
 *         dataseq - data sequence number of requested data packet        *
 *         redo     - number of times to retry the socket connection      *
 *                    after a timeout.                                    *
 *      return value: returns K2R_NO_ERROR on success;                    *
 *                   K2R_ERROR on error, or K2R_TIMEOUT on timeout        *
 **************************************************************************/

int k2pm_req_stmblk(unsigned char stmnum, uint32_t dataseq, int redo)
{
  static struct StrSendCmd cmdblk;

  cmdblk.stream = stmnum;              /* enter stream number */
  /* enter data sequence number */
  cmdblk.DataSeq = (uint32_t)BYTESWAP_UINT32(dataseq);
  cmdblk.cmd = (unsigned char)1;             /* set retransmit flag value */

  /* transmit stream send command packet; return code */
  return k2p_send_packet((unsigned char)PKC_STRSEND, (unsigned char)0,
                         (unsigned short)K2PM_PKT_SRCNUM,
                         sizeof(cmdblk), (unsigned char *)&cmdblk,redo);
}


/**************************************************************************
 * k2pm_get_stmblk:   receives next serial stream data packet and         *
 *      translates it into an array of 32-bit signed integers             *
 *         pdatahdr   - address of header structure to be filled          *
 *         pdatabuff  - address of 32-bit signed integer array to be      *
 *                      filled; should contain at least                   *
 *                      'K2PM_MIN_DBUFSIZ' (1485) elements                *
 *         prdatacnt  - pointer to a variable to receive the number of    *
 *                      entries filled into 'pdatabuff[]' (if not NULL)   *
 *         redo        - number of times to retry socket connection       *
 *                       after a timeout                                  *
 *      return value:  returns K2R_POSITIVE if specified packet recv'd;   *
 *                     K2R_NO_ERROR if something recv'd but not specified *
 *                       packet;                                          *
 *                     K2R_ERROR on error, or K2R_TIMEOUT on timeout      *
 **************************************************************************/

int k2pm_get_stmblk(struct StrDataHdr *pdatahdr, int32_t *pdatabuff,
                    int *prdatacnt, int redo)
{
  int rc;

#ifdef DEBUG
  if (pdatahdr == NULL || pdatabuff == NULL)
  {
    logit("et", "k2pm_get_stmblk: invalid parameters: pdatahdr (%p) pdatabuff (%p)\n",
          pdatahdr, pdatabuff);
    return K2R_ERROR;       /* if NULL ptr then return error code */
  }
#endif

  /* wait for and receive next SDS packet */
  if((rc=k2pm_recv_wstmpkt(&g_k2pm_hdrblk,g_k2pm_buff,
                               gcfg_commtimeout_itvl,redo)) != K2R_POSITIVE)
  {      /* error receiving packet */
    if(rc == K2R_PAYLOAD_ERR)
    {    /* payload error detected (packet header OK) */
      if(g_k2pm_hdrblk.dataLength >= STRBLKHDR_SIZE)
      {  /* received enough data for stream block header in payload */
        memcpy(pdatahdr,g_k2pm_buff,STRBLKHDR_SIZE);
              /* byte-swap 16-bit serial number value */
        pdatahdr->SerialNum = BYTESWAP_UINT16(pdatahdr->SerialNum);
              /* byte-swap 32-bit data sequence number */
        pdatahdr->DataSeq = BYTESWAP_UINT32(pdatahdr->DataSeq);
              /* byte-swap 32-bit time value (# of seconds since 1/1/1980) */
        pdatahdr->Seconds = BYTESWAP_UINT32(pdatahdr->Seconds);
              /* byte-swap 16-bit milliseconds value (0-999) */
        pdatahdr->Msecs = BYTESWAP_UINT16(pdatahdr->Msecs);
        if(gcfg_debug > 0)
        {    /* debug enabled; log message */
          logit("et", "k2pm_get_stmblk: packet payload error, stm#=%d, "
                   "seq#=%lu",(int)(pdatahdr->StreamNum),pdatahdr->DataSeq);
          if(g_k2pm_hdrblk.typeCode == PKR_STRRQDATA)
            logit("e", "; [re-requested packet]");
        }
        logit("e", "\n");
      }
      else
      {  /* not enough data for stream block header */
        memset(pdatahdr,0,sizeof(struct StrDataHdr));     /* clear header data */
        if(gcfg_debug > 0)
        {    /* debug enabled; log message */
          logit("et", "k2pm_get_stmblk: packet payload error, too few"
                  " bytes received (%d)\n",(int)(g_k2pm_hdrblk.dataLength));
        }
      }
    }
    return rc;          /* if error then return code */
  }

    /* packet received OK */
    /* convert received packet data to integer array; return code */
  return k2pm_proc_streamdata(g_k2pm_hdrblk.dataLength,g_k2pm_buff,
                                              pdatahdr,pdatabuff,prdatacnt);
}


/**************************************************************************
 * k2pm_recv_waitpkt:  receives packets from K2; waits for specified      *
 *      packet; will receive up to 'K2PM_WAITPKT_LIM' # of packets        *
 *      before returning with 'K2ERR_PKTNOT_FOUND' error code             *
 *         typecode    - type-code of packet to wait for                  *
 *         seqnum      - sequence number of packet to wait for            *
 *         srcnum      - source number of packet to wait for              *
 *         phdrblk     - address of header block which, on exit, will     *
 *                       contain the received header data                 *
 *         databuff    - address of buffer to receive packet data bytes;  *
 *                       should be at least 'PACKET_MAX_SIZE-11' bytes    *
 *         ignore_code - 'typeCode' value of messages that are to be      *
 *                       ignored (not logged) while waiting (use          *
 *                       'K2PM_IGNR_NONE' value to ignore none)           *
 *         toutms      - receive timeout value in milliseconds            *
 *         redo        - number of times to retry socket connection       *
 *                       after a timeout                                  *
 *      return value:  returns K2R_POSITIVE if specified packet recv'd;   *
 *                     K2R_NO_ERROR if something recv'd but not specified *
 *                       packet;                                          *
 *                     K2R_ERROR on error, or K2R_TIMEOUT on timeout      *
 **************************************************************************/

int k2pm_recv_waitpkt(unsigned char typecode, unsigned char seqnum,
                      unsigned short srcnum, struct PACKET_HDR *phdrblk,
                      unsigned char *databuff, unsigned char ignore_code, 
                      int toutms, int redo)

{
  int rc, pktcount, already;

#ifdef DEBUG               /* if debugging enabled */
  if(phdrblk == NULL || databuff == NULL)   /* if either pointer NULL then */
  {
    logit("et", "k2pm_recv_waitpkt: invalid parameters phdrblk (%p) databuff (%p)\n",
          phdrblk, databuff);
    return K2R_ERROR;                 /* return error code */
  }
#endif
  
  pktcount = K2PM_WAITPKT_LIM;         /* initialize packet count */
  do          /* loop while waiting for packet with desired type-code */
  {                     /* attempt to receive a packet */
    if ( (rc = k2p_recv_packet(phdrblk,databuff,toutms, redo)) == K2R_POSITIVE)
    {         /* packet received OK */
      if (gcfg_debug > 1)
        logit("e", "k2pm_recv_waitpkt: received packet type <%X> len %d\n", 
              (int)phdrblk->typeCode, (int)phdrblk->dataLength);
      if (phdrblk->seqNo == seqnum && phdrblk->destination == srcnum)
      {       /* sequence and source numbers match */
        if (phdrblk->typeCode == typecode)   /* if type-code matches then */
        {
          if (gcfg_debug > 1)
            logit("e", "k2pm_recv_waitpkt: packet is as requested\n");
          return K2R_POSITIVE;            /* return OK code */
        }
        /* if 'NAK' response then interpret and return code */
        if (phdrblk->typeCode == (unsigned char)PKR_STRNAK)
        {
          logit("et", "K2 STR_NAK rcvd: %s\n", 
                k2pm_strnak_errcd(phdrblk, databuff, &already));
          if (already == 1)
            return K2R_POSITIVE;
          else
            return K2R_NO_ERROR;
        }
      }

      /* check if ignoring packets & if packet type matches */
      if (ignore_code == K2PM_IGNR_NONE || phdrblk->typeCode != ignore_code)
        k2mi_log_pktinfo(phdrblk, databuff); /* not ignored; log packet info */
    }
    else
    {         /* problem receiving packet */
      if (gcfg_debug > 1)
        logit("e", "k2pm_recv_waitpkt: recv_packet returned %d\n", rc);
      if (rc == K2R_ERROR)
        return rc;      /* if one of above errors then return code */
      if (rc == K2R_TIMEOUT )
      {
	if (pktcount < K2PM_WAITPKT_LIM)
	  return K2R_NO_ERROR; /* Some packets received, but not what we wanted */
	else
	  return K2R_TIMEOUT;
      }
    }
  }
  while(--pktcount > 0);          /* loop if packet count limit not reached */
  return K2R_NO_ERROR;    /* Some packets received, but not what we wanted */

}

/**************************************************************************
 * k2pm_recv_wstmpkt:  waits for and receives the next serial data        *
 *      stream (SDS) packet from the K2; will receive up to               *
 *      'K2PM_WAITPKT_LIM' # of non-SDS packets before returning          *
 *      with 'K2ERR_PKTNOT_FOUND' error code                              *
 *         phdrblk  - address of header block which, on exit, will        *
 *                    contain the received header data                    *
 *         databuff - address of buffer to receive packet data bytes;     *
 *                    should be at least 'PACKET_MAX_SIZE-11' bytes       *
 *         toutms   - receive timeout value in milliseconds               *
 *         redo        - number of times to retry socket connection       *
 *                       after a timeout                                  *
 *      return value:  returns K2R_POSITIVE if specified packet recv'd;   *
 *                     K2R_NO_ERROR if something recv'd but not specified *
 *                       packet;                                          *
 *                     K2R_ERROR on error, or K2R_TIMEOUT on timeout      *
 **************************************************************************/

int k2pm_recv_wstmpkt(struct PACKET_HDR *phdrblk, unsigned char *databuff,
                      int toutms, int redo)

{
  int rc, pktcount;

#ifdef DEBUG
  if (phdrblk == NULL || databuff == NULL)   /* if either pointer NULL then */
  {
    logit("et", "k2pm_recv_wstmpkt: invalid parameters phdrblk (%p) databuff (%p)\n",
          phdrblk, databuff);
    return K2R_ERROR;                 /* return error code */
  }
#endif

  pktcount = K2PM_WAITPKT_LIM;         /* initialize packet count */
  do          /* loop while waiting for packet with desired type-code */
  {                     /* attempt to receive a packet */
    rc = k2p_recv_packet(phdrblk,databuff,toutms, redo);
    if (gcfg_debug > 3)
      logit("e", "k2pm_recv_wstmpkt: recv_packet ret %d\n", rc);
    if ( rc == K2R_POSITIVE)
    {         /* packet received OK */
      if(phdrblk->typeCode == (unsigned char)PKR_STRDATA ||
         phdrblk->typeCode == (unsigned char)PKR_STRRQDATA)
        return K2R_POSITIVE;         /* if SDS packet then return OK code */
      /* Some other kind of packet; log it */
      k2mi_log_pktinfo(phdrblk, databuff);        /* log packet information */
    }
    else
    {         /* problem receiving packet */
      if(rc == K2R_ERROR || rc == K2R_TIMEOUT || (rc == K2R_PAYLOAD_ERR &&
                         (phdrblk->typeCode == (unsigned char)PKR_STRDATA ||
                        phdrblk->typeCode == (unsigned char)PKR_STRRQDATA)))
      {  /* fatal error or payload error on stream data packet */
        return rc;      /* return code */
      }
    }
  }
  while(--pktcount > 0);          /* loop if packet count limit not reached */
  return K2R_NO_ERROR;      /* return not-found error code */
}


/**************************************************************************
 * k2pm_strnak_errcd:  analyzes given PKR_STRNAK packet and returns       *
 *      corresponding error code                                          *
 *         phdrblk  - pointer to header block for PKR_STRNAK packet       *
 *         databuff - addres of data buffer for PKR_STRNAK packet         *
 *         already  - pointer (return value): if not NULL and the STRNAK  *
 *                    is that command is already in effect, set to 1      *
 *      return value:  returns error text for STRNAK packet               *
 **************************************************************************/

char * k2pm_strnak_errcd(struct PACKET_HDR *phdrblk,
                                const unsigned char *databuff, int *already)
{
  if (already != NULL) *already = 0;
  
  if(phdrblk->dataLength >= (unsigned short)2)
  {      /* packet data length OK */
    switch(databuff[1])
    {    /* process error code in STRNAK packet data */
    case (unsigned char)0:     
      return "command rejected, stream command queue full";
    case (unsigned char)1:     
      if (already != NULL) *already = 1;
      return "CTRL command error, already started or stopped";
    case (unsigned char)2:     
      return "block request error - streaming is disabled";
    case (unsigned char)3:     
      return "block request error - time limit expired";
    }
  }
  return "bad packet data length or unknown code in STRNAK packet data";
}

/**************************************************************************
 * k2pm_proc_streamdata:  processes received packet data as a Serial      *
 *      Data Stream (SDS) block of data; computes and checks CRC;         *
 *      uncompresses difference compression (if necessary) and returns    *
 *      the stream data as an array of signed 32-bit signed integers      *
 *         pktdatalen - length of data in received packet                 *
 *         pktbuff    - address of buffer containing the received packet  *
 *                      data bytes                                        *
 *         pdatahdr   - address of header structure to be filled          *
 *         pdatabuff  - address of 32-bit signed integer array to be      *
 *                      filled; should contain at least 1485 elements     *
 *         prdatacnt  - pointer to a variable to receive the number of    *
 *                      entries filled into 'pdatabuff[]' (if not NULL)   *
 *      return value: returns K2R_POSITIVE when stream data successfully  *
 *                      decoded;                                          *
 *                    K2R_NO_ERROR when there were errors decoding stream *
 *                      data; these might seem like funny return values,  *
 *                      but they are consistent with the logic of         *
 *                      k2pm_get_stmblk().                                *
 **************************************************************************/

int k2pm_proc_streamdata(int pktdatalen, const unsigned char *pktbuff,
                         struct StrDataHdr *pdatahdr, int32_t *pdatabuff,
                         int *prdatacnt)
{
  unsigned short crcval;
  uint32_t crbuff;
  int pktpos,dpos;
  int32_t lastval;
  unsigned char *btptr, btarr[4];

#ifdef DEBUG
  if(pktdatalen < (STRBLKHDR_SIZE+4) ||
     pktdatalen > (PACKET_MAX_SIZE-11) || pktbuff == NULL ||
     pdatahdr == NULL || pdatabuff == NULL)
  {
    logit("et", "k2pm_proc_streamdata: invalid parameters pktdatalen (%d) "
          "pktbuff (%p) pdatahdr (%p) pdatabuff (%p)\n",
          pktdatalen, pktbuff, pdatahdr, pdatabuff);
    return K2R_ERROR;                 /* return error code */
  }
#endif

  /* copy data into stream data header block */
  crcval = (unsigned short)0;
  pktpos = 0;
  btptr = (unsigned char *)pdatahdr;
  do          /* loop through each byte in destination header block */
  {
    *btptr = pktbuff[pktpos];     /* copy over packet data byte */
    K2_CRC(crcval, *btptr);       /* enter packet data byte into CRC */
    ++btptr;                      /* increment destination position */
  }
  while(++pktpos < STRBLKHDR_SIZE); /* inc source; loop thru header */

  /* byte-swap 16-bit serial number value */
  pdatahdr->SerialNum = BYTESWAP_UINT16(pdatahdr->SerialNum);
  /* byte-swap 32-bit data sequence number */
  pdatahdr->DataSeq = BYTESWAP_UINT32(pdatahdr->DataSeq);
  /* byte-swap 32-bit time value (# of seconds since 1/1/1980) */
  pdatahdr->Seconds = BYTESWAP_UINT32(pdatahdr->Seconds);
  /* byte-swap 16-bit milliseconds value (0-999) */
  pdatahdr->Msecs = BYTESWAP_UINT16(pdatahdr->Msecs);

  /* process and store serial data stream values */
  pktdatalen -= 4;        /* subtract 4 from data len for CRC bytes */
  dpos = 0;          /* initialize position in 'pdatabuff[]' */
  if ( ((unsigned char)(pdatahdr->Status) & K2PM_STATUS_BITS) 
       != K2PM_STATUS_COMP)
  {      /* difference compression not used */
    while (pktpos < pktdatalen)
    {    /* loop through each serial data stream sample data value */
      btarr[1] = pktbuff[pktpos++];    /* copy MSByte of 24-bit value */
      K2_CRC(crcval,btarr[1]);         /* enter byte into CRC calculation */
      btarr[2] = pktbuff[pktpos++];    /* copy middle-byte of 24-bit value */
      K2_CRC(crcval,btarr[2]);         /* enter byte into CRC calculation */
      btarr[3] = pktbuff[pktpos++];    /* copy LSByte of 24-bit value */
      K2_CRC(crcval,btarr[3]);         /* enter byte into CRC calculation */
      /* sign extend to 32-bits */
      btarr[0] = ((btarr[1] & (unsigned char)0x80) == 
                  (unsigned char)0) ? (unsigned char)0 : (unsigned char)0xFF;
      /* byte-swap and enter value */
      pdatabuff[dpos++] = (int32_t)BYTESWAP_UINT32(*((int32_t *)(btarr)));
    }
  }
  else
  {      /* difference compression used */
    lastval = 0L;
    while (pktpos < pktdatalen)
    {    /* loop through each serial data stream sample data value */
      if ( (pktbuff[pktpos] & (unsigned char)0x80) != (unsigned char)0)
      {       /* data is 3-byte sample value */
        btarr[1] = pktbuff[pktpos++];  /* copy MSByte of 24-bit value */
        K2_CRC(crcval,btarr[1]);       /* enter byte into CRC calculation */
        btarr[2] = pktbuff[pktpos++];  /* copy middle-byte of 24-bit value */
        K2_CRC(crcval,btarr[2]);       /* enter byte into CRC calculation */
        btarr[3] = pktbuff[pktpos++];  /* copy LSByte of 24-bit value */
        K2_CRC(crcval,btarr[3]);       /* enter byte into CRC calculation */
        if ((btarr[1]&(unsigned char)0x40) == (unsigned char)0)
        {          /* shifted sign bit for value is not set */
          btarr[0] = (unsigned char)0;         /* set 4th byte to zero */
          btarr[1] &= (unsigned char)0x7F;     /* clear high-bit of 3rd byte */
        }
        else       /* shifted sign bit for value is set */
          btarr[0] = (unsigned char)0xFF;  /* sign extend ones through 4th byte */
        /*  (high-bit of 3rd byte already set) */
        /* byte-swap, shift value into place and enter & save it */
        pdatabuff[dpos++] = lastval =
          (int32_t)(BYTESWAP_UINT32(*((int32_t *)btarr)) << (unsigned char)1);
      }
      else
      {       /* data is 2-byte difference value */
        btarr[0] = pktbuff[pktpos++];  /* copy MSByte of 15-bit diff value */
        K2_CRC(crcval,btarr[0]);       /* enter byte into CRC calculation */
        btarr[1] = pktbuff[pktpos++];  /* copy LSByte of 15-bit diff value */
        K2_CRC(crcval,btarr[1]);       /* enter byte into CRC calculation */
        /* (don't need to do the sign-extension to 16-bits below */
        /*   because the value is shifted 1 left in the next statement) */
        /* if((btarr[0]&(unsigned char)0x40) != (unsigned char)0)*/  /*if 15-bit sign-bit set */
        /*    btarr[0] |= (unsigned char)0x80; */        /* then set 16-bit sign bit */
        /* byte-swap difference and add to last value; enter new value */
        /*  (difference needs to be shifted 1 left before added in) */
        pdatabuff[dpos++] = lastval +=
          (short)(BYTESWAP_UINT16(*((short *)btarr)) << (unsigned char)1);
      }
    }
  }
  
  if (pktpos != pktdatalen)        /* if not positioned at CRC location then */
  {
    logit("et", "k2pm_proc_streamdata: Stream data size mismatch\n");
    return K2R_NO_ERROR;       /* return error code */
  }
  
#if K2PM_ERRDEBUG_FLG        /* if error simulating debug enabled */
  if (gdebug_stmcrc_errflg != FALSE)
  {      /* simulate CRC error flag is set */
    ++crcval;                          /* increment CRC value for error */
    gdebug_stmcrc_errflg = FALSE;      /* clear flag */
  }
#endif

  memcpy(&crbuff, &pktbuff[pktpos], sizeof(int32_t));
  if ((uint32_t)crcval != BYTESWAP_UINT32(crbuff))
  {
    logit("et", "k2pm_proc_streamdata: stream CRC match failed\n");
    return K2R_NO_ERROR;         /* if CRC mismatch then return code */
  }
  
  if (prdatacnt != NULL)     /* if pointer not NULL then */
    *prdatacnt = dpos;       /* enter # of values loaded into 'pdatabuff[]' */
  return K2R_POSITIVE;          /* return OK code */
}


#if K2PM_ERRDEBUG_FLG        /* if error simulating debug enabled */

/**************************************************************************
 * k2pmdebug_sim_crcerr:  debug function with simulates CRC error in      *
 *      next SDS packet received                                          *
 **************************************************************************/

void k2pmdebug_sim_crcerr()
{
  gdebug_stmcrc_errflg = TRUE;         /* set simulate-CRC-error flag */
}

#endif
