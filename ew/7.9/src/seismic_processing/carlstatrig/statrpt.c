
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: statrpt.c 6229 2015-01-23 16:45:37Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.2  2004/05/05 23:54:03  lombard
 *     Added location code: reads TYPE_TRACEBUF2 messages,
 *     writes TYPE_CARLSTATRIG_SCNL messages.
 *     Removed OS2 support.
 *
 *     Revision 1.1  2000/02/14 16:12:07  lucky
 *     Initial revision
 *
 *
 */

/*
 * statrpt.c: Produce an error or heartbeat message on the output ring.
 *              1) Construct the correct type of message.
 *              2) Send the message to the output ring.
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: StatusReport                                          */
/*                                                                      */
/*      Inputs:         Pointer to the World structure                  */
/*                      Message type                                    */
/*                      Message id(code)                                */
/*                      Pointer to a string(message)                    */
/*                                                                      */
/*      Outputs:        Message structure sent to output ring           */
/*                                                                      */
/*      Returns:        Nothing                                         */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>
#include <sys/types.h>  /* time                                         */
#include <time.h>       /* time                                         */
#include <string.h>    /* strlen                                        */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */
#include <transport.h>  /* MSG_LOGO, SHM_INFO, tport_putmsg             */

/*******                                                        *********/
/*      CarlStaTrig Includes                                            */
/*******                                                        *********/
#include "carlstatrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: StatusReport                                          */
void StatusReport( WORLD* cstWorld, unsigned char type, short code, 
                   char* message )
{
  char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*    Validate the input parameters                                   */
  if ( cstWorld )
  {
    /*  Get the time of the message                                     */
    time( &msgTime );

    /*  Build the message based on the type                             */
    if ( cstWorld->cstEWH->typeHeartbeat == type )
    {   
      cstWorld->outLogo.type   = cstWorld->cstEWH->typeHeartbeat;
      sprintf( outMsg, "%ld %ld\n", (long) msgTime, (long) cstWorld->MyPid );
    }
    else
    {
      cstWorld->outLogo.type   = cstWorld->cstEWH->typeError;
      if ( message )
        sprintf( outMsg, "%ld %hd %s\n", (long) msgTime, code, message );
      else
        sprintf( outMsg, "%ld %hd (No description)\n", (long) msgTime, code );
    }

    /*  Write the message to the output region                          */
    if ( tport_putmsg( &(cstWorld->regionOut), &(cstWorld->outLogo), 
                       (long) strlen( outMsg ), outMsg ) != PUT_OK )
    {
      /*        Log an error message                                    */
      logit( "et", "carlStaTrig: Failed to send a status message (%d).\n",
                code );
    }
  }
}
