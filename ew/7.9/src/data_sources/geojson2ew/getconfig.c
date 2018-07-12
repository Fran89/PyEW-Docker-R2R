/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "earthworm.h"
#include "kom.h"
#include "transport.h"
#include "misc.h"
#include "externs.h"
#include "geojson_map.h"
#include "json_conn.h"

enum INIT_COMMANDS
{
    INIT_MODULEID,
    INIT_RINGNAME,
    INIT_HEARTBEATINT,
    INIT_LOGFILE,
    INIT_HOST,
    INIT_PORT,
    INIT_MAP_TIME,
    INIT_MAP_SNCL,
    INIT_MAP_SAMPLERATE,
    INIT_MAP_CHAN,
    NCOMMAND
 };

static char *EMPTY = "";

int IsComment( char [] );  
void setuplogo(MSG_LOGO *);


     /***************************************************************
      *                          GetConfig()                        *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

char k_char()
{
   char *str = k_str();
   if ( str && str[0] && !str[1] )
      return str[0];
   return 0;
}

char * k_str_dup()
{
   char *str = k_str();
   if ( str != NULL)
   {
      if ( *str == 0 )
        str = EMPTY;
      else
        str = strdup(str);
   }
   return str;
}

int GetConfig( char *configfile )
{
   char     init[NCOMMAND];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;
   GEOJSON_MAP_ENTRY entry;

   set_json_connection_params_to_defaults(&Conn_params);
/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < NCOMMAND; i++ )
      init[i] = 0;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      logit("e", "%s: Error opening configuration file <%s>\n", Progname, configfile );
      return -1;
   }

/* Process all nested configuration files
   **************************************/
   while ( nfiles > 0 )          /* While there are config files open */
   {
      while ( k_rd() )           /* Read next line from active file  */
      {
         int  success;
         char *com;
         char *str;

         com = k_str();          /* Get the first token from line */

         if ( !com ) continue;             /* Ignore blank lines */
         if ( com[0] == '#' ) continue;    /* Ignore comments */

/* Open another configuration file
   *******************************/
         if ( com[0] == '@' ) {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success ) {
               logit("e", "%s: Error opening command file <%s>.\n", Progname, &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         if ( k_its( "ModuleId" ) ) {
            if ( (str = k_str()) ) {
               if ( GetModId(str, &QModuleId) == -1 ) {
                  logit("e", "%s: Invalid ModuleId <%s>. \n", 
				Progname, str );
                  logit("e", "%s: Please Register ModuleId <%s> in earthworm.d!\n", 
				Progname, str );
                  return -1;
               }
            }
            init[INIT_MODULEID] = 1;
         } else if ( k_its( "RingName" ) ) {
            if ( (str = k_str()) != NULL ) {
               if ( (RingKey = GetKey(str)) == -1 )
               {
                  logit("e", "%s: Invalid RingName <%s>. \n", 
			Progname, str );
                  return -1;
               }
            }
            init[INIT_RINGNAME] = 1;
         } else if ( k_its( "HeartbeatInt" ) ) {
            HeartbeatInt = k_int();
            init[INIT_HEARTBEATINT] = 1;
         } else if ( k_its( "LogFile" ) ) {
            LogFile = k_int();
            init[INIT_LOGFILE] = 1;
         } else if ( k_its( "HOST" ) ) {
            Conn_params.hostname = k_str_dup();
            init[INIT_HOST] = 1;
         } else if ( k_its( "PORT" ) ) {
            Conn_params.port = k_int();
            init[INIT_PORT] = 1;
         } else if ( k_its( "MAP_TIME" ) ) {
            Map_time = k_str_dup();
	    if (Map_time)
	    {
               init[INIT_MAP_TIME] = 1;
	    }
         } else if ( k_its( "MAP_SNCL" ) ) {
            Map_sncl = k_str_dup();
	    if (Map_sncl)
	    {
               init[INIT_MAP_SNCL] = 1;
	    }
         } else if ( k_its( "MAP_SAMPLERATE" ) ) {
            Map_samplerate = k_str_dup();
	    if (Map_samplerate)
	    {
               init[INIT_MAP_SAMPLERATE] = 1;
	    }
	 } else if ( k_its( "MAP_CHAN") ) {
            memset(&entry, 0, sizeof(GEOJSON_MAP_ENTRY));
            entry.channelCode = k_char();
	    entry.path = k_str_dup();
	    entry.multiplier = k_val();
            if ( k_err() || !entry.channelCode ||
                 !entry.path || entry.multiplier <= 0)
            {
               logit("e", "%s: Bad <%s> command in <%s>.\n", 
		   Progname, com, configfile );
               return -1;
            }
	    str = k_str_dup();
            if ( !k_err() && str )
            {
	       entry.conditionPath = str;
	    }
            if (add_channel(&entry) == 0)
            {
               init[INIT_MAP_CHAN] = 1;
            }
            else
            {
               init[INIT_MAP_CHAN] = 0;
            }
	 } else if ( k_its( "Verbose") ) {
            Verbose = k_int();
	 } else if ( k_its( "VerboseSncl") ) {
            VerboseSncl = k_str_dup();
	 } else if ( k_its( "CHANNEL_NUMBER") ) {
	    Conn_params.channel_number = k_int();
	 } else if ( k_its( "FRAME_MAX_SIZE") ) {
	    Conn_params.frame_max_size = k_int();
	 } else if ( k_its( "HEARTBEAT_SECONDS") ) {
	    Conn_params.heartbeat_seconds = k_int();
	 } else if ( k_its( "MAX_CHANNELS") ) {
	    Conn_params.max_channels = k_int();
	 } else if ( k_its( "SERVERTYPE") ) {
	    Conn_params.servertype = k_str_dup();
         } else if ( k_its( "USERNAME" ) ) {
            Conn_params.username = k_str_dup();
         } else if ( k_its( "PASSWORD" ) ) {
            Conn_params.password = k_str_dup();
	 } else if ( k_its( "VHOST") ) {
	    Conn_params.vhost = k_str_dup();
         } else if ( k_its( "QUEUENAME" ) ) {
            Conn_params.queuename = k_str_dup();
         } else if ( k_its( "EXCHANGENAME" ) ) {
            Conn_params.exchangename = k_str_dup();
         } else if ( k_its( "EXCHANGETYPE" ) ) {
            Conn_params.exchangetype = k_str_dup();
         } else if ( k_its( "BINDKEY" ) ) {
            Conn_params.bindkey = k_str_dup();
         } else if ( k_its( "BINDARGS" ) ) {
            Conn_params.bindargs = k_str_dup();
	 } else if ( k_its( "READ_TIMEOUT") ) {
	    Conn_params.read_timeout = k_int();
	 } else if ( k_its( "DATA_TIMEOUT") ) {
	    Conn_params.data_timeout = k_int();
	 } else {
	    /* An unknown parameter was encountered */
            logit("e", "%s: <%s> unknown parameter in <%s>\n", 
		Progname,com, configfile );
            return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() ) {
            logit("e", "%s: Bad <%s> command in <%s>.\n", 
		Progname, com, configfile );
            return -1;
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check flags for missed commands
   ***********************************************************/
   nmiss = 0;
   for ( i = 0; i < NCOMMAND; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 ) {
      logit("e","%s: ERROR, no ", Progname );
      if ( !init[INIT_MODULEID]  ) logit("e", "<ModuleId> " );
      if ( !init[INIT_RINGNAME]  ) logit("e", "<RingName> " );
      if ( !init[INIT_HEARTBEATINT] ) logit("e", "<HeartbeatInt> " );
      if ( !init[INIT_LOGFILE] ) logit("e", "<LogFile> " );
      if ( !init[INIT_HOST] ) logit("e", "<HOST> " );
      if ( !init[INIT_PORT] ) logit("e", "<PORT> " );
      if ( !init[INIT_MAP_TIME] ) logit("e", "<MAP_TIME> " );
      if ( !init[INIT_MAP_SNCL] ) logit("e", "<MAP_SNCL> " );
      if ( !init[INIT_MAP_SAMPLERATE] ) logit("e", "<MAP_SAMPLERATE> " );
      if ( !init[INIT_MAP_CHAN] ) logit("e", "<MAP_CHAN> " );
      logit("e", "command(s) in <%s>.\n", configfile );
      return -1;
   }
	
   if ( GetType( "TYPE_HEARTBEAT", &TypeHB ) != 0 ) {
      logit("e",
              "%s: Invalid message type <TYPE_HEARTBEAT>\n",Progname);
      return( -1 );
   }
   if ( GetType( "TYPE_TRACEBUF2", &TypeTrace2 ) != 0 ) {
      logit("e",
              "%s: Message type <TYPE_TRACEBUF2> not found in earthworm_global.d; exiting!\n", Progname);
        return(-1);
   } 
   if ( GetType( "TYPE_ERROR", &TypeErr ) != 0 ) {
      logit("e",
              "%s: Invalid message type <TYPE_ERROR>\n", Progname);
      return( -1 );
   }
/* build the datalogo */
   setuplogo(&DataLogo);
   DataLogo.type=TypeTrace2;
   setuplogo(&OtherLogo);

   if (Verbose & VERBOSE_GENERAL)
   {
      GEOJSON_MAP_ENTRY *eptr = get_channel();
      while (eptr != NULL)
      {
         logit("e", "MAP_CHAN %c %s %lf %s\n", eptr->channelCode, eptr->path, eptr->multiplier, eptr->conditionPath);
         eptr = eptr->next;
      }
   }
   return 0;
}
