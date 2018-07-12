
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: serchannels.c 1199 2003-02-14 19:46:36Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2003/02/14 19:46:36  dietz
 *     Initial revision
 *
 *     Revision 1.1  2003/02/13 16:06:45  whitmore
 *     Added functions for transparent serial data - based on channels.c
 *
 *
 */

/*
 * channels.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "naqsser.h"

void logit( char *, char *, ... );   /* logit.c sys-independent  */
int  compare_SER_INFO( const void *s1, const void *s2 );
void Log_NMX_CHANNEL_INFO( NMX_CHANNEL_INFO *chinf, int nchan );
void Log_SER_INFO( SER_INFO *scn, int nscn, int flag );

/*********************************************************************
 * SelectSerChannels()  compares a list of serial channels from      *
 *   NaqsServer with the list of requested channels in the           *
 *   configuration file.                                             *
 *                                                                   *
 *   chinf    array of structures of channels available from         *
 *               NaqsServer                                          *
 *   nch      number of channels in 'chinf'                          *
 *   req      array of structures holding serial channels requested  *
 *              in the configuration file (may NOT contain wildcards)*
 *   nreq     number of channels in 'req'                            *
 *                                                                   *
 *   Returns:  N >= 0 (# channels to be subscribed to) on success,   *
 *                 -1 on failure                                     *
 *********************************************************************/
int SelectSerChannels( NMX_CHANNEL_INFO *chinf, int nch,
                       SER_INFO *req, int nreq )
{
   char sta[STATION_LEN+1];
   char chan[CHAN_LEN+1];
   int  usecount = 0;
   int  nmiss    = 0;
   int  ir, ic;

   Log_NMX_CHANNEL_INFO( chinf, nch );

/* Flag all requested channels as "not found"
 ********************************************/
   for( ir=0; ir<nreq; ir++ ) req[ir].flag = SER_NOTAVAILABLE;

/* Loop over all channels available from NaqsServer
 **************************************************/
   for( ic=0; ic<nch; ic++ )
   {
      if( chinf[ic].subtype != NMX_SUBTYPE_TRANSSERIAL ) continue;   
        /* naqsser only wants to see only transparent serial data */

      if( sscanf( chinf[ic].stachan, "%[^.].%s", sta, chan ) != 2 )
      {
          logit("e", "SelectSerChannels: error parsing stachan <%s>\n",
                         chinf[ic].stachan );
          continue;
      }

   /* Does this one match any of the requested channels?
    ****************************************************/
      for( ir=0; ir<nreq; ir++ )
      {
       /* Match sta,chan codes to those supplied in config file 
        *******************************************************/
          if( strcmp( req[ir].sta,  sta )  != 0 ) continue;
          if( strcmp( req[ir].chan, chan ) != 0 ) continue;
          req[ir].flag = SER_AVAILABLE;

       /* New channel, load its NMX_CHANNEL_INFO struct
        ************************************************/
          memcpy( &(req[ir].info), &(chinf[ic]), sizeof(NMX_CHANNEL_INFO) );
          usecount++;

      } /* end for all requested channels */

   } /*end for all available requested */

/* Sort the subscription list by chankey, print it out
 *****************************************************/
   qsort( req, nreq, sizeof(SER_INFO), &compare_SER_INFO );
   logit("o","\nSelectSerChannels: naqs2ew will subscribe to these %d channels:\n",
              usecount );
   Log_SER_INFO( req, nreq, SER_AVAILABLE );

/* Check for and log any requested serial channels 
   that are NOT available from this NaqsServer
 *********************************************/
   for( ir=0; ir<nreq; ir++ )  if( req[ir].flag == SER_NOTAVAILABLE ) nmiss++;
   if( nmiss )
   {
      logit("o","\nSelectSerChannels: %d requested channels not available "
                "from this NaqsServer:\n", nmiss );
      Log_SER_INFO( req, nreq, SER_NOTAVAILABLE );
   }

   return( usecount );
}

/*********************************************************************
 * FindSerChannel()  looks thru a list on SER_INFO structures to find*
 *   one that matches the given channel key.                         *
 *                                                                   *
 *   list      array of structures which holds all the channels      *
 *              to be searched thru (will not contain wildcards)     *
 *   nlist     number of channels in 'list'                          *
 *   chankey   channel key that you want to find in 'list'           *
 *                                                                   *
 *   Returns:  valid pointer to the matching SER_INFO structure      *
 *                  on success                                       *
 *             NULL on failure  or no match                          *
 *********************************************************************/
SER_INFO *FindSerChannel( SER_INFO *list, int nlist, int chankey )
{
   SER_INFO *pmatch, find;

   find.info.chankey = chankey;

   pmatch = (SER_INFO *) bsearch( &find, list, (size_t)nlist,
                                  sizeof(SER_INFO), compare_SER_INFO );
   return( pmatch );
}

/*********************************************************************
 * compare_SER_INFO()                                                *
 *   This function is passed to qsort() and bsearch().               *
 *   We use qsort() to sort the SER_INFO struct by channel key, and  *
 *   we use bsearch() to look up a channel key in the SER_INFO list. *
 *********************************************************************/
int compare_SER_INFO( const void *s1, const void *s2 )
{
   SER_INFO *ch1 = (SER_INFO *) s1;
   SER_INFO *ch2 = (SER_INFO *) s2;

   if( ch1->info.chankey > ch2->info.chankey ) return  1;
   if( ch1->info.chankey < ch2->info.chankey ) return -1;
   return 0;
}


/*************************************************************************
 * Log_NMX_CHANNEL_INFO()  Write the NaqsServer channel list to logfile  *
 *************************************************************************/
void Log_NMX_CHANNEL_INFO( NMX_CHANNEL_INFO *chinf, int nchan )
{
   int i;

   logit("o","\nNaqsServer is serving the following %d channels:\n", nchan );
   logit("o","     sta.chan.net  pinno   instr  ch  type    chankey\n"
             "     ------------  -----   -----  --  ----    -------\n" );
   for( i=0; i<nchan; i++ )
   {
      logit("e","%3d: %-12s         %s%03hd  %2d    %1d    0x%08x\n",
            i+1, chinf[i].stachan, 
            chinf[i].inst.code, chinf[i].inst.sn, chinf[i].channel+1,
            chinf[i].subtype, chinf[i].chankey );
   }
   return;
}

/*********************************************************************
 * Log_SER_INFO()  write SER_INFO struct contents to logfile         *
 *********************************************************************/
void Log_SER_INFO( SER_INFO *scn, int nscn, int flag )
{
   char str[20];
   int i;

   logit("o","      Earthworm\n"); 
   logit("o","     sta.chan.net  pinno   instr  ch  type    chankey   chankey  delay  buf\n"
             "     ------------  -----   -----  --  ----    -------   -------  -----  ---\n" );
   for( i=0; i<nscn; i++ )
   {
      if( scn[i].flag != flag ) continue;
      sprintf( str, "%s.%s.%s", scn[i].sta, scn[i].chan, scn[i].net );
      logit("e","%3d: %-12s  %5d  %s%03hd  %2d    %1d   0x%08x  %ld  %3d    %1d\n",
            i+1, str, scn[i].pinno, 
            scn[i].info.inst.code, scn[i].info.inst.sn, scn[i].info.channel+1,
            scn[i].info.subtype,  scn[i].info.chankey, scn[i].info.chankey,
            scn[i].delay, scn[i].sendbuffer );
   }
   return;
}
