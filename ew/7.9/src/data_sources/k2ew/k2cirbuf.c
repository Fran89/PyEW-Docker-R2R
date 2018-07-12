 /*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2cirbuf.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2007/12/18 13:36:03  paulf
 *     version 2.43 which improves modem handling for new kmi firmware on k2 instruments
 *
 *     Revision 1.8  2003/05/29 13:33:40  friberg
 *     cleaned up some warnings
 *
 *     Revision 1.7  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.5  2000/08/30 17:32:46  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.4  2000/08/12 18:10:04  lombard
 *     Fixed bug in cb_check_waits that caused circular buffer overflows
 *     Added cb_dumb_buf() to dump buffer indexes to file for debugging
 *
 *     Revision 1.2  2000/07/03 18:00:37  lombard
 *     Added code to limit age of waiting packets; stops circ buffer overflows
 *     Added and Deleted some config params.
 *     Added check of K2 station name against restart file station name.
 *     See ChangeLog for complete list.
 *
 *     Revision 1.1  2000/05/04 23:48:00  lombard
 *     Initial revision
 *
 *
 *
 */
/*  k2cirbuf.c:  K2 circular FIFO buffer routines                   *
 *                                                                  *
 *   2/26/99 -- [ET]  File started                                  *
 *                                                                  *
 *  Added earthworm mutex locks; removed `ready' flag. PNL 3/9/2000 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>        /* for INT_MAX */
#include <earthworm.h>
#include "glbvars.h"
#include "k2ewerrs.h"        /* K2-to-Earthworm error codes */
#include "k2cirbuf.h"        /* header file for this module */

#define K2CB_DEBUG_FLG 0         /* 1 = debug enabled */
                                 
#define K2CB_MAX_NUMENTS 60000   /* max # of circular buff entries */
#define K2CB_MAX_DBFENTS 10000   /* max # of '.databuff[]' entries */
                                 
typedef struct                   /* structure of circular buffer blocks */
{                                
  int nextwait;                  /* index of next entry waiting to be filled */
  int32_t *pdatabuff;            /* pointer to data buffer */
  uint32_t dataseq;              /* data block sequence number */
  uint32_t timestamp;            /* timestamp in seconds since 1/1/1980 */
  unsigned short msec;           /* milliseconds, 0..999 */
  unsigned char stmnum;          /* logical stream number */
  char waitflg;                  /* 1 if block is waiting to for data */
  char skipflg;                  /* 1 if block is to be "skipped" */
  char dummyflg;                 /* dummy flag for better byte alignment */
} s_circ_bblk;

static mutex_t cb_mutex;                   /* mutex lock for circular buffer */
static s_circ_bblk *cb_circ_buff=NULL;     /* pointer to circular buffer */
static int cb_buff_size = 0;          /* # of entries in circular buffer */
static int cb_dbuff_nents = 100;      /* # of entries in each '.pdatabuff[]' */
static int cb_bfin_pos=0,cb_bfout_pos=0; /* buffer in/out index variables */
static int cb_wait_top = K2CB_DBFIDX_NONE; 

                                             /* index of first waiting entry */
/**************************************************************************
 * k2cb_init_buffer:  allocates and initializes circular buffer for       *
 *      incoming K2 data                                                  *
 *         cbufsiz  - number of blocks circular buffer is to have         *
 *         dbufents - number of entries in each '.pdatabuff[]' member     *
 *                    of each circular buffer block                       *
 *      return value:  returns K2R_NO_ERROR on success;                   *
 *                    K2R_ERROR on errors.                                *
 **************************************************************************/

int k2cb_init_buffer(int cbufsiz, int dbufents)
{
  int cbcnt, dcnt;
  s_circ_bblk *cbptr;

  if (cbufsiz <= 0 || cbufsiz > K2CB_MAX_NUMENTS )
  {
    logit("et", "k2cb_init_buffer: cbufsiz (%d) out of range (%d - %d)\n",
          cbufsiz, 0, K2CB_MAX_NUMENTS);
    return K2R_ERROR;
  }
  if (dbufents <= 0 || dbufents > K2CB_MAX_DBFENTS)
  {
    logit("et", "k2cb_init_buffer: dbufents (%d) out of range (%d - %d)\n",
          dbufents, 0, K2CB_MAX_DBFENTS);
    return K2R_ERROR;
  }

  if (cb_circ_buff != NULL)        /* if circular buffer already allocated */
    k2cb_dealloc_buffer();         /*  then deallocate it all first */
  if ( (cb_circ_buff = (s_circ_bblk *)malloc(sizeof(s_circ_bblk)*cbufsiz)) ==
       NULL)
  {
    logit("et", "k2cb_dealloc_buffer: error allocating memory for circular buffer\n");
    return K2R_ERROR;              /* if unable to allocate then return code */
  }
  
  cb_buff_size = cbufsiz;          /* setup # of entries in circular buffer */
  cb_dbuff_nents = dbufents;       /* setup # of ents in each '.pdatabuff[]' */
  for (cbcnt = 0; cbcnt < cb_buff_size; ++cbcnt)
  {      /* initialize entries in each block */
    cbptr = &cb_circ_buff[cbcnt];  /* setup pointer to current block */
    cbptr->stmnum = (unsigned char)0;      /* logical stream number */
    cbptr->waitflg =               /* TRUE if block is waiting for data */
      cbptr->skipflg = (char)0;       /* TRUE if block is to be "skipped" */
    cbptr->dataseq = (uint32_t)0;     /* data block sequence number */
    cbptr->timestamp = (uint32_t)0;   /* timestamp in seconds since 1/1/1980 */
    cbptr->msec = (unsigned short)0;      /* milliseconds, 0..999 */
    cbptr->pdatabuff = NULL;       /* pointer to data buffer, NULL for now */
    cbptr->nextwait = K2CB_DBFIDX_NONE;  /* index of next waiting entry */
  }
  for (cbcnt = 0; cbcnt < cb_buff_size; ++cbcnt)
  {      /* allocate '.databuff[]' buffer in each block */
    cbptr = &cb_circ_buff[cbcnt];  /* setup pointer to current block */
    if ( (cbptr->pdatabuff = (int32_t *)malloc(sizeof(int32_t) * dbufents)) == NULL)
    {    /* unable to allocate data buffer */
      k2cb_dealloc_buffer();       /* deallocate everything allocate before */
      logit("et", "k2cb_init_buffer: error allocating memory for buffer data slot\n");
      return K2R_ERROR;
    }
    for ( dcnt = 0; dcnt < cb_dbuff_nents; ++dcnt)
      cbptr->pdatabuff[dcnt] = 0L;  /* set each data buff entry to 0 */
  }
  cb_bfin_pos = cb_bfout_pos = 0; 
  cb_wait_top = K2CB_DBFIDX_NONE; 

  CreateSpecificMutex(&cb_mutex);   /* initialize the mutex variable */
  
  return K2R_NO_ERROR;          /* return OK code */
}


/**************************************************************************
 * k2cb_dealloc_buffer:  deallocates circular buffer                      *
 *    Note: no mutex protection used here!                                *
 **************************************************************************/

void k2cb_dealloc_buffer()
{
  int cbcnt;
  s_circ_bblk *cbptr;

  if (cb_circ_buff != NULL)
  {      /* circular buffer was allocated */
    for (cbcnt = 0; cbcnt < cb_buff_size; ++cbcnt)
    {      /* deallocate '.databuff[]' buffer for each block */
      cbptr = &cb_circ_buff[cbcnt];    /* setup pointer to current block */
      if (cbptr->pdatabuff != NULL)     /* if data buffer allocated then */
        free(cbptr->pdatabuff);        /* deallocate data buffer */
    }
    free(cb_circ_buff);           /* free circular buffer allocation */
    cb_circ_buff = NULL;          /* indicate circular buffer not allocated */
  }
  cb_buff_size = 0;       /* clear # of entries in circular buffer */
  cb_dbuff_nents = 0;     /* clear # of ents in each '.pdatabuff[]' */
  cb_bfin_pos = cb_bfout_pos = 0; /* initialize buffer in/out index variables */
  cb_wait_top = K2CB_DBFIDX_NONE; /* initialize index of first waiting entry */
}


/**************************************************************************
 * k2cb_block_in:  enters block of data into next 'in' position in        *
 *      circular buffer                                                   *
 *         stmnum    - logical stream number                              *
 *         dataseq   - data block sequence number                         *
 *         timestamp - timestamp in seconds since 1/1/1980                *
 *         msec      - milliseconds, 0..999                               *
 *         pdatabuff - ptr to data buffer of signed 32-bit integers to    *
 *                     be entered (# of entries determined by 'dbufents'  *
 *                     parameter sent to 'k2cb_init_buffer()' function)   *
 *      return value:  returns K2R_NO_ERROR on success;                   *
 *                    K2R_ERROR on errors.                                *
 **************************************************************************/

int k2cb_block_in(unsigned char stmnum, uint32_t dataseq,
                  uint32_t timestamp, unsigned short msec,
                  int32_t *pdatabuff)
{
  int nextpos,dcnt;
  s_circ_bblk *cbptr;
  
#ifdef DEBUG
  if(pdatabuff == NULL)           /* if NULL pointer then */
  {
    logit("et", "k2cb_block_in: NULL pdatabuff parameter\n");
    return K2R_ERROR;       /* return bad parameter error code */
  }
  if (cb_circ_buff == NULL)        /* if circular buffer not allocated then */
  {
    logit("et", "k2cb_block_in: cb_circ_buff not initialized\n");
    return K2R_ERROR;      /* return not-initialized error code */
  }
#endif
  
  /* calculate next circular buffer position */
  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */

  /* see if there are any ancient wait packets in the buffer */
  if ( dataseq > (uint32_t) gcfg_pktwait_lim)
    k2cb_check_waits(dataseq - (uint32_t)gcfg_pktwait_lim);
  
  if ( (nextpos = cb_bfin_pos + 1)  >= cb_buff_size)
    nextpos = 0;          /* if past end then wrap around to beg */
  if (nextpos == cb_bfout_pos)     /* if reached 'out' position then */
  {
    ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
    logit("et", "k2cb_block_in: circular buffer full; report this to k2ew maintainer!!!\n");
    return K2R_ERROR;      /* return buffer-full error code */
  }

  cbptr = &cb_circ_buff[cb_bfin_pos];  /* setup pointer to block to load */
  if (cbptr->pdatabuff == NULL)   /* if its data buffer pointer is NULL then */
  {
    ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
    logit("et", "k2cb_block_in: internal error: unexpected null data block\n");
    return K2R_ERROR;     /* return internal-error code */
  }
  
  cbptr->stmnum = stmnum;         /* enter logical stream number */
  cbptr->waitflg =                /* clean "waiting" flag */
    cbptr->skipflg = 0;           /* clear "skip" flag */
  cbptr->dataseq = dataseq;       /* enter data block sequence number */
  cbptr->timestamp = timestamp;   /* enter timestamp */
  cbptr->msec = msec;             /* enter milliseconds */
  cbptr->nextwait = K2CB_DBFIDX_NONE;  /* no index of next waiting entry */
  /* copy over data buffer values */
  for (dcnt = 0; dcnt < cb_dbuff_nents; ++dcnt)
    cbptr->pdatabuff[dcnt] = pdatabuff[dcnt];
  
  if (gcfg_debug > 2 && cbptr->timestamp < 10000)
    fprintf(stderr, "[timestamp=%u in 'k2cb_block_in()']\n",(unsigned)cbptr->timestamp);
  
  cb_bfin_pos = nextpos;          /* move 'in' position to next entry in buff */
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2R_NO_ERROR;          /* return OK code */
}


/**************************************************************************
 * k2cb_block_out:  attempts to retrieve block of data from next 'out'    *
 *      position in circular buffer                                       *
 *         pstmnum    - ptr to variable to receive logical stream number  *
 *         pdataseq   - ptr to variable to receive data block sequence #  *
 *         ptimestamp - ptr to variable to receive timestamp (in seconds  *
 *                      since 1/1/1980)                                   *
 *         pmsec      - ptr to variable to receive milliseconds, 0..999   *
 *         pdatabuff  - ptr to data buffer of signed 32-bit integers to   *
 *                      receive 'dbufents' entries (value determined by   *
 *                      parameter sent to 'k2cb_init_buffer()' function)  *
 *      return value:  returns K2R_NO_ERROR on successful return of data; *
 *                     K2R_CB_BUFEMPTY, K2R_CB_WAITENT, K2R_CB_SKIPENT as *
 *                       appropriate;                                     *
 *                     K2R_ERROR on error.                                *
 **************************************************************************/

int k2cb_block_out(unsigned char *pstmnum, uint32_t *pdataseq,
                   uint32_t *ptimestamp, unsigned short *pmsec,
                   int32_t *pdatabuff)
{
  int dcnt;
  s_circ_bblk *cbptr;

#ifdef DEBUG
  if (pstmnum == NULL || pdataseq == NULL ||  ptimestamp == NULL ||
      pmsec == NULL || pdatabuff == NULL)
  {
    logit("et", "k2cb_block_out: null paramter pstmnum (%p) pdataseq (%p)"
          " ptimestamp (%p) pmsec (%p) pdatabuff (%p)\n",
          pstmnum, pdataseq, ptimestamp, pmsec, pdatabuff);
    return K2R_ERROR;       /* if NULL pointer then return error code */
  }
  if (cb_circ_buff == NULL)        /* if circular buffer not allocated then */
  {
    logit("et", "k2cb_block_out: cb_circ_buff not initialized\n");
    return K2R_ERROR;      /* return not-initialized error code */
  }
#endif

  if(cb_bfout_pos == cb_bfin_pos)      /* if no entries in buffer then */
    return K2R_CB_BUFEMPTY;            /* return empty code */

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */
  cbptr = &cb_circ_buff[cb_bfout_pos]; /* setup pointer to block to unload */
  if (cbptr->pdatabuff == NULL)   /* if its data buffer pointer is NULL then */
  {
    ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
    logit("et", "k2cb_block_out: internal error: unexpected null data block\n");
    return K2R_ERROR;     /* return internal-error code */
  }
  
  /* don't unload block values until after flags are tested */
  /*  (other task may be changing them) */

  if (cbptr->skipflg == 0)
  {      /* entry is not marked as "skip" */
    if (cbptr->waitflg == 1)
    {    /* entry is "waiting" */
      *pstmnum = cbptr->stmnum;       /* retrieve logical stream number */
      *pdataseq = cbptr->dataseq;     /* retrieve data block sequence number */
      ReleaseSpecificMutex(&cb_mutex);
      return K2R_CB_WAITENT;          /* return waiting-entry code */
    }
    *pstmnum = cbptr->stmnum;         /* retrieve logical stream number */
    *pdataseq = cbptr->dataseq;       /* retrieve data block sequence number */
    *ptimestamp = cbptr->timestamp;   /* retrieve timestamp */
    *pmsec = cbptr->msec;             /* retrieve milliseconds */
    /* retrieve data buffer values */
    for (dcnt = 0; dcnt < cb_dbuff_nents; ++dcnt)
      pdatabuff[dcnt] = cbptr->pdatabuff[dcnt];
  }
  else
  {      /* entry is marked as "skip" */
    *pstmnum = cbptr->stmnum;         /* retrieve logical stream number */
    *pdataseq = cbptr->dataseq;       /* retrieve data block sequence number */
    *ptimestamp = (uint32_t)0;        /* clear timestamp */
    *pmsec = (unsigned short)0;       /* clear milliseconds */
  }

  if (++cb_bfout_pos >= cb_buff_size) /* increment buffer 'out' position */
    cb_bfout_pos = 0;                 /* if past end then wrap around to beg */

  if (gcfg_debug > 2 && cbptr->timestamp < 10000)
    logit("et", "[timestamp=%lu in 'k2cb_block_out()']\n",
            cbptr->timestamp);

  /* return code based on whether or not entry marked as "skip" */
  ReleaseSpecificMutex(&cb_mutex);    /* let another thread have the mutex */
  return (cbptr->skipflg == 0) ? K2R_NO_ERROR : K2R_CB_SKIPENT;
}


/**************************************************************************
 * k2cb_blkwait_in:  enters new "waiting" data block into next 'out'      *
 *      position in circular buffer                                       *
 *         stmnum  - logical stream number                                *
 *         dataseq - data block sequence number                           *
 *      return value:  returns K2R_NO_ERROR on success;                   *
 *                    K2R_ERROR on errors.                                *
 **************************************************************************/

int k2cb_blkwait_in(unsigned char stmnum, uint32_t dataseq)
{
  int nextpos,npos, wtpos;
  s_circ_bblk *cbptr;
  
#ifdef DEBUG
  if(cb_circ_buff == NULL)        /* if circular buffer not allocated then */
  {
    logit("et", "k2cb_blkwait_in: cb_circ_buff not initialized\n");
    return K2R_ERROR;      /* return not-initialized error code */
  }
#endif
  
  /* calculate next circular buffer position */
  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */

  if ( (nextpos = cb_bfin_pos + 1) >= cb_buff_size)
    nextpos = 0;                  /* if past end then wrap around to beg */
  if (nextpos == cb_bfout_pos)     /* if reached 'out' position then */
  {
    ReleaseSpecificMutex(&cb_mutex);/* let another thread have the mutex */
    logit("et", "k2cb_blkwait_in: circular buffer full; report this to k2ew maintainer!!!\n");
    return K2R_ERROR;      /* return buffer-full error code */
  }

  cbptr = &cb_circ_buff[cb_bfin_pos];  /* setup pointer to block to load */
  if (cbptr->pdatabuff == NULL)   /* if its data buffer pointer is NULL then */
  {
    ReleaseSpecificMutex(&cb_mutex);/* let another thread have the mutex */
    logit("et", "k2cb_block_in: internal error: unexpected null data block\n");
    return K2R_ERROR;     /* return internal-error code */
  }
  
  cbptr->stmnum = stmnum;         /* enter logical stream number */
  cbptr->waitflg = 1;             /* set "waiting" flag (entry is waiting) */
  cbptr->skipflg = 0;             /* clear "skip" flag */
  cbptr->dataseq = dataseq;       /* enter data block sequence number */
  cbptr->timestamp = (uint32_t)0; /* 'timestamp' used as waiting "tick" count */
  cbptr->msec = (unsigned short)0;      /* 'msec' used as resend-count */
  cbptr->nextwait = K2CB_DBFIDX_NONE;  /* will be last entry on waiting list */

  /* add new entry to waiting list */
  if ( (wtpos = cb_wait_top) != K2CB_DBFIDX_NONE && 
       wtpos < cb_buff_size)
  {      /* waiting-list top value specifies a valid buffer value */
    while ( (npos = cb_circ_buff[wtpos].nextwait) != K2CB_DBFIDX_NONE &&
                                                        npos < cb_buff_size)
    {    /* loop while searching for next valid waiting list entry */
      wtpos = npos;               /* move to next position */
    }
    cb_circ_buff[wtpos].nextwait = cb_bfin_pos;       /* append new entry */
  }
  else   /* no entries in waiting list */
    cb_wait_top = cb_bfin_pos;    /* new entry is first waiting-list entry */

  cb_bfin_pos = nextpos;          /* move 'in' position to next entry in buff */
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2R_NO_ERROR;          /* return OK code */
}


#ifdef USE_BLKSKIP_IN
         /* this function is not currently being used */
/**************************************************************************
 * k2cb_blkskip_in:  enters new data block marked as "skip" into next     *
 *      'out' position in circular buffer                                 *
 *         stmnum  - logical stream number                                *
 *         dataseq - data block sequence number                           *
 *      return value:  returns K2R_NO_ERROR on success;                   *
 *                    K2R_ERROR on errors.                                *
 **************************************************************************/

int k2cb_blkskip_in(unsigned char stmnum, uint32_t dataseq)
{
  int nextpos;
  s_circ_bblk *cbptr;

#ifdef DEBUG
  if (cb_circ_buff == NULL)        /* if circular buffer not allocated then */
  {
    logit("et", "k2cb_blkskip_in: cb_circ_buff not initialized\n");
    return K2R_ERROR;      /* return not-initialized error code */
  }
#endif

  /* calculate next circular buffer position */
  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */

  if ( (nextpos = cb_bfin_pos + 1) >= cb_buff_size)
    nextpos = 0;                  /* if past end then wrap around to beg */
  if (nextpos == cb_bfout_pos)     /* if reached 'out' position then */
  {
    ReleaseSpecificMutex(&cb_mutex);/* let another thread have the mutex */
    logit("et", "k2cb_blkskip_in: circular buffer full; report this to k2ew maintainer!!!\n");
    return K2R_ERROR;      /* return buffer-full error code */
  }

  cbptr = &cb_circ_buff[cb_bfin_pos];  /* setup pointer to block to load */
  if (cbptr->pdatabuff == NULL)   /* if its data buffer pointer is NULL then */
  {
    ReleaseSpecificMutex(&cb_mutex);/* let another thread have the mutex */
    logit("et", "k2cb_blkskip_in: internal error: unexpected null data block\n");
    return K2R_ERROR;     /* return internal-error code */
  }
  
  cbptr->waitflg = 0;             /* clear "waiting" flag */
  cbptr->stmnum = stmnum;         /* enter logical stream number */
  cbptr->dataseq = dataseq;       /* enter data block sequence number */
  cbptr->timestamp = (uint32_t)0;   /* clear 'timestamp' value */
  cbptr->msec = (unsigned short)0;        /* clear 'msec' value */
  cbptr->nextwait = K2CB_DBFIDX_NONE;  /* no index of next waiting entry */
  cbptr->skipflg = 1;             /* set "skip" flag */

  cb_bfin_pos = nextpos;          /* move 'in' position to next entry in buff */
  
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2R_NO_ERROR;          /* return OK code */
}

#endif


/**************************************************************************
 * k2cb_fill_waitblk:  attempts to fill given data into block in          *
 *      circular buffer previously marked as "waiting" (by clearing its   *
 *      '->waitflg' and deleting it from the waiting list)                *
 *         stmnum  - logical stream number                                *
 *         dataseq - data block sequence number                           *
 *         ptimestamp - timestamp (in seconds since 1/1/1980)             *
 *         pmsec      - milliseconds, 0..999                              *
 *         pdatabuff  - ptr to data buffer of signed 32-bit integers to   *
 *                      receive 'dbufents' entries (value determined by   *
 *                      parameter sent to 'k2cb_init_buffer()' function)  *
 *         p_rerequest_idnum - ptr to integer; if the filled wait block   *
 *                      is not the first in the wait list then the ID     *
 *                      number of the first block in the wait list is     *
 *                      entered (if its resend count is not larger than   *
 *                      that of the filled block); otherwise it is left   *
 *                      unchanged                                         *
 *      return value:  returns K2R_NO_ERROR on success;                   *
 *                     K2ERR_CB_NOTFOUND if wait-block not found;         *
 *                    K2R_ERROR on other errors.                          *
 **************************************************************************/

int k2cb_fill_waitblk(unsigned char stmnum, uint32_t dataseq,
                                    uint32_t timestamp, unsigned short msec,
                                  int32_t *pdatabuff,int *p_rerequest_idnum)
{
  int wtpos,prevpos,dcnt;
  s_circ_bblk *cbptr;

#ifdef DEBUG
  if(cb_circ_buff == NULL)        /* if circular buffer not allocated then */
  {
    logit("et", "k2cb_fill_waitblk: cb_circ_buff not initialized\n");
    return K2R_ERROR;      /* return not-initialized error code */
  }
#endif

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available   */
  if (cb_bfout_pos != cb_bfin_pos &&
      (wtpos = cb_wait_top) != K2CB_DBFIDX_NONE && wtpos < cb_buff_size)
  {      /* buffer not empty and waiting list top specifies valid entry */
    prevpos = K2CB_DBFIDX_NONE;        /* initialize "previous" position */
    do
    {    /* search waiting list for entry matching 'stmnum' and 'dataseq' */
      cbptr = &cb_circ_buff[wtpos];    /* setup pointer to block */
      if (cbptr->stmnum == stmnum && cbptr->dataseq == dataseq)
      {       /* matching waiting entry found; delete from waiting list */
        if (prevpos == K2CB_DBFIDX_NONE)    /* if no "previous" pos then */
          cb_wait_top = cbptr->nextwait;    /* mv list-top to next entry */
        else  /* "previous" position exists (not top-of-list entry) */
        {     /* make "previous" pos point to next waiting list entry */
          cb_circ_buff[prevpos].nextwait = cbptr->nextwait;
          if(p_rerequest_idnum != NULL &&
                              cb_circ_buff[cb_wait_top].msec <= cbptr->msec)
          {   /* ptr not NULL and resend count of top entry not too large */
            *p_rerequest_idnum = cb_wait_top;    /* enter list-top ID # */
          }  
          if(gcfg_debug > 0)
          {   /* debug enabled; show message */
            logit("et","Received wait block (stm=%d,dseq=%lu) newer than"
                       " oldest wait block (stm=%d,dseq=%lu)\n",
                (int)stmnum,dataseq,(int)(cb_circ_buff[cb_wait_top].stmnum),
                                         cb_circ_buff[cb_wait_top].dataseq);
          }
        }
        cbptr->skipflg = 0;            /* make sure "skip" flag is clear */
        cbptr->waitflg = 0;            /* clear "waiting" flag */
        cbptr->timestamp = timestamp;  /* enter timestamp */
        cbptr->msec = msec;            /* enter milliseconds */
        cbptr->nextwait = K2CB_DBFIDX_NONE; /* no index of next waiting entry */
        /* copy over data buffer values */
        for (dcnt = 0; dcnt < cb_dbuff_nents; ++dcnt)
          cbptr->pdatabuff[dcnt] = pdatabuff[dcnt];
        ReleaseSpecificMutex(&cb_mutex); /*let another thread have the mutex */
#ifdef DEBUG
        if (cbptr->timestamp < 10000)
        {
          logit("et","[timestamp=%lu in 'k2cb_fill_waitblk()']\n",
                                                          cbptr->timestamp);
        }
#endif
        return K2R_NO_ERROR;         /* return OK code */
      }
      prevpos = wtpos;       /* setup "previous" position for next iteration */
    }                             /* loop if another waiting list entry found */
    while ( (wtpos = cbptr->nextwait) != K2CB_DBFIDX_NONE &&
            wtpos < cb_buff_size);
  }
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2ERR_CB_NOTFOUND;       /* return not-found code */
}


/**************************************************************************
 * k2cb_tick_waitents:  called after every successful SDS packet received *
 *      to update any slots waiting to be filled in circular buffer;      *
 *      "ticks" entries on waiting list by incrementing their             * 
 *      '->timestamp' values; returns information about the block         *
 *      on the waiting list with the lowest resend-count value (held      *
 *      in '->msec') and the largest "tick" count (sometimes blocks with  *
 *      higher resend-count values are returned when their tick counts    *
 *      get too high as measured against the 'resenditvl' parameter)      *
 *         resenditvl - interval between resend requests of the same      *
 *                      block, measured in ticks                          *
 *         pstmnum    - ptr to variable to receive logical stream number  *
 *                      of returned entry on waiting list                 *
 *         pdataseq   - ptr to variable to receive data block sequence #  *
 *                      of returned entry on waiting list                 *
 *         ptickcnt   - ptr to variable to receive "tick" count           *
 *                      of returned entry on waiting list                 *
 *         presendcnt - ptr to variable to receive resend-count           *
 *                      of returned entry on waiting list                 *
 *      return value:  returns block ID number for returned entry on      *
 *                     waiting list (for use with 'k2cb_incwt_resendcnt()'*
 *                     function) or 'K2CB_DBFIDX_NONE' if waiting list    *
 *                     is empty                                           *
 **************************************************************************/

int k2cb_tick_waitents(int resenditvl,unsigned char *pstmnum,
                     uint32_t *pdataseq,int *ptickcnt, int *presendcnt)
{
  int wtpos,retpos,rsval,rscnt_min;
  int32_t tick_max;
  s_circ_bblk *cbptr;

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */
  if((wtpos=cb_wait_top) != K2CB_DBFIDX_NONE && wtpos < cb_buff_size &&
                        cb_circ_buff != NULL && cb_bfout_pos != cb_bfin_pos)
  {      /* waiting list top specifies valid entry and buffer not empty */
    if(resenditvl <= 0)
      resenditvl = 1;        /* make sure 'resenditvl' value is > 0 */
    if(cb_circ_buff[cb_wait_top].timestamp < (uint32_t)resenditvl)
    {    /* first wait entry is not older than 'resenditvl' ticks */
      retpos = K2CB_DBFIDX_NONE;  /* init ID value for returned block */
      rscnt_min = INT_MAX;        /* init resend count minimum found */
      tick_max = -1L;             /* initialize tick count maximum found */
      do
      {  /* "tick" each waiting entry by incrementing its count */
        cbptr = &cb_circ_buff[wtpos];  /* setup pointer to block */
        ++(cbptr->timestamp);          /* increment tick count for entry */
                   /* get resend-request count for block; decrease it */
                   /*  by #-of-ticks divided by 'resenditvl' to force */
                   /*  extra resend-requests of "older" wait blocks   */
        if((rsval=cbptr->msec-(int)(cbptr->timestamp/resenditvl)) < 0)
          rsval = 0;         /* if negative then make it zero */
        if (rsval < rscnt_min || (rsval == rscnt_min &&
                                    (int32_t)(cbptr->timestamp) > tick_max))
        {  /* resend-count is smallest or equal with largest "tick" count */
          retpos = wtpos;                        /* save ID value for block */
          rscnt_min = rsval;                     /* save resend-count value */
          tick_max = (int32_t)(cbptr->timestamp);   /* save "tick" count value */
        }
      }                /* loop if another waiting list entry found */
      while((wtpos=cbptr->nextwait) != K2CB_DBFIDX_NONE &&
                                                      wtpos < cb_buff_size);
    }
    else      /* first wait entry is older than 'resenditvl' ticks */
      retpos = cb_wait_top;       /* force return of first wait entry */

    if (retpos != K2CB_DBFIDX_NONE && retpos < cb_buff_size)
    {    /* ID value for entry to be returned is OK */
      cbptr = &cb_circ_buff[retpos];   /* setup pointer to block */
      if (pstmnum != NULL)             /* if pointer not NULL then */
        *pstmnum = cbptr->stmnum;      /* enter logical stream # */
      if (pdataseq != NULL)            /* if pointer not NULL then */
        *pdataseq = cbptr->dataseq;    /* enter data block seq # */
      if (ptickcnt != NULL)
      {       /* ptr not NULL; if tick count in range enter it, else max val */
        *ptickcnt = (cbptr->timestamp <= (uint32_t)0xFFFF) ?
          (int)(cbptr->timestamp) : INT_MAX;
      }
      if (presendcnt != NULL)          /* if pointer not NULL then */
        *presendcnt = cbptr->msec;     /* enter resend-count value */
      ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
      return retpos;              /* return ID of entry */
    }
  }
         /* no valid waiting list entry found */
  ReleaseSpecificMutex(&cb_mutex);  /* let another thread have the mutex */
  return K2CB_DBFIDX_NONE;        /* return "none" value */
}


/**************************************************************************
 * k2cb_incwt_resendcnt:  increments the resend-count ('->msec')          *
 *      and clears the "tick" count ('->timestamp') of the given entry    *
 *      on the waiting list                                               *
 *         idnum - block ID number of the given entry on the waiting list *
 *      return value:  returns K2R_POSITIVE if successful;                *
 *                     K2R_ERROR if 'idnum' does not identify a valid     *
 *                     waiting list entry.                                *
 **************************************************************************/

int k2cb_incwt_resendcnt(int idnum)
{
  s_circ_bblk *cbptr;

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */
  if (idnum != K2CB_DBFIDX_NONE && idnum < cb_buff_size &&
      cb_circ_buff != NULL && cb_bfout_pos != cb_bfin_pos)
  {      /* given block ID number is valid and circular buffer not empty */
    cbptr = &cb_circ_buff[idnum];      /* setup pointer to block */
    ++(cbptr->msec);                   /* increment resend-count */
    cbptr->timestamp = 0L;             /* clear "tick" count for entry */
    ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
    return K2R_POSITIVE;                       /* return OK flag */
  }
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2R_ERROR;                /* return failed flag */
}


/**************************************************************************
 * k2cb_skip_waitent:  changes status of given entry on waiting list      *
 *      to "skip" (by setting its '->skipflg' and deleting it from the    *
 *      waiting list)                                                     *
 *         stmnum  - logical stream number of entry to "skip"             *
 *         dataseq - data block sequence number of entry to "skip"        *
 *      return value:  returns K2R_POSITIVE if given waiting list entry   *
 *                       found and set to "skip";                         *
 *                     returns K2R_ERROR if not found                     *
 **************************************************************************/

int k2cb_skip_waitent(unsigned char stmnum, uint32_t dataseq)
{
  int wtpos,prevpos;
  s_circ_bblk *cbptr;

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */
  if ( (wtpos = cb_wait_top) != K2CB_DBFIDX_NONE && wtpos < cb_buff_size &&
       cb_circ_buff != NULL && cb_bfout_pos != cb_bfin_pos)
  {      /* waiting list top specifies valid entry and buffer not empty */
    prevpos = K2CB_DBFIDX_NONE;        /* init "previous" position value */
    do
    {    /* search through waiting list until matching entry found */
      cbptr = &cb_circ_buff[wtpos];    /* setup pointer to block */
      if (cbptr->stmnum == stmnum && cbptr->dataseq == dataseq)
      {       /* matching waiting list entry found */
        if (prevpos == K2CB_DBFIDX_NONE)    /* if no "previous" pos then */
          cb_wait_top = cbptr->nextwait;    /* move list-top to next entry */
        else  /* make "previous" position point to next waiting list entry */
          cb_circ_buff[prevpos].nextwait = cbptr->nextwait;
        cbptr->nextwait = K2CB_DBFIDX_NONE; /* no index of next waiting entry */
        cbptr->skipflg = 1;             /* set "skip" flag */
        cbptr->waitflg = 0;             /* clear "waiting" flag */
        ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
        return K2R_POSITIVE;       /* return flag indicating entry "skipped" */
      }
      prevpos = wtpos;       /* setup "previous" position for next iteration */
    }                           /* loop if another waiting list entry found */
    while ( (wtpos = cbptr->nextwait) != K2CB_DBFIDX_NONE &&
            wtpos < cb_buff_size);
  }                                                    
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2R_ERROR;     /* return flag indicating no matching entry found */
}


/**********************************************************************
 * k2cb_check_waits: checks to see if old `wait' entries have clogged *
 *      up the buffer, letting it get too full. If so, it changes all *
 *      the old wait entries (at the start of the wait list) into     *
 *      `skip' entries. This should allow the output thread to move   *
 *      some entries out of the buffer.                               *
 *     Assumes that the queue mutex is already locked for             *
 *  inputs: oldseq - oldest sequence number to wait for               *
 *      access. Logs any actions it takes.                            *
 *  Returns nothing.                                                  *
 **********************************************************************/
void k2cb_check_waits( uint32_t oldseq )
{
  s_circ_bblk *cbptr;
  int n_rem = 0;

  if ( cb_wait_top != K2CB_DBFIDX_NONE )
  {      /* waiting list top specifies valid entry */
    cbptr = &cb_circ_buff[cb_wait_top];    /* setup pointer to block */
    while (cbptr->dataseq < oldseq)
    {    /* for any with data seq too old; turn them into skip blocks */
      cb_wait_top = cbptr->nextwait;   /* link to the next one in the list */
      cbptr->nextwait = K2CB_DBFIDX_NONE; /* take this one out of the list */
      cbptr->skipflg = 1;              /* set "skip" flag */
      cbptr->waitflg = 0;              /* clear "waiting" flag */
      if(g_wait_count > 0)   /* if greater than zero then */
        --g_wait_count;      /* decrement # of wait blocks counter */
      ++g_skip_errcnt;       /* increment count of # of packets skipped */
      if(g_req_pend > 0)     /* if greater than zero then */
        --g_req_pend;        /* decrement resend requests counter */
      ++n_rem;               /* increment number-removed counter */
      if(gcfg_debug > 0 && oldseq < (uint32_t)-1)
      {       /* debug enabled and not clearing all wait blocks */
        logit("et","Packet data seq# too old (skipcount=%lu), "
              "skipping:  stm#=%d, dseq=%lu\n",g_skip_errcnt,
                                       (int)(cbptr->stmnum),cbptr->dataseq);
      }
      /* move list-top to next entry, if there is one */
      if (cb_wait_top == K2CB_DBFIDX_NONE)
        break;
      cbptr = &cb_circ_buff[cb_wait_top];  /* set up for the next one */
    }
    if(n_rem > 0)
    {    /* at least one waiting block skipped */
      if(oldseq < (uint32_t)-1)
      {       /* not clearing all wait blocks; log message */
        logit("et","Cleared %d wait entr%s with seq < %lu; %d remaining\n",
                          n_rem,((n_rem==1)?"y":"ies"),oldseq,g_wait_count);
      }
      else
      {       /* clearing all wait blocks; log message */
        logit("et","Cleared all (%d) wait entries\n",n_rem);
      }                                        
    }
  }
  return;
}


/**************************************************************************
 * k2cb_get_entry:  returns information on the circular buffer entry      *
 *      specified by the given ID number                                  *
 *         idnum      - block ID number of the given entry                *
 *         pstmnum    - ptr to variable to receive logical stream number  *
 *                      of returned entry                                 *
 *         pdataseq   - ptr to variable to receive data block sequence #  *
 *                      of returned entry                                 *
 *      return value:  returns K2R_POSITIVE if successful;                *
 *                     K2R_ERROR if 'idnum' does not identify a valid     *
 *                     circular buffer entry                              *
 **************************************************************************/

int k2cb_get_entry(int idnum,unsigned char *pstmnum,
                      uint32_t *pdataseq,int *ptickcnt,int *presendcnt)
{
  int rc;
  s_circ_bblk *cbptr;

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */
  if (idnum != K2CB_DBFIDX_NONE && idnum < cb_buff_size &&
                        cb_circ_buff != NULL && cb_bfout_pos != cb_bfin_pos)
  {      /* given block ID number is valid and circular buffer not empty */
    cbptr = &cb_circ_buff[idnum];      /* setup pointer to block */
    if (pstmnum != NULL)               /* if pointer not NULL then */
      *pstmnum = cbptr->stmnum;        /* enter logical stream # */
    if (pdataseq != NULL)              /* if pointer not NULL then */
      *pdataseq = cbptr->dataseq;      /* enter data block seq # */
    if (ptickcnt != NULL)
    {    /* ptr not NULL; if tick count in range enter it, else max val */
      *ptickcnt = (cbptr->timestamp <= (uint32_t)0xFFFF) ?
                                          (int)(cbptr->timestamp) : INT_MAX;
    }
    if (presendcnt != NULL)            /* if pointer not NULL then */
      *presendcnt = cbptr->msec;       /* enter resend-count value */
    rc = K2R_POSITIVE;                 /* return OK value */
  }
  else   /* no match for ID number */
    rc = K2R_ERROR;                    /* return error value */
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return rc;
}


/**************************************************************************
 * k2cb_get_waitent:  returns the matching entry on the waiting list      *
 *         stmnum  - logical stream number of desired wait entry          *
 *         dataseq - data block sequence number of desired wait entry     *
 *      return value:  returns block ID number for returned entry on      *
 *                     waiting list (for use with 'k2cb_incwt_resendcnt()'*
 *                     function) or 'K2CB_DBFIDX_NONE' if no matching     *
 *                     entry found                                        *
 **************************************************************************/

int k2cb_get_waitent(unsigned char stmnum,uint32_t dataseq)
{
  int wtpos;
  s_circ_bblk *cbptr;

  RequestSpecificMutex(&cb_mutex); /* wait for the mutex to be available */
  if((wtpos=cb_wait_top) != K2CB_DBFIDX_NONE && wtpos < cb_buff_size &&
                        cb_circ_buff != NULL && cb_bfout_pos != cb_bfin_pos)
  {      /* waiting list top specifies valid entry and buffer not empty */
    do
    {    /* search through waiting list until matching entry found */
      cbptr = &cb_circ_buff[wtpos];    /* setup pointer to block */
      if(cbptr->stmnum == stmnum && cbptr->dataseq == dataseq)
      {       /* matching waiting list entry found */
        ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
        return wtpos;        /* return flag indicating entry "skipped" */
      }
    }                        /* loop if another waiting list entry found */
    while((wtpos=cbptr->nextwait) != K2CB_DBFIDX_NONE &&
                                                      wtpos < cb_buff_size);
  }                                                    
  ReleaseSpecificMutex(&cb_mutex); /* let another thread have the mutex */
  return K2CB_DBFIDX_NONE;        /* indicate no matching entry found */
}


/* Print the contents of the circular buffer to a file; only for debugging */
void k2cb_dump_buf( )
{
  char filename[80];
  FILE *fd;
  int i;
  s_circ_bblk *cbptr;

  sprintf(filename, "%s.cbd", g_stnid);
  logit("e", "dumping CB to %s\n", filename);

  if ( (fd = fopen(filename, "w")) == (FILE *) NULL)
  {
    logit("e", "Error opening cb dump file\n");
    return;
  }

  fprintf(fd, "in %d out %d wait_top %d wait_count %d\n", cb_bfin_pos,
          cb_bfout_pos, cb_wait_top, g_wait_count);

  for (i = 0; i < cb_buff_size; i++)
  {
    cbptr = &cb_circ_buff[i];
    fprintf(fd, "%6d %3d %6d %9d %d %d %6d\n", i, (int)cbptr->stmnum,
            (int)cbptr->dataseq, (int)cbptr->timestamp, (int)cbptr->waitflg,
            (int)cbptr->skipflg, cbptr->nextwait);
  }
  fclose(fd);
  logit("e", "CB dump done\n");
  return;
}

