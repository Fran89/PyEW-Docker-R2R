/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2cirbuf.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.6  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.4  2000/08/30 17:32:46  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.3  2000/08/12 18:10:04  lombard
 *     Fixed bug in cb_check_waits that caused circular buffer overflows
 *     Added cb_dumb_buf() to dump buffer indexes to file for debugging
 *
 *     Revision 1.1  2000/05/04 23:48:02  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2cirbuf.c:  Header file for 'k2cirbuf.c' circular buffer routines */
/*  */
/*   2/26/99 -- [ET]  File started */
/*  */

#ifndef K2CIRBUF_H                /* process file only once per compile */
#define K2CIRBUF_H 1

#define K2CB_DBFIDX_NONE -1   /* buffer index value for "none" */

int k2cb_init_buffer(int cbufsiz, int dbufents);
void k2cb_dealloc_buffer(void);
int k2cb_block_in(unsigned char stmnum, uint32_t dataseq,
                  uint32_t timestamp, unsigned short msec,
                  int32_t *pdatabuff);
int k2cb_block_out(unsigned char *pstmnum, uint32_t *pdataseq,
                   uint32_t *ptimestamp, unsigned short *pmsec,
                   int32_t *pdatabuff);
int k2cb_blkwait_in(unsigned char stmnum, uint32_t dataseq);
int k2cb_blkskip_in(unsigned char stmnum, uint32_t dataseq);
int k2cb_fill_waitblk(unsigned char stmnum, uint32_t dataseq,
                      uint32_t timestamp, unsigned short msec,
                      int32_t *pdatabuff,int *p_rerequest_idnum);
int k2cb_tick_waitents(int resenditvl,unsigned char *pstmnum,
                    uint32_t *pdataseq,int *ptickcnt, int *presendcnt);
int k2cb_incwt_resendcnt(int idnum);
int k2cb_skip_waitent(unsigned char stmnum, uint32_t dataseq);
void k2cb_check_waits( uint32_t oldseq );
int k2cb_get_entry(int idnum,unsigned char *pstmnum,
                     uint32_t *pdataseq,int *ptickcnt,int *presendcnt);
int k2cb_get_waitent(unsigned char stmnum,uint32_t dataseq);
void k2cb_dump_buf(void);

#endif

