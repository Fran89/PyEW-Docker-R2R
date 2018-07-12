
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: doit.c 1722 2005-01-24 21:17:53Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2005/01/24 21:17:53  dietz
 *     Added TRIG_VERSION definition to track version of output message.
 *
 *     Revision 1.4  2004/05/24 20:37:38  dietz
 *     Added location code; uses TYPE_LPTRIG_SCNL as input,
 *     outputs TYPE_TRIGLIST_SCNL.
 *
 *     Revision 1.3  2003/02/27 23:25:51  dietz
 *     Changed comment in TYPE_TRIGLIST2K msg and log from
 *     Sta/Net/Cmp to Sta/Cmp/Net to match the actual order of the fields.
 *
 *     Revision 1.2  2000/07/09 17:53:34  lombard
 *     Deleted unused variable TypeError; moved misplaced return(0) in Assoc().
 *
 *     Revision 1.1  2000/02/14 17:15:33  lucky
 *     Initial revision
 *
 *
 */

    /****************************************************************
     *                             doit.c                           *
     *                                                              *
     *    The long-period trigger associator is implemented here    *
     ****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <earthworm.h>         
#include "list.h"


            /*********************************************
             *          Public Global Variables          *
             *********************************************/
extern unsigned char  MyInstId;         /* Local installation */
extern unsigned char  MyModuleId;       /* Our module id */
extern unsigned char  TypeTrigListSCNL; /* message type we'll create */
extern int            TriggerTimeLimit; /* Purge triggers after this time */
extern int            CriticalNu;       /* Min sta count for event detection */
extern int            CriticalMu;       /* Min sta count for event continuation */
extern int            PreEventTime;     /* pre-event portion (s) of event record */
extern int            MinEventTime;     /* record length (s) of "normal" event */
extern int            MaxEventTime;     /* record length (s) of "big" event */

   
            /*********************************************
             *           Local Global Variables          *
             *********************************************/
static char OutMsg[MAX_BYTES_PER_EQ];   /* buffer for building TypeTrigListSCNL msg */

            /*********************************************
             *            Function declarations          *
             *********************************************/
int    DecodeTrg( char *, TRIGGER * );                 /* local        */
int    BuildTrigListSCNL( char *, int );               /* local        */
void   SendToRing( unsigned char, char *, long );      /* evansassoc.c */
long   GetEventID( void );                             /* evansassoc.c */
int    AppendExtraSCNL( char *, int, double, int );    /* evansassoc.c */
void   Time_DtoS( double, char * );                    /* Time_DtoS.c  */
double GetEarliestTriggerTime( void );                 /* list.c       */
int    EventIsBig( void );                             /* list.c       */
int    AppendSavedList( char *, int , double, int );   /* list.c       */


   /******************************************************************
    *                             Assoc()                            *
    *                                                                *
    *                  Process one trigger message                   *
    *                                                                *
    * Arguments:                                                     *
    *    trgmsg = String containing one trigger message              *
    ******************************************************************/

int Assoc( char *trgmsg )
{
   static int  event_active = 0;
   static int  max_sta;
   int         n_sta;
   TRIGGER     trg;
   int         rc;

/* Decode variables from the trigger string
   ****************************************/
   if( DecodeTrg( trgmsg, &trg ) != 0 ) return -1;

/* Append the new trigger to the trigger list
   ******************************************/
   AppendTrg( trg );

/* Purge all old triggers from the trigger list
   ********************************************/
   PurgeList( trg.time - TriggerTimeLimit );

/* Count the number of unique stations in the trigger list
   *******************************************************/
   n_sta = CountSta();

/* An event has just become active
   *******************************/
   if ( !event_active )
   {
      if ( n_sta >= CriticalNu )
      {
         event_active = 1;
         max_sta = n_sta;
         SaveList();               /* Make a copy of the trigger list.     */
      }                            /* This will be used by LogSavedList(). */
      return 0;
   }

/* The event is still active
   *************************/
   if ( event_active )
   {
      if ( n_sta >= CriticalMu )
      {
         if ( max_sta < n_sta )
         {
            max_sta = n_sta;
            SaveList();
         }
      }

/* The event has just terminated
   *****************************/
      else
      {
         rc = BuildTrigListSCNL( OutMsg, MAX_BYTES_PER_EQ );
         SendToRing( TypeTrigListSCNL, OutMsg, strlen( OutMsg ) );

/*
         char event_time[23];

         GetEarliestTriggerTime( event_time );

         ProdT
         logit( "e", "\n%s EVENT DETECTED     %22s\n\n", 
                 TRIG_VERSION, event_time );
         logit( "e", "Sta/Cmp/Net/Loc  Date   Time\n" );
         logit( "e", "---------------  ------ ---------------\n" );
         LogSavedList();
         logit( "e", "\n" );
 */

         event_active = 0;
      }
   }
   return 0;
}


   /*********************************************************************
    *                           DecodeTrg()                             *
    *********************************************************************/

int DecodeTrg( char *trgmsg, TRIGGER *trg )
{
   int nr;

   nr = sscanf( trgmsg, "%d %d %d %hd %s %s %s %s %lf %c",
                &trg->MsgType,
                &trg->ModId,
                &trg->InstId,
                &trg->pin,
                trg->sta,
                trg->comp,
                trg->net,
                trg->loc,
                &trg->time,
                &trg->event_type );

 /*logit( "", "%d %d %d %hd %s %s %s %s %.3lf %c\n",
           (int) trg->MsgType,  (int) trg->ModId, (int) trg->InstId, 
           trg->pin, trg->sta, trg->comp, trg->net, trg->loc,
           trg->time, trg->event_type );*/ /*DEBUG*/

   if( nr != 10 ) 
   {
     logit("t","Error decoded only %d of 10 fields from trigger msg:\n%s", 
                nr, trgmsg );
     return -1;
   }       
   return  0;   
}


   /*********************************************************************
    *                         BuildTrigListSCNL()                       *
    *             Build the output message (TYPE_TRIGLIST_SCNL)         *
    *********************************************************************/

int BuildTrigListSCNL( char *msg, int msglen )
{
   char   event_time[25];
   double tevent, ton;
   long   eventid;
   int    duration;
   int    rc1,rc2;

/* Get event time, id & ontime, duration of seismogram
   ***************************************************/
   tevent = GetEarliestTriggerTime();
   if( tevent == 0.0 ) 
   {
      logit("et", "BuildTrigListSCNL: trigger list is empty;"
                  " cannot build a triglist message!\n" );
      return( -1 );
   }
   eventid = GetEventID();
   ton     = tevent - (double) PreEventTime;
   if( EventIsBig() )  duration = MaxEventTime;
   else                duration = MinEventTime;


/* Build & log the TYPE_TRIGLIST_SCNL header
   *****************************************/
   Time_DtoS( tevent, event_time );
   sprintf( msg,
           "%s EVENT DETECTED    %s "
           "EVENT ID: %lu AUTHOR: %03u%03u%03u\n\n",
            TRIG_VERSION, event_time, eventid,
            TypeTrigListSCNL, MyModuleId, MyInstId );
   logit( "", "%s", msg );

   strcat( msg, "Sta/Cmp/Net/Loc  Date   Time                       " 
          "start save       duration in sec.\n" );
   logit( "",   "Sta/Cmp/Net/Loc  Date   Time                       " 
          "start save       duration in sec.\n" );
   strcat( msg, "---------------  ------ --------------- "
          "   -----------------------------------------\n" );
   logit( "",   "---------------  ------ --------------- "
          "   -----------------------------------------\n" );

/* Tack on the individual station triggers
   ***************************************/
   rc1 = AppendSavedList( msg, msglen, ton, duration );
   if( rc1 != 0 )
   {
      logit("et", "BuildTrigListSCNL: buffer overflow; %d station triggers"
                  " not included in msg for eventid %ld; see log file for"
                  " complete list\n", rc1, eventid );
   }

/* Tack on any extra channels
 ****************************/
   rc2 = AppendExtraSCNL( msg, msglen, ton, duration );
   if( rc2 != 0 )
   {
      logit("et", "BuildTrigListSCNL: buffer overflow; %d extra SCNLs"
                  " not included in msg for eventid %ld\n",
                    rc2, eventid );
   }

   logit("", "\n" ); /* add blankline to log so wave_viewer is happy */


   return( rc1+rc2 );
}
