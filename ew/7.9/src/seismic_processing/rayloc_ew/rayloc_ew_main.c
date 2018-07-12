/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 ***************************************************************************/



/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc_ew_main.c 2055 2006-01-19 19:04:55Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/01/19 19:04:55  friberg
 *     rayloc upgraded to use a library version of rayloc_message_rw.c
 *
 *     Revision 1.1  2004/08/05 04:15:11  friberg
 *     First commit of rayloc_ew in EW-CENTRAL CVS
 *
 *     Revision 1.13  2004/08/04 19:58:31  ilya
 *     Made a loop infinite
 *
 *     Revision 1.12  2004/08/04 19:27:54  ilya
 *     Towards version 1.0
 *
 *     Revision 1.11  2004/08/03 18:26:05  ilya
 *     Now we use stock EW functions from v6.2
 *
 *     Revision 1.10  2004/07/30 14:57:03  ilya
 *     Inf. loop installed
 *
 *     Revision 1.8  2004/06/25 15:22:19  ilya
 *     Working output
 *
 *     Revision 1.7  2004/06/25 15:08:05  ilya
 *     Working on output
 *
 *     Revision 1.6  2004/06/25 14:22:17  ilya
 *     Working version: no output
 *
 *     Revision 1.5  2004/06/24 19:06:28  ilya
 *     *** empty log message ***
 *
 *     Revision 1.4  2004/06/24 19:05:20  ilya
 *     Fixed logging
 *
 *     Revision 1.3  2004/06/24 16:47:05  ilya
 *     Version compiles
 *
 *     Revision 1.2  2004/06/24 16:15:07  ilya
 *     Integration phase started
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
 *     initial import into CVS
 *
 */

       /****************************************************************
       *                         rayloc_ew.c                           *
       *                                                               *
       *  This is the Earthscope/GLASS wrapper to FORTRAN rayloc.      *
       *                                                               *
       *  Written by Ilya Dricker, ISTI,  2004                         *
       *                                                               *
       *                                                               *
       *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <swap.h>

#include "rayloc_ew.h"
#include "rayloc1.h"
#include <rayloc_message_rw.h>

int rayloc_ew_checkAuxFiles( GPARM *Gparm );
int rayloc_ew_checkWorkDir(char *hb_dir);
int rayloc_ew_checkFile(char *dir, char *fname);

int rayloc_ew_checkAuxFiles( GPARM *Gparm )
{
   /* Check if the AUX dir is really a dir and writable */
   if (-1 == rayloc_ew_checkWorkDir(Gparm->workDirName))
      return -1;
   if (-1 == rayloc_ew_checkFile(Gparm->workDirName, "ak135.hed"))
      return -1;
   if (-1 == rayloc_ew_checkFile(Gparm->workDirName, "ak135.tbl"))
      return -1;
   if (-1 == rayloc_ew_checkFile(Gparm->workDirName, "tau.table"))
      return -1;
   return 1;
}

int rayloc_ew_checkFile(char *dir, char *fname)
{
  /* returns 1 on success, 1 on failure */
  /* checks for existance of dir and writability of dir*/

   char tmp[2000];
   struct stat buf;

   sprintf (tmp, "%s/%s", dir, fname);

   /* does hb_dir exist */
   if (stat(tmp, &buf) == 0)
   {
      /* hb_dir exists */
      /* is it a dir and is it writable*/
      if ((buf.st_mode & S_IFREG ))
      {
         /* if not a directory, return SSI_RESTART_FAILED */
         return 1;
      }
      if ((buf.st_mode & S_IFLNK))
      {
         /* if not writable, return SSI_RESTART_FAILED */
         return 1;
      }
      logit( "pt", "rayloc_ew_checkFile: required file %s is not a regular file or link\n", tmp);
      return -1;
  }
  logit( "pt", "rayloc_ew_checkFile: required file %s does not exist\n", tmp);
  return -1;
}

int rayloc_ew_checkWorkDir(char *hb_dir)
{
  /* returns 1 on success, 1 on failure */
  /* checks for existance of dir and writability of dir*/

   struct stat buf;

   /* does hb_dir exist */
   if (stat(hb_dir, &buf) == 0)
   {
      /* hb_dir exists */
      /* is it a dir and is it writable*/
      if (!(buf.st_mode & S_IFDIR ))
      {
         /* if not a directory, return SSI_RESTART_FAILED */
         logit( "pt", "rayloc_ew_checkWorkDir: %s is not a directory\n", hb_dir);
         return -1;
      }
      if (!(buf.st_mode & S_IWUSR))
      {
         /* if not writable, return SSI_RESTART_FAILED */
         logit( "pt", "rayloc_ew_checkWorkDir: %s is not writable\n", hb_dir);
         return -1;
      }
      return 1;
  }
  logit( "pt", "rayloc_ew_checkWorkDir: directory %s does not exist\n", hb_dir);
  return -1;
}


      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of picker configuration file         *
       ***********************************************************/

int main( int argc, char **argv )
{
   EWH                     Ewh;             /* Parameters from earthworm.h */
   GPARM                   Gparm;           /* Configuration file parameters */
   RAYLOC_PROC_FLAGS      *flags = NULL;
   RAYLOC_PHASES          *unused_list = NULL;
   RAYLOC_STATIONS        *stns;
   MSG_LOGO                hrtlogo;         /* Logo of outgoing heartbeats */
   MSG_LOGO                getlogo;         /* Logo of incoming data messages */
   MSG_LOGO                reclogo;         /* Logo of the incoming message */
   MSG_LOGO                putlogo;          /* Logo of the outgoing message */
   char                   *configfile;     /* Pointer to name of config file */
   pid_t                   myPid;           /* Process id of this process */
   time_t                  then;            /* Previous heartbeat time */
   time_t                  now;            /* Current heartbeat time */
   unsigned char           TypeLocGlobal;
   short int               nLogo=1;
   long                    recsize;
   char                    rec[100000];
   int                     retVal;
   int                     errLines;
   char                   *outStr = NULL;
   char                   *errStr = NULL;
   char                   *workDir = NULL;
   int                    loops = 0;
   RAYLOC_MESSAGE_HEADER_STRUCT *p_struct = NULL;

/* Check command line arguments
   ****************************/
   if ( argc != 2 )
   {
      fprintf( stderr, "Usage: rayloc_ew <configfile>\n" );
      return -1;
   }
   configfile = argv[1];

/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, 0, 1024, 2 );

   flags = (RAYLOC_PROC_FLAGS *)calloc(1, sizeof(RAYLOC_PROC_FLAGS));

 /* Get parameters from the configuration files
   *******************************************/
   if ( rayloc_ew_GetConfig( configfile, &Gparm, flags ) == -1 )
   {
      fprintf( stderr, "rayloc_ew: GetConfig() failed. Exiting.\n" );
      return -1;
   }

/* Look up info in the earthworm.h tables
   **************************************/
   if ( rayloc_ew_GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "rayloc_ew: GetEwh() failed. Exiting.\n" );
      return -1;
   }

/* Check AUX files and directories
   **************************************/
   if (-1 == rayloc_ew_checkAuxFiles(&Gparm))
   {
     logit("pt","rayloc_ew: rayloc_ew_checkAuxFiles failed, exiting!\n" );
     exit( -1 );
   }


/* Get Global location type */
   if( GetType( "TYPE_LOC_GLOBAL", &TypeLocGlobal ) != 0 ) {
     logit("pt","rayloc_ew: Invalid message type <TYPE_LOC_GLOBAL>; exiting!\n" );
     exit( -1 );
   }

/* Get RAY LOCATION type */
   if( GetType( "TYPE_RAYLOC", &(putlogo.type) ) != 0 ) {
     logit("pt","rayloc_ew: Invalid message type <TYPE_RAYLOC>; exiting!\n" );
     exit( -1 );
   }


/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
  getlogo.instid = Ewh.GetThisInstId;
  getlogo.mod    = Ewh.GetThisModId;
  getlogo.type   = TypeLocGlobal;

   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

   putlogo.mod = hrtlogo.mod;

  /* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "pt", "rayloc_ew: Can't get my pid. Exiting.\n" );
      return -1;
   }

  /* Log the configuration parameters
   ********************************/
   rayloc_ew_LogConfig( &Gparm );

   /* Attach to existing transport rings
   **********************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_attach( &Gparm.InRegion,  Gparm.InKey );
      tport_attach( &Gparm.OutRegion, Gparm.OutKey );
   }
   else
   {
      tport_attach( &Gparm.InRegion, Gparm.InKey );
      Gparm.OutRegion = Gparm.InRegion;
   }

/* Flush the input ring
 *    ********************/
   while ( tport_getmsg( &Gparm.InRegion, &getlogo, 1, &reclogo, &recsize,
                              rec, sizeof(rec)-1) != GET_NONE );

/* Get the time when we start reading messages.
   This is for issuing heartbeats.
   *******************************************/
   time( &then );

   unused_list = rayloc_new_phase_list();
   (void)rayloc_add_phase(unused_list, "?");

   stns = rayloc_stations_from_file(Gparm.StaFile, &retVal, &errLines);



   /* Loop to read waveform messages and invoke the picker
   ****************************************************/
   while ( tport_getflag( &Gparm.InRegion ) != TERMINATE  &&
           tport_getflag( &Gparm.InRegion ) != myPid )
   {
      /* Next three lines make the program stop after N loops: use it with memory checker */
      if (loops > 125)
         break;
     /*  loops++; */

      sleep(1);
      /* Send a heartbeat to the transport ring
       **************************************/
      time( &now );
      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         int  lineLen;
         char line[40];

         then = now;

         sprintf( line, "%d %d\n", (int)now, myPid );
         lineLen = strlen( line );

         if ( tport_putmsg( &Gparm.OutRegion, &hrtlogo, lineLen, line ) !=
              PUT_OK )
         {
            logit( "pt", "pick_ew: Error sending heartbeat. Exiting." );
            break;
         }
      }
      retVal = tport_getmsg( &Gparm.InRegion, &getlogo, nLogo,
                            &reclogo, &recsize, rec, sizeof(rec)-1 );
      switch( retVal )
      {
        case GET_NONE:
                if (1 == Gparm.Debug)
                   logit("pt", "rayloc_ew: Got NO  messages\n");
                continue;

        case GET_TOOBIG:
                logit("pt", "rayloc_ew: Input message is too big: length is %ld\n", recsize);
                continue;

        case GET_MISS:
                logit("pt", "rayloc_ew: Input Message is MISSED: length is %ld\n", recsize);
                continue;

        case GET_NOTRACK:
                logit("pt", "rayloc_ew: Input Message is NOTRACK: length is %ld\n", recsize);
                continue;

        case GET_OK:
                   logit("pt", "rayloc_ew: Got Input Message OK: length is %ld\n", recsize);
                break;
      }
      workDir = strdup(Gparm.workDirName);
      putlogo.instid = reclogo.instid;
      retVal = lib_rayloc(rec,
                       recsize,
                       unused_list,
                       flags,
                       "a",
                       workDir,
                       reclogo.type,
                       reclogo.mod,
                       reclogo.instid,
                       putlogo.type,
                       putlogo.type,
                       reclogo.instid,
                       &outStr,
                       &errStr,
                      stns);



      free(workDir);
      if (retVal > 0 && outStr)
      {
         recsize = retVal;
         retVal = tport_putmsg( &Gparm.OutRegion, &putlogo, recsize, outStr );
         if (PUT_OK == retVal)
           logit("pt", "rayloc_ew: PLACED Output TYPE_RAYLOC message (%d bytes) in the output ring\n", recsize);
         else
           logit("pt", "rayloc_ew: FAILED to place Output TYPE_RAYLOC message (%d bytes) in the output ring: retVal = %d\n", recsize, retVal);

      }
      else
      {
          logit("pt", "rayloc_ew: lib_rayloc FAILED: retVal = %d\n", retVal);
          if (!outStr)
             logit("pt", "rayloc_ew: Output Message size = NULL\n");
      }
      if (1 == Gparm.Debug)
      {
         /* This is a debug code to test output */
         retVal = rayloc_MessageToRaylocHeader(&p_struct, outStr);
         if (RAYLOC_MSG_SUCCESS == retVal)
            rayloc_logRayloc(p_struct);
         else
            logit("pt", "Failed to convert RAYLOC message into structure\n");
         rayloc_FreeRaylocHeader( &p_struct ); 
      }

      if (outStr)
      {
         free(outStr);
         outStr = NULL;
      }

      if (errStr)
      {
        free(errStr);
        errStr = NULL;
      }

      fflush( stdout );
   } /* End of while */

   /* Detach from the ring buffers
   ****************************/
   if ( Gparm.OutKey != Gparm.InKey )
   {
      tport_detach( &Gparm.InRegion );
      tport_detach( &Gparm.OutRegion );
   }
   else
      tport_detach( &Gparm.InRegion );

   /* Clean-up operations */
   if (flags)
      free(flags);
   rayloc_destroy_phase_list(unused_list);
   rayloc_destroy_stations(stns);

   logit( "pt", "Termination requested. Exiting.\n" );
   return 0;
}
