/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nmx_api.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2009/06/05 17:33:06  dietz
 *     Changed debug logging in packet-reading code (nmx_api.c) and bundle-reading
 *     code (nmxp_packet.c) from compile-time definition to run-time configurable
 *     option.
 *
 *     Revision 1.9  2008/11/07 23:27:38  dietz
 *     Changed logging of null bundle to debug-logging only
 *
 *     Revision 1.8  2003/10/07 17:59:53  dietz
 *     added ConnectReponse message
 *
 *     Revision 1.7  2003/09/12 22:55:07  dietz
 *     Turned off DEBUG compilation flag.
 *
 *     Revision 1.6  2003/02/11 01:07:00  dietz
 *     Added functions to read transparent serial packets
 *
 *     Revision 1.5  2002/11/06 19:32:31  dietz
 *     Added new field to NMX_CHANNEL_INFO struct
 *
 *     Revision 1.4  2002/11/04 18:32:47  dietz
 *     Added support for extended seismic data headers in the
 *     compressed data packet (needed for Trident data).
 *
 *     Revision 1.1  2002/10/15 23:52:58  dietz
 *     Initial revision
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:35:07  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 *  nmx_api.c
 *
 *  A library of functions to Interact with Nanometrics programs
 *  such as NaqsServer and DataServer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time_ew.h>
#include <socket_ew.h>
#include <errno.h>
#include "nmx_api.h"

#define NMXAPI_DEBUG 3

extern int Debug;     /* Debug logging flag (optionally configured) */
extern int errno;


void logit( char *, char *, ... );   /* logit.c sys-independent  */

/* Protoypes for internally-used functions 
 *****************************************/
void   nmx_loadhdr( NMXHDR *nhd, char *msg );
int    compare_NMX_CHANNEL_INFO( const void *s1, const void *s2 );
int    compare_NMX_PRECIS_INFO( const void *s1, const void *s2 );
double ntohd( double data );  /*network to host byte order for a double */

/*********************************************************************
 * nmx_checklen() checks the current length of a buffer against a    *
 *      required length and realloc's it if necessary                *
 *********************************************************************/
int nmx_checklen( char **buf, int *buflen, int reqlen )
{
   char *pchar;

   if( *buflen < reqlen )
   {
      if( (pchar = (char *)realloc( *buf, (size_t)reqlen )) == NULL )
      {
         logit("et","nmx_checklen: could not realloc buffer(%d) from %d "
                    "to %d bytes (errno:%d %s)\n",
                     *buf, *buflen, reqlen, errno, strerror(errno) );
         return( NMX_FAILURE );
      }
      logit("et","nmx_checklen: realloc'd from buffer(%d) %d bytes"
                 " to buffer(%d) %d bytes\n",
                  *buf, *buflen, pchar, reqlen );
     *buf    = pchar;
     *buflen = reqlen;
   }
   return( NMX_SUCCESS );
}

/*********************************************************************
 * nmx_loadhdr() converts a NMXHDR structure to a 12-byte string     *
 *      in network byte order to be shipped to over a socket         *
 *********************************************************************/
void nmx_loadhdr( NMXHDR *nhd, char *msg )
{
   char  *phd;  /* working pointer */
   int    tmp;

/* Put the header into network byte order
 ****************************************/
   phd = msg;
   tmp = (int) htonl( (uint32_t)nhd->signature );
   memcpy( phd, &tmp, sizeof(int) );
   phd += sizeof( int );

   tmp =  (int) htonl( (uint32_t)nhd->msgtype );
   memcpy( phd, &tmp, sizeof(int) );
   phd += sizeof( int );

   tmp =  (int) htonl( (uint32_t)nhd->msglen );
   memcpy( phd, &tmp, sizeof(int) );

   return;
}

/*********************************************************************
 * nmx_unloadhdr() converts a 12-byte header string in network byte  *
 *      order into a NMXHDR structure in local byte order            *
 *********************************************************************/
void nmx_unloadhdr( NMXHDR *nhd, char *chd )
{
   char         *phd;  /* working pointer */
   uint32_t     tmp;

/* Translate header from network -> host byte order
 **************************************************/
   phd = chd;
   memcpy( &tmp, phd, 4 );
   nhd->signature = (int) ntohl( tmp );
 /*logit("et", "nmx_unloadhdr signature:  raw: %0x  ntohl: %0x\n",
          tmp,  nhd->signature );*/ /*DEBUG*/

   phd += 4;
   memcpy( &tmp, phd, 4 );
   nhd->msgtype = (int) ntohl( tmp );

   phd += 4;
   memcpy( &tmp, phd, 4 );
   nhd->msglen = (int) ntohl( tmp );

 /*printf("nmx_unloadhdr: signature: 0x%0x  msgtype: %d  msglen: %d\n",
           nhd->signature, nhd->msgtype, nhd->msglen );*/ /*DEBUG*/
   return;
}


/*********************************************************************
 * nmx_bld_connect()  Build a connection message (NMXMSG_CONNECT)    *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send)          *
 *                                                                   *
 *   Returns NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_connect( char **buf, int *buflen, int *totlen )
{
   NMXHDR  nhd;
   char   *pbuf;  /* working pointer */

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   if( nmx_checklen( buf, buflen, NMXHDR_LEN ) != 0 ) return( NMX_FAILURE );

/* Build header & it into target address
 ***************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_CONNECT;
   nhd.msglen    = 0;

   nmx_loadhdr( &nhd, pbuf );
   *totlen = NMXHDR_LEN;

/* DEBUG
   {
   int *a;
   int *b;
   int *c;
   a = (int *) &pbuf[0];
   b = (int *) &pbuf[4];
   c = (int *) &pbuf[8];
   logit("e","nmx_bld_connect: 0-3: 0x%0x  4-7: %d  8-11: %d\n",
              *a, *b, *c );
   }
   END DEBUG */

   return( NMX_SUCCESS );
}

/*********************************************************************
 * nmx_bld_connectrequest()  Build a connect request message         *
 *                           (NMXMSG_CONNECTREQUEST)                 *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send)          *
 *   conreq  structure containing info necessary to build the msg    *
 *                                                                   *
 *   Returns NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_connectrequest( char **buf, int *buflen, int *totlen,
                            NMX_CONNECT_REQUEST *conreq )
{
   NMXHDR        nhd;
   int           msglen;  /* message content length */
   char         *pbuf;    /* working pointer */
   int32_t       tmp;

   *totlen = 0;
   msglen = 24;

/* Make sure the target address is big enough
 ********************************************/
   if( nmx_checklen( buf, buflen, NMXHDR_LEN+msglen ) != 0 ) {
       return( NMX_FAILURE );
   }

/* Build header & it into target address
 ***************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_CONNECT_REQUEST;
   nhd.msglen    = msglen;
   nmx_loadhdr( &nhd, pbuf );
   pbuf += NMXHDR_LEN;

/* Add the message contents in network byte order.
   Compute CRC along the way (NOTE: don't include any
   trailing null bytes of the user & password in the CRC!)     
 *********************************************************/
   crc32_init();
   memcpy( pbuf,  conreq->user, NMX_DS_LEN ); pbuf += NMX_DS_LEN;
   crc32_update( (unsigned char *)conreq->user, (uint32_t)strlen(conreq->user) );

   tmp = htonl( (uint32_t) conreq->DAPversion );
   memcpy( pbuf, &tmp, sizeof(int32_t) );     pbuf += sizeof(int32_t);
   crc32_update( (unsigned char *)&tmp, (uint32_t)sizeof(int32_t) );

   tmp = (int) htonl( (uint32_t)conreq->tconnect );
   memcpy( pbuf, &tmp, sizeof(int32_t) );     pbuf += sizeof(int32_t);
   crc32_update( (unsigned char *)&tmp, (uint32_t)sizeof(int32_t) );

/* Add the password to the CRC, but not to the msg.
   Then put the CRC in the msg in network byte order */
   crc32_update( (unsigned char *)conreq->pswd, (uint32_t)strlen(conreq->pswd) );

   tmp = (int) htonl( crc32_value() );
   memcpy( pbuf, &tmp, sizeof(int32_t) );

   *totlen = NMXHDR_LEN + msglen;

   return( NMX_SUCCESS );
}



/*********************************************************************
 * nmx_bld_request_pending()  Build a request pending message        *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send)          *
 *                                                                   *
 *   Returns  NMX_SUCCESS or NMX_FAILURE                             *
 *********************************************************************/
int nmx_bld_request_pending( char **buf, int *buflen, int *totlen )
{
   NMXHDR nhd;

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   if( nmx_checklen( buf, buflen, NMXHDR_LEN ) != 0 ) {
      return( NMX_FAILURE );
   }

/* Build header & it into target address
 ***************************************/
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_REQUEST_PENDING;
   nhd.msglen    = 0;

   nmx_loadhdr( &nhd, *buf );
   *totlen += NMXHDR_LEN;

   return( NMX_SUCCESS );
}


/*********************************************************************
 * nmx_bld_terminate()  Build a termination message                  *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send)          *
 *   term    ptr to NMX_TERMINATE structure describing termination   *
 *                                                                   *
 *   Returns  NMX_SUCCESS or NMX_FAILURE                             *
 *********************************************************************/
int nmx_bld_terminate( char **buf, int *buflen, int *totlen,
                        NMX_TERMINATE *term )
{
   NMXHDR  nhd;
   char   *pbuf;    /* working pointer into output */
   int     msglen;  /* message content length */
   int     tmp;

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   msglen = sizeof(int);
   if( term->note != NULL ) msglen += (int)strlen(term->note);
   if( nmx_checklen( buf, buflen, NMXHDR_LEN+msglen ) != 0 ) {
      return( NMX_FAILURE );
   }

/* Build header & load it into target address
 ********************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_TERMINATE;
   nhd.msglen    = msglen;
   nmx_loadhdr( &nhd, pbuf );
   pbuf += NMXHDR_LEN;

/* Add the message contents, in network byte order
 *************************************************/
   tmp = (int) htonl( (uint32_t)term->reason );
   memcpy( pbuf, &tmp, sizeof(int) );  pbuf += sizeof( int );
   if( term->note != NULL ) memcpy( pbuf, term->note, strlen(term->note) );

   *totlen = NMXHDR_LEN + msglen;

   return( NMX_SUCCESS );
}


/*********************************************************************
 * nmx_bld_add_timeseries()  Build an "add timeseries data" message  *
 *                                                                   *
 *   buf     prt to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send)          *
 *   keys    input: pointer to array of channel keys to subscribe to,*
 *           same as chankey in the NMX_CHANNEL_INFO structure       *
 *   nkeys   input: number of elements in 'keys'                     *
 *   delay   input: short-term-completion time (in seconds)          *
 *           valid range is -1s <= s <= 300s.  When NaqsNT misses    *
 *           packets from the field, it will wait for the given      *
 *           amount of time for the gap to be filled by re-sent      *
 *           packets before sending data to the subscriber.          *
 *           Specifying delay=0 will guarantee that packets are in   *
 *           chronological order, without waiting for missed data.   *
 *  format   input: requested data format.                           *
 *             -1 = compressed packets (raw format from HRD).        *
 *              0 = uncompressed packets, original sample rate.      *
 *            0<r = requested output sample rate.                    *
 *  sendbuf  input: flag for requesting buffered dat.                *
 *              0 = do not send buffered packets.                    *
 *              1 = send buffered packets. This effectively moves    *
 *                   the start of the data stream several packets    *
 *                   into the past.                                  *
 *                                                                   *
 *  Returns  NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_add_timeseries( char **buf, int *buflen, int *totlen,
                             int *keys, int nkeys, int delay, int format,
                             int sendbuf )
{
   NMXHDR  nhd;     /* header for new message */
   char   *pbuf;    /* working pointer into output */
   int     msglen;  /* message content length */
   int     tmp;
   int     ik;

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   msglen = 16 + 4*nkeys;
   if( nmx_checklen( buf, buflen, NMXHDR_LEN+msglen ) != 0 ) {
      return( NMX_FAILURE );
   }

/* Build header & it load into target address
 ********************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_ADD_TIMESERIES;
   nhd.msglen    = msglen;

   nmx_loadhdr( &nhd, pbuf );
   pbuf += NMXHDR_LEN;

/* Add the message contents, in network byte order
 *************************************************/
   tmp = (int) htonl( (uint32_t)nkeys );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   for( ik=0; ik<nkeys; ik++ )
   {
      tmp = (int) htonl( (uint32_t)keys[ik] );
      memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );
   }
   tmp = (int) htonl( (uint32_t)delay );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   tmp = (int) htonl( (uint32_t)format );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   tmp = (int) htonl( (uint32_t)sendbuf );
   memcpy( pbuf, &tmp, sizeof(int) );

   *totlen = NMXHDR_LEN + msglen;

   return( NMX_SUCCESS );
}

/*********************************************************************
 * nmx_bld_add_soh()  Build an "add state-of-health data" message    *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send to NMX)   *
 *   keys    input: pointer to array of channel keys to subscribe to,*
 *           same as chankey in the NMX_CHANNEL_INFO structure       *
 *   nkeys   input: number of elements in 'keys'                     *
 *   delay   input: short-term-completion time (in seconds)          *
 *           valid range is -1s <= s <= 300s.  When NaqsNT misses    *
 *           packets from the field, it will wait for the given      *
 *           amount of time for the gap to be filled by re-sent      *
 *           packets before sending data to the subscriber.          *
 *           Specifying delay=0 will guarantee that packets are in   *
 *           chronological order, without waiting for missed data.   *
 *  sendbuf  input: flag for requesting buffered dat.                *
 *              0 = do not send buffered packets.                    *
 *              1 = send buffered packets. This effectively moves    *
 *                   the start of the data stream several packets    *
 *                   into the past.                                  *
 *                                                                   *
 *  Returns  NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_add_soh( char **buf, int *buflen, int *totlen,
                      int *keys, int nkeys, int delay, int sendbuf )
{
   NMXHDR  nhd;     /* header for new message */
   char   *pbuf;    /* working pointer into output */
   int     msglen;  /* message content length */
   int     tmp;
   int     ik;

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   msglen = 12 + 4*nkeys;
   if( nmx_checklen( buf, buflen, NMXHDR_LEN+msglen ) != 0 ) {
      return( NMX_FAILURE );
   }

/* Build header & it into target address
 ***************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_ADD_SOH;
   nhd.msglen    = msglen;

   nmx_loadhdr( &nhd, pbuf );
   pbuf += NMXHDR_LEN;

/* Add the message contents, in network byte order
 *************************************************/
   tmp = (int) htonl( (uint32_t)nkeys );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   for( ik=0; ik<nkeys; ik++ )
   {
      tmp = (int) htonl( (uint32_t)keys[ik] );
      memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );
   }
   tmp = (int) htonl( (uint32_t)delay );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   tmp = (int) htonl( (uint32_t)sendbuf );
   memcpy( pbuf, &tmp, sizeof(int) );

   *totlen = NMXHDR_LEN + msglen;

   return( NMX_SUCCESS );
}

/*********************************************************************
 * nmx_bld_add_serial()  Build an "add SerialChannels" message       *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send to NMX)   *
 *   keys    input: pointer to array of channel keys to subscribe to,*
 *           same as chankey in the NMX_CHANNEL_INFO structure       *
 *   nkeys   input: number of elements in 'keys'                     *
 *   delay   input: short-term-completion time (in seconds)          *
 *           valid range is -1s <= s <= 300s.  When NaqsNT misses    *
 *           packets from the field, it will wait for the given      *
 *           amount of time for the gap to be filled by re-sent      *
 *           packets before sending data to the subscriber.          *
 *           Specifying delay=0 will guarantee that packets are in   *
 *           chronological order, without waiting for missed data.   *
 *  sendbuf  input: flag for requesting buffered dat.                *
 *              0 = do not send buffered packets.                    *
 *              1 = send buffered packets. This effectively moves    *
 *                   the start of the data stream several packets    *
 *                   into the past.                                  *
 *                                                                   *
 *  Returns  NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_add_serial( char **buf, int *buflen, int *totlen,
                        int *keys, int nkeys, int delay, int sendbuf )
{
   NMXHDR  nhd;     /* header for new message */
   char   *pbuf;    /* working pointer into output */
   int     msglen;  /* message content length */
   int     tmp;
   int     ik;

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   msglen = 12 + 4*nkeys;
   if( nmx_checklen( buf, buflen, NMXHDR_LEN+msglen ) != 0 ) {
      return( NMX_FAILURE );
   }

/* Build header & it into target address
 ***************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_ADD_SERIAL;
   nhd.msglen    = msglen;

   nmx_loadhdr( &nhd, pbuf );
   pbuf += NMXHDR_LEN;

/* Add the message contents, in network byte order
 *************************************************/
   tmp = (int) htonl( (uint32_t)nkeys );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   for( ik=0; ik<nkeys; ik++ )
   {
      tmp = (int) htonl( (uint32_t)keys[ik] );
      memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );
   }
   tmp = (int) htonl( (uint32_t)delay );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   tmp = (int) htonl( (uint32_t)sendbuf );
   memcpy( pbuf, &tmp, sizeof(int) );

   *totlen = NMXHDR_LEN + msglen;

   return( NMX_SUCCESS );
}

/*********************************************************************
 * nmx_bld_channellist_request()  Build a ChannelListRequest message *
 *                              (NMXMSG_CHANNELLIST_REQUEST)         *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send)          *
 *                                                                   *
 *   Returns NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_channellist_request( char **buf, int *buflen, int *totlen )
{
   NMXHDR  nhd;
   char   *pbuf;  /* working pointer */

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   if( nmx_checklen( buf, buflen, NMXHDR_LEN ) != 0 ) {
      return( NMX_FAILURE );
   }

/* Build header & it into target address
 ***************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_CHANNELLIST_REQUEST;
   nhd.msglen    = 0;

   nmx_loadhdr( &nhd, pbuf );
   *totlen = NMXHDR_LEN;

   return( NMX_SUCCESS );
}

/*********************************************************************
 * nmx_bld_precislist_request()  Build a "precise list request"      *
 *           message                                                 *
 *                                                                   *
 *   buf     ptr to target address to contain the message; on return *
 *           will hold the NMX header and the message body, in       *
 *           network byte order,as expected by Nmx socket programs.  *
 *   buflen  current length of buf (in bytes); this function may     *
 *           increase buflen and realloc(buf) if it needs more space *
 *   totlen  returns the number of bytes that were written into      *
 *           buf by this function (number of bytes to send to NMX)   *
 *   instrid input: Instrument ID for which data are requested       *
 *                  (or -1 for all instruments)                      *
 *   dtype   input: data type for which data are requested           *
 *                    1 = time series                                *
 *                    2 = state of health                            * 
 *                    6 = transparent serial                         *
 *                   -1 = all data types                             *
 *   chan    input: Channel for which data are requested             *
 *                  (or -1 for all instruments)                      *
 *                                                                   *
 *  Returns  NMX_SUCCESS or NMX_FAILURE                              *
 *********************************************************************/
int nmx_bld_precislist_request( char **buf, int *buflen, int *totlen,
                                int instrid, int dtype, int chan )
{
   NMXHDR  nhd;     /* header for new message */
   char   *pbuf;    /* working pointer into output */
   int     msglen;  /* message content length */
   int     tmp;

   *totlen = 0;

/* Make sure the target address is big enough
 ********************************************/
   msglen = 12;
   if( nmx_checklen( buf, buflen, NMXHDR_LEN+msglen ) != 0 ) {
       return( NMX_FAILURE );
   }

/* Build header & it into target address
 ***************************************/
   pbuf = *buf;
   nhd.signature = NMXHDR_SIGNATURE;
   nhd.msgtype   = NMXMSG_PRECISLIST_REQUEST;
   nhd.msglen    = msglen;

   nmx_loadhdr( &nhd, pbuf );
   pbuf += NMXHDR_LEN;

/* Add the message contents, in network byte order
 *************************************************/
   tmp = (int) htonl( (uint32_t)instrid );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   tmp = (int) htonl( (uint32_t)dtype );
   memcpy( pbuf, &tmp, sizeof(int) ); pbuf += sizeof( int );

   tmp = (int) htonl( (uint32_t)chan );
   memcpy( pbuf, &tmp, sizeof(int) );

   *totlen = NMXHDR_LEN + msglen;

   return( NMX_SUCCESS );
}


/*********************************************************************
 * nmx_rd_connect_response()  Reads a connect response message       *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, still in network byte order as read   *
 *            from the socket                                        *
 *   cresp    address of connect response structure to be filled in  *
 *                                                                   *
 *   Returns  NMX_SUCCESS or NMX_FAILURE                             *
 *********************************************************************/

int nmx_rd_connect_response( NMXHDR *nhd, char *msg, 
                             NMX_CONNECT_RESPONSE *cresp )
{
   int                fixedlen = 4;  /* length of fixed msg content */
   char              *pmsg;          /* working pointer into msg    */
   uint32_t           tmp;

/* Clear return values
 *********************/
   cresp->DAPversion = 0;

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_CONNECT_RESPONSE )
   {
      logit("et","nmx_rd_connect_response: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Read fixed-length content of message
 **************************************/
   if( nhd->msglen != fixedlen )
   {
      logit("et","nmx_rd_connect_response: Broken msg; contains only %d of"
                 " %d bytes required\n",
                  nhd->msglen, fixedlen );
      return( NMX_FAILURE );
   }
   pmsg = msg;
   memcpy( &tmp, pmsg, 4 );
   cresp->DAPversion = (int) ntohl( tmp );

   return( NMX_SUCCESS );
}



/*********************************************************************
 * nmx_rd_channel_list()  Reads a channel list message               *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, still in network byte order as read   *
 *            from the socket                                        *
 *   naqschan pointer to array of channel info structures to be      *
 *            filled in. memory will be allocated here               *
 *   nchan    returns the number of channels put into the naqschan   *
 *            array                                                  *
 *                                                                   *
 *   Returns  # channels being served on success,                    *
 *            NMX_FAILURE on failure                                 *
 *********************************************************************/
int nmx_rd_channel_list( NMXHDR *nhd, char *msg,
                         NMX_CHANNEL_INFO **naqschan, int *nchan )
{
   int                fixedlen = 4;  /* length of fixed msg content       */
   char              *pmsg;          /* working pointer into msg          */
   NMX_CHANNEL_INFO  *nci;           /* working pointer into channel list */
   uint32_t           tmp;
   int                calclen;
   int                totchan;
   int i;

/* Clear returned values
 ***********************/
   *nchan  = 0;

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_CHANNEL_LIST )
   {
      logit("et","nmx_rd_channel_list: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Read fixed-length content of message
 **************************************/
   if( nhd->msglen < fixedlen )
   {
      logit("et","nmx_rd_channel_list: Broken msg; contains only %d of"
                 " %d bytes required for fixed-length content\n",
                  nhd->msglen, fixedlen );
      return( NMX_FAILURE );
   }
   pmsg    = msg;
   memcpy( &tmp, pmsg, 4 );   pmsg += 4;
   totchan = (int) ntohl( tmp );

/* Verify header's message length with number of channels
 ********************************************************/
   calclen = fixedlen + (totchan * 16);
   if( nhd->msglen != calclen )
   {
      logit("et","nmx_rd_channel_list: Broken msg; discrepancy between"
                 " actual:%d and derived:%d message length\n",
                   nhd->msglen, calclen );
      return( NMX_FAILURE );
   }

/* Allocate space for the channel list
 *************************************/
   calclen = sizeof(NMX_CHANNEL_INFO)*totchan;
   *naqschan = (NMX_CHANNEL_INFO *) malloc( calclen );
   if( *naqschan == NULL )
   {
      logit("et","nmx_rd_channel_list: error allocating %d bytes for"
                 " channel list\n", calclen );
      return( NMX_FAILURE );
   }

/* Finish reading the message; fill the CHANNEL_INFO structures
 **************************************************************/
   nci = *naqschan;
   for( i=0; i<totchan; i++ )
   {
      memcpy( &tmp, pmsg, 4 );            pmsg += 4;
      nci->chankey = (int) ntohl( tmp );
      nci->instrid = (nci->chankey>>16) & 0xffff;  /* 16 bits */
      nci->subtype = (nci->chankey>>8) & 255;
      nci->channel = (nci->chankey) & 255;
      unpack_instid( nci->instrid, &(nci->inst) );
      memcpy( nci->stachan, pmsg, 12 );   pmsg += 12;
      nci++;
   }

/* Sort the list by chankey
 **************************/
   qsort( *naqschan, totchan, sizeof(NMX_CHANNEL_INFO),
          &compare_NMX_CHANNEL_INFO );

   *nchan = totchan;
   return( totchan );
}


/*********************************************************************
 * nmx_rd_precis_list()  Reads a precise_list message                *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, still in network byte order as read   *
 *            from the socket                                        *
 *   naqschan pointer to array of precise info structures to be      *
 *            filled in. memory will be allocated here               *
 *   nchan    returns the number of channels put into the naqschan   *
 *            array                                                  *
 *                                                                   *
 *   Returns  # channels being served on success,                    *
 *            NMX_FAILURE on failure                                 *
 *********************************************************************/
int nmx_rd_precis_list( NMXHDR *nhd, char *msg,
                        NMX_PRECIS_INFO **naqschan, int *nchan )
{
   int                fixedlen = 4;  /* length of fixed msg content       */
   char              *pmsg;          /* working pointer into msg          */
   NMX_PRECIS_INFO   *npi;           /* working pointer into precise list */
   uint32_t           tmp;
   int                calclen;
   int                totchan;
   int i;

/* Clear returned values
 ***********************/
   *nchan  = 0;

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_PRECIS_LIST )
   {
      logit("et","nmx_rd_precis_list: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Read fixed-length content of message
 **************************************/
   if( nhd->msglen < fixedlen )
   {
      logit("et","nmx_rd_precis_list: Broken msg; contains only %d of"
                 " %d bytes required for fixed-length content\n",
                  nhd->msglen, fixedlen );
      return( NMX_FAILURE );
   }
   pmsg    = msg;
   memcpy( &tmp, pmsg, 4 );   pmsg += 4;
   totchan = (int) ntohl( tmp );

/* Verify header's message length with number of channels
 ********************************************************/
   calclen = fixedlen + (totchan * 24);
   if( nhd->msglen != calclen )
   {
      logit("et","nmx_rd_precis_list: Broken msg; discrepancy between"
                 " actual:%d and derived:%d message length\n",
                   nhd->msglen, calclen );
      return( NMX_FAILURE );
   }

/* Allocate space for the channel list
 *************************************/
   calclen = sizeof(NMX_PRECIS_INFO)*totchan;
   *naqschan = (NMX_PRECIS_INFO *) malloc( calclen );
   if( *naqschan == NULL )
   {
      logit("et","nmx_rd_precis_list: error allocating %d bytes for"
                 " channel list\n", calclen );
      return( NMX_FAILURE );
   }

/* Finish reading the message; fill the PRECIS_INFO structures
 **************************************************************/
   npi = *naqschan;
   for( i=0; i<totchan; i++ )
   {
      memcpy( &tmp, pmsg, 4 );            pmsg += 4;
      npi->chankey = (int) ntohl( tmp );
      npi->instrid = (npi->chankey>>16) & 0xffff;  /* 16 bits */
      npi->subtype = (npi->chankey>>8) & 255;
      npi->channel = (npi->chankey) & 255;
      unpack_instid( npi->instrid, &(npi->inst) );
      memcpy( npi->stachan, pmsg, 12 );   pmsg += 12;
      memcpy( &tmp, pmsg, 4 );            pmsg += 4;
      npi->tstart  = ntohl( tmp );
      memcpy( &tmp, pmsg, 4 );            pmsg += 4;
      npi->tend    = ntohl( tmp );
      npi++;
   }

/* Sort the list by chankey
 **************************/
   qsort( *naqschan, totchan, sizeof(NMX_PRECIS_INFO),
          &compare_NMX_PRECIS_INFO );

   *nchan = totchan;
   return( totchan );
}


/*********************************************************************
 * nmx_rd_decompress_data()  Reads a decompressed data message       *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, still in network byte order as read   *
 *            from the socket                                        *
 *   naqsbuf  address of naqs decompressed data structure to be      *
 *            filled in                                              *
 *                                                                   *
 *   Returns  0 on success,                                          *
 *           -1 on trouble with message from socket                  *
 *          n>0 on buffer overflow, where n is the buffer length     *
 *              required to safely contain all the data in the msg   *
 *              Here's your chance to realloc & call function again  *
 *********************************************************************/
int nmx_rd_decompress_data( NMXHDR *nhd, char *msg,
                            NMX_DECOMP_DATA *naqsbuf )
{
   int                fixedlen = 20;  /* length of fixed msg content */
   char              *pmsg;           /* working pointer into msg    */
   int               *pdata;          /* working pointer into data   */
   uint32_t           tmp;
   double             dtmp;
   int                calclen;
   int                chankey;
   double             tstart;
   int                nsamp;
   int                samprate;
   int i;

/* Clear all returned values
 ***************************/
   naqsbuf->chankey   = 0;
   naqsbuf->starttime = 0;
   naqsbuf->nsamp     = 0;
   naqsbuf->isamprate = 0;
   memset( naqsbuf->data, 0, naqsbuf->maxdatalen );

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_DECOMPRESSED_DATA )
   {
      logit("et","nmx_rd_decompress_data: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Read fixed-length content of message
 **************************************/
   if( nhd->msglen < fixedlen )
   {
      logit("et","nmx_rd_decompress_data: Broken msg; contains only %d of"
                 " %d bytes required for fixed-length content\n",
                  nhd->msglen, fixedlen );
      return( NMX_FAILURE );
   }
   pmsg    = msg;
   memcpy( &tmp, pmsg, 4 );   pmsg += 4;
   chankey = (int) ntohl( tmp );

   memcpy( &dtmp, pmsg, 8 );  pmsg += 8;
   tstart = ntohd( dtmp );

   memcpy( &tmp, pmsg, 4 );   pmsg += 4;
   nsamp = (int) ntohl( tmp );

   memcpy( &tmp, pmsg, 4 );   pmsg += 4;
   samprate = (int) ntohl( tmp );

/* Verify header's message length with number of samples
 *******************************************************/
   calclen = fixedlen + (4 * nsamp);
   if( nhd->msglen != calclen )
   {
      logit("et","nmx_rd_decompress_data: Broken msg; discrepancy between"
                 " actual:%d and derived:%d message length\n",
                   nhd->msglen, calclen );
      return( NMX_FAILURE );
   }

/* Make sure there's enough room for all the data bytes
 ******************************************************/
   if( nsamp*sizeof(int) > naqsbuf->maxdatalen )
   {
      logit("et","nmx_rd_decompress_data: data buffer overflow; "
                 "have %d bytes, need %d bytes\n",
                  naqsbuf->maxdatalen, nsamp*sizeof(int) );
      return( nsamp*sizeof(int) );
   }

/* Fill the return data array
 ****************************/
   pdata = naqsbuf->data;
   for( i=0; i<nsamp; i++ )
   {
      memcpy( &tmp, pmsg, 4 );     pmsg += 4;
     *pdata = (int) ntohl( tmp );  pdata++;
   }

/* Fill the returned structure elements
 **************************************/
   naqsbuf->chankey   = chankey;
   naqsbuf->starttime = tstart;
   naqsbuf->nsamp     = nsamp;
   naqsbuf->isamprate = samprate;

   return( 0 );
}


/*********************************************************************
 * nmx_rd_compressed_data()  Reads a compressed data message         *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, in raw format as received from field  *
 *            NOTE: data is in little endian (Intel) byte order      *
 *   naqsbuf  address of naqs decompressed data structure to be      *
 *            filled in                                              *
 *                                                                   *
 *   Returns  0 on success,                                          *
 *           -1 on trouble with message from socket                  *
 *          n>0 on buffer overflow, where n is the buffer length     *
 *              required to safely contain all the data in the msg   *
 *              Here's your chance to realloc & call function again  *
 *********************************************************************/
int nmx_rd_compressed_data( NMXHDR *nhd, char *msg,
                            NMX_DECOMP_DATA *naqsbuf )
{
   char           *pbundle;   /* working pointer into msg           */
   int            *pdata;     /* working pointer into returned data */
   NMXPDATAHDR     hdr;       /* contents of header bundle          */
   NMXPDATAHDRX    xhdr;      /* contents of extended header bundle */
   NMXPDATAVAL     xdat;      /* contents of data bundle            */
   double          tstartval; /* timestamp of first value */
   int32_t         startval;
   int             nsamp=0;   /* total # samples in this packet */
   int             nbundle;   /* number of bundles in this packet */
   int             ibundle;   /* bundle we're working on */
   int             i;

/* Clear all returned values
 ***************************/
   naqsbuf->chankey   = 0;
   naqsbuf->starttime = 0;
   naqsbuf->nsamp     = 0;
   naqsbuf->isamprate = 0;
   memset( naqsbuf->data, 0, naqsbuf->maxdatalen );
   pdata = naqsbuf->data;

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_COMPRESSED_DATA )
   {
      logit("et","nmx_rd_compressed_data: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Make sure we have a proper length packet 
 ******************************************/
   if( (nhd->msglen-4)%NMXP_BYTES_PER_BUNDLE != 0  )
   {
      logit("et","nmx_rd_compressed_data: Broken packet (%d bytes); valid "
                 "NMXP pkts contain 4+%d*n bytes!\n", 
                  nhd->msglen, NMXP_BYTES_PER_BUNDLE );
      return( NMX_FAILURE );
   } 
   nbundle = (nhd->msglen-4)/NMXP_BYTES_PER_BUNDLE;

/* Read Header Bundle 
 ********************/
   pbundle = msg+4;  /* point to beginning of first bundle */ 
   if( unpack_tsheader_bundle( pbundle, &hdr )!= 0 ) {
      logit("et","nmx_rd_compressed_data: error reading timeseries header bundle\n");
      return( NMX_FAILURE );
   }
   if( hdr.pkttype != NMXP_COMPRESSDATA_PKT ) {
      logit("et","nmx_rd_compressed_data: cannot read packet type:%d\n",
             (int)hdr.pkttype );
      return( NMX_FAILURE );
   }
   tstartval = (double)hdr.sec + (double)hdr.subsec/10000.;
   startval  = hdr.firstsample;

   if(Debug>=NMXAPI_DEBUG) {
     char stime[30];
     datestr23( tstartval, stime, 30 );
     logit("e","\nnmx_rd_compressed_data: header tstart:  %s (%.4lf)\n", 
                stime, tstartval );
     logit("e","nmx_rd_compressed_data: header startval (3byte):  %d\n", 
                (int)startval );
   }
   
/* Read timeseries compressed data bundles
 *****************************************/
   for( ibundle=1; ibundle<nbundle; ibundle++ )
   {
      pbundle += NMXP_BYTES_PER_BUNDLE;  

   /* For Trident data, an extended header bundle follows the header */
      if( ibundle==1 && pbundle[0]==NMXP_EXTENDEDHDR_BUNDLE ) {  
        if( unpack_tsxheader_bundle( pbundle, &xhdr ) != 0 ) {
          logit("et","nmx_rd_compressed_data: error reading timeseries extended header bundle\n");
          return( NMX_FAILURE );
        }
        if(Debug>=NMXAPI_DEBUG) {
          logit("e","nmx_rd_compressed_data: exthdr startval (4byte):  %d\n", 
                 xhdr.firstsample );
          logit("e","nmx_rd_compressed_data: exthdr calibstatus bit0:%d  bit1:%d  bit2:%d\n",
                (int)xhdr.calibstat[0], (int)xhdr.calibstat[1], (int)xhdr.calibstat[2] );
        }
        startval = xhdr.firstsample;  /* replaces value from original header! */
        continue;
      }

   /* Check for the null bundle indicating no more data in packet */
      if( pbundle[0]==NMXP_NULL_BUNDLE ) {
        if(Debug>=NMXAPI_DEBUG) {
          logit("e","nmx_rd_compressed_data: found null bundle; no more data in packet\n" );
        }
        break;  /*no more data bundles in pkt*/
      }

   /* OK, it must be a real compressed data bundle */
      if( unpack_tsdata_bundle( pbundle, startval, &xdat ) != 0 ) {
        logit("et","nmx_rd_compressed_data: error uncompressing timeseries data bundle\n");
        return( NMX_FAILURE );
      }
      startval = xdat.data[xdat.ndata-1];  /* save last sample value for next bundle */
      if(Debug>=NMXAPI_DEBUG) {
        logit("e","nmx_rd_compressed_data: bundle%02d dataval: ",ibundle); 
        for( i=0; i<xdat.ndata; i++ ) logit("e"," %d", xdat.data[i] );
        logit("e","\n"); 
      }

   /* Make sure there's enough room for all the data bytes
    ******************************************************/
      nsamp += xdat.ndata;
      if( nsamp*sizeof(int) > naqsbuf->maxdatalen )
      {
         logit("et","nmx_rd_compressed_data: data buffer overflow; "
                    "have %d bytes, need at least %d bytes\n",
                     naqsbuf->maxdatalen, nsamp*sizeof(int) );
         return( nsamp*sizeof(int) );
      }

   /* Copy data to the return data array 
    ************************************/
      memcpy( pdata, xdat.data, xdat.ndata*sizeof(int32_t) );
      pdata += xdat.ndata;
   }

/* Fill other return structure elements.
 * NOTE WELL: The timestamp and first sample in the header are 
 * the same as the time and data value of the last sample in the 
 * previous packet. Since we're only handing back "new" data in
 * the return structure, we've left out the header sample value, 
 * and we'll add 1 sample interval to the header timestamp to get 
 * the correct timestamp for the first compressed data sample.
 ****************************************************************/
   naqsbuf->chankey   = (hdr.instrumentid<<16) | 
                        (NMX_SUBTYPE_TIMESERIES<<8) |  hdr.chan;
   naqsbuf->isamprate = NMXP_SampleRate[hdr.isamprate];
   naqsbuf->starttime = tstartval + (double)1.0/(double)naqsbuf->isamprate;
   naqsbuf->nsamp     = nsamp;

   if(Debug>=NMXAPI_DEBUG) {
     char   stime[30];
     double tend = tstartval + (double)nsamp/(double)naqsbuf->isamprate;
     datestr23( tend, stime, 30 );
     logit("e","nmx_rd_compressed_data: computed tend:    %s (%.4lf)  nsamp: %d\n"
               "nmx_rd_compressed_data: computed endval:  %d\n", 
            stime, tend, nsamp, naqsbuf->data[naqsbuf->nsamp-1] );
   }

   return( 0 );
}

/*********************************************************************
 * nmx_rd_serial_data()  Reads a compressed data message containing  *
 *                       transparent serial data                     *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, in raw format as received from field  *
 *            NOTE: data is in little endian (Intel) byte order      *
 *   sbuf     address of serial data structure to be filled in       *
 *            NOTE: The caller must set sbuf->maxdatalen and         *
 *            allocate char array sbuf->data to that length before   *
 *            this function is used.  The caller also has the option *
 *            of reallocing if sbuf->data is not large enough for    *
 *            this packet.                                           *
 *                                                                   *
 *   Returns  0 on success,                                          *
 *           -1 on trouble with message from socket                  *
 *          n>0 on buffer overflow, where n is the buffer length     *
 *              required to safely contain all the data in the msg   *
 *              Here's your chance to realloc & call function again  *
 *********************************************************************/
int nmx_rd_serial_data( NMXHDR *nhd, char *msg,
                        NMX_SERIAL_DATA *sbuf )
{
   char           *pbundle;   /* working pointer into msg           */
   NMXPSERIALHDR   hdr;       /* contents of header bundle          */

/* Clear all returned values
 ***************************/
   memset( &(sbuf->hdr), 0, sizeof(NMXPSERIALHDR) );
   sbuf->starttime = 0.0;
   sbuf->chankey   = 0;
   sbuf->nbyte     = 0;
   memset( sbuf->data, 0, sbuf->maxdatalen );

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_COMPRESSED_DATA )
   {
      logit("et","nmx_rd_serial_data: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Make sure we have a proper length packet 
 ******************************************/
   if( (nhd->msglen-4)%NMXP_BYTES_PER_BUNDLE != 0  )
   {
      logit("et","nmx_rd_serial_data: Broken packet (%d bytes); valid "
                 "NMXP pkts contain 4+%d*n bytes!\n", 
                  nhd->msglen, NMXP_BYTES_PER_BUNDLE );
      return( NMX_FAILURE );
   } 

/* Read Header Bundle 
 ********************/
   pbundle = msg+4;  /* point to beginning of first bundle */ 
   if( unpack_serialheader_bundle( pbundle, &hdr )!= 0 ) {
      logit("et","nmx_rd_serial_data: error reading transparent "
                 "serial header bundle\n");
      return( NMX_FAILURE );
   }
 /*logit("et","instr:%d port:%d seqnum:%d\n",
          hdr.instrumentid,hdr.port,hdr.seqnum );*/ /*DEBUG*/

   if( hdr.pkttype != NMXP_TRANSPARENTSERIAL_PKT ) {
      logit("et","nmx_rd_serial_data: cannot read packet type:%d\n",
             (int)hdr.pkttype );
      return( NMX_FAILURE );
   }
   
/* Make sure there's enough room for all the data bytes
 ******************************************************/
   if( hdr.nbyte > sbuf->maxdatalen )
   {
      logit("et","nmx_rd_serial_data: data buffer overflow; "
                 "have %d bytes, need at least %d bytes\n",
                  sbuf->maxdatalen, hdr.nbyte );
      return( hdr.nbyte );
   }

/* Load payload bytes into structure; 
   there's no more bundle structure to this message
 **************************************************/
   pbundle += NMXP_BYTES_PER_BUNDLE;  /* point to beginning of data */
   memcpy( sbuf->data, pbundle, hdr.nbyte );

/* Fill other return structure elements.
 ***************************************/
   memcpy( &(sbuf->hdr), &hdr, sizeof(NMXPSERIALHDR) );
   sbuf->starttime = (double)hdr.sec + (double)hdr.subsec/10000.;
   sbuf->chankey   = (hdr.instrumentid<<16) | 
                     (NMX_SUBTYPE_TRANSSERIAL<<8) |  hdr.port;
   sbuf->nbyte     = hdr.nbyte;

   return( 0 );
}


/*********************************************************************
 * nmx_rd_terminate()  Reads a termination message                   *
 *                                                                   *
 *   nhd      NMX header structure belonging to msg                  *
 *   msg      body of message, still in network byte order as read   *
 *            from the socket                                        *
 *   term     address of termination structure to be filled in       *
 *                                                                   *
 *   Returns  NMX_SUCCESS or NMX_FAILURE                             *
 *********************************************************************/

int nmx_rd_terminate( NMXHDR *nhd, char *msg, NMX_TERMINATE *term )
{
   int                fixedlen = 4;  /* length of fixed msg content */
   char              *pmsg;          /* working pointer into msg    */
   uint32_t           tmp;

/* Clear return values
 *********************/
   term->reason = 0;
   term->note   = NULL;

/* Make sure we were handed the correct message type
 ***************************************************/
   if( nhd->msgtype != NMXMSG_TERMINATE )
   {
      logit("et","nmx_rd_terminate: cannot read msgtype:%d\n",
             nhd->msgtype );
      return( NMX_FAILURE );
   }

/* Read fixed-length content of message
 **************************************/
   if( nhd->msglen < fixedlen )
   {
      logit("et","nmx_rd_terminate: Broken msg; contains only %d of"
                 " %d bytes required for fixed-length content\n",
                  nhd->msglen, fixedlen );
      return( NMX_FAILURE );
   }
   pmsg = msg;
   memcpy( &tmp, pmsg, 4 );   pmsg += 4;
   term->reason = (int) ntohl( tmp );

/* Set the pointer to note string, if there is one
 *************************************************/
   if( nhd->msglen > fixedlen )  term->note = pmsg;

   return( NMX_SUCCESS );
}


/*********************************************************************
 * compare_NMX_CHANNEL_INFO()                                        *
 *   This function is passed to qsort() and bsearch().               *
 *   We use qsort() to sort a NMX_CHANNEL_INFO array by chankey,     *
 *   and we use bsearch() to look up a chankey in the list.          *
 *********************************************************************/
int compare_NMX_CHANNEL_INFO( const void *s1, const void *s2 )
{
   NMX_CHANNEL_INFO *ch1 = (NMX_CHANNEL_INFO *) s1;
   NMX_CHANNEL_INFO *ch2 = (NMX_CHANNEL_INFO *) s2;

   if( ch1->chankey > ch2->chankey ) return  1;
   if( ch1->chankey < ch2->chankey ) return -1;
   return 0;
}

/*********************************************************************
 * compare_NMX_PRECIS_INFO()                                         *
 *   This function is passed to qsort() and bsearch().               *
 *   We use qsort() to sort a NMX_PRECIS_INFO array by chankey,      *
 *   and we use bsearch() to look up a chankey in the list.          *
 *********************************************************************/
int compare_NMX_PRECIS_INFO( const void *s1, const void *s2 )
{
   NMX_PRECIS_INFO *ch1 = (NMX_PRECIS_INFO *) s1;
   NMX_PRECIS_INFO *ch2 = (NMX_PRECIS_INFO *) s2;

   if( ch1->chankey > ch2->chankey ) return  1;
   if( ch1->chankey < ch2->chankey ) return -1;
   return 0;
}


/***************************************************************
 *  ntohd()  converts a double from network to host byte order *
 ***************************************************************/
double ntohd( double data )
{
   char temp;

   union {
       double d;
       char   c[8];
   } dat;

/* don't need to swap if you're on a SPARC */
#ifdef _SPARC
   return( data );
#endif

/* otherwise, swap */
   dat.d = data;
   temp     = dat.c[0];
   dat.c[0] = dat.c[7];
   dat.c[7] = temp;

   temp     = dat.c[1];
   dat.c[1] = dat.c[6];
   dat.c[6] = temp;

   temp     = dat.c[2];
   dat.c[2] = dat.c[5];
   dat.c[5] = temp;

   temp     = dat.c[3];
   dat.c[3] = dat.c[4];
   dat.c[4] = temp;
   dat.d;

   return( dat.d );
}


