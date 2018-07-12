/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2pktman.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.1  2000/05/04 23:48:30  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2pktman.h:  Header file for "k2pktman.c" -- 1/7/99 -- [ET] */

#ifndef K2PKTMAN_H           /* process file only once */
#define K2PKTMAN_H 1

#include "k2pktdef.h"        /* K2 packet definitions and types */

#define K2PM_ERRDEBUG_FLG 0       /* 1 to enable debug error simulations */

#define K2PM_MIN_DBUFSIZ 1485          /* minimum size for int32 buffer */
#define K2PM_IGNR_NONE ((unsigned char)0xFF)    /* "ignore none" value for */
                                          /*  'k2pm_recv_waitpkt()' function */

/*  Stream Control Command bytes, used with 'k2pm_send_strctrl()': */
#define K2SCC_STOP_STREAM ((unsigned char)0)   /* stop serial data stream */
#define K2SCC_START_STREAM ((unsigned char)1)  /* start serial data stream */
#define K2SCC_CONT_STREAM ((unsigned char)2)   /* continue stream (SDS mode 2) */
#define K2SCC_RESET_SEQNUM ((unsigned char)3)  /* reset block sequence number */

int k2pm_stop_streaming(unsigned char *pseqnum, int redo);
int k2pm_start_streaming(unsigned char *pseqnum, int redo);
int k2pm_send_strctrl(unsigned char cmd, unsigned char *pseqnum, int redo);
int k2pm_req_stmblk(unsigned char stmnum, uint32_t dataseq, int redo);
int k2pm_get_stmblk(struct StrDataHdr *pdatahdr, int32_t *pdatabuff,
                    int *prdatacnt, int redo);
int k2pm_recv_waitpkt(unsigned char typecode, unsigned char seqnum,
                      unsigned short srcnum, struct PACKET_HDR *phdrblk,
                      unsigned char *databuff, unsigned char ignore_code, 
                      int toutms, int redo);
int k2pm_recv_wstmpkt(struct PACKET_HDR *phdrblk, unsigned char *databuff,
                      int toutms, int redo);
char * k2pm_strnak_errcd(struct PACKET_HDR *phdrblk,
                                const unsigned char *databuff, int *already);
int k2pm_proc_streamdata(int pktdatalen, const unsigned char *pktbuff,
                         struct StrDataHdr *pdatahdr, int32_t *pdatabuff,
                         int *prdatacnt);

#if K2PM_ERRDEBUG_FLG        /* if error simulating debug enabled */
void k2pmdebug_sim_crcerr(void);
#endif

#endif

