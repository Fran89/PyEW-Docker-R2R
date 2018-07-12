/*****************************************************************************
 *  config() processes command file(s) using kom.c functions;                *
 *                    exits if any errors are encountered.                   *
 *****************************************************************************/
 
 
// Standard includes
#include <stdlib.h>
#include <string.h>

// Earthworm Includes
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <chron3.h>

// Local Includes
#include "mseedarchplayer.h" 
 
// Defines
#define ncommand 9       

// Function starts here
void config(char *configfile, PARAMS *Parm) 
{
   char    init[ncommand]; // init flags, one byte for each required command
   int     nmiss; // number of required commands that were missed  
   char   *com;
   char   *str;
   int     nfiles;
   int     success;
   int     i;

   /* Set to zero one init flag for each required command
    *****************************************************/
   for (i = 0; i < ncommand; i++)
      init[i] = 0;

   /* Open the main configuration file
    **********************************/
   nfiles = k_open(configfile);
   if (nfiles == 0) 
   {
      logit("e",
         "mseedarchplayer: Error opening command file <%s>; exiting!\n",
         configfile);
      exit(-1);
   }
   
   
   // Set all default parameters
   Parm->InterMessageDelayMillisecs = 0;
   Parm->nChannels = 0;
   Parm->SendLate = -1.0;

   /* Process all command files
    ***************************/
   while (nfiles > 0) /* While there are command files open */ 
   {
      while (k_rd()) /* Read next line from active file  */ 
      {
         com = k_str(); /* Get the first token from line */

         /* Ignore blank lines & comments
          *******************************/
         if (!com) continue;
         if (com[0] == '#') continue;

         /* Open a nested configuration file
          **********************************/
         if (com[0] == '@') {
            success = nfiles + 1;
            nfiles = k_open(&com[1]);
            if (nfiles != success) 
            {
               logit("e",
                  "mseedarchplayer: Error opening command file <%s>; exiting!\n",
                  &com[1]);
               exit(-1);
            }
            continue;
         }

         /* Process anything else as a command
          ************************************/
   /*0*/ if (k_its("LogFile")) 
         {
            Parm->LogSwitch = k_int();
            if (Parm->LogSwitch < 0 || Parm->LogSwitch > 2) 
            {
               logit("e",
                  "mseedarchplayer: Invalid <LogFile> value %d; "
                  "must = 0, 1 or 2; exiting!\n", Parm->LogSwitch);
               exit(-1);
            }
            init[0] = 1;
         } 
   /*1*/ else if (k_its("MyModuleId"))
         {
            if ((str = k_str()) != NULL) {
               if (GetModId(str, &(Parm->MyModId)) != 0) 
               {
                   logit("e",
                      "mseedarchplayer: Invalid module name <%s> "
                      "in <MyModuleId> command; exiting!\n", str);
                   exit(-1);
               }
            }
            init[1] = 1;
            }
   /*2*/ else if (k_its("OutRing")) 
         {
            if ((str = k_str()) != NULL) 
            {
               if ((Parm->OutKey = GetKey(str)) == -1) 
               {
                  logit("e",
                     "ewhtmlemail: Invalid ring name <%s> "
                     "in <InRing> command; exiting!\n", str);
                  exit(-1);
               }
            }
            init[2] = 1;
         } 
   /*3*/ else if (k_its("HeartbeatInt"))
         {
            Parm->HeartbeatInt = k_long();
            init[3] = 1;
         }
   /*4*/ else if (k_its("Debug")) 
         {
            Parm->Debug = k_int();
            init[4] = 1;
         }
   /*5*/ else if (k_its("StartTime")) 
         {
            str = k_str();
            if ((epochsec17(&Parm->StartTime, str)) == -1) 
            {
               logit("et",
                  "mseedarchplayer: Invalid start time <%s>. Use yyyymmddhhmmss.ff \n", str);
               exit(-1);
            }
            init[5] = 1;
         }
   /*6*/ else if (k_its("Duration")) 
         {
            Parm->EndTime = (double)k_int();
            init[6] = 1;
         }
   /*7*/ else if (k_its("MseedArchiveFolder")) 
         {
            if( ( str=k_str() ) != NULL ) 
            {
	       if( strlen(str) > (size_t)FNAME_LEN-1 ) 
	       {
	          logit( "e",
		     "mseedarchplayer: Archive folder name <%s> too long.\n", str );
	          logit( "e", " cmd; max=%d; exiting!\n", FNAME_LEN-1 );
	          exit( -1 );
	       }
	    }
	    strcpy( Parm->MseedArchiveFolder, str );
            init[7] = 1;
         }
   /*8*/ else if (k_its("MseedArchiveFormat")) 
         {
            Parm->MseedArchiveFormat = k_int();
            if (Parm->MseedArchiveFormat<0 || Parm->MseedArchiveFormat>1)
            {
               logit( "e",
                  "Invalid archive format: %d.\n"
                  "Use %d for Buffer of Uniform Data (BUD) or\n"
                  "1 for SeisComP default formats.\n",
		  MAX_CHANNELS, FORMAT_BUD, FORMAT_SCP);
	       exit( -1 );
            }
            init[8] = 1;
         }       
         
         else if (k_its("SendLate")) 
         {
            Parm->SendLate = k_val();
         }
         
         
         //Stations
         else if (k_its("SCNLocSz")) 
         {
            if (Parm->nChannels == MAX_CHANNELS)
            {
               logit( "e",
                  "mseedarchplayer: Maximum number of channels (%d) exceeded.\n",
		  MAX_CHANNELS);
	       exit( -1 );
            }
            strcpy(Parm->Channels[Parm->nChannels].stat, k_str());
            strcpy(Parm->Channels[Parm->nChannels].chan, k_str());
            strcpy(Parm->Channels[Parm->nChannels].net, k_str());
            str = k_str();
            if (strlen(str)>3)  //It is NONE
            {
               strcpy(Parm->Channels[Parm->nChannels].loc,"");
            }
            else
            {
               strcpy(Parm->Channels[Parm->nChannels].loc, str);
            }
            //Reserve memory for samples
	    /*
            if ((Parm->Channels[Parm->nChannels].samples = malloc( MAX_SAMPLE_BUFFER * sizeof(int)))==NULL)
            {
               logit( "et",
                  "mseedarchplayer: Unable to allocate memory buffers.\n");
               exit(-1);
            }
	    */
            //Parm->Channels[Parm->nChannels].nsamples = 0;
            Parm->Channels[Parm->nChannels].msfp = NULL;
	    Parm->Channels[Parm->nChannels].msr = NULL;
	    //Parm->Channels[Parm->nChannels].startTime = 0;
	    //Parm->Channels[Parm->nChannels].endTime = 0;
            Parm->Channels[Parm->nChannels].nextRead = 0;
            Parm->nChannels++;
         }       
          
         else if (k_its("InterMessageDelayMillisecs")) 
         {
            Parm->InterMessageDelayMillisecs = k_int();
         }

         /* See if there were any errors processing the command
          *****************************************************/
         if (k_err())
         {
            logit("e",
               "mseedarchplayer: Bad <%s> command in <%s>; exiting!\n",
               com, configfile);
            exit(-1);
         }
      }
      nfiles = k_close();
   }

   /* After all files are closed, check init flags for missed commands
    ******************************************************************/
   nmiss = 0;
   for (i = 0; i < ncommand; i++) if (!init[i]) nmiss++;
   if (nmiss) 
   {
      logit("e", "mseedarchplayer: ERROR, no ");
      if (!init[0]) logit("e", "<LogFile> ");
      if (!init[1]) logit("e", "<MyModuleId> ");
      if (!init[2]) logit("e", "<InRing> ");
      if (!init[3]) logit("e", "<HeartbeatInt> ");
      if (!init[4]) logit("e", "<Debug> ");
      if (!init[5]) logit("e", "<StartTime> ");
      if (!init[6]) logit("e", "<Duration> ");
      if (!init[7]) logit("e", "<MseedArchiveFolder> ");
      if (!init[8]) logit("e", "<MseedArchiveFormat> ");
      logit("e", "command(s) in <%s>; exiting!\n", configfile);
      exit(-1);
   }
   
   //Correct endtime
   Parm->EndTime += Parm->StartTime;
   
   //Check station count
   if (Parm->nChannels==0)
   {
      logit("e", "No stations configured.\n");
      exit(-1);
   }
   
   //Done!
   return;
}
