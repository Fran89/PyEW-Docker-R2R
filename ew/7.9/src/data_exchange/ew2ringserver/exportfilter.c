
/*
 *   exportfilter.c:
 *   This file started as scnfilter.c (from the export module)
 *
 *    Revision history:
 *     Revision 2013/06/15 Chad Trabant
 *     - rename source file to exportfilter.c to differentiate from original
 *     - fix error messages to report proper function names (e.g. scnlfilter -> exportfilter)
 *     - add support for MSEED message types including remapping, but no MaxLatency support
 *
 *     Revision 1.17  2009/01/16 23:00:16  dietz
 *     Added support for filtering TYPE_STRONGMOTIONII messages based on SCNL
 *
 *     Revision 1.16  2005/11/16 23:58:29  dietz
 *     Modified debug statements
 *
 *     Revision 1.15  2005/09/21 18:36:43  dietz
 *     Fixed bug in exportfilter() which caused segmentation fault when handling
 *     TYPE_TRACEBUF or TYPE_TRACE_COMP_UA messages in either MaxLatency or
 *     remap SCNL mode.
 *
 *     Revision 1.14  2005/04/26 21:20:40  dietz
 *     Added timestamping to logging in exportfilter_logmsg()
 *
 *     Revision 1.13  2005/04/22 17:15:23  dietz
 *     added msglen arg to exportfilter_logmsg() to correct logging of msgs.
 *     modified to use rdpickcoda.c functions to interpret all picks/codas.
 *
 *     Revision 1.12  2005/03/23 19:13:01  dietz
 *     Added more logging in "Verbose" mode using the new function
 *     exportfilter_logmsg().
 *
 *     Revision 1.11  2004/04/21 17:45:24  dietz
 *     included ctype.h for isdigit()
 *
 *     Revision 1.10  2004/04/20 22:52:52  dietz
 *     moved export_actv.c and its sample configs from subdir export_actv into subdir export to eliminated duplication of export filter code
 *
 *     Revision 1.9  2004/04/16 23:25:46  dietz
 *     modified to handle msgs with location code fields
 *
 *     Revision 1.8  2002/10/29 19:21:13  dietz
 *     Modified to use bsearch to match SCNs when no wildcards are
 *     present in the requested channel list.  Increases efficiency
 *     for long channel lists!
 *
 *     Revision 1.7  2002/07/19 23:00:39  dietz
 *     Merged in priority features from scnprifilter.c
 *
 *     Revision 1.6  2002/07/11 00:31:39  dietz
 *     typo fix.
 *
 *     Revision 1.5  2002/06/11 17:22:18  dietz
 *     Added Send_scn_remap command to rename trace data on the fly.
 *     Added TYPE_TRACE_COMP_UA as a message type it can process.
 *     Moved Latency check to after SCN check.
 *
 *     Revision 1.4  2002/03/21 17:27:30  dhanych
 *     fixed bug in exportfilter() that caused exception on termination 
 *     ('=' changed to 'memcpy')
 *
 *     Revision 1.3  2001/01/31 16:58:39  alex
 *     first cut at fixing MaxLatency bug. Alex 1/31/1
 *
 *     Revision 1.2  2001/01/02 16:56:14  lucky
 *     Implemented MaxLatency parameter to reject sending old data
 *
 *     Revision 1.2  2000/12/07 21:23:11  whitmore
 *     Add MaxLatency check
 *
 *     Revision 1.1  2000/02/14 17:23:11  lucky
 *     Initial revision
 */

/*
 *  scnfilter.c contains a function to filter messages based
 *               on content of site-component-network-loc fields.
 *               Works on TYPE_TRACEBUF, TYPE_TRACE_COMP_UA,
 *               TYPE_PICK2K, TYPE_CODA2K, messages.  
 *
 *               It can also rename channels in waveform data
 *               if desired.
 *              
 *               Returns: 1 if this site-component-network-loc
 *                          is on the list of requested scnl's.
 *                        0 if it isn't.
 *                       -1 if it's an unknown message type.
 *
 *   981112 Lynn Dietz
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <time_ew.h>
#include <earthworm.h>
#include <trace_buf.h>
#include <kom.h>
#include <swap.h>
#include <priority_queue.h>
#include <rdpickcoda.h>

#include <libmseed.h>

#include "exportfilter.h"

/* Globals for site-comp-net-loc filter
 **************************************/
int   FilterInit    = 0;     /* initialization flag                    */
int   Max_SCNL      = 0;     /* working limit on # scnl's to ship      */
int   nSCNL         = 0;     /* # of scnl's we're configured to ship   */
int   MaxLatencySec = 0;     /* In seconds, latency filter, in .d file */
int   UsePriority   = 0;     /* counter of non-default priorities      */
int   nWild         = 0;     /* # of Wildcards used in config file     */
char *Wild          = "*";   /* wildcard string for SCNL               */

#define INCREMENT_SCNL 10                /* increment the limit of # scnl's    */
#define STATION_LEN    TRACE2_STA_LEN-1  /* max string-length of station code  */
#define CHAN_LEN       TRACE2_CHAN_LEN-1 /* max string-length of component code*/
#define NETWORK_LEN    TRACE2_NET_LEN-1  /* max string-length of network code  */
#define LOC_LEN        TRACE2_LOC_LEN-1  /* max string-length of location code */

typedef struct {             
   char sta[STATION_LEN+1];   /* original station name  */
   char chan[CHAN_LEN+1];     /* original channel name  */
   char net[NETWORK_LEN+1];   /* original network name  */
   char loc[LOC_LEN+1];       /* original location code */
   char rsta[STATION_LEN+1];  /* remapped station name  */
   char rchan[CHAN_LEN+1];    /* remapped channel name  */
   char rnet[NETWORK_LEN+1];  /* remapped network name  */
   char rloc[LOC_LEN+1];      /* remapped location code */
   int  remap;                /* flag: 1=remap SCNL, 0=don't */
   EW_PRIORITY pri;           /* 20020319 dbh added    */
} SCNLstruct;

SCNLstruct *Send = NULL;       /* list of SCNL's to accept & rename   */

/* Message types this filter can handle
 **************************************/
unsigned char TypeTraceBuf;  
unsigned char TypeTraceComp; 
unsigned char TypePick2K;
unsigned char TypeCoda2K;
unsigned char TypeTraceBuf2;   /* with Loc */
unsigned char TypeTraceComp2;  /* with Loc */
unsigned char TypePickSCNL;    /* with Loc */
unsigned char TypeCodaSCNL;    /* with Loc */
unsigned char TypeStrongMotionII;
unsigned char TypeMseed;

/* Local function prototypes & macros
 ************************************/
char  *copytrim( char *, char *, int );
double WaveMsgTime( TRACE2_HEADER * );
int    CompareSCNLs( const void *s1, const void *s2 );  /* qsort & bsearch */

#define NotWild(x)     strcmp((x),Wild) 
#define NotMatch(x,y)  strcmp((x),(y))

/*********************************************************
 * exportfilter_com() processes config-file commands to  *
 *               set up the filter & send-list.          *
 * Returns  1 if the command was recognized & processed  *
 *          0 if the command was not recognized          *
 * Note: this function may exit the process if it finds  *
 *       serious errors in any commands                  *
 *********************************************************/
int exportfilter_com(void)
{
   char   *str;
   size_t  size;

/* Add an entry to the send-list without remapping
 *************************************************/
   if( k_its("Send_scnl") ) 
   {
        int iarg; 

   	if( nSCNL >= Max_SCNL )
        {
            Max_SCNL += INCREMENT_SCNL;
            size     = Max_SCNL * sizeof( SCNLstruct );
            Send     = (SCNLstruct *) realloc( Send, size );
            if( Send == NULL )
            {
               logit( "e",
                      "exportfilter_com: Error allocating %d bytes"
                      " for SCNL list; exiting!\n", size );
               exit( -1 );
            }
        }
        for( iarg=1; iarg<=4; iarg++ )   /* Read original SCNL   */
        {
	   str = k_str();
           if( !str ) break;             /* no string; error!   */
	   if( iarg==1 )  /* original station code */ 
	   {
              if( strlen(str)>(size_t)STATION_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].sta,str);
              continue;
           }
	   if( iarg==2 ) /* original component code */
	   {
              if( strlen(str)>(size_t)CHAN_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
	      strcpy(Send[nSCNL].chan,str);
              continue;
           }
	   if( iarg==3 ) /* original network code */
	   {
              if( strlen(str)>(size_t)NETWORK_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].net,str);
              continue;
           }
	   if( iarg==4 ) /* original location code */
	   {
              if( strlen(str)>(size_t)LOC_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].loc,str);

           /* check for optional priority; 20020319 dbh */
              str = k_str();

           /* no additional token, use default priority */
              if( k_err() || !str || !isdigit(str[0]) )
              {
                 Send[nSCNL].pri = EW_PRIORITY_DEF;
              }
           /* found another token, convert it to int,
            * expecting atoi() to return zero if not valid number */
              else
              {
                 EW_PRIORITY pri = (EW_PRIORITY)atoi( str );
                 iarg++;
                 if( pri < EW_PRIORITY_MIN || EW_PRIORITY_MAX < pri )
                 {
                    logit("e","exportfilter_com: Priority must be between %d and %d.\n",
                           EW_PRIORITY_MIN, EW_PRIORITY_MAX );
                    break;
                 }
                 else
                 {
                    Send[nSCNL].pri = (EW_PRIORITY)pri;
                    UsePriority++;
                 }
              }
           }
           Send[nSCNL].remap = 0;
           nSCNL++;
           return( 1 );
	}
      
        logit( "e", "exportfilter_com: Error in <Send_scnl> command "
                    "(argument %d either missing, too long, or invalid): "
                    "\"%s\"\n", iarg, k_com() );
        logit( "e", "exportfilter_com: exiting!\n" );
        exit( -1 );
   }

/* Add an entry to the send-list without remapping
 * LEGACY: original SCN command (use wildcard for Loc)
 *****************************************************/
   if( k_its("Send_scn") ) 
   {
        int iarg; 

   	if( nSCNL >= Max_SCNL )
        {
            Max_SCNL += INCREMENT_SCNL;
            size     = Max_SCNL * sizeof( SCNLstruct );
            Send     = (SCNLstruct *) realloc( Send, size );
            if( Send == NULL )
            {
               logit( "e",
                      "exportfilter_com: Error allocating %d bytes"
                      " for SCNL list; exiting!\n", size );
               exit( -1 );
            }
        }
        for( iarg=1; iarg<=3; iarg++ )   /* Read original SCN   */
        {
	   str = k_str();
           if( !str ) break;             /* no string; error!   */
	   if( iarg==1 )  /* original station code */ 
	   {
              if( strlen(str)>(size_t)STATION_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].sta,str);
              continue;
           }
	   if( iarg==2 ) /* original component code */
	   {
              if( strlen(str)>(size_t)CHAN_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
	      strcpy(Send[nSCNL].chan,str);
              continue;
           }
	   if( iarg==3 ) /* original network code */
	   {
              if( strlen(str)>(size_t)NETWORK_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].net,str);

           /* assign wildcard to location code */
              strcpy(Send[nSCNL].loc,Wild);
              nWild++;

           /* check for optional priority; 20020319 dbh */
              str = k_str();

           /* no additional token, use default priority */
              if( k_err() || !str || !isdigit(str[0]) )
              {
                 Send[nSCNL].pri = EW_PRIORITY_DEF;
              }
           /* found another token, convert it to int,
            * expecting atoi() to return zero if not valid number */
              else
              {
                 EW_PRIORITY pri = (EW_PRIORITY)atoi( str );
                 iarg++;
                 if( pri < EW_PRIORITY_MIN || EW_PRIORITY_MAX < pri )
                 {
                    logit("e","exportfilter_com: Priority must be between %d and %d.\n",
                           EW_PRIORITY_MIN, EW_PRIORITY_MAX );
                    break;
                 }
                 else
                 {
                    Send[nSCNL].pri = (EW_PRIORITY)pri;
                    UsePriority++;
                 }
              }
           }
           Send[nSCNL].remap = 0;
           nSCNL++;
           return( 1 );
	}
      
        logit( "e", "exportfilter_com: Error in <Send_scn> command "
                    "(argument %d either missing, too long, or invalid): "
                    "\"%s\"\n", iarg, k_com() );
        logit( "e", "exportfilter_com: exiting!\n" );
        exit( -1 );
   }

/* Add an entry to the send-list with remapping
 **********************************************/
   if( k_its("Send_scnl_remap") ) 
   {
        int iarg;

   	if( nSCNL >= Max_SCNL )
        {
            Max_SCNL += INCREMENT_SCNL;
            size     = Max_SCNL * sizeof( SCNLstruct );
            Send     = (SCNLstruct *) realloc( Send, size );
            if( Send == NULL )
            {
               logit( "e",
                      "exportfilter_com: Error allocating %d bytes"
                      " for SCNL list; exiting!\n", size );
               exit( -1 );
            }
        }
        for( iarg=1; iarg<=8; iarg++ )
        {
	   str=k_str();
           if( !str ) break;             /* no string; error!   */
	   if( iarg==1 )  /* original station code */ 
	   {
              if( strlen(str)>(size_t)STATION_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
	      strcpy(Send[nSCNL].sta,str);
              continue;
           }
	   if( iarg==2 ) /* original component code */
	   {
              if( strlen(str)>(size_t)CHAN_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
	      strcpy(Send[nSCNL].chan,str);
              continue;
           }
	   if( iarg==3 ) /* original network code */
	   {
              if( strlen(str)>(size_t)NETWORK_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].net,str);
              continue;
           }
	   if( iarg==4 ) /* original location code */
	   {
              if( strlen(str)>(size_t)LOC_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].loc,str);
              continue;
           }
	   if( iarg==5 )  /* remap station code */ 
	   {
              if( strlen(str)>(size_t)STATION_LEN ) break;
	      strcpy(Send[nSCNL].rsta,str);
              continue;
           }
	   if( iarg==6 ) /* remap component code */
	   {
              if( strlen(str)>(size_t)CHAN_LEN ) break;
	      strcpy(Send[nSCNL].rchan,str);
              continue;
           }
	   if( iarg==7 ) /* remap network code */
	   {
              if( strlen(str)>(size_t)NETWORK_LEN ) break;
	      strcpy(Send[nSCNL].rnet,str);
              continue;
           }
	   if( iarg==8 ) /* remap location code */
	   {
              if( strlen(str)>(size_t)LOC_LEN ) break;
	      strcpy(Send[nSCNL].rloc,str);
 
           /* check for optional priority; 20020319 dbh */
              str=k_str();

           /* no additional token, use default priority */
              if( k_err() || !str || !isdigit(str[0]))
              {
                 Send[nSCNL].pri = EW_PRIORITY_DEF;
              }
           /* found another token, convert it to int,
            * expecting atoi() to return zero if not valid number */
              else
              {
                 EW_PRIORITY pri = (EW_PRIORITY)atoi( str );
                 iarg++;
                 if( pri < EW_PRIORITY_MIN || EW_PRIORITY_MAX < pri )
                 {
                    logit("e","exportfilter_com: Priority must be between %d and %d.\n",
                           EW_PRIORITY_MIN, EW_PRIORITY_MAX );
                    break;
                 }
                 else
                 {
                    Send[nSCNL].pri = (EW_PRIORITY)pri;
                    UsePriority++;
                 }
              }
           }
           Send[nSCNL].remap = 1;
           nSCNL++;
           return( 1 );
	}
      
        logit( "e", "exportfilter_com: Error in <Send_scnl_remap> command "
                    "(argument %d either missing, too long, or invalid): "
                    "\"%s\"\n", iarg, k_com() );
        logit( "e", "exportfilter_com: exiting!\n" );
        exit( -1 );
   }

/* Add an entry to the send-list with remapping
 * LEGACY: original SCN command (use wildcard for Loc)
 *****************************************************/
   if( k_its("Send_scn_remap") ) 
   {
        int iarg;

   	if( nSCNL >= Max_SCNL )
        {
            Max_SCNL += INCREMENT_SCNL;
            size     = Max_SCNL * sizeof( SCNLstruct );
            Send     = (SCNLstruct *) realloc( Send, size );
            if( Send == NULL )
            {
               logit( "e",
                      "exportfilter_com: Error allocating %d bytes"
                      " for SCNL list; exiting!\n", size );
               exit( -1 );
            }
        }
        for( iarg=1; iarg<=6; iarg++ )
        {
	   str=k_str();
           if( !str ) break;             /* no string; error!   */
	   if( iarg==1 )  /* original station code */ 
	   {
              if( strlen(str)>(size_t)STATION_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
	      strcpy(Send[nSCNL].sta,str);
              continue;
           }
	   if( iarg==2 ) /* original component code */
	   {
              if( strlen(str)>(size_t)CHAN_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
	      strcpy(Send[nSCNL].chan,str);
              continue;
           }
	   if( iarg==3 ) /* original network code */
	   {
              if( strlen(str)>(size_t)NETWORK_LEN ) break;
              if( !NotWild(str) ) nWild++;  /* count SCNL wildcards */
              strcpy(Send[nSCNL].net,str);
 
              strcpy(Send[nSCNL].loc,Wild); /* use Wildcard for loc */
              nWild++;
              continue;
           }
	   if( iarg==4 )  /* remap station code */ 
	   {
              if( strlen(str)>(size_t)STATION_LEN ) break;
	      strcpy(Send[nSCNL].rsta,str);
              continue;
           }
	   if( iarg==5 ) /* remap component code */
	   {
              if( strlen(str)>(size_t)CHAN_LEN ) break;
	      strcpy(Send[nSCNL].rchan,str);
              continue;
           }
	   if( iarg==6 ) /* remap network code */
	   {
              if( strlen(str)>(size_t)NETWORK_LEN ) break;
	      strcpy(Send[nSCNL].rnet,str);

              strcpy(Send[nSCNL].rloc,Wild); /* use Wildcard for rloc */
 
           /* check for optional priority; 20020319 dbh */
              str=k_str();

           /* no additional token, use default priority */
              if( k_err() || !str || !isdigit(str[0]))
              {
                 Send[nSCNL].pri = EW_PRIORITY_DEF;
              }
           /* found another token, convert it to int,
            * expecting atoi() to return zero if not valid number */
              else
              {
                 EW_PRIORITY pri = (EW_PRIORITY)atoi( str );
                 iarg++;
                 if( pri < EW_PRIORITY_MIN || EW_PRIORITY_MAX < pri )
                 {
                    logit("e","exportfilter_com: Priority must be between %d and %d.\n",
                           EW_PRIORITY_MIN, EW_PRIORITY_MAX );
                    break;
                 }
                 else
                 {
                     Send[nSCNL].pri = (EW_PRIORITY)pri;
                    UsePriority++;
                 }
              }
           }
           Send[nSCNL].remap = 1;
           nSCNL++;
           return( 1 );
	}
      
        logit( "e", "exportfilter_com: Error in <Send_scn_remap> command "
                    "(argument %d either missing, too long, or invalid): "
                    "\"%s\"\n", iarg, k_com() );
        logit( "e", "exportfilter_com: exiting!\n" );
        exit( -1 );
   }


/* Optional command: Read max latency allowed for 
   waveform shipment (read minutes, store seconds)
 *************************************************/
   else if( k_its("MaxLatency") )
   {
      MaxLatencySec = 60 * k_int();
      if( MaxLatencySec < 0 ) {
         logit("e", "exportfilter_com: Invalid MaxLatency value: %d  "
                    "(must be >= 0); exiting!\n", MaxLatencySec/60 );
         exit( -1 );
      }
      return( 1 );
   }

   return( 0 );
}


/***********************************************************
 * exportfilter_init()   Make sure all the required        *
 * commands were found in the config file, do any other    *
 * startup things necessary for filter to work properly    *
 ***********************************************************/
int exportfilter_init(void)
{
   int i;

/* Check to see if required config-file commands were processed
 **************************************************************/
   if( nSCNL == 0 ) {
      logit( "e", "exportfilter_init: No <Send_scnl> or <Send_scnl_remap> "
                  "commands in config file; no data will be exported!\n" );
      return( -1 );
   }

/* Look up message types of we can deal with
   *****************************************/
   if ( GetType( "TYPE_TRACEBUF", &TypeTraceBuf ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_TRACEBUF>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_TRACE_COMP_UA", &TypeTraceComp ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_TRACE_COMP_UA>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_PICK2K", &TypePick2K ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_PICK2K>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_CODA2K", &TypeCoda2K ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_CODA2K>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTraceBuf2 ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_TRACEBUF2>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_TRACE2_COMP_UA", &TypeTraceComp2 ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_TRACE2_COMP_UA>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_PICK_SCNL", &TypePickSCNL ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_PICK_SCNL>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_CODA_SCNL", &TypeCodaSCNL ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_CODA_SCNL>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeStrongMotionII ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_STRONGMOTIONII>\n" );
      return( -1 );
   }
   if ( GetType( "TYPE_MSEED", &TypeMseed ) != 0 ) {
      logit( "e",
             "exportfilter_init: Invalid message type <TYPE_MSEED>\n" );
      return( -1 );
   }
                    
/* Sort and Log list of SCNL's that we're handling
 ************************************************/
   if( nWild==0 ) {
      qsort( Send, nSCNL, sizeof(SCNLstruct), CompareSCNLs );
      logit("o", "exportfilter_init: no wildcards in requested channel list;\n"
                 "                 will use binary search for SCNL matching.\n" );
   } else {
      logit("o", "exportfilter_init: wildcards present in requested channel list;\n"
                 "                 must use linear search for SCNL matching.\n" );
   }

   logit("o", "exportfilter_init: configured to ship %d channels:", nSCNL );
   for( i=0; i<nSCNL; i++ ) {
      logit("o","\n    channel[%d]: %5s %3s %2s %2s",
             i, Send[i].sta, Send[i].chan, Send[i].net, Send[i].loc );
      if( Send[i].remap ) {
          logit("o","   mapped to %5s %3s %2s %2s",
                Send[i].rsta, Send[i].rchan, Send[i].rnet, Send[i].rloc );
      } 
      if( UsePriority ) {
          logit("o","   priority %d", Send[i].pri );
      }
   }
   logit("o","\n" );

/* Log MaxLatency parameter
 **************************/
   if( MaxLatencySec > 0 ) {
      logit("o","exportfilter_init: Maximum allowable data latency: %d minutes\n", 
             MaxLatencySec/60 );
   } else {
      logit("o","exportfilter_init: Tracebuf packets will be exported "
                "regardless of latency.\n"
                "                 To filter based on time, add "
                "MaxLatency to config file.\n");    
   }

   FilterInit = 1;
   return( 0 );
}


/**********************************************************
 * exportfilter() looks at the candidate message.         *
 *   returns: the priority if SCNL is in "send" list      *   
 *            otherwise EW_PRIORITY_NONE (0) if not found *
 *                                                        *
 *   20020319 dbh Changed the return code handling        *
 **********************************************************/
int exportfilter( char  *inmsg,  long inlen,   unsigned char  intype, 
                  char **outmsg, long *outlen, unsigned char *outtype )
{
   TRACE2_HEADER *thd2;         /* header pntr to use for all trace data */
   SCNLstruct     key;          /* Key for binary search       */
   SCNLstruct    *match;        /* Pointer into Send-list      */
   double         packettime;   /* starttime of data in packet */

   if( !FilterInit ) exportfilter_init();

   thd2 = (TRACE2_HEADER *)inmsg;

/* Read SCNL of a TYPE_TRACEBUF2 or TYPE_TRACE2_COMP_UA message
 **************************************************************/
   if( intype == TypeTraceBuf2   ||
       intype == TypeTraceComp2    )
   {
      copytrim( key.sta,  thd2->sta,  STATION_LEN );
      copytrim( key.chan, thd2->chan, CHAN_LEN    );
      copytrim( key.net,  thd2->net,  NETWORK_LEN );
      copytrim( key.loc,  thd2->loc,  LOC_LEN     );
   }

/* Read SCN of a TYPE_TRACEBUF or TYPE_TRACE_COMP_UA message
 ***********************************************************/
   else if( intype == TypeTraceBuf  ||
            intype == TypeTraceComp   )
   {
      copytrim( key.sta,  thd2->sta,       STATION_LEN );
      copytrim( key.chan, thd2->chan,      CHAN_LEN    );
      copytrim( key.net,  thd2->net,       NETWORK_LEN );
      copytrim( key.loc,  LOC_NULL_STRING, LOC_LEN     );
   }

/* Read SCNL of a TYPE_PICK_SCNL, TYPE_PICK2K 
 ********************************************/
   else if( intype == TypePickSCNL || 
            intype == TypePick2K      )
   {
      EWPICK pick;
      int    rc;
 
      if( intype == TypePickSCNL ) rc = rd_pick_scnl( inmsg, inlen, &pick );
      if( intype == TypePick2K )   rc = rd_pick2k(    inmsg, inlen, &pick );

      if( rc != EW_SUCCESS ) {
         logit("t","exportfilter: error reading SCNL from msg (type %d):\n%s\n",
               (int)intype, inmsg );
         return( EW_PRIORITY_NONE );
      }
      strcpy( key.sta,  pick.site );
      strcpy( key.chan, pick.comp );
      strcpy( key.net,  pick.net  );
      strcpy( key.loc,  pick.loc  );
   }

/* Read SCNL of a TYPE_CODA_SCNL, TYPE_CODA2K 
 ********************************************/
   else if( intype == TypeCodaSCNL || 
            intype == TypeCoda2K      )
   {
      EWCODA coda;
      int    rc;

      if( intype == TypeCodaSCNL ) rc = rd_coda_scnl( inmsg, inlen, &coda );
      if( intype == TypeCoda2K )   rc = rd_coda2k(    inmsg, inlen, &coda );

      if( rc != EW_SUCCESS ) {
         logit("t","exportfilter: error reading SCNL from msg (type %d):\n%s\n",
               (int)intype, inmsg );
         return( EW_PRIORITY_NONE );
      }
      strcpy( key.sta,  coda.site );
      strcpy( key.chan, coda.comp );
      strcpy( key.net,  coda.net  );
      strcpy( key.loc,  coda.loc  );         
   }

/* Read first SCNL of a TYPE_STRONGMOTIONII  
 ******************************************/
   else if( intype == TypeStrongMotionII )
   {
      int rc;

      rc = sscanf( inmsg, "SCNL: %[^.].%[^.].%[^.].%s", 
                   key.sta, key.chan, key.net, key.loc );
      if( rc != 4 ) {
         logit("t","exportfilter: error reading SCNL from msg (type %d):\n%s\n",
               (int)intype, inmsg );
         return( EW_PRIORITY_NONE );
      }
   } 

/* Read SCNL of a TYPE_MSEED
 ******************************************/
   else if( intype == TypeMseed )
   {
     struct fsdh_s *fsdh = (struct fsdh_s *)inmsg;

     ms_strncpcleantail (key.net, fsdh->network, 2);
     ms_strncpcleantail (key.sta, fsdh->station, 5);
     ms_strncpcleantail (key.loc, fsdh->location, 2);
     ms_strncpcleantail (key.chan, fsdh->channel, 3);
   }

/* Or else we don't know how to read this type of message
 ********************************************************/
   else 
   {
   /* logit("e","exportfilter: rejecting msgtype:%d - cannot read SCNL\n",
            intype ); */   /*DEBUG*/
      return( EW_PRIORITY_NONE );
   }

/* Look for the message's SCNL in the Send-list.
 ***********************************************/
   match = (SCNLstruct *) NULL;

/* Use the more efficient binary search if there
   were no wildcards in the original SCNL request list. */
   if( nWild==0 ) {
      match = (SCNLstruct *)bsearch( &key, Send, nSCNL, 
                                     sizeof(SCNLstruct), CompareSCNLs );
   } 

/* Gotta do it the linear way if wildcards were used! */
   else {
      int i;
      for( i=0; i<nSCNL; i++ )
      {
         SCNLstruct *next = &(Send[i]);
         if( NotWild(next->sta)  && NotMatch(next->sta, key.sta)  ) continue;
         if( NotWild(next->chan) && NotMatch(next->chan,key.chan) ) continue;
         if( NotWild(next->net)  && NotMatch(next->net, key.net)  ) continue;
         if( NotWild(next->loc)  && NotMatch(next->loc, key.loc)  ) continue;
         match = next;  /* found a match! */
         break;
      }
   }

   if( match == NULL ) {
   /* logit("e","exportfilter: rejecting msgtype:%d from %s %s %s %s\n",
            intype, key.sta, key.chan, key.net, key.loc); */   /*DEBUG*/
      return( EW_PRIORITY_NONE );   /* SCNL not in Send list */
   }

/* Found a message we want to ship! 
   Do some extra checks for trace data only 
 ******************************************/
   if( intype == TypeTraceBuf2  ||
       intype == TypeTraceComp2 || 
       intype == TypeTraceBuf   ||
       intype == TypeTraceComp    )
   {
   /* Reject message with excessive latency */
      if( MaxLatencySec > 0 )
      { 
         packettime = WaveMsgTime( thd2 );
         if( packettime < 0) {
            logit("","exportfilter: Unknown datatype %s in traceheader\n",
                      thd2->datatype );
            return( EW_PRIORITY_NONE );
         }
         if( packettime < (double)(time(NULL) - MaxLatencySec) ) {
            return( EW_PRIORITY_NONE );
         }
      }

   /* Rename its SCNL if appropriate */
      if( match->remap ) {
         if( NotWild(match->rsta)  )    strcpy( thd2->sta,  match->rsta  );
         if( NotWild(match->rchan) )    strcpy( thd2->chan, match->rchan );
         if( NotWild(match->rnet)  )    strcpy( thd2->net,  match->rnet  );
         if( NotWild(match->rloc)  &&
            (intype==TypeTraceBuf2 ||
             intype==TypeTraceComp2  )) strcpy( thd2->loc,  match->rloc  );
      }
   }
   else if( intype == TypeMseed )
   {
     struct fsdh_s *fsdh = (struct fsdh_s *) inmsg;

      if( match->remap ) {
	if( NotWild(match->rsta)  ) ms_strncpopen (fsdh->station,  match->rsta, 5);
	if( NotWild(match->rchan) ) ms_strncpopen (fsdh->channel,  match->rchan, 3);
	if( NotWild(match->rnet)  ) ms_strncpopen (fsdh->network,  match->rnet, 2);
	if( NotWild(match->rloc)  ) ms_strncpopen (fsdh->location, match->rloc, 2);
      }
   }

/* Copy message to output buffer
 *******************************/
   memcpy( *outmsg, inmsg, inlen );
  *outlen  = inlen;
  *outtype = intype;

/* logit("e","exportfilter: accepting msgtype:%d from %s %s %s %s\n",
           intype, key.sta, key.chan, key.net, key.loc); */   /*DEBUG*/

   return( match->pri );
}

/**********************************************************
 * exportfilter_logmsg()  simple logging of message       *
 **********************************************************/
void exportfilter_logmsg( char *msg, int msglen,
                          unsigned char msgtype, char *note )
{
   TRACE2_HEADER *thd2 = (TRACE2_HEADER *)msg;
   char tmpstr[100];
   double tstart;

   if( msgtype == TypeTraceBuf2  ||
       msgtype == TypeTraceComp2    )
   {
      tstart = WaveMsgTime(thd2);
      datestr23( tstart, tmpstr, sizeof(tmpstr) );
      logit("t","%s t%d %s.%s.%s.%s %s\n",
            note, (int)msgtype,
            thd2->sta, thd2->chan, thd2->net, thd2->loc, tmpstr );
   }
   else if( msgtype == TypeTraceBuf  ||
            msgtype == TypeTraceComp    )
   {
      tstart = WaveMsgTime(thd2);
      datestr23( tstart, tmpstr, sizeof(tmpstr) );
      logit("t","%s t%d %s.%s.%s %s\n",
            note, (int)msgtype,
            thd2->sta, thd2->chan, thd2->net, tmpstr );   
   }
   else 
   {
      int endstr = (int)sizeof(tmpstr)-1;
      if( msglen < endstr ) endstr = msglen;
      strncpy( tmpstr, msg, endstr );
      tmpstr[endstr] = 0;
      logit("t","%s t%d %s\n",
            note, (int)msgtype, tmpstr );
   }
   return;
}


/**********************************************************
 * exportfilter_shutdown()  frees allocated memory and    *
 *         does any other cleanup stuff                   *
 **********************************************************/
void exportfilter_shutdown(void)
{
   free( Send );
   return;
}


/**********************************************************
 * copytrim()  copies n bytes from one string to another, *
 *   trimming off any leading and trailing blank spaces   *
 **********************************************************/
char *copytrim( char *str2, char *str1, int n )
{
   int i, len;

 /*printf( "copy %3d bytes of str1: \"%s\"\n", n, str1 );*/ /*DEBUG*/

/* find first non-blank char in str1 (trim off leading blanks) 
 *************************************************************/
   for( i=0; i<n; i++ ) if( str1[i] != ' ' ) break;

/* copy the remaining number of bytes to str2
 ********************************************/
   len = n-i;
   strncpy( str2, str1+i, len );
   str2[len] = '\0';
 /*printf( "  leading-trimmed str2: \"%s\"\n", str2 );*/ /*DEBUG*/

/* find last non-blank char in str2 (trim off trailing blanks) 
 *************************************************************/
   for( i=len-1; i>=0; i-- ) if( str2[i] != ' ' ) break;
   str2[i+1] = '\0';
 /*printf( " trailing-trimmed str2: \"%s\"\n\n", str2 );*/ /*DEBUG*/

   return( str2 );
}

/********************************************************************
*  WaveMsgTime  Return value of starttime from a tracebuf header    *
*               Returns -1. if unknown data type,                   *
*********************************************************************/
 
double WaveMsgTime( TRACE2_HEADER* wvmsg )
{
   char   byteOrder;
   char   dataSize;
   double packettime;  /* time from trace_buf header */
 
/* See what sort of data it carries
 **********************************/
   byteOrder = wvmsg->datatype[0];
   dataSize  = wvmsg->datatype[1];

/* Return now if we don't know this message type
 ***********************************************/
   if( byteOrder != 'i'  &&  byteOrder != 's' ) return(-1.);
   if( dataSize  != '2'  &&  dataSize  != '4' ) return(-1.);
 
   packettime = wvmsg->starttime;
#if defined( _SPARC )
   if( byteOrder == 'i' ) SwapDouble( &packettime );
 
#elif defined( _INTEL )
   if( byteOrder == 's' ) SwapDouble( &packettime );

#else
   logit("", "WaveMsgTime warning: _INTEL and _SPARC are both undefined." );
   return( -1. );
#endif

   return packettime;
}


/*************************************************************
 *                       CompareSCNLs()                      *
 *                                                           *
 *  This function is passed to qsort() and bsearch().        *
 *  We use qsort() to sort the station list by SCNL numbers, *
 *  and we use bsearch to look up an SCNL in the list if no  *
 *  wildcards were used in the requested channel list        *
 *************************************************************/
 
int CompareSCNLs( const void *s1, const void *s2 )
{
   int rc;
   SCNLstruct *t1 = (SCNLstruct *) s1;
   SCNLstruct *t2 = (SCNLstruct *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return rc;

   rc = strcmp( t1->chan, t2->chan );
   if ( rc != 0 ) return rc;

   rc = strcmp( t1->net,  t2->net );
   if ( rc != 0 ) return rc;

   rc = strcmp( t1->loc,  t2->loc );
   return rc;
}
