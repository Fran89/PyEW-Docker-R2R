/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getconfig.c 6094 2014-05-26 01:56:04Z baker $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.17  2008/10/02 21:22:08  kress
 *     V.C.Kress: made k2ew compile properly under 32 bit linux
 *
 *     Revision 1.16  2005/07/27 19:28:49  friberg
 *     2.40 changes for ForceBlockMode and comm stats
 *
 *     Revision 1.15  2005/03/26 00:17:46  kohler
 *     Version 2.38.  Added capability to get network code values from the K2
 *     headers.  The "Network" parameter in the config file is now optional.
 *     WMK 3/25/05
 *
 *     Revision 1.14  2004/06/03 16:54:01  lombard
 *     Updated for location code (SCNL) and TYPE_TRACEBUF2 messages.
 *
 *     Revision 1.13  2003/05/29 13:33:40  friberg
 *     cleaned up some warnings
 *
 *     Revision 1.12  2001/08/08 16:11:48  lucky
 *     version 2.23
 *
 *     Revision 1.10  2001/05/23 00:18:10  kohler
 *     Reads new optional parameter: HeaderFile
 *
 *     Revision 1.9  2001/04/23 20:22:19  friberg
 *     Added station name remapping using the StationId config parameter.
 *
 *     Revision 1.8  2000/11/07 19:38:33  kohler
 *     Modified to get parameter GPSLockAlarm from config file.
 *
 *     Revision 1.7  2000/08/30 17:32:46  lombard
 *     See ChangeLog entry for 30 August 2000
 *
 *     Revision 1.6  2000/07/28 22:36:10  lombard
 *     Moved heartbeats to separate thread; added DontQuick command; removed
 *     redo_com() since it doesn't do any good; other minor bug fixes
 *
 *     Revision 1.5  2000/07/03 18:00:37  lombard
 *     Added code to limit age of waiting packets; stops circ buffer overflows
 *     Added and Deleted some config params.
 *     Added check of K2 station name against restart file station name.
 *     See ChangeLog for complete list.
 *
 *     Revision 1.4  2000/06/09 23:14:23  lombard
 *     Several bug fixes and improvements; See Changelog entry of 2000-06-09.
 *
 *     Revision 1.3  2000/05/16 23:39:16  lombard
 *     bug fixes, removed OutputThread Keepalive, added OnBattery alarm
 *     made alarms report only once per occurence
 *
 *     Revision 1.2  2000/05/09 23:58:29  lombard
 *     Added restart mechanism
 *
 *     Revision 1.1  2000/05/04 23:47:45  lombard
 *     Initial revision
 *
 *
 *
 */
/*  getconfig.c:  Processes "k2ew.d" command configuration file */
/*  */
/*    3/12/99 -- [ET]  File modified from 'getconfig.c' from q2ew */
/*  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>       /* Earthworm main include file */
#include <kom.h>             /* command parcer functions (Earthworm util) */
#include "glbvars.h"         /* externs for global vars from 'k2ewmain.c' */


     /***************************************************************
      *                          get_config()                       *
      *         Processes command file using kom.c functions.       *
      *           Returns -1 if any errors are encountered.         *
      ***************************************************************/

#define NCOMMAND 7             /*  Number of commands in the config file */
#define NREQ_CMDS 7            /*  Number of required commands in config file */
#define LOCATION_ID_LENGTH 3   /* Length of location name, plus terminal null */
static int parse_chnames_str(char *chnames_str);   /* function defined below */
static int parse_locnames_str(char *locnames_str); /* function defined below */
static int parse_invpol_str(char *invpol_str);     /* function defined below */

                        /* string of "ChannelNames" remapping names: */
static char channel_names_str[(K2_MAX_STREAMS+2)*(CHANNEL_ID_LENGTH+3)];
                        /* string of "LocationNames" */
static char location_names_str[(K2_MAX_STREAMS+2)*(LOCATION_ID_LENGTH+3)];
                        /* string of invert-polarity flags: */
static char invpol_flags_str[K2_MAX_STREAMS*6];


int get_config( char *configfile )
{
   char     init[NCOMMAND];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;
   char     *sptr,*sptr2;
   int      len;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < NCOMMAND; i++ )
      init[i] = 0;

/* Initialize "ChannelNames" string ptrs and invert-polarity flags: */
   for(i=0; i<K2_MAX_STREAMS; ++i)
   {     /* for each channel (or stream) */
     gcfg_chnames_arr[i] = NULL;
     gcfg_locnames_arr[i] = NULL;
     gcfg_invpol_arr[i] = 0;
   }

   /* Initialize the network code */
   gcfg_network_buff[0] = 0;

   /* Initialize the IO mode to `none' */
   gen_io.mode = IO_NONE;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 )
   {
      fprintf(stderr, "%s: Error opening configuration file <%s>\n",
                                               g_progname_str, configfile );
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
         if ( com[0] == '@' )
         {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success )
            {
               fprintf(stderr, "%s: Error opening command file <%s>.\n",
                                                  g_progname_str, &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         if ( k_its( "ComPort" ) )
         {
           if ( gen_io.mode != IO_NONE )
           {
             fprintf(stderr, "%s: Too many comm modes used; second is %s\n",
                     g_progname_str, "ComPort");
             return -1;
           }
           gen_io.mode = IO_COM_NT;
           if((gen_io.com_io.commsel=k_int()) < 1 ||
                                      gen_io.com_io.commsel > 255)
           {      /* COM port value out of range; display error message */
             fprintf(stderr,"%s: ComPort value (%d) out of range\n",
                     g_progname_str,gen_io.com_io.commsel);
             return -1;          /* return error code */
           }
           init[0] = 1;
         }

         else if ( k_its( "TcpAddr" ) )
         {
           if ( gen_io.mode != IO_NONE )
           {
             fprintf(stderr, "%s: Too many comm modes used; second is %s\n",
                     g_progname_str, "TcpAddr");
             return -1;
           }
           gen_io.mode = IO_TCP;
           if ( (str = k_str()) != 0 )
           {
             if (strlen(str) > KC_MAX_ADDR_LEN - 1)
             {
               fprintf(stderr, "%s: TcpAddr too long; max is %d\n",
                       g_progname_str, KC_MAX_ADDR_LEN - 1);
               return -1;
             }
             strcpy(gen_io.tcp_io.k2_address, str);
             init[0] = 1;
           }
           /* Null string; error picked up later */
         }

         else if ( k_its( "TtyName" ) )
         {
           if ( gen_io.mode != IO_NONE )
           {
             fprintf(stderr, "%s: Too many comm modes used; second is %s\n",
                     g_progname_str, "TtyName");
             return -1;
           }
           gen_io.mode = IO_TTY_UN;
           if ( (str = k_str()) != 0 )
           {
             if (strlen(str) >= KC_MAX_TTY_LEN - 1)
             {
               fprintf(stderr, "%s: TtyName too long; max is %d\n",
                       g_progname_str, KC_MAX_TTY_LEN - 1);
               return -1;
             }
             strcpy(gen_io.tty_io.ttyname, str);
             init[0] = 1;
           }
           /* Null string; error picked up later */
         }

         else if ( k_its( "Speed" ) )
         {
           if (gen_io.mode != IO_COM_NT && gen_io.mode != IO_TTY_UN)
           {
             fprintf(stderr, "%s: %s specifed but serial mode comms not set\n",
                     g_progname_str, "Speed");
             return -1;
           }
           if (gen_io.mode == IO_COM_NT)
           {
             if((gen_io.com_io.speed=k_long()) < 150L ||
                gen_io.com_io.speed > 10000000L)
             {      /* baud rate value out of range; display error message */
               fprintf(stderr,"%s: Speed value (%d) out of range\n",
                       g_progname_str, gen_io.com_io.speed);
               return -1;          /* return error code */
             }
             init[1] = 1;
           }
           else
           {   /* mode == IO_TTY_UN */
             if((gen_io.tty_io.speed=k_long()) < 150L ||
                gen_io.tty_io.speed > 10000000L)
             {      /* baud rate value out of range; display error message */
               fprintf(stderr,"%s: Speed value (%d) out of range\n",
                       g_progname_str, gen_io.tty_io.speed);
               return -1;          /* return error code */
             }
             init[1] = 1;
           }
         }

         else if ( k_its( "TcpPort" ) )
         {
           if (gen_io.mode != IO_TCP)
           {
             fprintf(stderr, "%s: %s specifed but TCP mode comms not set\n",
                     g_progname_str, "TcpPort");
             return -1;
           }
           if((gen_io.tcp_io.k2_port=k_long()) < 1024L ||
              gen_io.tcp_io.k2_port > 100000L)
           {      /* tcp portvalue out of range; display error message */
             fprintf(stderr,"%s: TcpPort value (%d) out of range\n",
                     g_progname_str, gen_io.tcp_io.k2_port);
             return -1;          /* return error code */
           }
           init[1] = 1;
         }

         else if ( k_its( "ModuleId" ) )
         {
            if ( (str = k_str()) != 0 )
            {
                        /* copy module name; make sure NULL terminated */
               strncpy(gcfg_module_name,str,sizeof(gcfg_module_name)-1);
               gcfg_module_name[sizeof(gcfg_module_name)-1] = '\0';
               if ( GetModId(str, &gcfg_module_idnum) == -1 )
               {
                  fprintf( stderr, "%s: Invalid ModuleId <%s>. \n",
                                                      g_progname_str, str );
                  fprintf( stderr,
                      "%s: Please Register ModuleId <%s> in earthworm.d!\n",
                                                      g_progname_str, str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "RingName" ) )
         {
            if ( (str = k_str()) != NULL )
            {
                        /* copy ring name; make sure NULL terminated */
               strncpy(gcfg_ring_name,str,sizeof(gcfg_ring_name)-1);
               gcfg_ring_name[sizeof(gcfg_ring_name)-1] = '\0';
               if ( (gcfg_ring_key = GetKey(str)) == -1 )
               {
                  fprintf( stderr, "%s: Invalid RingName <%s>. \n",
                                                      g_progname_str, str );
                  return -1;
               }
            }
            init[3] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            gcfg_heartbeat_itvl = k_int();
            init[4] = 1;
         }

         else if ( k_its( "LogFile" ) )
         {
            gcfg_logfile_flgval = k_int();
            init[5] = 1;
         }

         else if ( k_its( "StationID" ) )
         {              /* copy stationID name; make sure NULL terminated */
            strncpy(gcfg_stnid_remap,k_str(),sizeof(gcfg_stnid_remap)-1);
            gcfg_stnid_remap[sizeof(gcfg_stnid_remap)-1] = '\0';
         }

         else if ( k_its( "ChannelNames" ) )
         {              /* get line of data from config file: */
            if((sptr=k_com()) != NULL && strlen(sptr) > 0)
            {      /* line of data fetched OK */
              sptr += strlen(k_get());      /* move past command string */
              len = strlen(sptr);           /* get current length */
              if((sptr2=strchr(sptr,'#')) != NULL)    /* if comment then */
                len = sptr2 - sptr - 1;     /* setup to remove comment */
              if(len > 0)
              {    /* still data remaining to process */
                if ( (size_t) len >= sizeof( channel_names_str ) )
                                                      /* if too long then */
                  len = sizeof(channel_names_str)-1;  /* shorten length */
                strncpy(channel_names_str,sptr,len);  /* copy data */
                channel_names_str[len] = '\0';        /* put in null char */
                if(parse_chnames_str(channel_names_str) < 0)
                  return -1;          /* if error then exit function */
              }
              else      /* no data found to process */
                channel_names_str[0] = '\0';     /* indicate error */
            }
            else   /* line of data not fetched OK */
              channel_names_str[0] = '\0';       /* indicate error */
            if(channel_names_str[0] == '\0')
            {      /* no data was found to process */
              fprintf(stderr,"%s: No data items found after '%s' command\n",
                                                    g_progname_str,k_get());
              return -1;      /* return error value */
            }
         }

         else if ( k_its( "LocationNames" ) )
         {              /* get line of data from config file: */
            if((sptr=k_com()) != NULL && strlen(sptr) > 0)
            {      /* line of data fetched OK */
              sptr += strlen(k_get());      /* move past command string */
              len = strlen(sptr);           /* get current length */
              if((sptr2=strchr(sptr,'#')) != NULL)    /* if comment then */
                len = sptr2 - sptr - 1;     /* setup to remove comment */
              if(len > 0)
              {    /* still data remaining to process */
                if ( (size_t) len >= sizeof( location_names_str ) )
                                                       /* if too long then */
                  len = sizeof(location_names_str)-1;  /* shorten length */
                strncpy(location_names_str,sptr,len);  /* copy data */
                location_names_str[len] = '\0';        /* put in null char */
                if(parse_locnames_str(location_names_str) < 0)
                  return -1;          /* if error then exit function */
              }
              else      /* no data found to process */
                location_names_str[0] = '\0';     /* indicate error */
            }
            else   /* line of data not fetched OK */
              location_names_str[0] = '\0';       /* indicate error */
            if(location_names_str[0] == '\0')
            {      /* no data was found to process */
              fprintf(stderr,"%s: No data items found after '%s' command\n",
                                                    g_progname_str,k_get());
              return -1;      /* return error value */
            }
         }

         else if ( k_its( "InvPolFlags" ) )
         {              /* get line of data from config file: */
            if((sptr=k_com()) != NULL && strlen(sptr) > 0)
            {      /* line of data fetched OK */
              sptr += strlen(k_get());      /* move past command string */
              len = strlen(sptr);           /* get current length */
              if((sptr2=strchr(sptr,'#')) != NULL)    /* if comment then */
                len = sptr2 - sptr - 1;     /* setup to remove comment */
              if(len > 0)
              {    /* still data remaining to process */
                if ( (size_t) len >= sizeof( invpol_flags_str ) )
                                                      /* if too long then */
                  len = sizeof(invpol_flags_str)-1;   /* shorten length */
                strncpy(invpol_flags_str,sptr,len);   /* copy data */
                invpol_flags_str[len] = '\0';         /* put in null char */
                if(parse_invpol_str(invpol_flags_str) < 0)
                  return -1;          /* if error then exit function */
              }
              else      /* no data found to process */
                invpol_flags_str[0] = '\0';     /* indicate error */
            }
            else   /* line of data not fetched OK */
              invpol_flags_str[0] = '\0';       /* indicate error */
            if(invpol_flags_str[0] == '\0')
            {      /* no data was found to process */
              fprintf(stderr,"%s: No data items found after '%s' command\n",
                                                    g_progname_str,k_get());
              return -1;      /* return error value */
            }
         }

         else if (k_its( "LCFlag" ) )
         {
           gcfg_lc_flag = k_int();
	   init[6] = 1;
         }

         /* Optional Commands */
         else if ( k_its( "Network" ) )
         {                             /* copy over network name */
            strncpy(gcfg_network_buff,k_str(),sizeof(gcfg_network_buff)-1);
            /* make sure buffer is NULL terminated */
            gcfg_network_buff[sizeof(gcfg_network_buff)-1] = '\0';
         }

         else if (k_its( "BasePinno" ) )
         {
           gcfg_base_pinno = k_int();
         }

         else if (k_its( "StatusInterval" ) )
         {
           gcfg_status_itvl = 60 * k_int();
         }

         else if (k_its( "CommTimeout" ) )
         {
           gcfg_commtimeout_itvl = k_int();
         }

         else if (k_its( "DontQuit" ) )
         {
           gcfg_dont_quit = 1;
         }

         else if (k_its( "InjectInfo" ) )
         {
           gcfg_inject_info = 1;
         }

         else if (k_its( "ForceBlockMode" ) )
         {
           gcfg_force_blkmde = k_int();
         }

         else if (k_its( "WaitTime" ) )
         {
           gcfg_pktwait_lim = k_int();
         }

         else if (k_its( "MaxBlkResends" ) )
         {
           gcfg_max_blkresends = k_int();
         }

         else if (k_its( "MaxReqPending" ) )
         {
           gcfg_max_reqpending = k_int();
         }

         else if (k_its( "ResumeReqVal" ) )
         {
           gcfg_resume_reqval = k_int();
         }

         else if (k_its( "WaitResendVal" ) )
         {
           gcfg_wait_resendval = k_int();
         }

         else if (k_its( "RestartComm" ) )
         {
           gcfg_restart_commflag = 1;
         }

         else if (k_its( "OnBattery" ) )
         {
           gcfg_on_batt = 1;
         }

         else if (k_its( "LowBattAlarm" ) )
         {
           gcfg_battery_alarm = k_int();
         }

         else if (k_its( "MinDiskKB" ) )
         {
           gcfg_disk_alarm[0] = 1024.0 * (double)k_int();
           gcfg_disk_alarm[1] = 1024.0 * (double)k_int();
         }

         else if (k_its( "ExtStatus" ) )
         {
           gcfg_ext_status = 1;
         }

         else if (k_its( "LowExtAlarm" ) )         {  /* Not currently used; leave here to be compatable with old config files */
           gcfg_extern_alarm = k_int();
         }

         else if (k_its( "LowTempAlarm" ) )
         {
           gcfg_templo_alarm = k_int();
         }

         else if (k_its( "HighTempAlarm" ) )
         {
           gcfg_temphi_alarm = k_int();
         }

         else if (k_its( "GPSLockAlarm" ) )
         {
           gcfg_gpslock_alarm = k_val();
         }

         else if (k_its( "MaxRestartAge" ) )
         {
           gcfg_restart_maxage = k_int();
         }

         else if (k_its( "RestartFile" ) )
         {
           if ( (str = k_str()) != NULL)
           {
             if (strlen(str) >= MAXNAMELEN-1)
             {
               fprintf(stderr, "%s: RestartFile name too long; max is %d\n",
                       g_progname_str, MAXNAMELEN-1);
               return -1;
             }
             strcpy(gcfg_restart_filename, str);
           }
         }

         else if (k_its( "HeaderFile" ) )
         {
           if ( (str = k_str()) != NULL)
           {
             if (strlen(str) >= MAXNAMELEN-1)
             {
               fprintf(stderr, "%s: HeaderFile name too long; max is %d\n",
                       g_progname_str, MAXNAMELEN-1);
               return -1;
             }
             strcpy(gcfg_header_filename, str);
           }
         }

         else if (k_its( "Debug" ) )
         {
           gcfg_debug = k_int();
         }

         else
         {
            /* An unknown parameter was encountered */
               fprintf( stderr, "%s: <%s> unknown parameter in <%s>\n",
                                           g_progname_str,com, configfile );
               return -1;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            fprintf( stderr, "%s: Bad <%s> command in <%s>.\n",
                                          g_progname_str, com, configfile );
            return -1;
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check flags for missed required commands
   ***********************************************************/
   nmiss = 0;
   for ( i = 0; i < NREQ_CMDS; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 )
   {
      fprintf( stderr,"%s: ERROR, no ", g_progname_str );
      if ( !init[0] ) fprintf(stderr, "<ComPort>, <TcpAddr> or <TtyName> " );
      if ( !init[1] ) fprintf(stderr, "<Speed> or <TcpPort>" );
      if ( !init[2] ) fprintf(stderr, "<ModuleId> " );
      if ( !init[3] ) fprintf(stderr, "<RingName> " );
      if ( !init[4] ) fprintf(stderr, "<HeartbeatInt> " );
      if ( !init[5] ) fprintf(stderr, "<LogFile> " );
      if ( !init[6] ) fprintf(stderr, "<LCFlag> " );
      fprintf(stderr, "command(s) in <%s>.\n", configfile );
      return -1;
   }

   return 0;
}


/************************************************************************
 * parse_chnames_str:  Parses channel-name items from the given string; *
 *      items may be separated by spaces or commas; the global          *
 *      'gcfg_chnames_arr[]' array is loaded with pointers to           *
 *      the items found in the given string; returns 0 if successful,   *
 *      returns -1 if error.                                            *
 ************************************************************************/
int parse_chnames_str(char *chnames_str)
{
  int spos,epos,len,stmnum,val;
  char ch;

  if(chnames_str == NULL || (len=strlen(chnames_str)) <= 0)
    return 0;           /* if no data then just return */

  spos = stmnum = 0;
  do
  {      /* for each channel name item in string */
    while(chnames_str[spos] <= ' ')
    {    /* skip any leading whitespace */
      if(++spos >= len)      /* if end of string then */
        return 0;            /* exit function */
    }
    epos = spos;
    while(epos < len &&           /* find next whitespace or separator */
                     (ch=chnames_str[epos]) > ' ' && ch != ',' && ch != ';')
    {
      ++epos;
    }
    if((val=epos-spos) > 0)
    {    /* item contains data */
      if(stmnum >= K2_MAX_STREAMS)
      {    /* too many items; show error message and exit function */
        fprintf(stderr,"%s: Too many 'ChannelNames' items (max=%d)\n",
                                             g_progname_str,K2_MAX_STREAMS);
        return -1;      /* return error value */
      }
                                       /* set pointer to start of item: */
      gcfg_chnames_arr[stmnum] = &chnames_str[spos];
      if(val <= CHANNEL_ID_LENGTH+1)
      {       /* item length is within limit */
                   /* null terminate string after item: */
        if(epos < len)                 /* if not at end of string then */
          chnames_str[epos] = '\0';    /* put null-char over separator */
      }
      else    /* item length too long; shorten length of item */
        chnames_str[spos+CHANNEL_ID_LENGTH+1] = '\0';
    }
    spos = epos + 1;         /* parse at next char after separator */
    ++stmnum;                /* increment stream number */
  }
  while(epos < len);         /* loop if not at end of string */
  return 0;        /* return OK value */
}


/************************************************************************
 * parse_locnames_str:  Parses location-name items from the given string; *
 *      items may be separated by spaces or commas; the global          *
 *      'gcfg_locnames_arr[]' array is loaded with pointers to           *
 *      the items found in the given string; returns 0 if successful,   *
 *      returns -1 if error.                                            *
 ************************************************************************/
int parse_locnames_str(char *locnames_str)
{
  int spos,epos,len,stmnum,val;
  char ch;

  if(locnames_str == NULL || (len=strlen(locnames_str)) <= 0)
    return 0;           /* if no data then just return */

  spos = stmnum = 0;
  do
  {      /* for each location name item in string */
    while(locnames_str[spos] <= ' ')
    {    /* skip any leading whitespace */
      if(++spos >= len)      /* if end of string then */
        return 0;            /* exit function */
    }
    epos = spos;
    while(epos < len &&           /* find next whitespace or separator */
                     (ch=locnames_str[epos]) > ' ' && ch != ',' && ch != ';')
    {
      ++epos;
    }
    if((val=epos-spos) > 0)
    {    /* item contains data */
      if(stmnum >= K2_MAX_STREAMS)
      {    /* too many items; show error message and exit function */
        fprintf(stderr,"%s: Too many 'LocationNames' items (max=%d)\n",
                                             g_progname_str,K2_MAX_STREAMS);
        return -1;      /* return error value */
      }
                                       /* set pointer to start of item: */
      gcfg_locnames_arr[stmnum] = &locnames_str[spos];
      if(val <= LOCATION_ID_LENGTH+1)
      {       /* item length is within limit */
                   /* null terminate string after item: */
        if(epos < len)                 /* if not at end of string then */
          locnames_str[epos] = '\0';    /* put null-char over separator */
      }
      else    /* item length too long; shorten length of item */
        locnames_str[spos+LOCATION_ID_LENGTH+1] = '\0';
    }
    spos = epos + 1;         /* parse at next char after separator */
    ++stmnum;                /* increment stream number */
  }
  while(epos < len);         /* loop if not at end of string */
  return 0;        /* return OK value */
}


/************************************************************************
 * parse_invpol_str:  Parses invert-polarity flag values ("0" or "1")
 *      from the given string; values may be separated by spaces or
 *      commas; the global 'gcfg_invpol_arr[]' array is loaded with
 *      the flag values (0 or 1); returns 0 if successful, returns -1
 *      if error.
 ************************************************************************/
int parse_invpol_str(char *invpol_str)
{
  int spos,epos,len,stmnum,val;
  char ch;

  if(invpol_str == NULL || (len=strlen(invpol_str)) <= 0)
    return 0;           /* if no data then just return */

  spos = stmnum = 0;
  do
  {      /* for each invert-polarity flag item in string */
    while(invpol_str[spos] <= ' ')
    {    /* skip any leading whitespace */
      if(++spos >= len)      /* if end of string then */
        return 0;            /* exit function */
    }
    epos = spos;
    while(epos < len &&           /* find next whitespace or separator */
                      (ch=invpol_str[epos]) > ' ' && ch != ',' && ch != ';')
    {
      ++epos;
    }
    if((val=epos-spos) > 0)
    {    /* item contains data */
      if(stmnum >= K2_MAX_STREAMS)
      {    /* too many items; show error message and exit function */
        fprintf(stderr,"%s: Too many 'InvPolFlags' items (max=%d)\n",
                                             g_progname_str,K2_MAX_STREAMS);
        return -1;      /* return error value */
      }
                   /* null terminate string after item: */
      if(epos < len)                   /* if not at end of string then */
        invpol_str[epos] = '\0';       /* put null-char over separator */
      if(val > 1 || (invpol_str[spos] != '0' && invpol_str[spos] != '1'))
      {       /* item length not 1 or item is not "0" or "1" */
        fprintf(stderr,"%s: Illegal 'InvPolFlags' item:  \"%s\"\n",
                                          g_progname_str,&invpol_str[spos]);
        return -1;      /* return error value */
      }
                   /* convert to integer and save in array: */
      gcfg_invpol_arr[stmnum] = atoi(&invpol_str[spos]);
    }
    spos = epos + 1;         /* parse at next char after separator */
    ++stmnum;                /* increment stream number */
  }
  while(epos < len);         /* loop if not at end of string */
  return 0;        /* return OK value */
}

