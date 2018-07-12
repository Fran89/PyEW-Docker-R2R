/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: channels.c 1396 2004-04-14 20:07:09Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2004/04/14 20:07:09  dietz
 *     modifications to support location code
 *
 *     Revision 1.4  2002/11/06 19:29:45  dietz
 *     Changed to handle new fields in NMX_CHANNEL_INFO struct
 *
 *     Revision 1.3  2002/07/09 18:13:11  dietz
 *     *** empty log message ***
 *
 *     Revision 1.2  2002/03/15 23:10:09  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/06/20 22:34:45  dietz
 *     Initial revision
 *
 *
 *
 */

/*
 * channels.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "naqs2ew.h"

void logit( char *, char *, ... );   /* logit.c sys-independent  */
int  compare_SCN_INFO( const void *s1, const void *s2 );
void Log_NMX_CHANNEL_INFO( NMX_CHANNEL_INFO *chinf, int nchan );
void Log_SCN_INFO( SCN_INFO *scn, int nscn, int flag );


/*********************************************************************
 * SelectChannels()  compares a list of channels from NaqsServer     *
 *   with the list of requested channels in the configuration file.  *
 *                                                                   *
 *   chinf    array of structures of channels available from         *
 *               NaqsServer                                          *
 *   nch      number of channels in 'chinf'                          *
 *   req      array of structures holding channels requested in the  *
 *              configuration file (may NOT contain wildcards)       *
 *   nreq     number of channels in 'req'                            *
 *                                                                   *
 *   Returns:  N >= 0 (# channels to be subscribed to) on success,   *
 *                 -1 on failure                                     *
 *********************************************************************/
int SelectChannels( NMX_CHANNEL_INFO *chinf, int nch,
                    SCN_INFO *req, int nreq )
{
   char sta[STATION_LEN+1];
   char chan[CHAN_LEN+1];
   int  usecount = 0;
   int  nmiss    = 0;
   int  ir, ic;

   Log_NMX_CHANNEL_INFO( chinf, nch );

/* Flag all requested channels as "not found"
 ********************************************/
   for( ir=0; ir<nreq; ir++ ) req[ir].flag = SCN_NOTAVAILABLE;

/* Loop over all channels available from NaqsServer
 **************************************************/
   for( ic=0; ic<nch; ic++ )
   {
      if( chinf[ic].subtype != NMX_SUBTYPE_TIMESERIES ) continue;   
        /* naqs2ew only wants to see only waveform data */

      if( sscanf( chinf[ic].stachan, "%[^.].%s", sta, chan ) != 2 )
      {
          logit("e", "SelectChannels: error parsing stachan <%s>\n",
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
          req[ir].flag = SCN_AVAILABLE;

       /* New channel, load its NMX_CHANNEL_INFO struct
        ************************************************/
          memcpy( &(req[ir].info), &(chinf[ic]), sizeof(NMX_CHANNEL_INFO) );
          usecount++;

      } /* end for all requested channels */

   } /*end for all available requested */

/* Sort the subscription list by chankey, print it out
 *****************************************************/
   qsort( req, nreq, sizeof(SCN_INFO), &compare_SCN_INFO );
   logit("o","\nSelectChannels: naqs2ew will subscribe to these %d channels:\n",
              usecount );
   Log_SCN_INFO( req, nreq, SCN_AVAILABLE );

/* Check for and log any requested channels 
   that are NOT available from this NaqsServer
 *********************************************/
   for( ir=0; ir<nreq; ir++ )  if( req[ir].flag == SCN_NOTAVAILABLE ) nmiss++;
   if( nmiss )
   {
      logit("o","\nSelectChannels: %d requested channels not available "
                "from this NaqsServer:\n", nmiss );
      Log_SCN_INFO( req, nreq, SCN_NOTAVAILABLE );
   }

   return( usecount );
}

/*********************************************************************
 * FindChannel()  looks thru a list on SCN_INFO structures to find   *
 *   one that matches the given channel key.                         *
 *                                                                   *
 *   list      array of structures which holds all the channels      *
 *              to be searched thru (will not contain wildcards)     *
 *   nlist     number of channels in 'list'                          *
 *   chankey   channel key that you want to find in 'list'           *
 *                                                                   *
 *   Returns:  valid pointer to the matching SCN_INFO structure      *
 *                  on success                                       *
 *             NULL on failure  or no match                          *
 *********************************************************************/
SCN_INFO *FindChannel( SCN_INFO *list, int nlist, int chankey )
{
   SCN_INFO *pmatch, find;

   find.info.chankey = chankey;

   pmatch = (SCN_INFO *) bsearch( &find, list, (size_t)nlist,
                                  sizeof(SCN_INFO), compare_SCN_INFO );
   return( pmatch );
}


/*********************************************************************
 * compare_SCN_INFO()                                                *
 *   This function is passed to qsort() and bsearch().               *
 *   We use qsort() to sort the SCN_INFO struct by channel key, and  *
 *   we use bsearch() to look up a channel key in the SCN_INFO list. *
 *********************************************************************/
int compare_SCN_INFO( const void *s1, const void *s2 )
{
   SCN_INFO *ch1 = (SCN_INFO *) s1;
   SCN_INFO *ch2 = (SCN_INFO *) s2;

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
   logit("o","     sta.chan.net.loc  pinno   instr  ch  type    chankey\n"
             "     ----------------  -----   -----  --  ----    -------\n" );
   for( i=0; i<nchan; i++ )
   {
      logit("e","%3d: %-12s             %s%03hd  %2d    %1d    0x%08x\n",
            i+1, chinf[i].stachan, 
            chinf[i].inst.code, chinf[i].inst.sn, chinf[i].channel+1,
            chinf[i].subtype, chinf[i].chankey );
   }
   return;
}

/*********************************************************************
 * Log_SCN_INFO()  write SCN_INFO struct contents to logfile         *
 *********************************************************************/
void Log_SCN_INFO( SCN_INFO *scn, int nscn, int flag )
{
   char str[20];
   int i;

   logit("o","      Earthworm\n"); 
   logit("o","     sta.chan.net.loc  pinno   instr  ch  type    chankey  delay  fmt  buf\n"
             "     ----------------  -----   -----  --  ----    -------  -----  ---  ---\n" );
   for( i=0; i<nscn; i++ )
   {
      if( scn[i].flag != flag ) continue;
      sprintf( str, "%s.%s.%s.%s", scn[i].sta, scn[i].chan, scn[i].net, scn[i].loc );
      logit("e","%3d: %-12s     %5d  %s%03hd  %2d    %1d   0x%08x  %3d    %2d    %1d\n",
            i+1, str, scn[i].pinno, 
            scn[i].info.inst.code, scn[i].info.inst.sn, scn[i].info.channel+1,
            scn[i].info.subtype,  scn[i].info.chankey,
            scn[i].delay, scn[i].format, scn[i].sendbuffer );
   }
   return;
}
