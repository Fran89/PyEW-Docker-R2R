
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: list.c 1510 2004-05-24 20:37:38Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/24 20:37:38  dietz
 *     Added location code; uses TYPE_LPTRIG_SCNL as input,
 *     outputs TYPE_TRIGLIST_SCNL.
 *
 *     Revision 1.1  2000/02/14 17:15:33  lucky
 *     Initial revision
 *
 *
 */

         /*****************************************************
          *                    File list.c                    *
          *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include "list.h"

static LINK head = NULL;
static LINK head_save = NULL;

void Time_DtoS( double, char * );


   /*********************************************************************
    *                            AppendTrg()                            *
    *               Append a trigger to the trigger list                *
    *********************************************************************/

void AppendTrg( TRIGGER trg )
{
   LINK p = head;

   if ( head == NULL )
   {
      head = (LINK) malloc( sizeof(ELEMENT) );
      head->trg = trg;
      head->next = NULL;
   }
   else
   {
      while ( p->next != NULL )
         p = p->next;

      p->next = (LINK) malloc( sizeof(ELEMENT) );
      p = p->next;
      p->trg = trg;
      p->next = NULL;
   }
   return;
}


   /*********************************************************************
    *                            DeleteTrg()                            *
    *              Delete one trigger from the trigger list             *
    *********************************************************************/

LINK DeleteTrg( LINK p_in )
{
   LINK p = p_in;
   LINK q;

   if ( p == NULL )
   {
      q = head;
      head = head->next;
   }
   else
   {
      q = p->next;
      p->next = q->next;
   }
   free( q );
   return p;
}


   /*********************************************************************
    *                            PurgeList()                            *
    *            Purge all old triggers from the trigger list           *
    *********************************************************************/

void PurgeList( double min_time )
{
   LINK p = NULL;
   LINK q = head;

   while ( q != NULL )
   {
      if ( q->trg.time < min_time )
         p = DeleteTrg( p );
      else
         p = q;
      q = q->next;
   }
   return;
}


   /*********************************************************************
    *                            CountSta()                             *
    *         Count the number of stations in the trigger list          *
    *                                                                   *
    *  First, set all flags to 1.  Then, search the remainder of the    *
    *  for triggers with the same sta/net codes.  If any are found,     *
    *  set their flags to 0.                                            *
    *  Returns the number of unique stations in the list.               *
    *********************************************************************/

int CountSta( void )
{
   int n_sta = 0;
   LINK p = head;

/* Set all trigger flags to 1
   **************************/
   while ( p != NULL )
   {
      p->trg.flag = 1;
      p = p->next;
   }

/* Search for triggers with flags set to 1.
   Then, set all flags with this sta/net code to zero.
   ***************************************************/
   p = head;
   while ( p != NULL )
   {
      if ( p->trg.flag == 1 )       /* We found one! */
      {
         LINK q = p;

         while ( q != NULL )        /* Set flag to 0 for this sta/net */
         {
          /*if ( p->trg.pin == q->trg.pin ) */        /*unique test on pins*/
            if( strcmp(p->trg.sta, q->trg.sta)==0 &&  /*unique test on sta/net*/
                strcmp(p->trg.net, q->trg.net)==0   )
            {
               q->trg.flag = 0;
            }
            q = q->next;
         }
         n_sta++;                   /* Increment station count */
      }
      p = p->next;
   }
   return n_sta;
}


   /*********************************************************************
    *                             SaveList()                            *
    *                   Save the entire trigger list                    *
    *********************************************************************/

void SaveList( void )
{
   LINK p, q, r;

/* Get rid of the old trigger list
   *******************************/
   if ( head_save == NULL )
      while ( head_save != NULL )
      {
         p = head_save;
         head_save = p->next;
         free( p );
      }

/* Save the first trigger node
   ***************************/
   if ( head == NULL ) return;
   head_save = (LINK) malloc( sizeof( ELEMENT) );
   head_save->trg = head->trg;
   head_save->next = NULL;

/* Save the remaining trigger nodes
   ********************************/
   p = head->next;
   q = head_save;
   while ( p != NULL )
   {
      r = (LINK) malloc( sizeof( ELEMENT) );
      r->trg = p->trg;
      r->next = NULL;
      q->next = r;
      p = p->next;
      q = q->next;
   }
   return;
}


   /*********************************************************************
    *                         AppendSavedList()                         *
    *   Append the entire trigger list to the TYPE_TRIGLIST_SCNL msg    *
    *   Returns: 0 if all went well,                                    *
    *            N the number of triggers that were not included in msg *
    *              when msglen is too small for entire SavedList.       *
    *********************************************************************/

int AppendSavedList( char *msg, int msglen, double ton, int duration )
{
   LINK p = head_save;
   char time_str[25];
   char ton_str[25];
   char line[256];
   int  nskip = 0;

   if ( head_save == NULL ) return( nskip );

   while ( p != NULL )
   {
      Time_DtoS( p->trg.time, time_str );
      Time_DtoS( ton, ton_str );
      ton_str[21] = '\0';

      sprintf( line, " %s %s %s %s %c %s save: %s   %d\n",
               p->trg.sta, p->trg.comp, p->trg.net, p->trg.loc,
               p->trg.event_type, time_str, ton_str, duration );

      if( ( strlen(msg) + strlen(line) ) >= (unsigned)msglen )  nskip++;
      else  strcat( msg, line );   /* only copy to msg if there's room, */
      logit("", "%s", line );      /* but always write it to log file   */

      p = p->next;
   }
   return( nskip );
}


   /*********************************************************************
    *                     GetEarliestTriggerTime()                      *
    *          Return the earliest trigger time as a double.            *
    *********************************************************************/

#define BIG_DOUBLE 1.e12

double GetEarliestTriggerTime( void )
{
   double earliest = BIG_DOUBLE;
   LINK   p = head_save;

   if ( head_save == NULL ) return( 0.0 );

   while ( p != NULL )
   {
      if ( earliest > p->trg.time ) earliest = p->trg.time;
      p = p->next;
   }

   return( earliest );
}

   /*********************************************************************
    *                          EventIsBig()                             *
    *   Returns 1 if at least one trigger is event_type = 'B' (big)     * 
    *           0 if all triggers are 'N' (normal)                      *
    *********************************************************************/

int EventIsBig( void )
{
   LINK   p = head_save;

   if ( head_save == NULL ) return( 0 );

   while ( p != NULL )
   {
      if ( p->trg.event_type == 'B' ) return( 1 );
      p = p->next;
   }

   return( 0 );
}
