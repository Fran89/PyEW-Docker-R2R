/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2pktio.h 90 2000-05-04 23:48:43Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/05/04 23:48:27  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2pktio.h:  Header file for 'k2pktio.c' K2 packet I/O functions */

#ifndef K2PKTIO_H            /* compile file only once */
#define K2PKTIO_H 1          /* indicate file has been compiled */

#include "k2pktdef.h"        /* K2 packet definitions and types */


int k2p_send_packet(unsigned char typecd, unsigned char seqnum,
                    unsigned short srcnum, int datalen,
                    const unsigned char *databuff, int redo);
int k2p_xlat_pckdata(unsigned char *dstptr, const unsigned char *srcptr, 
                     int srclen, unsigned short *pchksum);
int k2p_recv_packet(struct PACKET_HDR *phdrblk, unsigned char *databuff, 
                    int toutms, int redo);

#endif

