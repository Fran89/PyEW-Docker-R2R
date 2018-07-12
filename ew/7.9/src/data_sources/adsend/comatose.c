/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: comatose.c 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

       /***********************************************************
        *                       comatose.c                        *
        *                                                         *
        *  If adsend hasn't been broadcasting data for more than  *
        *  TimeoutNoSend seconds, the system is declared dead     *
        *  and the NoSend() function returns TRUE.  Otherwise,    *
        *  NoSend() returns FALSE.                                *
        *                                                         *
        *  NoTimeSynch() works like NoSend(), except that it      *
        *  monitors how long time code synch has been lost.       *
        ***********************************************************/

#include <time.h>
#include "irige.h"

#define TRUE  1
#define FALSE 0

extern int TimeoutNoSend;                  // Config file parameter
extern int TimeoutNoSynch;                 // Config file parameter


                   /****************************************
                    *               NoSend()               *
                    ****************************************/

int NoSend( int now_sending )
{
   static int    first = TRUE;
   static int    was_sending = FALSE;
   static time_t time_stopped_sending;

/* This feature disabled
   *********************/
   if ( TimeoutNoSend == 0 )
      return FALSE;

/* First time through
   ******************/
   if ( first )
   {
      if ( now_sending )
         was_sending = TRUE;
      else
      {
         was_sending = FALSE;
         time( &time_stopped_sending );
      }
      first = FALSE;
      return FALSE;
   }

/* The system is currently sending
   *******************************/
   if ( now_sending )
   {
      was_sending = TRUE;
      return FALSE;
   }

/* The system is not currently sending
   ***********************************/
   else
   {
       if ( was_sending )
       {
          was_sending = FALSE;
          time( &time_stopped_sending );
          return FALSE;
       }
       else
       {
          time_t current_time;
          time( &current_time );
          if ( (current_time - time_stopped_sending) >=  TimeoutNoSend )
          {
             time_stopped_sending = current_time;
             return TRUE;
          }
          else
             return FALSE;
       }
   }
}


                   /****************************************
                    *            NoTimeSynch()             *
                    ****************************************/

int NoTimeSynch( int IrigeStatus )
{
   static int    first = TRUE;
   static int    was_synched = FALSE;
   static time_t time_lost_synch;

/* This feature disabled
   *********************/
   if ( TimeoutNoSynch == 0 )
      return FALSE;

/* First time through
   ******************/
   if ( first )
   {
      if ( IrigeStatus == TIME_OK )
         was_synched = TRUE;
      else
      {
         was_synched = FALSE;
         time( &time_lost_synch );
      }
      first = FALSE;
      return FALSE;
   }

/* The time is currently synched
   *****************************/
   if ( IrigeStatus == TIME_OK )
   {
      was_synched = TRUE;
      return FALSE;
   }

/* The time is not currently synched
   *********************************/
   else
   {
       if ( was_synched )
       {
          was_synched = FALSE;
          time( &time_lost_synch );
          return FALSE;
       }
       else
       {
          time_t current_time;
          time( &current_time );
          if ( (current_time - time_lost_synch) >=  TimeoutNoSynch )
          {
             time_lost_synch = current_time;
             return TRUE;
          }
          else
             return FALSE;
       }
   }
}
