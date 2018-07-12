/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: heartbeat.c 2718 2007-02-26 17:16:53Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/02/26 17:16:53  paulf
 *     made sure time_t are casted to long for heartbeat sprintf()
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>


/* Global variables
   ****************/
extern unsigned char ModuleId;         // Data source id
extern int           HeartbeatInt;     // Heartbeat interval in seconds
extern SHM_INFO      OutRegion;        // In adsend.c
extern pid_t         MyPid;            // process id, sent with heartbeat

void Heartbeat( void )
{
   long              msgLen;           // Length of the heartbeat message
   char              msg[40];          // To hold the heartbeat message
   static int        first = TRUE;     // 1 the first time Heartbeat() is called
   static time_t     time_prev;        // When Heartbeat() was last called
   time_t            time_now;         // The current time
   static MSG_LOGO   logo;             // Logo of heartbeat messages

/* Initialize the heartbeat variables
   **********************************/
   if ( first )
   {
      GetLocalInst( &logo.instid );
      logo.mod = ModuleId;
      GetType( "TYPE_HEARTBEAT", &logo.type );
      time_prev = 0;  // force heartbeat first time thru
      first = FALSE;
   }

/* Is it time to beat the heart?
   *****************************/
   time( &time_now );
   if ( (time_now - time_prev) < HeartbeatInt )
      return;

/* It's time to beat the heart
   ***************************/
   sprintf( msg, "%ld %d\n", (long) time_now, (int) MyPid );
   msgLen = strlen( msg );

   if ( tport_putmsg( &OutRegion, &logo, msgLen, msg ) != PUT_OK )
      logit( "et", "Error sending heartbeat." );

   time_prev = time_now;
   return;
}
