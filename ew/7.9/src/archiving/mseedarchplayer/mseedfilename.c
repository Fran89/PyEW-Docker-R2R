/******************************************************************************
 * MseedFileName() : Function to compute a filename from station data, time   *
 *                   and type of miniseed archive                             *
 ******************************************************************************/
//Defines
#define FILE_SEPARATOR "/"  // File separator

#include <earthworm.h>
//Libmseed include
#include <libmseed.h>

//Local Includes
#include "mseedarchplayer.h"

// Local includes
char* MseedFileName(char* filename, char* foldername, double t,
   char* stat, char* chan, char* net, char* loc,
   int archiveFormat)
{
   BTime btime;
   
   //Compute btime to have a day of year estimate
   if ((ms_hptime2btime ( (hptime_t)(t * 1000000 + 0.5), &btime )) == -1)
   {
      logit("e","Error computing time: %f\n", t);
      exit(-1);
   }
   //Compute file name based on archive type
   switch (archiveFormat)
   {
      case FORMAT_BUD:
         sprintf(filename, "%s/%s/%s/%s.%s.%s.%s.%d.%03d",
            foldername, net, stat, stat, net, loc, chan, btime.year, btime.day);
         break;
      case FORMAT_SCP:
         sprintf(filename, "%d/%s/%s/%s.D/%s.%s.%s.%s.D.%d.%03d", 
	    btime.year, net, stat, chan, net, stat, loc, chan, btime.year, btime.day);
         break;
   }
   return filename;
} 

