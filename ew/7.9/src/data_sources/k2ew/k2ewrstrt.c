/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2ewrstrt.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.9  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.8  2005/05/27 15:05:00  friberg
 *     new version 2.39 fixes restart file issue with netcode and location codes
 *
 *     Revision 1.7  2001/10/16 22:03:56  friberg
 *     Upgraded to version 2.25
 *
 *     Revision 1.6  2001/08/08 16:11:48  lucky
 *     version 2.23
 *                            
 *     Revision 1.4  2001/04/23 20:23:14  friberg
 *     Added station name remapping using the StationId config parameter.
 *     The restart file needs to report the old mapping and it should not
 *     be checked against the new mapping.
 *
 *     Revision 1.3  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *
 *
 */

/*  k2ewrstrt.c:  functions for restarting k2ew           */
/*                                                        */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <time.h>
#include <kom.h>     /* Earthworm KOM routines for parsing config files */
#include "error_ew.h"
#include "glbvars.h"
#include "k2ewrstrt.h"

#define MAX_ERR_MSG  128
static char errormsg[MAX_ERR_MSG];  /* error message buffer */

void k2ew_write_rsfile( uint32_t dataseq, unsigned char stmnum)
{
  int i;
  FILE *rsf;
  time_t now;
  
  if (gcfg_debug)
    logit("et", "Writing restart file <%s>\n", gcfg_restart_filename);
  
  if ( (rsf = fopen(gcfg_restart_filename, "w")) == NULL)
  {
    ew_fmt_err_msg(GetLastError_ew(), errormsg, MAX_ERR_MSG);
    logit("et", "k2ew_write_rsfile: error opening restart file <%s>: %s\n",
          gcfg_restart_filename, errormsg);
    return;
  }
    
  time(&now);
  fprintf(rsf, "Time %ld\n", (long)now);
  fprintf(rsf, "LastSeq %lu\n", (long)dataseq);
  fprintf(rsf, "LastStm %u\n", (unsigned)stmnum);
  fprintf(rsf, "NumStreams %d\n", g_numstms);
  fprintf(rsf, "SampRate %d\n", g_smprate);
                        /* always store original name read from K2: */
  fprintf(rsf, "Station %s\n", g_k2_stnid);
                        /* store current channel names in use: */
  for (i = 0; i < g_numstms; i++)
    fprintf(rsf, "NCL %s.%s.%s\n", g_netcode_arr[i], g_stmids_arr[i], gcfg_locnames_arr[i]);

  fclose(rsf);
  if (gcfg_debug)
    logit("et", "closed restart file\n");
  return;
}

#define NCOMMAND  7  /* Number of commands in restart file; all are required */

void k2ew_read_rsfile(int *valid_restart, uint32_t *last_dataseq,
                      unsigned char *last_stmnum)
{
  char     init[NCOMMAND];     /* Flags, one for each command */
  int      nmiss;              /* Number of commands that were missed */
  int      nfiles, i, stmnum = 0;
  time_t now, last_exit = 0L;

  if (valid_restart == NULL || last_dataseq == NULL || last_stmnum == NULL)
  {
    logit("et", "k2ew_read_rsfile: illegal NULL parameters\n");
    return;
  }

  if (gcfg_debug)
    logit("et", "Reading restart file <%s>\n", gcfg_restart_filename);
  
  
/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < NCOMMAND; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( gcfg_restart_filename );
   if ( nfiles == 0 )
   {
      logit("et", "k2ew_read_rsfile: error opening restart file <%s>\n",
            gcfg_restart_filename );
      return;
   }

   /* Process all nested configuration files */
   while ( nfiles > 0 )          /* While there are config files open */
   {
      while ( k_rd() )           /* Read next line from active file  */
      {
         char *com;
         char *str;

         com = k_str();          /* Get the first token from line */

         if ( !com ) continue;             /* Ignore blank lines */
         if ( com[0] == '#' ) continue;    /* Ignore comments */

         /* Read configuration parameters */
         if ( k_its( "Time" ) )
         {
           last_exit = k_long();
           init[0] = 1;
         }
         else if ( k_its( "LastSeq" ) )
         {
           *last_dataseq = k_long();
           init[1] = 1;
         }
         else if ( k_its( "LastStm" ) )
         {
           *last_stmnum = (unsigned char)k_int();
           init[2] = 1;
         }
         else if ( k_its( "NumStreams" ) )
         {
           g_numstms= k_int();
           init[3] = 1;
         }
         else if ( k_its( "SampRate" ) )
         {
           g_smprate = k_int();
           init[4] = 1;
         }
         else if ( k_its( "Station" ) )
         {
           if ( (str = k_str()) != 0)
           {       /* station name read in OK; save it */
             strncpy(g_k2_stnid,str,sizeof(g_k2_stnid)-1);
             g_k2_stnid[sizeof(g_k2_stnid)-1] = '\0';
             init[5] = 1;         /* indicate parameter received */
           }
         }
         else if ( k_its( "Channel" ) )
 	 {
 		logit("et", "Fatal Error, old style restart file found, please remove the file %s\n", gcfg_restart_filename);
		exit(2);
         }
         else if ( k_its( "NCL" ) )
         {
           if (stmnum > g_numstms)
           {
             logit("et", "k2ew_read_rsfile: too many channels in restart file\n");
           }
           else
           {
             if ( (str = k_str()) != 0)
             {
	       /* now parse the NN.CCC.LL instead of just Channel name */ 
	        char *cptr, *cptr2;
	        cptr = strchr(str, '.');
		if (cptr == NULL)
                {
                  logit("et", "k2ew_read_rsfile: FATAL ERROR: NCL not given as NN.CCC.LL <%s>\n",
                        str);
	          exit(3);
                 }
		 *cptr = '\0';
                 strcpy(g_netcode_arr[stmnum], str);
		 cptr++;
		 cptr2 = strchr(cptr, '.');
		 if (cptr2 == NULL)
                 {
                  logit("et", "k2ew_read_rsfile: FATAL ERROR: NCL not given as NN.CCC.LL \n");
	          exit(3);
                 }
		 *cptr2 = '\0';
                 strcpy(g_stmids_arr[stmnum], cptr);
		 cptr2++;
		 if (gcfg_locnames_arr[stmnum] == NULL) 
                 {
                 	gcfg_locnames_arr[stmnum]=strdup(cptr2);
 		 }
		 else
		 {
			strcpy(gcfg_locnames_arr[stmnum], cptr2);
		 }
                 stmnum++;
                 init[6] = 1;
               }
             }
         }
         else
         {
            /* An unknown parameter was encountered */
           logit("et", "k2ew_read_rsfile: <%s> unknown parameter in <%s>\n",
                 com, gcfg_restart_filename );
         }

         /* See if there were any errors processing the command */
         if ( k_err() )
         {
           logit("et", "k2ew_read_rsfile: bad <%s> command in <%s>.\n",
                 com, gcfg_restart_filename );
         }
      }
      nfiles = k_close();
   }

   /* Check if this looks like a valid restart file */
   time(&now);
   if (gcfg_debug > 3)
     logit("et", "k2ew_read_rsfile: now %ld last exit %ld diff %f lim %d\n",
           now, last_exit, difftime(now, last_exit), gcfg_restart_maxage);
   if (difftime(now, last_exit) < (double)gcfg_restart_maxage)
   {
     if (gcfg_debug)
       logit("et", "k2ew_read_rsfile: restart file is new enough\n");
     *valid_restart = 1;     /* file isn't too old: OK */
   }
   else
   {
     logit("et", "Restart file data is too old; discarded\n");
     *valid_restart = 0;     /* file is too old */
     return;
   }

   nmiss = 0;
   for ( i = 0; i < NCOMMAND; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 )
   {
     logit("et","k2ew_read_rsfile: ERROR, no ");
     if ( !init[0]  ) logit("e", "<Time> ");
     if ( !init[1]  ) logit("e", "<LastSeq> ");
     if ( !init[2]  ) logit("e", "<LastStm> " );
     if ( !init[3]  ) logit("e", "<NumStreams> " );
     if ( !init[4] ) logit("e", "<SampRate> " );
     if ( !init[5] ) logit("e", "<Station> " );
     if ( !init[6] ) logit("e", "<Channel> " );
     logit("e", "command(s) in <%s>.\n",gcfg_restart_filename );
     *valid_restart = 0;
     return;
   }

   if (g_numstms == 0 || g_numstms != stmnum)
   {
     logit("et", "k2ew_read_rsfile: number of streams (%d) doesn't equal number"
           " of Channel commands (%d); invalid restart file\n", g_numstms, 
           stmnum);
     *valid_restart = 0;    /* file appears to be incomplete: BAD */
   }
   
    while (stmnum < K2_MAX_STREAMS)      /* fill in remaining entries */
      strcpy(g_stmids_arr[stmnum++],"???");   /*  with dummy strings */

   return;
}

