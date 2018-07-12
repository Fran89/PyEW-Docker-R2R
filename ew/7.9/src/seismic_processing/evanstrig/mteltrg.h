
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mteltrg.h 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/21 22:32:23  dietz
 *     added location code; inputs TYPE_TRACEBUF2, outputs TYPE_LPTRIG_SCNL
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */

/* FILE: mteltrg.h                              (D. Tottingham  05/12/90)

This is an include file of defines, data structure definitions, and
external declarations that are common in the mteltrg module.

*/

#ifndef _MTELTRG_
#define _MTELTRG_


/************************************************************************
                               INCLUDES

************************************************************************/
#include <earthworm.h>       /* needed for 'int32_t' type on Windows */
#include "mconst.h"


/************************************************************************
 *                             TYPEDEFS                                 *
 ************************************************************************/
typedef long   ST_TIME;   /* Stamp GMT time in seconds before or after Jan 1,
                             1970, resolution is one second */
typedef double MS_TIME;   /* GMT time in seconds before or after Jan 1,
                             1970, resolution finer than microseconds */


/************************************************************************
 *                        STRUCTURE DEFINITIONS                         *
 ************************************************************************/

/* TEL_STATIDENT:  Station identification.  Used below.                      */

typedef struct {             /* station component identifier                 */
   char   network[4];        /* network name                                 */
   char   st_name[5];        /* name of station where equipment is located   */
   char   component;         /* component v,n,e                              */
   short  inst_type;         /* instrument type                              */
} TEL_STATIDENT;


/* TEL_TRIGGER:  Teleseimic event detector trigger statistic                 */

typedef struct {
   TEL_STATIDENT tr_name;    /* station component identification             */
   float   tlta;             /* current Trigger band LTA level               */
   float   tsta;             /* current Trigger band STA level               */
   float   tstav;            /* current Trigger band STAV level              */
   long    dur;              /* current event's duration through this moment */
   char    durflg;           /* TRUE while actively measuring duration       */
   char    bigev;            /* TRUE if tth2 hit during this trigger sequence*/
   float   mlta;             /* current Masking band LTA level               */
   float   msta;             /* current Masking band STA level               */
   float   mstav;            /* current Masking band STAV level              */
   char    astat;            /* TRUE if A clock was on at 1st threshold Xing */
   long    aclk;             /* current A clock time remaining               */
   long    bclk;             /* current B clock time remaining               */
   long    cclk;             /* current C clock time remaining               */
   long    eclk;             /* current E clock time remaining               */
   long    sclk;             /* current S clock time remaining               */
   float   dc;               /* low-passed (DC) part of input trace          */
   char    first;            /* TRUE when first exceeding tth1               */
   short   trig_value;       /* value of trigger level (eta)                 */
   short   num_trigs;        /* number of times triggered during this buffer */
   MS_TIME trig_time;        /* time of FIRST trigger during this buffer     */
   char    event_type;       /* Type of trigger (N = normal; B = big)        */
   char    spareN[3];        /* spare                                        */
} TEL_TRIGGER;


/* TEL_TRIGSETTING:  Settings for teleseismic trigger system.
                     These get set by tel_initialize_params()                */

typedef struct {
   char    netwname[4];      /* network name                                 */
   MS_TIME beginttime;       /* time these values come into effect           */
   float   ctlta;            /* Coefficient of Trigger band LTA              */
   float   ctsta;            /* Coefficient of Trigger band STA              */
   float   ctstav;           /* Coefficient of Trigger band STAV             */
   float   tth1;             /* Trigger band THreshold 1 (normal event)      */
   float   tth2;             /* Trigger band THreshold 2 (big event)         */
   float   tminen;           /* Trigger band MINimum ENergy                  */
   long    mndur1;           /* trigger band MiNimum DURation 1 (normal event)*/
   long    mndur2;           /* trigger band MiNimum DURation 2 (big event)  */
   float   cmlta;            /* Coefficient of Masking band LTA              */
   float   cmsta;            /* Coefficient of Masking band STA              */
   float   cmstav;           /* Coefficient of Masking band STAV             */
   float   mth;              /* Masking band THreshold                       */
   float   mminen;           /* Masking band MINimum ENergy                  */
   long    aset;             /* run time of A clock ("microseism" or "short-cycling")*/
   long    bset;             /* run time of B clock (trigger band "quiet")   */
   long    cset;             /* run time of C clock (bset + masking band "quiet") */
   long    eset;             /* run time of E clock (delay in effect of aclk)*/
   long    sset;             /* run time of S clock (startup settling period)*/
   float   dccoef;           /* DC-removal filter COEFficient (1-pole IIR)   */
   char    algorithm;        /* triggering algorithm used: t=teltrg          */
   char    spareO[3];        /* spare                                        */
} TEL_TRIGSETTING;

typedef struct {               /* was in mqueue.h */
     int     pin;              /* Earthworm pin number                   */
     int32_t dec_buf[31];      /* 31-pt buffer for input convert & save  */
     int32_t lp_buf[21];       /* 20-sps decimated-trace buffer          */
     short   i_dec_buf;        /* dec_buf cyclic; i_dec_buf -> youngest  */
     short   i_lp_buf;         /* lp_buf cyclic; i_lp_buf -> youngest    */
     short   need_for_dec_buf; /* # of points needed to refresh dec_buf  */
     TEL_TRIGGER info;
} TEL_CHANNEL;


/************************************************************************
                         EXTERNAL DECLARATIONS

  These functions can be called from all modules that include this file.
 ************************************************************************/
FLAG tel_energy_valid_on_channel( TEL_CHANNEL *, unsigned *, unsigned );
TEL_TRIGSETTING *tel_get_trigsetting();
void tel_initialize_channel( TEL_CHANNEL * );
void tel_initialize_params( void );
void tel_set_aset( double );
void tel_set_bset( double );
void tel_set_cmlta( double );
void tel_set_cmsta( double );
void tel_set_cmstav( double );
void tel_set_cset( double );
void tel_set_ctlta (double);
void tel_set_ctsta (double);
void tel_set_ctstav (double);
void tel_set_dccoef (double);
void tel_set_eset (double);
void tel_set_mminen (long);
void tel_set_mndur1 (double);
void tel_set_mndur2 (double);
void tel_set_mth (double);
void tel_set_sset (double);
void tel_set_tminen (long);
void tel_set_tth1 (double);
void tel_set_tth2 (double);
long tel_triggered_on_channel( TEL_CHANNEL *, short *, int, int, int );
long tel_triggered_on_tracebuf( TEL_CHANNEL *, int32_t *, int );

#endif
