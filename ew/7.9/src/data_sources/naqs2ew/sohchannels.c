/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sohchannels.c 1134 2002-11-06 19:29:45Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2002/11/06 19:29:45  dietz
 *     Changed to handle new fields in NMX_CHANNEL_INFO struct
 *
 *     Revision 1.2  2002/07/09 18:13:11  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2002/03/15 23:10:09  dietz
 *     Initial revision
 *
 *
 */

/*
 * sohchannels.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "naqssoh.h"

void logit( char *, char *, ... );   /* logit.c sys-independent  */
int  compare_SOH_INFO( const void *s1, const void *s2 );
void Log_SOH_CHANNEL_INFO( NMX_CHANNEL_INFO *chinf, int nchan );
void Log_SOH_INFO( SOH_INFO *scn, int nscn, int flag );


/*********************************************************************
 * SelectSOHChannels()  compares a list of channels from NaqsServer  *
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
int SelectSOHChannels( NMX_CHANNEL_INFO *chinf, int nch,
                       SOH_INFO *req, int nreq )
{ 
   char sta[STATION_LEN+1];
   char chan[CHAN_LEN+1];
   int  usecount = 0;
   int  nmiss    = 0;
   int  ir, ic;

   Log_SOH_CHANNEL_INFO( chinf, nch );

/* Flag all requested channels as "not found"
 ********************************************/
   for( ir=0; ir<nreq; ir++ ) req[ir].flag = SOH_NOTAVAILABLE;

/* Loop over all channels available from NaqsServer
 **************************************************/
   for( ic=0; ic<nch; ic++ )
   {
      if( chinf[ic].subtype != NMX_SUBTYPE_SOH ) continue; 
      /* naqssoh only wants to see only SOH data */

      if( sscanf( chinf[ic].stachan, "%[^.].%s", sta, chan ) != 2 )
      {
          logit("e", "SelectSOHChannels: error parsing stachan <%s>\n",
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
          req[ir].flag = SOH_AVAILABLE;

       /* New channel, load its NMX_CHANNEL_INFO struct
        ************************************************/
          memcpy( &(req[ir].info), &(chinf[ic]), sizeof(NMX_CHANNEL_INFO) );
          usecount++;

      } /* end for all requested channels */

   } /*end for all available requested */

/* Sort the subscription list by chankey, print it out
 *****************************************************/
   qsort( req, nreq, sizeof(SOH_INFO), &compare_SOH_INFO );
   logit("o","\nSelectSOHChannels: naqssoh will subscribe to "
             "these %d channels:\n", usecount );
   Log_SOH_INFO( req, nreq, SOH_AVAILABLE );

/* Check for and log any requested channels 
   that are NOT available from this NaqsServer
 *********************************************/
   for( ir=0; ir<nreq; ir++ )  if( req[ir].flag == SOH_NOTAVAILABLE ) nmiss++;
   if( nmiss )
   {
      logit("o","\nSelectSOHChannels: %d requested channels not available "
                "from this NaqsServer:\n", nmiss );
      Log_SOH_INFO( req, nreq, SOH_NOTAVAILABLE );
   }

   return( usecount );
}

/*********************************************************************
 * FindSOHChan()  looks thru a list on SOH_INFO structures to find   *
 *   one that matches the given channel key.                         *
 *                                                                   *
 *   list      array of structures which holds all the channels      *
 *              to be searched thru (will not contain wildcards)     *
 *   nlist     number of channels in 'list'                          *
 *   chankey   channel key that you want to find in 'list'           *
 *                                                                   *
 *   Returns:  valid pointer to the matching SOH_INFO structure      *
 *                  on success                                       *
 *             NULL on failure  or no match                          *
 *********************************************************************/
SOH_INFO *FindSOHChan( SOH_INFO *list, int nlist, int chankey )
{
   SOH_INFO *pmatch, find;

   find.info.chankey = chankey;

   pmatch = (SOH_INFO *) bsearch( &find, list, (size_t)nlist,
                                  sizeof(SOH_INFO), compare_SOH_INFO );
   return( pmatch );
}


/*********************************************************************
 * compare_SOH_INFO()                                                *
 *   This function is passed to qsort() and bsearch().               *
 *   We use qsort() to sort the SOH_INFO struct by channel key, and  *
 *   we use bsearch() to look up a channel key in the SOH_INFO list. *
 *********************************************************************/
int compare_SOH_INFO( const void *s1, const void *s2 )
{
   SOH_INFO *ch1 = (SOH_INFO *) s1;
   SOH_INFO *ch2 = (SOH_INFO *) s2;

   if( ch1->info.chankey > ch2->info.chankey ) return  1;
   if( ch1->info.chankey < ch2->info.chankey ) return -1;
   return 0;
}


/*************************************************************************
 * Log_SOH_CHANNEL_INFO()  Write the NaqsServer SOH channels to logfile  *
 *************************************************************************/
void Log_SOH_CHANNEL_INFO( NMX_CHANNEL_INFO *chinf, int nchan )
{
   int i;

   logit("o","\nNaqsServer is serving the following SOH channels:\n");
   logit("o","      sta.chan   name             instr  ch type   chankey\n"
             "      --------   ----             -----  -- ----   -------\n" );
   for( i=0; i<nchan; i++ )
   {
      if( chinf[i].subtype != NMX_SUBTYPE_SOH ) continue;
      logit("e","%3d:  %-9s                  %s%03hd %3d   %1d   0x%08x\n",
            i+1, chinf[i].stachan, 
            chinf[i].inst.code, chinf[i].inst.sn, chinf[i].channel+1,
            chinf[i].subtype, chinf[i].chankey );
   }
   return;
}

/*********************************************************************
 * Log_SOH_INFO()  write SOH_INFO struct contents to logfile         *
 *********************************************************************/
void Log_SOH_INFO( SOH_INFO *scn, int nscn, int flag )
{
   int i;

   logit("o","      Earthworm\n"); 
   logit("o","      sta.chan   name             instr  ch type   chankey    del buf\n"
             "      --------   ----             -----  -- ----   -------    --- ---\n" );
   for( i=0; i<nscn; i++ )
   {
      if( scn[i].flag != flag ) continue;
      logit("e","%3d:  %-9s  %-15s %s%03hd %3d   %1d   0x%08x  %3d  %1d\n",
            i+1, scn[i].info.stachan, scn[i].name, 
            scn[i].info.inst.code, scn[i].info.inst.sn, 
            scn[i].info.channel+1, scn[i].info.subtype, scn[i].info.chankey,
            scn[i].delay, scn[i].sendbuffer );
   }
   return;
}
