

// Earthworm Includes
#include <earthworm.h>
#include <trace_buf.h>

// Local Includes
#include "mseedarchplayer.h"

/* internal func prototypes */
int sendSamples(MSEEDCHANNEL *channel, double starttime, double endtime, 
   double deltaTime, PARAMS *Parm, EWH * Ewh);
int getRecord(MSEEDCHANNEL *channel, double starttime, double endtime, PARAMS *Parm);
int getDay(double t);



int ProcessData(MSEEDCHANNEL *channel, double starttime, double endtime, 
   double deltaTime, PARAMS *Parm, EWH * Ewh)
{

   //Try to read and send all the data for this slot
   channel->nextRead = starttime;
   while (channel->nextRead<endtime)
   {
      //Check if the current record contains samples for this slot
      if (channel->msr != NULL &&
         channel->recStart < endtime && 
	 channel->recEnd > channel->nextRead)
      {	 
         //Try to send the samples
	 if (sendSamples(channel, channel->nextRead, endtime, 
	    deltaTime, Parm, Ewh) != 0)
            return -1; //Failed to send samples
      }
      else
      {
	 if (getRecord(channel, channel->nextRead, endtime, Parm) != 0)
	    return -1; //Unable to capture a record
      }
   }
   return 0;
}


/*
 * Function to find the first suitable record for a given slot 
 */
int getRecord(MSEEDCHANNEL *channel, double starttime, double endtime, PARAMS *Parm)
{
   char             mseedFile[2 * FNAME_LEN];
   FILE            *tempFile;
   
   // Check if there is a record already or if existing, the day is the same
   //Compute name for new file
   MseedFileName(mseedFile, Parm->MseedArchiveFolder, starttime,
      channel->stat, channel->chan, channel->net, channel->loc,
      Parm->MseedArchiveFormat);
      
   if (channel->msr == NULL ||
      (getDay(channel->recStart) != getDay(starttime)))
   {
      if (Parm->Debug) printf("Loading file: %s\n", mseedFile);
      
      //Release last record
      if (channel->msr != NULL)
         ms_readmsr_r (&channel->msfp, &channel->msr, 
	    NULL, 0,NULL, NULL, 0, 0, 0); //Reset record and free file
   }
   
   //Start search
   do
   {
      //Check if file exists
      if ((tempFile = fopen(mseedFile,"r")) != NULL)
         fclose(tempFile); //The file exists
      else
         return -1; //The file does not exist
               
      //Open file to get record
      if (ms_readmsr_r(&channel->msfp, &channel->msr, 
         mseedFile, 0, NULL, NULL, 1, 1, 0) !=MS_NOERROR)
      {
         //logit("e", "Unable to open miniSEED file %s\n", mseedFile);
         ms_readmsr_r (&channel->msfp, &channel->msr, 
            NULL, 0,NULL, NULL, 0, 0, 0); //Reset record and free file
	 
         //Check if the endtime is on the same day
         if (getDay(starttime) != getDay(endtime))
         {
            //There is still a possibility to get a record for part of the slot
            //Compute name for new file, this time with endtime
            MseedFileName(mseedFile, Parm->MseedArchiveFolder, endtime,
               channel->stat, channel->chan, channel->net, channel->loc,
               Parm->MseedArchiveFormat);
            if (Parm->Debug) printf("As alternative, to file %s\n", mseedFile);
         
            //Check if file exists
            if ((tempFile = fopen(mseedFile,"r")) != NULL)
               fclose(tempFile); //The file exists
            else
               return -1; //The file does not exist
            
            if (ms_readmsr_r(&channel->msfp, &channel->msr, 
               mseedFile, 0, NULL, NULL, 1, 1, 0) !=MS_NOERROR)
            {
               //logit("e", "Unable to open miniSEED file %s\n", mseedFile);
               ms_readmsr_r (&channel->msfp, &channel->msr, 
                  NULL, 0,NULL, NULL, 0, 0, -1); //Reset record and free file
	       return -1;
            }
         }
         else
         {
            return -1;
         }
      }
      //Compute record limits
      channel->recStart = (double)msr_starttime(channel->msr) / 1000000.0;
      channel->recEnd = channel->recStart + 
         (double)channel->msr->samplecnt / channel->msr->samprate; 
      //Make sure that we do not overshoot the interval - case of gap!
      if (channel->recStart > endtime)
         return -1;
   } while (channel->recEnd < starttime);
      
   return 0;
}




/*
 * Function to send the available samples until the maximum of the slot end
 */
int sendSamples(MSEEDCHANNEL *channel, double starttime, double endtime, 
   double deltaTime, PARAMS *Parm, EWH * Ewh)
{
   char            *traceBuf;                      
   TRACE2_HEADER   *traceHead;
   int             *traceDat;
   int              readPos;
   double           sampleInstant;

   
   // Reserve memory for tracebuf
   if ((traceBuf = (char*) malloc(4096)) == NULL)
   {
      logit("e", "Unable to reserve memory for tracebuf messages\n");
      exit(-1);
   }
   traceHead = (TRACE2_HEADER*)traceBuf;
   traceDat = (int*)(traceBuf + 64);
   
   // Use the stupid approach
   traceHead->starttime = 0;
   traceHead->nsamp = 0;
   for (readPos = 0; readPos < channel->msr->samplecnt; readPos++)
   {
      //Calculate sample instant
      sampleInstant = channel->recStart
         + (double)readPos / channel->msr->samprate;
      
      //Check if sample is valid
      if (sampleInstant >= (starttime - 0.5 / channel->msr->samprate))
      {
         if (sampleInstant >= (endtime - 0.5 / channel->msr->samprate))
            break;  //Already over the interval
            
         //Check if this is the first sample
         if (traceHead->starttime == 0)
            traceHead->starttime = sampleInstant; //Update trace header
         else
            traceHead->endtime = sampleInstant;
            
         //Include sample
         *(traceDat + traceHead->nsamp++) = 
            *((int*)channel->msr->datasamples + readPos);
      }
   }
   
   //Update next read
   channel->nextRead = traceHead->endtime + 0.5 / channel->msr->samprate;
   if ((endtime - channel->nextRead) < (1 / channel->msr->samprate))
   {
      channel->nextRead = endtime;
   }
   else if ((channel->recEnd - channel->nextRead) < (1 / channel->msr->samprate))
   {
      channel->nextRead = channel->recEnd;
   }
      
   //Send tracebuf message
   if (traceHead->nsamp > 0)
   {
      // Prepare message header
      traceHead->pinno = 0;
      traceHead->samprate = channel->msr->samprate; // Sample rate; nominal
      traceHead->version[0] = TRACE2_VERSION0; // Header version number
      traceHead->version[1] = TRACE2_VERSION1; // Header version number
      traceHead->quality[0] = '\0';            // One bit per condition
      traceHead->quality[1] = '\0';            // One bit per condition

#ifdef _INTEL
      strcpy(traceHead->datatype, "i4");
#endif
#ifdef _SPARC
      strcpy(traceHead->datatype, "s4");
#endif

      strcpy(traceHead->sta, channel->stat);  // Site name
      strcpy(traceHead->net, channel->net);   // Network name
      strcpy(traceHead->chan, channel->chan); // Component/channel code
      if (strlen(channel->loc)==0)
         strcpy(traceHead->loc, "--");
      else
         strcpy(traceHead->loc, channel->loc);  // Location code
         
      //Set sendlate shift
      traceHead->starttime -= deltaTime;
      traceHead->endtime -= deltaTime;
      
      //Send message to ring
      tport_putmsg(&Ewh->OutRegion, &Ewh->trb2logo, 
         64 + traceHead->nsamp * 4, traceBuf );   
   }
   return 0;
}




/*
 * Function to compute the day for a given time
 */
int getDay(double t)
{
   return (int)(t / 24.0 / 3600.0);
}


























