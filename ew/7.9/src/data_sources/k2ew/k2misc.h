/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2misc.h 1965 2005-07-27 19:28:49Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.3  2003/05/15 00:42:45  lombard
 *     Changed to version 2.31: fixed handling of channel map.
 *
 *     Revision 1.2  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.1  2000/05/04 23:48:22  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2misc.h:  Header file for 'k2misc.c' -- 1/1/99 -- [ET] */

#ifndef K2MISC_H             /* process file only once */
#define K2MISC_H 1

#include "k2pktdef.h"        /* K2 packet definitions and types */

int k2mi_init_blkmde(unsigned char *pseqnum);
int k2mi_force_blkmde();
int k2mi_ping_test(int count, unsigned char *pseqnum);
int k2mi_get_params(K2_HEADER *pk2hdr, unsigned char *pseqnum);
int k2mi_get_status(struct STATUS_INFO *pk2stat, unsigned char *pseqnum);
int k2mi_get_extstatus(struct EXT2_STATUS_INFO *pk2stat, 
                       unsigned char *pseqnum, int *pext_size);
int k2mi_req_status(unsigned char *pseqnum);
int k2mi_req_extstatus(unsigned char *pseqnum);
int k2mi_req_params(unsigned char *pseqnum);
void k2mi_log_pktinfo(struct PACKET_HDR *phdrblk, unsigned char *databuff);
void k2mi_report_status( struct STATUS_INFO *pk2stat);
void k2mi_report_extstatus( struct EXT2_STATUS_INFO *pk2stat, int ext_size);
void k2mi_report_params( K2_HEADER *pk2hdr);
void k2mi_status_hb(unsigned char type, short code, char* message );
     

#endif

