
// Standard includes
#include <stdio.h>
#include <time.h>

// Earthworm includes
#include <earthworm.h>
#include <transport.h>
#include <time_ew.h>
#include <trace_buf.h>

// Local includes
#include "mseedarchplayer.h"

// Global Variables


//
// Main program starts here
//
int main(int argc, char **argv) 
{

   PARAMS       Parm;         // Input parameters
   EWH          Ewh;          // Earthworm settings
   
   time_t       timeNow;      // current time
   time_t       timeLastBeat; // time of last heartbeat
   double       slotStart;    // Start time of current slot
   double       slotEnd;      // End time of current slot
   double       deltaTime;    // Shift for SendLate option
   double       SiniTime;     // To compute waiting time
   double       SendTime;     // To compute waiting time
   long         waitTime;     // Waiting time on each slot
   int          i;            // A counter
   
   // Check command line arguments
   if (argc != 2) 
   {
      fprintf(stderr, "Usage: mseedarchplayer <configfile>\n");
      return EW_FAILURE;
   }
   

   // Initialize name of log-file & open it
   logit_init(argv[1], 0, 256, 1);
   

   // Read the configuration file(s)
   config(argv[1], &Parm);
   

   // Lookup important information from earthworm.d
   lookup(&Ewh, &Parm);
   

   // Set logit to LogSwitch read from configfile
   logit_init(argv[1], 0, 256, Parm.LogSwitch);
   logit("", "mseedarchplayer: Read command file <%s>\n", argv[1]);
   
   // Attach to shared memory rings
   tport_attach( &Ewh.OutRegion, Parm.OutKey );
   logit("", "mseedarchplayer: Attached to public memory region: %ld\n",
      Ewh.OutRegion );
      
   // Force a heartbeat to be issued in first pass thru main loop
   timeLastBeat = time(&timeNow) - Parm.HeartbeatInt - 1;
   
   // Define initial slot times
   slotStart = Parm.StartTime;
   slotEnd = slotStart + (double)SLOT_DURATION / 1000;   
   
   // Define send late option
   if (Parm.SendLate != -1.0)
   {
      hrtime_ew(&deltaTime);
      deltaTime = slotStart - deltaTime;
   }
      
      
   // Start main loop
   while ( tport_getflag( &Ewh.OutRegion ) != TERMINATE  &&
      tport_getflag( &Ewh.OutRegion ) != Ewh.MyPid &&
      slotStart < Parm.EndTime ) 
   {
      // Compute current time
      hrtime_ew(&SiniTime);
      
      // Send heartbeat
      if( Parm.HeartbeatInt  &&  time(&timeNow)-timeLastBeat >= Parm.HeartbeatInt ) 
      {
         timeLastBeat = timeNow;
         status(Ewh.TypeHeartBeat, 0, "", Ewh.OutRegion, Parm, Ewh);
      }
      
      // Process data for each station individually
      fprintf(stdout,".");
      fflush( stdout );
      for (i=0; i<Parm.nChannels; i++)
         if (ProcessData(&Parm.Channels[i], slotStart, slotEnd, deltaTime,
            &Parm, &Ewh) != 0)
            {
               //printf("Channel     %s.%s\n", Parm.Channels[i].stat, Parm.Channels[i].chan);
               if (Parm.Debug) 
                  logit("e", "Error processing slot %f - %f\n", slotStart, slotEnd);
            }
      
      // Compute waiting time
      SendTime = SiniTime + (double)SLOT_DURATION / 1000; //Next slot time
      hrtime_ew(&SiniTime); //Current time
      waitTime = (long)((SendTime - SiniTime) * 1000 + 0.5); //Waiting time
      
      if (Parm.InterMessageDelayMillisecs == 0 && waitTime>0) // Wait until next slot time
         sleep_ew(waitTime);
      if (Parm.InterMessageDelayMillisecs > 1) // Wait that many miliseconds
         sleep_ew(Parm.InterMessageDelayMillisecs);
         
      // Update slot times
      slotStart += (double)SLOT_DURATION / 1000;
      slotEnd += (double)SLOT_DURATION / 1000;
   }
   printf("\n");
   
   /* detach from shared memory */
   tport_detach( &Ewh.OutRegion );

   /* write a termination msg to log file */
   logit( "t", "mseedarchplayer: Termination requested; exiting!\n" );
   fflush( stdout );
   return(0);
}














