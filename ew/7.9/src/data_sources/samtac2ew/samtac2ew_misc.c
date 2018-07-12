/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2ew_misc.c 3536 2009-01-15 22:09:51Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

#include <time.h>
#include <earthworm.h> //logit
#include <stdio.h>
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */
// glbvars.h NEEDED for gcfg_heartbeat_itvl, g_heart_ltype, g_heartbeat_logo
#include "transport.h" //tport_putmsg

#define MAX_MSG_SIZE      256       // Large enough for all faults in extended
									// status message

/*******************************************************************************
 * samtac2mi_status_hb: sends heartbeat or status messages to                  *
 *         earthworm transport ring.                                           *
 *       type: the message type: heartbeat or status                           *
 *       code: the error code for status messages                              *
 *       message: message text, if any                                         *
 ******************************************************************************/
 
void samtac2mi_status_hb(unsigned char type, short code, char* message )
{
  char          outMsg[MAX_MSG_SIZE];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*  Get the time of the message                                       */
  time( &msgTime );

  /*  Build & process the message based on the type                     */
  if ( g_heartbeat_logo.type == type )
  {

    sprintf( outMsg, "%ld %d\n\0", (long) msgTime, MyPid );

    /*Write the message to the output region                            */
    if ( tport_putmsg( &g_tport_region, &g_heartbeat_logo,
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                       */
      logit( "et", "samtac2ew: Failed to send a heartbeat message (%d).\n",
             code );
    }
  }
  else
  {
    if ( message )
    {
      sprintf( outMsg, "%ld %hd %s\n\0", (long) msgTime, code, message );
      logit("et","Error:%d (%s)\n", code, message );
    }
    else
    {
      sprintf( outMsg, "%ld %hd\n\0", (long) msgTime, code );
      logit("et","Error:%d (No description)\n", code );
    }

    /*Write the message to the output region                         */
    if ( tport_putmsg( &g_tport_region, &g_error_logo,
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*     Log an error message                                    */
      logit( "et", "samtac2ew: Failed to send an error message (%d).\n",
             code );
    }
  }
}
