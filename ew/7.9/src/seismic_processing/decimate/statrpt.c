
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: statrpt.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.2  2002/10/25 19:48:15  dietz
 *     added #include <time.h>
 *
 *     Revision 1.1  2000/02/14 16:56:25  lucky
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
/*      Inputs:         Pointer to the Network structure                */
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
#include <sys/types.h> /* time                                          */
#include <string.h>    /* strlen                                        */
#include <time.h>      /* time                                          */

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: SubtusReport                                          */
void StatusReport( WORLD* pDcm, unsigned char type, short code, 
                   char* message )
{
  char          outMsg[MAXMESSAGELEN];  /* The outgoing message.        */
  time_t        msgTime;        /* Time of the message.                 */

  /*  Get the time of the message                                     */
  time( &msgTime );

  /*  Build & process the message based on the type                   */
    if ( pDcm->dcmEWH.typeHeartbeat == type )
    {
      sprintf( outMsg, "%ld %ld\n", (long) msgTime, (long) pDcm->MyPid );
      
      /*Write the message to the output region                          */
      if ( tport_putmsg( &(pDcm->regionOut), &(pDcm->hrtLogo), 
                         (long) strlen( outMsg ), outMsg ) != PUT_OK )
      {
        /*     Log an error message                                    */
        logit( "et", "decimate: Failed to send a heartbeat message (%d).\n",
               code );
      }
    }
    else
    {
      if ( message ) {
        sprintf( outMsg, "%ld %hd %s\n", (long) msgTime, code, message );
        logit("t","Error:%d (%s)\n", code, message );
      }
      else {
        sprintf( outMsg, "%ld %hd\n", (long) msgTime, code );
        logit("t","Error:%d (No description)\n", code );
      }

      /*Write the message to the output region                         */
      if ( tport_putmsg( &(pDcm->regionOut), &(pDcm->errLogo), 
                         (long) strlen( outMsg ), outMsg ) != PUT_OK )
      {
        /*     Log an error message                                    */
        logit( "et", "decimate: Failed to send an error message (%d).\n",
               code );
      }
      
    }
}


