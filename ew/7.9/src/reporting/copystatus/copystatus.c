
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: copystatus.c 3837 2010-03-09 22:03:03Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2010/03/09 22:03:03  paulf
 *      last time, I swear, fixed now
 *
 *     Revision 1.4  2010/03/09 22:01:58  paulf
 *     again....
 *
 *     Revision 1.3  2010/03/09 22:01:31  paulf
 *     fixed size of getlogo array
 *
 *     Revision 1.2  2010/03/09 21:58:10  paulf
 *     added type_stop and type_restart messages
 *
 *     Revision 1.1  2000/02/14 16:49:09  lucky
 *     Initial revision
 *
 *
 */

  /********************************************************************
   *                           copystatus                             *
   *                                                                  *
   *   Program to copy all error messages and heartbeats from one     *
   *   ring to another.  Preserve transport layer sequence number     *
   *   from input ring in output ring by using tport_copyfrom()       *
   *   and tport_copyto() to transfer the message                     *
   ********************************************************************/

/* changes: 
	
  March 9, 2010
  For EW7.4 copystatus now copies TYPE_STOP and TYPE_RESTART for statmgr to see.

  IMPORTANT NOTE: If any stop or restart module messages (or any other new status
	messages) needed by statmgr are created, they need to get added to copystatus.

  Lombard: 11/19/98: V4.0 changes: None!
     0) no Y2k dates 
     1) changed argument of logit_init to the config file name: no logit here
     2) process ID in heartbeat message: not applcable
     3) flush input transport ring: not applicable
     4) add `restartMe' to .desc file: not applicable
     5) multi-threaded logit: not applicable
*/

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>

#define MAX_SIZE 300            /* Largest message size in characters */

#define NUM_LOGOS_TOMONITOR 4

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static unsigned char InstId;        /* local installation id          */
static unsigned char InstWildCard;  /* wildcard for installations     */
static unsigned char ModWildCard;   /* wildcard for module            */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeStop;
static unsigned char TypeRestart;


int main( int argc, char *argv[] )
{
   SHM_INFO inRegion;
   SHM_INFO outRegion;
   MSG_LOGO getlogo[NUM_LOGOS_TOMONITOR];
   MSG_LOGO logo;
   char     *inRing;           /* Name of input ring           */
   char     *outRing;          /* Name of output ring          */
   long     inkey;             /* Key to input ring            */
   long     outkey;            /* Key to output ring           */
   char     msg[MAX_SIZE];
   unsigned char inseq;        /* transport seq# in input ring */
   int      rc;
   long     length;

/* Check program arguments
   ***********************/
   if ( argc != 3 )
   {
      printf( "Usage:  copystatus <input_ringname> <output_ringname>\n" );
      return -1;
   }
   inRing  = argv[1];
   outRing = argv[2];

/* Look up local installation id
   *****************************/
   if ( GetLocalInst( &InstId ) != 0 )
   {
      printf( "copystatus: error getting local installation id; exitting!\n" );
      return -1;
   }
   if ( GetInst( "INST_WILDCARD", &InstWildCard ) != 0 )
   {
      printf( "copystatus: Invalid installation name <INST_WILDCARD>" );
      printf( "; exitting!\n" );
      return -1;
   }

/* Look up module ids & message types earthworm.h tables
   ****************************************************/
   if ( GetModId( "MOD_WILDCARD", &ModWildCard ) != 0 )
   {
      printf( "copystatus: Invalid module name <MOD_WILDCARD>; exitting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 )
   {
      printf( "copystatus: Invalid message type <TYPE_HEARTBEAT>; exitting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 )
   {
      printf( "copystatus: Invalid message type <TYPE_ERROR>; exitting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_STOP", &TypeStop ) != 0 )
   {
      printf( "copystatus: Invalid message type <TYPE_STOP>; exitting!\n" );
      return -1;
   }
   if ( GetType( "TYPE_RESTART", &TypeRestart ) != 0 )
   {
      printf( "copystatus: Invalid message type <TYPE_RESTART>; exitting!\n" );
      return -1;
   }

/* Look up transport region keys earthworm.h tables
   ************************************************/
   if( ( inkey = GetKey(inRing) ) == -1 )
   {
        printf( "copystatus: Invalid input ring name <%s>; exitting!\n",
                 inRing );
        return -1;
   }
   if( ( outkey = GetKey(outRing) ) == -1 )
   {
        printf( "copystatus: Invalid output ring name <%s>; exitting!\n",
                 outRing );
        return -1;
   }

/* Attach to input and output transport rings
   ******************************************/
   tport_attach( &inRegion,  inkey );
   tport_attach( &outRegion, outkey );

/* Initialize getlogo with logos of messages to get
   ************************************************/
   getlogo[0].type   = TypeError;
   getlogo[0].mod    = ModWildCard;
   getlogo[0].instid = InstWildCard;
   getlogo[1].type   = TypeHeartBeat;
   getlogo[1].mod    = ModWildCard;
   getlogo[1].instid = InstWildCard;
   getlogo[2].type   = TypeStop;
   getlogo[2].mod    = ModWildCard;
   getlogo[2].instid = InstWildCard;
   getlogo[3].type   = TypeRestart;
   getlogo[3].mod    = ModWildCard;
   getlogo[3].instid = InstWildCard;

/* Copy errors and heartbeats from inRegion to outRegion
   *****************************************************/
   while ( tport_getflag( &inRegion ) != TERMINATE )
   {
      rc = tport_copyfrom( &inRegion, getlogo, (short)NUM_LOGOS_TOMONITOR, &logo,
                           &length, msg, MAX_SIZE, &inseq );

      if ( rc == GET_OK )
      {
/* This is the most likely case.  Output msg at bottom of if-elseif */
      }

      else if ( rc == GET_NONE )
      {
         sleep_ew( 1000 );
         continue;
      }

      else if ( rc == GET_NOTRACK )
      {
         printf( "copystatus error in %s: no tracking for msg; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
      }

      else if ( rc == GET_MISS_LAPPED )
      {
         printf( "copystatus error in %s: msg(s) overwritten; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
      }

      else if ( rc == GET_MISS_SEQGAP )
      {
         printf( "copystatus error in %s: gap in msg sequence; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
      }

      else if ( rc == GET_TOOBIG )
      {
         printf( "copystatus error in %s: input msg too big; i:%d m:%d t:%d seq:%d\n",
                  inRing, (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq );
         continue;
      }

   /* Put message in outregion (all cases except GET_NONE and GET_TOOBIG)
    *********************************************************************/
      rc = tport_copyto( &outRegion, &logo, length, msg, inseq );
      if( rc != PUT_OK )
      {
         printf( "copystatus error writing msg (i:%d m:%d t:%d seq:%d) to %s\n",
                 (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq, outRing );
         continue;
      }

#ifdef _OS2
      printf("Moved msg(i:%d m:%d t:%d seq:%d) from %s to %s\n",
              (int)logo.instid, (int)logo.mod, (int)logo.type, (int)inseq,
              inRing, outRing ); /*DEBUG*/
#endif
   }

/* Detach from shared memory regions and terminate
   ***********************************************/
   tport_detach( &inRegion );
   tport_detach( &outRegion );
   return 0;
}

