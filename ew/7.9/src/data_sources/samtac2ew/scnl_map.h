/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: scnl_map.h 3537 2009-01-15 22:16:39Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2009/01/15 22:16:39  tim
 *     *** empty log message ***
 *
 *
 */

/* public SCN procs */
#ifndef SCNL_MAP_H                 /* process file only once per compile */
#define SCNL_MAP_H

#define SEED_SITE_MAX_CHAR 5
#define SEED_CHAN_MAX_CHAR 3
#define SEED_NET_MAX_CHAR 2
#define SEED_LOC_MAX_CHAR 2
 
#define SYS_STRM_COMBO_SIZE 20
 
typedef struct _SCN {
        char *System_id;
        char *Stream_id;
        char site[SEED_SITE_MAX_CHAR+1];        /* site name SEED */
        char chan[SEED_CHAN_MAX_CHAR+1];        /* channel name SEED */
        char net[SEED_NET_MAX_CHAR+1];          /* network code SEED */
        char loc[SEED_LOC_MAX_CHAR+1];  /* location code, to be used later */
        char comb[SYS_STRM_COMBO_SIZE];
        int pinno;                              /* EW PinNumber */
        struct _SCN *next;      /* linked list to next mapping */
} SCN; 

/* the two public functions */
int insertSCN( char * Sys, char *Strm, char * S, char *C, char *N);
int getSCN(char * Sys, char *Strm, SCN **scn);
int insertSCNL( char * Sys, char *Strm, char * S, char *C, char *N, char *L);
int getFirstSCN(char *Sys, SCN **scn);

#endif

