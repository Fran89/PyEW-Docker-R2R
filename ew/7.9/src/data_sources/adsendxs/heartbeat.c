#include <stdio.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>

#define MSGSIZE 40


/* Global variables
   ****************/
extern unsigned char ModuleId;         // Data source id
extern int           HeartbeatInt;     // Heartbeat interval in seconds
extern SHM_INFO      Region;           // In adsendxs.c
extern pid_t         MyPid;            // Process id, sent with heartbeat

void SendHeartbeat( void )
{
   long              msgLen;           // Length of the heartbeat message
   char              msg[MSGSIZE];     // To hold the heartbeat message
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
   sprintf_s( msg, MSGSIZE, "%ld %d\n", (long) time_now, (int) MyPid );
   msgLen = strlen( msg );

   if ( tport_putmsg( &Region, &logo, msgLen, msg ) != PUT_OK )
      logit( "et", "Error sending heartbeat." );

   time_prev = time_now;
   return;
}
