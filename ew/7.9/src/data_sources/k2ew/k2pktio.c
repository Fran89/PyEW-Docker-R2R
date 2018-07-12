/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2pktio.c 6086 2014-05-26 00:46:54Z baker $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2007/05/10 00:20:14  dietz
 *     included <string.h> to fix missing prototypes
 *
 *     Revision 1.8  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.6  2001/05/15 22:46:43  kohler
 *     Minor logging change.
 *
 *     Revision 1.5  2001/05/14 18:10:43  kohler
 *     Minor logging changes.
 *
 *     Revision 1.4  2001/05/08 00:13:37  kohler
 *     Minor logging changes.
 *
 *     Revision 1.3  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *     Revision 1.2  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.1  2000/05/04 23:48:26  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2pktio.c:  K2 packet input/output functions; uses 'k2comif.c' fns */
/*  */
/*  12/30/98 -- [ET]  File started */
/*  */

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include "glbvars.h"
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */
#include "k2comif.h"         /* K2 communication interface routines */
#include "byteswap.h"        /* integer byte-swap macros */
#include "k2pktio.h"         /* header file for this module */

#define K2P_TRAN_TOUTMS 500  /* transmit timeout in milliseconds */
#define K2P_SENT_DSTVAL ((unsigned short)0)  /* val sent in 'destination' hdr field */


/**************************************************************************
 * k2p_send_packet:  sends packet from PC to K2                           *
 *         typecd   - packet type code value ('PKC_...'/'PKR_...' value)  *
 *         seqnum   - sequence number                                     *
 *         srcnum   - source number                                       *
 *         datalen  - number of data bytes in 'databuf[]' to send         *
 *         databuff - address of buffer containing packet data bytes      *
 *         redo     - number of times to retry the socket connection      *
 *                    after a timeout.                                    *
 *      return value: returns K2R_NO_ERROR on sucessful transmission;     *
 *                    K2R_ERROR on error or                               *
 *                    K2R_TIMEOUT on timeout while sending                *
 **************************************************************************/

int k2p_send_packet(unsigned char typecd, unsigned char seqnum,
                    unsigned short srcnum, int datalen,
                    const unsigned char *databuff, int redo)
{
  int p, cnt;
  unsigned short chksum;
  unsigned char bt;
  static struct PACKET_HDR hdrblk;
  static unsigned char msgbuff[PACKET_MAX_SIZE*2+5];

  if(datalen > (PACKET_MAX_SIZE-11))
  {
    logit("et", "k2p_send_packet: datalen (%d) too long for one packet\n",
          datalen);
    return K2R_ERROR;       /* if too large then return error code */
  }

  /* fill header block data */
  hdrblk.typeCode = typecd;                 /* packet type number */
  hdrblk.seqNo = seqnum;                    /* sequence number */
  hdrblk.source = BYTESWAP_UINT16(srcnum);  /* source number */
  hdrblk.destination = BYTESWAP_UINT16(K2P_SENT_DSTVAL);   /* destination # */
  hdrblk.dataLength = BYTESWAP_UINT16((unsigned short)datalen);
  /* load complete message data into 'msgbuff[]' */
  p = 0;           /* initialize position in buffer */
  msgbuff[p++] = (unsigned char)PKTFRAME;        /* load leading frame byte */
  /* translate header data into buffer */
  p += k2p_xlat_pckdata(&msgbuff[p], (unsigned char *)&hdrblk, sizeof(hdrblk),
                        &chksum);

  bt = (unsigned char)chksum;             /* convert checksum to byte-sized */
  p += k2p_xlat_pckdata(&msgbuff[p], &bt, 1, NULL); /* load header checksum byte */
  p += k2p_xlat_pckdata(&msgbuff[p], databuff, datalen, &chksum);
  chksum = BYTESWAP_UINT16(chksum);         /* byte-swap 16-bit checksum val */
  /* load data checksum value */
  p += k2p_xlat_pckdata(&msgbuff[p],(unsigned char *)&chksum, 2, NULL);
  msgbuff[p++] = (unsigned char)PKTFRAME;       /* load ending frame bytes */
  msgbuff[p++] = (unsigned char)PKTFRAME;
  /* transmit buffer; save transmit count */
  cnt = k2c_tran_buff(msgbuff, p, gcfg_commtimeout_itvl, redo);

  if (cnt > 0)
    return K2R_NO_ERROR;
  else
    return cnt;  /* return status code from k2c_tran_buff */
}


/**************************************************************************
 * k2p_xlat_pckdata:  translates binary data into K2 packet data by       *
 *      replacing all occurrances of PKTFRAME (0xC0) and PKTESC (0x5C)    *
 *      with the sequences PKTESC/END_EQUIVALENT (0x5C/0xDD) and          *
 *      PKTESC/ESC_EQUIVALENT (0x5C/0xDC) respectively; and calculates    *
 *      and returns checksum for data                                     *
 *         dstptr  - address of buffer to receive the translated data     *
 *         srcptr  - address of source buffer of binary data              *
 *         srclen  - number of bytes in 'srcptr[]' to translate           *
 *         pchksum - pointer to 16-bit checksum variable loaded with the  *
 *                   computed checksum of the binary data processed       *
 *                   (if pointer not NULL)                                *
 *      return value:  returns the number of bytes (after translation)    *
 *                     loaded into 'dstptr[]'                             *
 **************************************************************************/

int k2p_xlat_pckdata(unsigned char *dstptr, const unsigned char *srcptr,
                     int srclen, unsigned short *pchksum)
{
  int spos,dpos;
  unsigned short chksum;

  spos = dpos = 0;
  chksum = (unsigned short)0;

  if (srclen > 0 && srcptr != NULL && dstptr != NULL)
  {      /* source buffer length greater than zero and pointers not NULL */
    do
    {    /* loop through each byte in source buffer */
      if(srcptr[spos] == (unsigned char)PKTFRAME)
      {       /* byte has 'PKTFRAME' value; put in substitute sequence */
        dstptr[dpos++] = (unsigned char)PKTESC;
        dstptr[dpos++] = (unsigned char)END_EQUIVALENT;
      }
      else
      {
        if(srcptr[spos] == (unsigned char)PKTESC)
        {     /* byte has 'PKTESC' value; put in substitute sequence */
          dstptr[dpos++] = (unsigned char)PKTESC;
          dstptr[dpos++] = (unsigned char)ESC_EQUIVALENT;
        }
        else  /* byte value OK; copy over */
          dstptr[dpos++] = srcptr[spos];
      }
      chksum += srcptr[spos];     /* add byte to checksum */
    }
    while(++spos < srclen);       /* increment source pos; loop if more data */
  }
  if(pchksum != NULL)             /* if pointer not NULL then */
    *pchksum = chksum;            /* load checksum value */
  return dpos;                    /* return # of bytes processed */
}


/**************************************************************************
 * k2p_recv_packet:  receives one packet from K2                          *
 *         phdrblk  - pointer to header block to receive packet header    *
 *                    data                                                *
 *         databuff - address of buffer to receive packet data bytes or   *
 *                    NULL for no data bytes returned; if not NULL then   *
 *                    should be at least 'PACKET_MAX_SIZE-11' bytes long  *
 *         toutms   - receive timeout value in milliseconds               *
 *         redo     - number of times to retry the socket connection      *
 *                    after a timeout.                                    *
 *      return value: returns K2R_POSITIVE when a packet is received and  *
 *                      decoded successfully                              *
 *                    K2R_NO_ERROR when at least something was received   *
 *                       but its decoding could not be completed          *
 *                    K2R_ERROR when a fatal error occurred               *
 *                    K2R_TIMEOUT if no data returned before timeout      *
 **************************************************************************/

int k2p_recv_packet(struct PACKET_HDR *phdrblk, unsigned char *databuff,
                    int toutms, int redo)
{
  int rc = 0;
  int spos, dpos, msgcnt, totcount;
  unsigned short chksum, chbuff;
  int escflg, esc_errflg;
  static unsigned char msgbuff[PACKET_MAX_SIZE+6];

#ifdef DEBUG
  if(phdrblk == NULL)                  /* if NULL parameter then */
  {
    logit("et", "k2p_recv_packet: invalid NULL parameter phdrblk\n");
    return K2R_ERROR;            /* abort with error code */
  }
#endif

  totcount = 0;              /* initialize total byte count */
  do     /* loop while message data size too small for valid packet */
  {
    /* loop while looking for frame byte value */
    /* (skip if last byte from previous iteration was frame byte) */
    while(rc != PKTFRAME)
    {         /* get received data byte */
      if ( (rc = k2c_rcvbt_tout(toutms, redo)) < 0 )
      {       /* timeout or error; give up */
        if (rc == K2R_ERROR)
          return K2R_ERROR;
        else   /* timeout: return it if we have read no bytes so far */
          return ((totcount == 0) ? K2R_TIMEOUT : K2R_NO_ERROR);
      }
      /* Got a byte; tell heartbeat thread we're happy */
      g_mt_working = 1;

      if(rc > 255)
      {
        logit("et", "k2p_recv_packet: unexpected byte (%X) from rcvbt_tout\n",
              rc);
        return K2R_ERROR;          /* if other code then return error */
      }
      if(++totcount > (PACKET_MAX_SIZE*2))       /* inc total byte count */
      {
        logit("et", "k2p_recv_packet: too much non-packet data\n");
        /* if too many bytes then give up on this packet */
        return K2R_NO_ERROR;
      }
    }

    escflg = esc_errflg = 0;  /* initialize PKTESC-found and error flags */
    msgcnt = 0;         /* initialize position in 'msgbuff[]' */
    do        /* loop while loading possible packet data bytes */
    {
      if ( (rc = k2c_rcvbt_tout(toutms, redo)) < 0)
      {       /* timeout or error; give up */
        if (rc == K2R_ERROR)
          return K2R_ERROR;
        else   /* timeout */
          return K2R_TIMEOUT;
      }
      /* Got a byte; tell heartbeat thread we're happy */
      g_mt_working = 1;

      if(rc > 255)
      {
        logit("et", "k2p_recv_packet: unexpected byte (%X) from rcvbt_tout\n",
              rc);
        return K2R_ERROR;          /* if other code then return error */
      }

      if(rc == PKTFRAME)
        break;          /* if frame byte value found then exit loop */
      if(msgcnt <= (PACKET_MAX_SIZE+3))
      {       /* still enough room in buffer for another byte */
        if(escflg == 0)
        {     /* previous byte was not PKTESC; copy current byte into buffer; */
          /*  set flag if current byte has PKTESC value */
          escflg = ( (msgbuff[msgcnt] = (unsigned char)rc) ==
                     (unsigned char) PKTESC ) ? 1 : 0;
        }
        else       /* previous byte was PKTESC (0x5C) */
        {          /* replace PKTESC byte with proper value */
          if ( rc == END_EQUIVALENT)
            msgbuff[--msgcnt] = (unsigned char) PKTFRAME;       /* 0xC0 */
          else
          {
            if ( rc == ESC_EQUIVALENT)
              msgbuff[--msgcnt] = (unsigned char) PKTESC;       /* 0x5C */
            else
            {      /* unknown byte value after PKTESC byte */
              msgbuff[msgcnt] = (unsigned char) rc;   /* put in byte value */
              esc_errflg = 1;            /* set error flag */
            }
          }
          escflg = 0;              /* clear PKTESC-found flag */
        }
      }
      else         /* if message size too large for packet then */
        break;     /* exip loop */
      ++msgcnt;              /* increment buffer position */
    }
    while(rc != PKTFRAME);      /* loop until frame byte value found */
  }
  /* loop while size too small for packet */
  while ( (size_t) msgcnt < ( sizeof( *phdrblk ) + 3 ) );

  /* make sure 2nd frame byte is cleared */
  if ( (rc = k2c_rcvbt_tout(toutms, 0)) != PKTFRAME)
  {
    if (rc >= 0)  /* Something other than a frame byte */
      if ( gcfg_debug > 0 )
         logit("et", "k2p_recv_packet: abnormal 2nd frame byte: <%X>\n", rc);
    /* We'll ignore error or timeout; maybe they'll be fixed for next packet */
  }

  if (msgcnt > (PACKET_MAX_SIZE+3))     /* if message size too large then */
  {
    logit("et", "k2p_recv_packet: too large packet: %d; max is %d\n", msgcnt,
          PACKET_MAX_SIZE+3);
    return K2R_NO_ERROR;
  }

  /* attempt to interpret message data into packet */
  if (esc_errflg != 0)
  {
    if ( gcfg_debug > 0 )
       logit("et", "k2p_recv_packet: Received illegal code after escape code (0x5C)\n");
    return K2R_NO_ERROR;
  }

  chksum = (unsigned short) 0;        /* initialize checksum */
  spos = 0;                  /* initialize message buffer position */
  do          /* copy message bytes into packet header block */
  {           /* calculate checksum during copy */
    chksum += (((unsigned char *)phdrblk)[spos] = msgbuff[spos]);
  }
  /* loop until header filled */
  while ( (size_t) ++spos < sizeof( *phdrblk ) );

  /* byte-swap 16-bit data values in header */
  phdrblk->source = BYTESWAP_UINT16(phdrblk->source);
  phdrblk->destination = BYTESWAP_UINT16(phdrblk->destination);
  phdrblk->dataLength = BYTESWAP_UINT16(phdrblk->dataLength);

  /* compare header checksum byte */
  if(msgbuff[spos++] != (unsigned char)chksum)
  {
    if ( gcfg_debug > 0 )
       logit("et", "k2p_recv_packet: packet hdr checksum match failed\n");
    return K2R_NO_ERROR;
  }

  if(phdrblk->dataLength > PACKET_MAX_SIZE-11)
  {
    if ( gcfg_debug > 0 )
    {
      logit("et", "k2p_recv_packet: packet hdr datalength too large (%d)\n",
                                                (int)(phdrblk->dataLength));
    }
    return K2R_NO_ERROR;
  }

  if(msgcnt - spos - 2 < phdrblk->dataLength)
  {      /* not enough data in packet */
    if ( gcfg_debug > 0 )
    {
      logit("et", "k2p_recv_packet: packet data less than header datalen (%d)"
                " by %d bytes\n",(int)(phdrblk->dataLength),
                                     (phdrblk->dataLength-(msgcnt-spos-2)));
    }                                 
              /* enter actual length of data into header item */
    phdrblk->dataLength = (unsigned short)(
                            ((totcount=msgcnt-spos-2) >= 0) ? totcount : 0);
    return K2R_PAYLOAD_ERR;  /* indicate err in payload after pkt header */
  }

  if(msgcnt - spos - 2 > phdrblk->dataLength)    /* if too much data */
  {
    if ( gcfg_debug > 0 )
    {
      logit("et", "k2p_recv_packet: packet data more than header datalen"
                  " by %d bytes\n",(msgcnt-spos-2-phdrblk->dataLength));
    }
              /* enter actual length of data into header item */
    phdrblk->dataLength = (unsigned short)(
                  ((totcount=msgcnt-spos-2) <= 0xFFFF) ? totcount : 0xFFFF);
    return K2R_PAYLOAD_ERR;  /* indicate err in payload after pkt header */
  }

  /* packet data section */
  chksum = (unsigned short) 0;
  if(databuff != NULL)
  {      /* not NULL 'databuff' pointer; load buffer with packet data */
    for (dpos = 0; dpos < (phdrblk->dataLength); ++dpos)
    {    /* load packet data bytes into 'databuff[]'; calculate checksum */
      chksum += (databuff[dpos] = msgbuff[spos++]);
    }
  }
  else
  {      /* NULL 'databuff' pointer; just calculate checksum */
    for (dpos = 0; dpos < (phdrblk->dataLength); ++dpos)
      chksum += msgbuff[spos++];
  }
  /* get 16-bit data checksum from message data and swap bytes */
  memcpy(&chbuff, &msgbuff[spos], 2);  /* Avoid possible alignment error */
  if (chksum != BYTESWAP_UINT16(chbuff))
  {
    if ( gcfg_debug > 0 )
       logit("et", "k2p_recv_packet: packet data checksum match failed\n");
    return K2R_PAYLOAD_ERR;  /* indicate err in payload after pkt header */
  }

  return K2R_POSITIVE;               /* return OK code */
}

