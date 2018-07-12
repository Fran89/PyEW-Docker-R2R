
      /*****************************************************************
       *                        latency_mon.c                          *
       *                                                               *
       *  This program takes waveforms dropped in InRing and           *
       *  logs latency and outages of the data.  Each time a trace     *
       *  arrives, its start time is compared to the end time of the   *
       *  last packet.  When there is a gap the outage is logged to    *
       *  a data file for that station.  Also, the start time is       *
       *  compared to real-time, and latency is determined.  Whenever  *
       *  the latency changes, it is also logged with start and end    *
       *  time to the file.  Latencies are grouped generally: <1' = 0; *
       *  1'-2' = 1; 2'-3' = 2; 3'-5' = 3; >5' = 4.                    *
       *  Here, latency is described as present time minus mid-time    *
       *  of the packet.                                               *
       *                                                               *
       *  The graphical output shows a line for each station in the    *
       *  .sta file for a set length of time.  Colors on the line      *
       *  represent the latency of the station over that time interval.*
       *                                                               *
       *  This will only give accurate outage values if this module is *
       *  running all the time the earthworm system is up.             *
       *                                                               *
       *  This module is strictly for use under Windows.               *
       *                                                               *
       *  Written by Paul Whitmore, (WC/ATWC) August, 2001             *
       *                                                               *
       ****************************************************************/

/* slightly hacked by alex to read FindWave files 6/12/2 */
/* John Patton did:
     Thickened status lines.
	 Changed status line colors to:
	    Green < 1
		Yellow = 1-2 min
		Orange = 2-3 min
		Red = 3 - 5 min
		White > 5 min or out
	 Added network to Station ID.
	 Changed Station ID from red to black.
	 Added Current status indicator between status lines and Summary
	 Enabled the program to save a file summary.txt which is
	 identical to the printer output.
	 Made the refresh button redraw summary lines to current time
*/

#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <earthworm.h>
#include <transport.h>
#include <swap.h>
#include "latency_mon.h"

#define MAX_DOUBLE 1.7976931348623158E+308
#define MIN_DOUBLE 2.2250738585072014E-308

/* Global Variables
   ****************/
EWH               Ewh;            /* Parameters from earthworm.h */
MSG_LOGO          getlogoW;       /* Logo of requested waveforms */
GPARM             Gparm;          /* Configuration file parameters */
HINSTANCE         hInstMain;      /* Copy of main program instance (process id) */
MSG_LOGO          hrtlogo;        /* Logo of outgoing heartbeats */
HWND              hwndWndProc;    /* Client window handle */
int               iRead;          /* A retrieval has been performed */
LATENCY         * Latency;        /* Latency data array */
time_t            lStartTime
     ,            lEndTime        /* Station status time period (1/1/70 sec) */
     ;
MSG               msg;            /* Windows control message variable */
pid_t             myPid;          /* Process id of this process */
int               Station_Count;  /* Number of stations to display */
LATENCY_STATION * StaArray;       /* Station data array */
char              szProcessName[] = { "Latency Monitor" };
time_t            then;           /* Previous heartbeat time */
char            * WaveBuf;        /* Pointer to waveform buffer */
TRACE_HEADER    * WaveHead;       /* Pointer to waveform header */


int WritePeriod2File(       FILE           * p_file
                    , const LATENCY_PERIOD * p_period
                    , const char           * p_filename
                    )
{
   LATN_FILE_HEADER   FileHeader;

   /*
   ** Tried to write directly from p_period,
   ** but that didn't work
   */
   LATENCY_PERIOD     period;


   /* beginning of file */
   fseek( p_file , 0 , SEEK_SET );

   /* get the file header */
   fread( &FileHeader, sizeof(LATN_FILE_HEADER), 1, p_file );

   if ( FileHeader.version != CURRENT_LATN_FILE_VERSION )
   {
      /*
      ** This error should have already been dealt with in
      ** the initialiation section of the main function.
      */
      if ( 1 <= Gparm.Debug )
      {
         logit( "t"
              , "Invalid version (%d) found in file %s\n"
              , FileHeader.version
              , p_filename
              );
      }
      return 1;
   }

   period.stt_time = p_period->stt_time;
   period.end_time = p_period->end_time;
   period.latency  = p_period->latency;

   if ( 5 <= Gparm.Debug )
   {
      logit( ""
           , "WritePeriod2File() next row %d [%d]: %d  %.3f - %.3f\n"
           , FileHeader.next_row
           , sizeof(LATN_FILE_HEADER) + sizeof(LATENCY_PERIOD) * FileHeader.next_row
           , period.latency
           , period.stt_time
           , period.end_time
           );
   }

   /* seek to write location for this period's write */
   fseek( p_file
        ,  sizeof(LATN_FILE_HEADER)
         + sizeof(LATENCY_PERIOD) * FileHeader.next_row
        , SEEK_SET
        );

   fwrite( &period, sizeof(LATENCY_PERIOD), 1, p_file );

   fflush( p_file );

   FileHeader.last_time = p_period->end_time;

   if ( (++FileHeader.next_row) == FileHeader.row_count )
   {
      FileHeader.next_row = 0;
   }

   if ( FileHeader.next_row == FileHeader.start_row )
   {
      if ( ++FileHeader.start_row == FileHeader.row_count )
      {
         FileHeader.start_row = 0;
      }
   }

   /* return to beginning of file */
   fseek( p_file , 0 , SEEK_SET );

   /* write the updated header */
   fwrite( &FileHeader, sizeof(LATN_FILE_HEADER), 1, p_file );

   return 0;
}



int InitStoreFile( const char * p_filename )
{
   int                r_status = 0;

   FILE             * hFile = NULL;
   LATN_FILE_HEADER   fileHeader;

   unsigned long _sz;

   if ( (hFile = fopen( p_filename, "r+b" )) == NULL )
   {
      /* File does not yet exist */

      if ( (hFile = fopen( p_filename, "wb" )) == NULL )
      {
         /* Failed to open the file */
         if ( 1 <= Gparm.Debug )
         {
            logit( "", "InitStoreFile(): failed opening new file for initialization\n%s\n", p_filename );
         }
         r_status = -1;
      }
      else
      {
         // write new file

         unsigned int _writeRow = 0;

         LATENCY_PERIOD dummyPeriod;
         dummyPeriod.latency  = 4;
         dummyPeriod.stt_time = MAX_DOUBLE;
         dummyPeriod.end_time = MIN_DOUBLE;

         /*
         ** write file header
         */
         fileHeader.version   = CURRENT_LATN_FILE_VERSION;
         fileHeader.row_count = Gparm.FileRowSize;
         fileHeader.start_row = 0;
         fileHeader.next_row  = 0;
         fileHeader.last_time = 0;

         fwrite( &fileHeader, sizeof(LATN_FILE_HEADER), 1, hFile );

         fflush( hFile );

         while( _writeRow++ < Gparm.FileRowSize )
         {
            if (  fwrite( &dummyPeriod, sizeof(LATENCY_PERIOD), 1, hFile ) < 1 )
            {
               logit( "e", "Failed writing dummy data to new file\n", p_filename );
               r_status = -9;
               break;
            }
         }

      }

   }
   else
   {
      // existing file

      // Check file header

      fseek( hFile , 0 , SEEK_SET );

      if ( fread( &fileHeader, sizeof(LATN_FILE_HEADER), 1, hFile ) != 1 )
      {
         /* failed to read the header */
         logit( "e", "Failed to read header from file %s, try deleting the file before restarting.\n", p_filename );
         r_status = -2;
      }
      else
      {
         if ( fileHeader.version != CURRENT_LATN_FILE_VERSION )
         {
            logit( "e", "Invalid version in file %s, must delete it before restarting.\n", p_filename );
            r_status = -3;
         }
         else
         {
            if ( fileHeader.row_count != Gparm.FileRowSize )
            {
               /*
               ** file is not the same size requested in parameter file
               */
               logit( ""
                    , "Existing files are not the same row size as requested in parameter file,\n%s%d\n"
                    , "Must either delete the existing files or change the <FileRowSize> to"
                    , fileHeader.row_count
                    );
               r_status = -4;
            }
            else
            {
               // check true file size

               fseek( hFile , 0 , SEEK_END );

               _sz =  ftell( hFile );

               if ( _sz != ( sizeof(LATN_FILE_HEADER) + sizeof(LATENCY_PERIOD) * Gparm.FileRowSize )
                  )
               {
                  logit( "", "File length does not match size in header\n" );
                  r_status = -5;
               }
            }
         } /* valid file header */
      } /* read file header */

      fclose( hFile );
   }

   return r_status;
}


int LoadSpanFromFile( HWND p_hwnd  )
{
   FILE             * hFile;        /* File handle */
   LATENCY          * pLatency;     /* Pointer to the latency structure processed */
   LATENCY_STATION  * Sta;          /* Pointer to the station being processed */
   LATN_FILE_HEADER   fileHeader;
   LATENCY_PERIOD     readPeriod;
   unsigned int       current_row;
   int                usedPeriods;
   time_t             lTotalTime;   /* Time (sec) in timespan of interest */
   double             dLastEnd;        /* End time of last latency interval */

   int     stationIdx   /*  i  */
     ,     periodIdx    /*  j  */
     ,     intervalIdx  /*  k  */
     ;

   /*
   ** Read latency files, station-by-station
   */
   for ( stationIdx = 0 ; stationIdx < Station_Count ; stationIdx++ )
   {
      Sta = (LATENCY_STATION *) &StaArray[stationIdx];
      pLatency = (LATENCY *) &Latency[stationIdx];
      InitLatency( pLatency );

      usedPeriods = 0;

      if ( (hFile = fopen( Sta->StoreFileName, "rb" )) != NULL )
      {

         /* read the header */

         if ( fread( &fileHeader, sizeof(LATN_FILE_HEADER), 1, hFile ) != 1 )
         {
            /* failed to read the header */
            logit("", "Failed reading header from %s\n", Sta->StoreFileName );
            fclose( hFile );
            hFile = NULL;
            continue;
         }

         if ( fileHeader.version != CURRENT_LATN_FILE_VERSION )
         {
            logit("", "Invalid header version in %s\n", Sta->StoreFileName );
            fclose( hFile );
            hFile = NULL;
            continue;
         }

         if ( fileHeader.last_time == 0 )
         {
            /*
            ** File has never been written to, nothing to read.
            ** (More importantly, the start and next row counters will
            ** both be zero -- which would cause problems determining
            ** the bounds of the following loop).
            */
            if ( 4 <= Gparm.Debug )
            {
               logit("", "No data in file %s\n", Sta->StoreFileName );
            }
            fclose( hFile );
            hFile = NULL;
            continue;
         }


         /* set the current row to the earliest in the file */
         current_row = fileHeader.start_row;

         for ( ; ; )
         {
            /* seek to the current row */
            if ( fseek( hFile
                      ,  sizeof(LATN_FILE_HEADER)
                       + sizeof(LATENCY_PERIOD) * current_row
                      , SEEK_SET
                      ) != 0 )
            {
               logit( "", "LoadSpanFromFile() error: Failed fseek()\n" );
               break;
            }

            if ( fread( &readPeriod, sizeof(LATENCY_PERIOD), 1, hFile ) != 1 )
            {
               logit( "", "LoadSpanFromFile() error: failed reading data from %s\n", Sta->StoreFileName );
               break;
            }
/*
logit( ""
     , "DEBUG  LoadSpanFromFile() C %ld - %ld ;  row %d : %d  %.3f - %.3f\n"
     , lStartTime
     , lEndTime
     , current_row
     , readPeriod.latency
     , readPeriod.stt_time
     , readPeriod.end_time
     );
*/

            /*
            ** Compare period times to span of interest
            */
            if (   (double)lStartTime   < readPeriod.end_time
                &&  readPeriod.stt_time < (double)lEndTime
               )
            {
               pLatency->Periods[usedPeriods].latency  = readPeriod.latency;
               pLatency->Periods[usedPeriods].stt_time = readPeriod.stt_time;
               pLatency->Periods[usedPeriods].end_time = readPeriod.end_time;

               usedPeriods++;

               if ( usedPeriods == MAX_ONOFF )
               {
                  logit( ""
                       , "%s %s Max latency periods reached\n"
                       , Sta->szStation
                       , Sta->szChannel
                       );
                  break;
               }
            }

            if ( (double)lEndTime < readPeriod.stt_time )
            {
               /*
               ** Passed timespan of interest, stop reading this station
               */
               break;
            }


            if ( ++current_row == fileHeader.row_count )
            {
               current_row = 0;
            }

            if ( current_row == fileHeader.next_row )
            {
               /* no more values in the file */
               break;
            }

         } /* looping through file */

         fclose( hFile );

         pLatency->UsedPeriods = (unsigned short)usedPeriods;

         /*
         ** Compute percentages of times in each latency interval
         */
         if ( usedPeriods == 0 )
         {
            /*
            ** No used periods means: completely out
            */
            for ( intervalIdx = 0 ; intervalIdx < NUM_INTERVALS ; intervalIdx++ )
            {
               pLatency->Percent[intervalIdx] = 0.0;
            }
            pLatency->Percent[NUM_INTERVALS-1] = 100.0;
         }
         else
         {
            lTotalTime = lEndTime - lStartTime;

            for ( periodIdx = 0 ; periodIdx < usedPeriods ; periodIdx++ )
            {
               if (   periodIdx == 0
                   && pLatency->Periods[periodIdx].stt_time > (double)lStartTime
                  )
               {
                  /*
                  ** If startup time was after than start of span interest
                  ** remove the outage counts for that uncounted time
                  */
                  pLatency->Percent[NUM_INTERVALS-1] += pLatency->Periods[periodIdx].stt_time - (double)lStartTime;
               }

               if (   periodIdx == usedPeriods - 1
                   && pLatency->Periods[periodIdx].end_time < (double) lEndTime
                  )
               {
                  /*
                  ** If last packet end time was less than end time of interest
                  ** remove outage counts for that uncounted time
                  */
                  pLatency->Percent[NUM_INTERVALS-1] += (double)lEndTime - pLatency->Periods[periodIdx].end_time;
               }

               /*
               ** Add the time of this latency to proper index
               */
               for ( intervalIdx = 0 ; intervalIdx < NUM_INTERVALS ; intervalIdx++ )
               {
                  if ( pLatency->Periods[periodIdx].latency == intervalIdx )
                  {
                     if    ( pLatency->Periods[periodIdx].stt_time > (double)lStartTime )
                     {
                        if ( pLatency->Periods[periodIdx].end_time < (double)lEndTime )
                        {
                           pLatency->Percent[intervalIdx] +=  pLatency->Periods[periodIdx].end_time
                                                            - pLatency->Periods[periodIdx].stt_time
                                                            ;
                        }
                        else
                        {
                           pLatency->Percent[intervalIdx] += (double)lEndTime - pLatency->Periods[periodIdx].stt_time;
                        }
                     }
                     else /* pLatency->Periods[j].stt_time < (double)lStartTime  */
                     {
                        if ( pLatency->Periods[periodIdx].end_time < (double)lEndTime )
                        {
                           pLatency->Percent[intervalIdx] += pLatency->Periods[periodIdx].end_time - (double)lStartTime;
                        }
                        else
                        {
                           pLatency->Percent[intervalIdx] += (double)lEndTime - (double)lStartTime;
                        }
                     }
                  }
               }

               if ( 0 < periodIdx )
               {
                  if ( pLatency->Periods[periodIdx].stt_time - dLastEnd > 2.0 / Sta->dSampRate )
                  {
                     pLatency->Percent[NUM_INTERVALS-1] += pLatency->Periods[periodIdx].stt_time - dLastEnd;
                  }
               }
               dLastEnd = pLatency->Periods[periodIdx].end_time;
            }

            /*
            ** Calculate percentages
            */
            for ( intervalIdx = 0 ; intervalIdx < NUM_INTERVALS ; intervalIdx++ )
            {
               pLatency->Percent[intervalIdx] = 100.0 * pLatency->Percent[intervalIdx]
                                              / (double) lTotalTime
                                              ;
            }
         }
      }
      else
      {
         logit( "", "Failed opening file %s\n", Sta->StoreFileName );
      }

      iRead = 1;

      if ( p_hwnd != NULL )
      {
         InvalidateRect( p_hwnd, NULL, TRUE );
      }

   } /* each station */

   return 0;
}

      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Argument:                                              *
       *     argv[1] = Name of configuration file                *
       ***********************************************************/

int WINAPI WinMain( HINSTANCE hInst
                  , HINSTANCE hPreInst
                  , LPSTR lpszCmdLine
                  , int iCmdShow
                  )
{
   char          configfile[64];  /* Name of config file */
   int           i;
   long          InBufl;          /* Maximum message size in bytes */
   char          line[40];        /* Heartbeat message */
   int           lineLen;         /* Length of heartbeat message */
   MSG_LOGO      logo;            /* Logo of retrieved msg */
   long          MsgLen;          /* Size of retrieved message */
   static unsigned tidW;          /* Waveform getter Thread */
   static WNDCLASS wc;
   time_t  now;              /* Current time */

   hInstMain = hInst;
   iRead = 0;

   /* Get config file name (format "latency_mon latency_mon.d")
   ***********************************************************/
   if ( strlen( lpszCmdLine ) <= 0 )
   {
      fprintf( stderr, "Need configfile in start line....exiting\n" );
      _sleep(20);
      return -1;
   }


   strcpy( configfile, lpszCmdLine );

/* Get parameters from the configuration files
   *******************************************/
   if ( GetConfig( configfile, &Gparm ) == -1 )
   {
      fprintf( stderr, "GetConfig() failed. file %s.\n", configfile );
      _sleep(20);
      return -1;
   }


/* Look up info in the earthworm.h tables
   **************************************/
   if ( GetEwh( &Ewh ) < 0 )
   {
      fprintf( stderr, "latency_mon: GetEwh() failed. Exiting.\n" );
      _sleep(20);
      return -1;
   }


/* Initialize name of log-file & open it
   *************************************/
   logit_init( configfile, Gparm.MyModId, 1024, 1 );

/* Specify logos of incoming waveforms and outgoing heartbeats
   ***********************************************************/
   getlogoW.instid = Ewh.GetThisInstId;
   getlogoW.mod    = Ewh.GetThisModId;
   getlogoW.type   = Ewh.TypeWaveform;

   hrtlogo.instid = Ewh.MyInstId;
   hrtlogo.mod    = Gparm.MyModId;
   hrtlogo.type   = Ewh.TypeHeartBeat;

/* Get our own pid for restart purposes
   ************************************/
   myPid = getpid();
   if ( myPid == -1 )
   {
      logit( "e", "latency_mon: Can't get my pid. Exiting.\n" );
      _sleep(5);
      return -1;
   }


/* Log the configuration parameters
   ********************************/
   LogConfig( &Gparm );


/* Allocate the waveform buffer
   ****************************/
   InBufl = MAX_TRACEBUF_SIZ*2;
   WaveBuf = (char *) malloc( (size_t) InBufl );
   if ( WaveBuf == NULL )
   {
      logit( "et", "latency_mon: Cannot allocate waveform buffer\n" );
      return -1;
   }


/* Point to header and data portions of waveform message
   *****************************************************/
   WaveHead  = (TRACE_HEADER *) WaveBuf;

/* Read the station list and return the number of stations found.
   Allocate the station list array.
   *************************************************************/
   /* We can tolerate two schemes: The original scheme, which reads
   a station list and a station data file. Or, we can read a FindWave
   debug file. */

   /* Read original station files */
   if(Gparm.FindWaveFile[0] == '0' && Gparm.StaFile[0] != '0')
   {
      if ( GetStaList( &StaArray, &Station_Count, &Gparm ) == -1 )
      {
         logit( "e", "latency_mon: GetStaList() failed. Exiting.\n" );
         free( WaveBuf );
         return -1;
      }
   }
   else if ( Gparm.FindWaveFile[0] != '0' && Gparm.StaFile[0] == '0')
   {
      /* Read FindWave station list */
      if ( GetFindWaveStaList( &StaArray, &Station_Count, &Gparm ) == -1 )
      {
         logit( "e", "latency_mon: GetStaList() failed. Exiting.\n" );
         free( WaveBuf );
         return -1;
      }
   }
   else  /* either both or neither were specified */
   {
      logit( "e", "Bad station list specification: .%s. .%s.\n",
             Gparm.FindWaveFile, Gparm.StaFile );
      free( WaveBuf );
      return -1;
   }

   if ( Station_Count == 0 )
   {
      logit( "e", "latency_mon: Empty station list. Exiting.\n" );
      free( WaveBuf );
      free( StaArray );
      return -1;
   }

   logit( "t", "latency_mon: Displaying %d stations.\n", Station_Count );

/* Log the station list
   ********************/
   LogStaList( StaArray, Station_Count );


/* Allocate and init the Latency buffer
   ***********************************/

   InBufl = sizeof( LATENCY ) * Station_Count;
   Latency = (LATENCY *) malloc( (size_t) InBufl );
   if ( Latency == NULL )
   {
      free( WaveBuf );
      free( StaArray );
      logit( "et", "latency_mon: Cannot allocate latency buffer\n");
      return -1;
   }

   /* Attach to existing transport rings
   ************************************/
   tport_attach( &Gparm.InRegion,  Gparm.InKey );

   /*
   ** Send first heartbeat to ring
   */
   time( &then );
   sprintf( line, "%ld %d\n", (long) then, (int) myPid );
   lineLen = strlen( line );
   if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK )
   {
      logit( "et", "latency_mon: Error sending 1st heartbeat. Exiting.\n" );
      tport_detach( &Gparm.InRegion );
      free( WaveBuf );
      free( StaArray );
      return -1;
   }

   for ( i = 0 ; i < Station_Count ; i++ )
   {
      /* Initialize latency structure */
      InitLatency( &Latency[i] );

      if ( InitStoreFile( StaArray[i].StoreFileName )!= 0 )
      {
         logit( "e", "Failed initializing latency file %s\n", StaArray[i].StoreFileName );
         return -1;
      }

      /*
      ** Large files can take a long time to create, so
      ** send heartbeat between files.
      */

      time( &now );

      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         /*
         ** Send heartbeat to the transport ring
         */
         then = now;
         sprintf( line, "%ld %d\n", (long) now, (int) myPid );
         lineLen = strlen( line );
         if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK )
         {
            logit( "et", "latency_mon: Error sending heartbeat.\n" );
            return -1;
         }
      }
   }


/* If this is the first instance of this program, init window stuff and
   register window (it always is)
   ********************************************************************/
   if ( !hPreInst )
   {  /* Force PAINT when sized and give double click notification */
      wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
      wc.lpfnWndProc   = WndProc;         /* Control Window process */
      wc.cbClsExtra    = 0;
      wc.cbWndExtra    = 0;
      wc.hInstance     = hInst;           /* Process id */
      wc.hIcon         = LoadIcon( hInst, "latency_mon" );/* System app icon */
      wc.hCursor       = LoadCursor( NULL, IDC_ARROW );    /* Pointer */
      wc.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH ); /* White */
      wc.lpszMenuName  = "latency_mon_menu";   /* Relates to .RC file */
      wc.lpszClassName = szProcessName;
      if ( !RegisterClass( &wc ) )        /* Window not registered */
      {
         logit( "t", "RegisterClass failed\n" );
         free( WaveBuf );
         free( StaArray );
         return -1;
      }
   }

/* Create the window
   *****************/
   hwndWndProc = CreateWindow(
                 szProcessName,          /* Process name */
                 szProcessName,          /* Initial title bar caption */
                 WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                 CW_USEDEFAULT,          /* top left x starting location */
                 CW_USEDEFAULT,          /* top left y starting location */
                 CW_USEDEFAULT,          /* Initial screen width in pixels */
                 CW_USEDEFAULT,          /* Initial screen height in pixels */
                 NULL,                   /* No parent window */
                 NULL,                   /* Use standard system menu */
                 hInst,                  /* Process id */
                 NULL );                 /* No extra data to pass in */
   if ( hwndWndProc == NULL )            /* Window not created */
   {
      logit( "t", "CreateWindow failed\n" );
      free( WaveBuf );
      free( StaArray );
      return 0;
   }

   ShowWindow( hwndWndProc, iCmdShow );  /* Show the Window */
   UpdateWindow( hwndWndProc );          /* Force an initial PAINT call */

/* Flush the input waveform ring
   *****************************/
   while ( tport_getmsg( &Gparm.InRegion, &getlogoW, 1, &logo, &MsgLen,
                         WaveBuf, MAX_TRACEBUF_SIZ ) != GET_NONE );

/* Start the waveform getter tread
   *******************************/
   if ( StartThread( WThread, 8192, &tidW ) == -1 )
   {
      tport_detach( &Gparm.InRegion );
      free( WaveBuf );
      free( StaArray );
      logit( "et", "Error starting W thread; exiting!\n" );
      return -1;
   }

/* Main windows thread
   *******************/
   while ( GetMessage( &msg, NULL, 0, 0 ) != 0 )
   {
      TranslateMessage( &msg );
      DispatchMessage( &msg );
   }

/* Detach from the ring buffer
   ***************************/
   tport_detach( &Gparm.InRegion );
   PostMessage( hwndWndProc, WM_DESTROY, 0, 0 );
   free( WaveBuf );
   free( StaArray );
   logit( "t", "Termination requested. Exiting.\n" );
   return (msg.wParam);
}

 /********************************************************************
  *                 DisplayChannelID()                               *
  *                                                                  *
  * This function displays the station and channel name in the       *
  * display window.                                                  *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumStas          Number of stations in Sta and Trace arrays    *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *                                                                  *
  ********************************************************************/

void DisplayChannelID( HDC hdc, LATENCY_STATION Sta[],
      int iNumStas, long lTitleFHt, long lTitleFWd,
      int cxScreen, int cyScreen, int iNumTracePerScreen, int iVScrollOffset,
      int iTitleOffset )
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i;
   long    lOffset;              /* Offset to center of trace from top */
   POINT   pt;                   /* Screen location for trace name */

/* Create font */
   hNFont = CreateFont( lTitleFHt, lTitleFWd,
             0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
             DEFAULT_PITCH | FF_MODERN, "Elite" );
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 0, 0, 0 ) );   /* Color trace names red */

/* Display all station/channel names */
   for ( i = 0 ; i < iNumStas ; i++ )
   {
      pt.x = cxScreen / 500;
      lOffset = i*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                iVScrollOffset + iTitleOffset;
      pt.y = lOffset - 3*lTitleFHt/8;
      TextOut( hdc, pt.x, pt.y, Sta[i].szStation, strlen( Sta[i].szStation ) );
      pt.x = 5*cxScreen / 100;
      TextOut( hdc, pt.x, pt.y, Sta[i].szChannel, strlen( Sta[i].szChannel ) );
      pt.x = 9*cxScreen / 100;
      TextOut( hdc, pt.x, pt.y, Sta[i].szNetID, strlen( StaArray[i].szNetID ) );
   }
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                 DisplayLatencyGraph()                            *
  *                                                                  *
  * This function displays colored lines which indicate latencies for*
  * stations over the time period entered.                           *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  Sta               Array of all station data structures          *
  *  LatencyBuf        Latency buffer                                *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iChanIDOffset     Open space at left of screen for channel names*
  *  iSummaryOffset    Open space at rt. of screen for latencies     *
  *  iNumStas          Number of stations in Sta and Trace arrays    *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *                                                                  *
  ********************************************************************/

void DisplayLatencyGraph( HDC hdc, int cxScreen, int cyScreen, LATENCY_STATION Sta[],
      LATENCY LatencyBuf[], int iVScrollOffset, int iChanIDOffset,
      int iSummaryOffset, int iNumStas, int iTitleOffset,
      int iNumTracePerScreen )
{
   double  dTemp;
   HPEN    hRPen, hYPen, hGPen, hWPen, hOPen, hBBPen, hlRPen, hlYPen, hlGPen, hlWPen, hlOPen; /* Pen handles */
   int     i, j;
   int last_status;
   POINT   pt;                   /* Screen location for trace name */

/* Create pens */
   /*hGPen = CreatePen( PS_SOLID, 8, RGB( 0,222,0 ) );    */  /* green  */
   /*hOPen = CreatePen( PS_SOLID, 8, RGB( 255,128,0 ) );  */  /* Orange */
   /*hYPen = CreatePen( PS_SOLID, 8, RGB( 255,255,0 ) );  */  /* yellow */
   /*hRPen = CreatePen( PS_SOLID, 8, RGB( 222,0,0 ) );    */  /* red    */
   /*hWPen = CreatePen( PS_SOLID, 8, RGB( 255,255,255 ) );*/  /* white  */

   LOGBRUSH lb;

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 0,222,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hGPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT |
            PS_JOIN_MITER, 8, &lb, 0, NULL );      /* green  */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 255,128,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hOPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT |
            PS_JOIN_MITER, 8, &lb, 0, NULL );      /* Orange */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 255,255,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hYPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT |
            PS_JOIN_MITER, 8, &lb, 0, NULL );     /* yellow */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 222,0,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hRPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT |
            PS_JOIN_MITER, 8, &lb, 0, NULL );     /* red    */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 255,255,255 );
   lb.lbHatch = HS_BDIAGONAL;
   hWPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT |
            PS_JOIN_MITER, 8, &lb, 0, NULL );     /* white  */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 0,0,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hBBPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND |
             PS_JOIN_ROUND, 10, &lb, 0, NULL );   /* black box  */

/*   hBBPen = CreatePen( PS_SOLID, 10, RGB( 0,0,0 ) );       black line  */

   hlGPen = CreatePen( PS_SOLID, 6, RGB( 0,222,0 ) );      /* green dot  */
   hlOPen = CreatePen( PS_SOLID, 6, RGB( 255,128,0 ) );    /* Orange dot */
   hlYPen = CreatePen( PS_SOLID, 6, RGB( 255,255,0 ) );    /* yellow dot */
   hlRPen = CreatePen( PS_SOLID, 6, RGB( 222,0,0 ) );      /* red dot    */
   hlWPen = CreatePen( PS_SOLID, 6, RGB( 255,255,255 ) );  /* white dot  */


/* Draw lines for each channel */
   if ( iRead == 1 )
   {
      for ( i = 0 ; i < iNumStas ; i++ )
      {
         last_status = 4;
         pt.y = i*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                iVScrollOffset + iTitleOffset+3;

         for ( j = 0 ; j < LatencyBuf[i].UsedPeriods ; j++ )
         {
            if ( LatencyBuf[i].Periods[j].latency == 0 )
            {
               SelectObject( hdc, hGPen );          /* Green pen = <1 minute */
               last_status = 0;
            }
            else if ( LatencyBuf[i].Periods[j].latency == 1 )
            {
               SelectObject( hdc, hYPen );          /* Yellow pen = 1-2 min */
               last_status = 1;
            }
            else if ( LatencyBuf[i].Periods[j].latency == 2 )
            {
               SelectObject( hdc, hOPen );          /* Orange pen = 2-3 minutes*/
               last_status = 2;
            }
            else if ( LatencyBuf[i].Periods[j].latency == 3 )
            {
               SelectObject( hdc, hRPen );          /* Red pen = 3-5 minutes */
               last_status = 3;
            }
            else if ( LatencyBuf[i].Periods[j].latency == 4 )
            {
               SelectObject( hdc, hWPen );          /* White pen = >5 minutes */
               last_status = 4;
            }
            if ( j == 0 )
            {
               dTemp = LatencyBuf[i].Periods[j].stt_time - (double) lStartTime;
               if ( dTemp < 0. ) dTemp = 0.;
               pt.x = (long) ((double) (cxScreen-iChanIDOffset-iSummaryOffset) *
                       dTemp / (double) (lEndTime-lStartTime)) + iChanIDOffset;
            }
            else
               pt.x = (long) ((double) (cxScreen-iChanIDOffset-iSummaryOffset) *
                      (LatencyBuf[i].Periods[j].stt_time - (double) lStartTime) /
                      (double) (lEndTime-lStartTime)) + iChanIDOffset;
            if ( pt.x < iChanIDOffset ) pt.x = iChanIDOffset;
            MoveToEx( hdc, pt.x, pt.y, NULL );

            if ( j == LatencyBuf[i].UsedPeriods-1 )
            {
               dTemp = LatencyBuf[i].Periods[j].end_time - (double) lStartTime;
               if ( dTemp > (double) (lEndTime-lStartTime) )
                  dTemp = (double) (lEndTime-lStartTime);
               pt.x = (long) ((double) (cxScreen-iChanIDOffset-iSummaryOffset) *
                      dTemp / (double) (lEndTime-lStartTime)) + iChanIDOffset;
            }
            else
               pt.x = (long) ((double) (cxScreen-iChanIDOffset-iSummaryOffset) *
                      (LatencyBuf[i].Periods[j].end_time - (double) lStartTime) /
                      (double) (lEndTime-lStartTime)) + iChanIDOffset;
            if ( pt.x > cxScreen-iSummaryOffset )
               pt.x = cxScreen-iSummaryOffset;
            LineTo( hdc, pt.x, pt.y );
         } /* used periods */

         pt.x = 68*cxScreen/100;
         SelectObject( hdc, hBBPen );
         MoveToEx( hdc, pt.x, pt.y, NULL );
         pt.x = 69*cxScreen/100;
         LineTo( hdc, pt.x, pt.y );
         if ( last_status == 0 )
            SelectObject( hdc, hlGPen );          /* Green pen = <1 minute */
         else if ( last_status == 1 )
            SelectObject( hdc, hlYPen );          /* Yellow pen = 1-2 min */
         else if ( last_status == 2 )
            SelectObject( hdc, hlOPen );          /* Orange pen = 2-3 minutes*/
         else if ( last_status == 3 )
            SelectObject( hdc, hlRPen );          /* Red pen = 3-5 minutes */
         else if ( last_status == 4 )
            SelectObject( hdc, hlWPen );          /* White pen = >5 minutes */
         pt.x = 68*cxScreen/100;
         MoveToEx( hdc, pt.x, pt.y, NULL );
         pt.x = 69*cxScreen/100;
         LineTo( hdc, pt.x, pt.y );
      }
   }
   DeleteObject( hOPen );                           /* Delete Pens */
   DeleteObject( hRPen );
   DeleteObject( hGPen );
   DeleteObject( hWPen );
   DeleteObject( hYPen );
   DeleteObject( hBBPen );
   DeleteObject( hlOPen );
   DeleteObject( hlRPen );
   DeleteObject( hlGPen );
   DeleteObject( hlWPen );
   DeleteObject( hlYPen );
}

     /**************************************************************
      *                 DisplayTitles()                            *
      *                                                            *
      * This function outputs the title and latencies to the       *
      * display window along with the color legend and start/end   *
      * times.                                                     *
      *                                                            *
      * Arguments:                                                 *
      *  hdc               Device context to use                   *
      *  lTitleFHt         Title font height                       *
      *  lTitleFWd         Title font width                        *
      *  cxScreen          Screen width in pixels                  *
      *  cyScreen          Screen height in pixels                 *
      *  LatencyBuf        Latency buffer                          *
      *  iVScrollOffset    Vertical scroll setting                 *
      *  iNumStas          Number of stations in Sta array         *
      *  iTitleOffset      Title offset to give room at top        *
      *  iNumTracePerScreen Number of traces to put on screen      *
      *                                                            *
      **************************************************************/

void DisplayTitles( HDC hdc, long lTitleFHt, long lTitleFWd, int cxScreen,
                    int cyScreen, LATENCY LatencyBuf[], int iVScrollOffset,
                    int iNumStas, int iTitleOffset, int iNumTracePerScreen )
{
   HFONT   hOFont, hNFont;      /* Font handles */
   HPEN    hRPen, hYPen, hGPen, hWPen, hOPen; /* Pen handles */
   int     i, j;
   long    lOffset;             /* Offset to center of trace from top */
   char   *pszTime;             /* ASCII start and end times */
   POINT   pt;
   char    szBuf[64];           /* Latency summary for each station */
   char    szTemp[8];


   LOGBRUSH lb;

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 0,222,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hGPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 8, &lb, 0, NULL );      /* green  */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 255,128,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hOPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 8, &lb, 0, NULL );    /* Orange */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 255,255,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hYPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 8, &lb, 0, NULL );    /* yellow */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 222,0,0 );
   lb.lbHatch = HS_BDIAGONAL;
   hRPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 8, &lb, 0, NULL );      /* red    */

   lb.lbStyle = BS_SOLID;
   lb.lbColor = RGB( 255,255,255 );
   lb.lbHatch = HS_BDIAGONAL;
   hWPen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 8, &lb, 0, NULL );  /* white  */

/* Create font */
   hNFont = CreateFont( lTitleFHt, lTitleFWd,
             0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
             FIXED_PITCH | FF_MODERN, "Elite" );

/* Select font and color */
   hOFont = (HFONT) SelectObject( hdc, hNFont );    /* Select the font */
   SetTextColor( hdc, RGB( 0, 0, 0 ) );             /* Black text */

/* Write latency summary title */
   pt.x = 73*cxScreen/100;
   pt.y = iVScrollOffset;
   TextOut( hdc, pt.x, pt.y, "Latency Summary (Minutes)", 25 );
   pt.x = 67*cxScreen/100;
   pt.y = 3*cyScreen/100 + iVScrollOffset;
   TextOut( hdc, pt.x, pt.y, "Now  <1    1-2   2-3   3-5   Off", 32 );

/* Show line colors under related latency times */
   pt.y = 6*cyScreen/100 + iVScrollOffset;
   pt.x = 143*cxScreen/200;               /* Green pen = <1 minute */
   SelectObject( hdc, hGPen );
   MoveToEx( hdc, pt.x, pt.y, NULL );
   pt.x = 149*cxScreen/200;
   LineTo( hdc, pt.x, pt.y );
   pt.x = 155*cxScreen/200;               /* Yellow pen = 1-2 minutes */
   SelectObject( hdc, hYPen );
   MoveToEx( hdc, pt.x, pt.y, NULL );
   pt.x = 161*cxScreen/200;
   LineTo( hdc, pt.x, pt.y );
   pt.x = 167*cxScreen/200;               /* Orange pen = 2-3 minutes */
   SelectObject( hdc, hOPen );
   MoveToEx( hdc, pt.x, pt.y, NULL );
   pt.x = 173*cxScreen/200;
   LineTo( hdc, pt.x, pt.y );
   pt.x = 179*cxScreen/200;               /* Red pen = 3-5 minutes */
   SelectObject( hdc, hRPen );
   MoveToEx( hdc, pt.x, pt.y, NULL );
   pt.x = 185*cxScreen/200;
   LineTo( hdc, pt.x, pt.y );
   pt.x = 191*cxScreen/200;               /* White pen = >5 minutes (or out) */
   SelectObject( hdc, hWPen );
   MoveToEx( hdc, pt.x, pt.y, NULL );
   pt.x = 197*cxScreen/200;
   LineTo( hdc, pt.x, pt.y );

/* Write start and end times */
   if ( iRead == 1 )
   {
      pt.y = iVScrollOffset;
      pt.x = 14*cxScreen/100;
      pszTime = asctime( gmtime( &lStartTime ) );
      strcpy( szBuf, "\0" );
      strncpy( szBuf, pszTime, 24 );
      szBuf[24] = '\0';
      strcat( szBuf, " TO " );
      pszTime = asctime( gmtime( &lEndTime ) );
      strncat( szBuf, pszTime, 24 );
      TextOut( hdc, pt.x, pt.y, szBuf, strlen( szBuf ) );
   }

/* Write latency summary */
   if ( iRead == 1 )
      for ( i=0; i<iNumStas; i++ )
      {
         pt.x = 70*cxScreen/100;
         strcpy( szBuf, "" );
         for ( j=0; j<NUM_INTERVALS; j++ )
         {
            sprintf( szTemp, "%5.1lf ", LatencyBuf[i].Percent[j] );
            strcat( szBuf, szTemp );
         }
         lOffset = i*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset;
         pt.y = lOffset - 3*lTitleFHt/8;
         TextOut( hdc, pt.x, pt.y, szBuf, strlen( szBuf ) );
      }

   DeleteObject( hOPen );                           /* Delete Pens */
   DeleteObject( hRPen );
   DeleteObject( hGPen );
   DeleteObject( hWPen );
   DeleteObject( hYPen );
   DeleteObject( SelectObject( hdc, hOFont ) );     /* Reset font */
}

      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.d file.      *
       *******************************************************/

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 )
   {
      fprintf( stderr, "latency_mon: Error getting MyInstId.\n" );
      return -1;
   }
   if ( GetInst( "INST_WILDCARD", &Ewh->GetThisInstId ) != 0 )
   {
      fprintf( stderr, "latency_mon: Error getting GetThisInstId.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->GetThisModId ) != 0 )
   {
      fprintf( stderr, "latency_mon: Error getting GetThisModId.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      fprintf( stderr, "latency_mon: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      fprintf( stderr, "latency_mon: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_TRACEBUF", &Ewh->TypeWaveform ) != 0 )
   {
      fprintf( stderr, "latency_mon: Error getting TYPE_TRACEBUF.\n" );
      return -6;
   }
   return 0;
}

   /*******************************************************************
    *                         InitLatency()                           *
    *                                                                 *
    * This function initializes the latency structure for one station.*
    *                                                                 *
    *  Arguments:                                                     *
    *    pLatency      Pointer to latency structure                   *
    *                                                                 *
    *******************************************************************/

void InitLatency( LATENCY *pLatency )
{
   int i;

   for ( i = 0 ; i < MAX_ONOFF ; i++ )
   {
      pLatency->Periods[i].latency  = 0;
      pLatency->Periods[i].stt_time = 0.0;
      pLatency->Periods[i].end_time = 0.0;
   }
   for ( i = 0 ; i < NUM_INTERVALS ; i++ )
   {
      pLatency->Percent[i] = 0.;
   }
   pLatency->UsedPeriods = 0;
}

      /***********************************************************
       *                   StationStatusDlgProc()                *
       *                                                         *
       * This dialog procedure lets the user specify the time    *
       * interval over which to display the latencies and summary*
       *                                                         *
       ***********************************************************/

long WINAPI StationStatusDlgProc( HWND hwnd, UINT msg, UINT wParam, long lParam)
{
   int        iTemp;       /* Dummy variable for GetDlg... */
   int        iTT;         /* Time in hours to retrieve from data files */
   static time_t lTime;    /* Present time (1/1/70 seconds */
   static struct tm *tm;   /* C time structure */

   switch ( msg )
   {
      case WM_INITDIALOG:  /* Pre-set entry field to present number */
         time( &lTime );
         tm = gmtime( &lTime );
         SetDlgItemInt( hwnd, EF_DISPLAYYEAR, (int) tm->tm_year+1900, TRUE );
         SetDlgItemInt( hwnd, EF_DISPLAYMONTH, (int) tm->tm_mon+1, TRUE );
         SetDlgItemInt( hwnd, EF_DISPLAYDAY, (int) tm->tm_mday, TRUE );
         SetDlgItemInt( hwnd, EF_DISPLAYHOUR, (int) tm->tm_hour, TRUE );
         SetDlgItemInt( hwnd, EF_DISPLAYTOTALTIME, 24, TRUE );
         SetFocus( hwnd );
         break;

      case WM_COMMAND:
         switch ( LOWORD (wParam) )
         {
            case IDOK:     /* Accept user input */
               tm->tm_year = GetDlgItemInt( hwnd, EF_DISPLAYYEAR, &iTemp, TRUE)
                             - 1900;
               if ( tm->tm_year < 100 || tm->tm_year > 200  ) /* Check year */
               {  MessageBox( hwnd, "Invalid Year",
                              NULL, MB_OK | MB_ICONEXCLAMATION ); break; }
               tm->tm_mon =
                GetDlgItemInt( hwnd, EF_DISPLAYMONTH, &iTemp, TRUE)-1;
               if ( tm->tm_mon < 0 || tm->tm_mon > 11  )   /* Check month */
               {  MessageBox( hwnd, "Invalid Month",
                              NULL, MB_OK | MB_ICONEXCLAMATION ); break; }
               tm->tm_mday = GetDlgItemInt( hwnd, EF_DISPLAYDAY, &iTemp, TRUE);
               if ( tm->tm_mday < 1 || tm->tm_mday > 31  ) /* Check day */
               {  MessageBox( hwnd, "Invalid Day",
                              NULL, MB_OK | MB_ICONEXCLAMATION ); break; }
               tm->tm_hour = GetDlgItemInt( hwnd, EF_DISPLAYHOUR, &iTemp, TRUE);
               if ( tm->tm_hour < 0 || tm->tm_hour > 23  ) /* Check hour */
               {  MessageBox( hwnd, "Invalid Hour",
                              NULL, MB_OK | MB_ICONEXCLAMATION ); break; }
               tm->tm_min = 0;
               tm->tm_sec = 0;
               tm->tm_isdst = 0;
               iTT = GetDlgItemInt( hwnd, EF_DISPLAYTOTALTIME, &iTemp, TRUE);
               if ( iTT <= 0 )                         /* Check total time */
               {  MessageBox( hwnd, "Invalid Total Time",
                              NULL, MB_OK | MB_ICONEXCLAMATION ); break; }
               lStartTime = mktime( tm );
               if ( lStartTime < 0 )                   /* Check mktime return */
               {  MessageBox( hwnd, "Invalid Time",
                              NULL, MB_OK | MB_ICONEXCLAMATION ); break; }
               lEndTime = lStartTime + iTT*3600;
               EndDialog (hwnd, IDOK);
               break;

            case IDCANCEL: /* Escape - don't accept input */
               EndDialog (hwnd, IDCANCEL);
               break;
         }
         break;
   }
   return 0;
}

      /***********************************************************
       *                 TracePerScreenDlgProc()                 *
       *                                                         *
       * This dialog procedure lets the user set the number of   *
       * traces to be shown on the visible part of the screen    *
       * display.                                                *
       *                                                         *
       ***********************************************************/

long WINAPI TracePerScreenDlgProc( HWND hwnd, UINT msg, UINT wParam,
                                   long lParam )
{
   int     iTemp;

   switch ( msg )
   {
      case WM_INITDIALOG:  /* Pre-set entry field to present number */
         SetDlgItemInt( hwnd, EF_NUMSTATODISP, Gparm.NumTracePerScreen, TRUE );
         SetFocus( hwnd );
         break;

      case WM_COMMAND:
         switch ( LOWORD (wParam) )
         {
            case IDOK:     /* Accept user input */
               Gparm.NumTracePerScreen =
                GetDlgItemInt( hwnd, EF_NUMSTATODISP, &iTemp, TRUE );
               if ( Gparm.NumTracePerScreen > Station_Count )
                  Gparm.NumTracePerScreen = Station_Count;
               if ( Gparm.NumTracePerScreen < 2 )
                  Gparm.NumTracePerScreen = 2;
               EndDialog (hwnd, IDOK);
               break;

            case IDCANCEL: /* Escape - don't accept input */
               EndDialog (hwnd, IDCANCEL);
               break;

            default:
               break;
         }
   }
   return 0;
}

      /***********************************************************
       *                      WndProc()                          *
       *                                                         *
       *  This dialog procedure processes messages from the      *
       *  windows screen.  All                                   *
       *  display is performed in PAINT.  Menu options (on main  *
       *  menu and through right button clicks) control the      *
       *  display.                                               *
       *                                                         *
       ***********************************************************/

long WINAPI WndProc( HWND hwnd, UINT msg, UINT wParam, long lParam )
{
   static  int     cxScreen
             ,     cyScreen        /* Window size in pixels */
             ;
   HCURSOR         hCursor;         /* Present cursor handle (hourglass or arrow) */
   static HDC      hdc;         /* Device context of screen */
   FILE          * hFile;          /* File handle */
   static HANDLE   hMenu;   /* Handle to the menu */
   FILE          * hPrinter;       /* Printer handle */
   FILE          * hSaveFile;       /* Printer handle */
   int             i;
   static  int     iChanIDOffset;    /* Trace offset at left of screen */
   static  int     iScrollVertBuff;  /* Small vertical scroll amount (pxl) */
   static  int     iScrollVertPage;  /* Large vertical scroll amount (pxl) */
   static  int     iSummaryOffset;   /* Summary offset at right of screen */
   static  int     iVScrollOffset;   /* Vertical scoll bar setting */
   static  long    lTitleFHt
              ,    lTitleFWd /* Font height and width */
              ;
   static int       iTitleOffset;         /* Trace offset at top of screen */
   static LATENCY         * pLatency; /* Pointer to the latency structure processed */
   static LATENCY_STATION * Sta;    /* Pointer to the station being processed */
   PAINTSTRUCT ps;          /* Paint structure used in WM_PAINT command */
   RECT    rct;             /* RECT (rectangle structure) */

//   char    szFileName[64];  /* Latency log file name */
/*   int     iLateT;     */     /* Latency value read from file */
//   char    cLateT;          /* Latency value read from file */
//   double  dStart, dEnd;    /* Latency time period read from file */
//   double  dLastEnd;        /* End time of last latency interval */
//   long    lTotalTime;      /* Time (sec) in period of interest */


/* Respond to user input (menu choices, etc.) and system messages */
   switch ( msg )
   {
      case WM_CREATE:         /* Do this the first time through */
         hCursor = LoadCursor( NULL, IDC_ARROW );
         SetCursor( hCursor );
         hMenu = GetMenu( hwnd );
         iVScrollOffset = 0;       /* Start with no vertical scrolling */
         break;

/* Get screen size in pixels, re-paint, and re-proportion screen */
      case WM_SIZE:
         cyScreen = HIWORD (lParam);
         cxScreen = LOWORD (lParam);

/* Compute font size */
         lTitleFHt = cyScreen / 33;
         lTitleFWd = cxScreen / 100;

/* Compute vertical scrolling amounts */
         iScrollVertBuff = cyScreen / Gparm.NumTracePerScreen;
         iScrollVertPage = cyScreen / 2;

/* Compute offsets from top and left sides of screen */
         iTitleOffset = 8*cyScreen / 100;
         iChanIDOffset = 13*cxScreen / 100;
         iSummaryOffset = 34*cxScreen / 100; /*31*/

/* Set scroll thumb positions */
         SetScrollRange( hwnd, SB_VERT, 0, 100, FALSE );  /* 100/50 arbitrary */
         SetScrollPos( hwnd, SB_VERT, 50, TRUE );

         InvalidateRect( hwnd, NULL, TRUE );    /* Force a re-PAINT */
         break;

/* Respond to menu selections */
      case WM_COMMAND:
           switch ( LOWORD (wParam) )
           {
            /* This menu option lets the user change the number of traces
               which are squished into the visible (vertically) part of the
               screen display. */
             case IDM_TRACEPERSCREEN:
                  if ( DialogBox( hInstMain, "TracePerScreen", hwndWndProc,
                      (DLGPROC) TracePerScreenDlgProc ) == IDOK )
                  {
                     iScrollVertBuff = cyScreen / Gparm.NumTracePerScreen;
                     iScrollVertPage = cyScreen / 8;
                     InvalidateRect( hwnd, NULL, TRUE );
                  }
                  break;

                  /* Print a summary of the latency percentages to the printer */
             case IDM_PRINTSUMMARY:
                  if ( iRead == 1 )
                  {
                     hPrinter = fopen( Gparm.PrinterPath, "w" );
                     if ( hPrinter == NULL )
                     {
                        logit( "et", "latency_mon: can't open printer.\n" );
                        break;
                     }
                     fprintf( hPrinter, "STATION DATA TRANSMISSION STATUS\n\n" );
                     fprintf( hPrinter, "Time Period (UTC):\n" );
                     fprintf( hPrinter, "Start: %s",
                              asctime( gmtime( &lStartTime ) ) );
                     fprintf( hPrinter, "End:   %s\n",
                              asctime( gmtime( &lEndTime ) ) );
                     fprintf( hPrinter, "                       Latency Summary\n");
                     fprintf( hPrinter, "Stn  Chn  Net    <1    1-2   2-3   3-5   Off\n" );
                     fprintf( hPrinter, "--------------------------------------------\n" );
                     for ( i=0; i<Station_Count; i++ )
                     {
                        fprintf( hPrinter
                               , "%-4s %-4s %-4s %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf\n"
                               , StaArray[i].szStation
                               , StaArray[i].szChannel
                               , StaArray[i].szNetID
                               , Latency[i].Percent[0]
                               , Latency[i].Percent[1]
                               , Latency[i].Percent[2]
                               , Latency[i].Percent[3]
                               , Latency[i].Percent[4]
                               );
                     }
                     fprintf( hPrinter, "%c", '\014');   /* Formfeed */
                     fclose( hPrinter );
                  }
                  break;

                  /* Save a summary of the latency percentages to a file */
             case IDM_SAVESUMMARY:
                  if ( iRead == 1 )
                  {
                     hSaveFile = fopen( "summary.txt", "w" );
                     if ( hSaveFile == NULL )
                     {
                        logit( "et", "latency_mon: can't open save file.\n" );
                        break;
                     }
                     fprintf( hSaveFile, "STATION DATA TRANSMISSION STATUS\n\n" );
                     fprintf( hSaveFile, "Time Period (UTC):\n" );
                     fprintf( hSaveFile, "Start: %s",
                              asctime( gmtime( &lStartTime ) ) );
                     fprintf( hSaveFile, "End:   %s\n",
                              asctime( gmtime( &lEndTime ) ) );
                     fprintf( hSaveFile, "                       Latency Summary\n");
                     fprintf( hSaveFile, "Stn  Chn  Net    <1    1-2   2-3   3-5   Off\n" );
                     fprintf( hSaveFile, "--------------------------------------------\n" );
                     for ( i=0; i<Station_Count; i++ )
                        fprintf( hSaveFile
                               , "%-4s %-4s %-4s %5.1lf %5.1lf %5.1lf %5.1lf %5.1lf\n"
                               , StaArray[i].szStation
                               , StaArray[i].szChannel
                               , StaArray[i].szNetID
                               , Latency[i].Percent[0]
                               , Latency[i].Percent[1]
                               , Latency[i].Percent[2]
                               , Latency[i].Percent[3]
                               , Latency[i].Percent[4]
                               );
                     fclose( hSaveFile );
                  }
                  break;

            case IDM_STATION_STATUS:
                 /*
                 ** Let user enter latency period of interest, and read in files
                 */
                 if ( DialogBox( hInstMain
                               , "StationStatus"
                               , hwndWndProc
                               , (DLGPROC)StationStatusDlgProc
                               ) == IDOK )
                 {
                    LoadSpanFromFile( hwnd );
                 }
                 break;

            case IDM_REFRESH:       // redraw the screen
                 if ( 5 <= Gparm.Debug )
                 {
                    logit("", "DEBUG WM_COMMAND::IDM_REFRESH; calling LoadSpanFromFile()\n");
                 }
                 LoadSpanFromFile( hwnd );
                 break;
           }
           break;

            /* Fill in screen display with traces, Ps, mag info, and stn names */
      case WM_PAINT:
           hdc = BeginPaint( hwnd, &ps );         /* Get device context */
           GetClientRect( hwnd, &rct );
           FillRect( hdc, &rct, (HBRUSH) GetStockObject( WHITE_BRUSH ) );
           DisplayTitles( hdc, lTitleFHt, lTitleFWd, cxScreen, cyScreen,
                          Latency, iVScrollOffset, Station_Count, iTitleOffset, Gparm.NumTracePerScreen
                         );
           DisplayChannelID( hdc, StaArray, Station_Count, lTitleFHt,
                             lTitleFWd, cxScreen, cyScreen, Gparm.NumTracePerScreen,
                             iVScrollOffset, iTitleOffset
                            );
           if ( 5 <= Gparm.Debug )
           {
              logit("", "DEBUG WM_PAINT; calling DisplayLatencyGraph()\n");
           }
           DisplayLatencyGraph( hdc, cxScreen, cyScreen, StaArray,
                                Latency, iVScrollOffset, iChanIDOffset, iSummaryOffset, Station_Count,
                                iTitleOffset, Gparm.NumTracePerScreen );
           EndPaint( hwnd, &ps );
           break;

/* Vertical scroll message */
      case WM_VSCROLL:
         if ( LOWORD (wParam) == SB_LINEUP )
            iVScrollOffset += iScrollVertBuff;
         else if ( LOWORD (wParam) == SB_PAGEUP )
            iVScrollOffset += iScrollVertPage;
         else if ( LOWORD (wParam) == SB_LINEDOWN )
            iVScrollOffset -= iScrollVertBuff;
         else if ( LOWORD (wParam) == SB_PAGEDOWN )
            iVScrollOffset -= iScrollVertPage;
         InvalidateRect( hwnd, NULL, TRUE );
         break;

/* Close up shop and return */
      case WM_DESTROY:
           logit( "", "WM_DESTROY posted - log last latency\n" );

/* Log last end time to each station's file */
         for ( i = 0 ; i < Station_Count ; i++ )
            if ( StaArray[i].LastPeriod.end_time > 0.0 )
            {
               Sta = (LATENCY_STATION *) &StaArray[i];

               if ( (hFile = fopen( Sta->StoreFileName, "r+b" )) != NULL )
               {
                  WritePeriod2File( hFile, &Sta->LastPeriod, Sta->StoreFileName );
                  fclose( hFile );
               }
/*
               if ( (hFile = fopen( Sta->StoreFileName, "ab" )) != NULL )
               {
                  fwrite( &Sta->LastPeriod.latency, sizeof(PERIOD_LATENCY), 1, hFile );
                  fwrite( &Sta->LastPeriod.stt_time, sizeof(PERIOD_TIME), 1, hFile );
                  fwrite( &Sta->LastPeriod.end_time, sizeof(PERIOD_TIME), 1, hFile );
                  fclose( hFile );
               }
*/
            }
         PostQuitMessage( 0 );
         break;

      default:
         return ( DefWindowProc( hwnd, msg, wParam, lParam ) );
   }
return 0;
}

      /*********************************************************
       *                     WThread()                         *
       *                                                       *
       *  This thread gets earthworm waveform messages.        *
       *                                                       *
       *********************************************************/

thr_ret WThread( void *dummy )
{
   int                i;
   char               line[40];        /* Heartbeat message */
   int                lineLen;         /* Length of heartbeat message */
   FILE             * hFile;           /* File handle */
   MSG_LOGO           logo;            /* Logo of retrieved msg */
   long               MsgLen;          /* Size of retrieved message */

   char               ThisLatency;

   double             test_intrv;


   /* Loop to read waveform messages
   ********************************/
   long    lGapSize;         /* Number of missing samples (integer) */
   time_t  now;              /* Current time */
   int     rc;               /* Return code from tport_getmsg() */

   int  _flag;

   for ( i = 0 ; i < Station_Count ; i++ )
   {
      StaArray[i].LastPeriod.stt_time = 0.0;
      StaArray[i].LastPeriod.end_time = 0.0;
      StaArray[i].LastPeriod.latency  = 0;
   }


   _flag = tport_getflag( &Gparm.InRegion );

   while ( _flag != TERMINATE && _flag != Gparm.MyModId )
   {
      static LATENCY_STATION *Sta;     /* Pointer to the station being processed */

      /* Send a heartbeat to the transport ring
      ****************************************/
      time( &now );

      if ( (now - then) >= Gparm.HeartbeatInt )
      {
         then = now;
         sprintf( line, "%ld %d\n", (long) now, (int) myPid );
         lineLen = strlen( line );
         if ( tport_putmsg( &Gparm.InRegion, &hrtlogo, lineLen, line ) != PUT_OK )
         {
            logit( "et", "latency_mon: Error sending heartbeat.\n" );
            break;
         }
      }

      /* Get a waveform from transport region
      **************************************/
      rc = tport_getmsg( &Gparm.InRegion, &getlogoW, 1, &logo, &MsgLen,
                         WaveBuf, MAX_TRACEBUF_SIZ);

      if ( rc == GET_NONE )
      {
         sleep_ew( 100 );
         continue;
      }

      if ( rc == GET_NOTRACK )
         logit( "et", "latency_mon: Tracking error.\n");

      if ( rc == GET_MISS_LAPPED )
         logit( "et", "latency_mon: Got lapped on the ring.\n");

      if ( rc == GET_MISS_SEQGAP )
         logit( "et", "latency_mon: Gap in sequence numbers.\n");

      if ( rc == GET_MISS )
         logit( "et", "latency_mon: Missed messages.\n");

      if ( rc == GET_TOOBIG )
      {
         if ( 1 <= Gparm.Debug )
         {
            logit( "et"
                 , "latency_mon: Retrieved message too big (%d) for msg.\n"
                 , MsgLen
                 );
         }
         continue;
      }

/* If necessary, swap bytes in the message
   ***************************************/
      if ( WaveMsgMakeLocal( WaveHead ) < 0 )
      {
         if ( 2 <= Gparm.Debug )
         {
            logit( "et", "latency_mon: Unknown waveform type.\n" );
         }
         continue;
      }

/* If sample rate is 0, get out of here before it kills program
   ************************************************************/
      if ( WaveHead->samprate == 0.0 )
      {
         if ( 2 <= Gparm.Debug )
         {
            logit( "", "Sample rate = 0.0 for %s %s message\n", WaveHead->sta, WaveHead->chan );
         }
         continue;
      }

/* Look up SCN number in the station list
   **************************************/
      Sta = NULL;
      for ( i = 0 ; i < Station_Count ; i++ )
      {
         if ( !strcmp( WaveHead->sta,  StaArray[i].szStation ) &&
              !strcmp( WaveHead->chan, StaArray[i].szChannel ) &&
              !strcmp( WaveHead->net,  StaArray[i].szNetID ) )
         {
            Sta = (LATENCY_STATION *) &StaArray[i];
            break;
         }
      }

      if ( Sta == NULL )
      {
         /* SCN not found */
         continue;
      }

      // 2002.10.19  times ~=  1034880000.000
      if (   WaveHead->endtime   < 1000000000.0 || 2000000000.0 < WaveHead->endtime
          || WaveHead->starttime < 1000000000.0 || 2000000000.0 < WaveHead->starttime
          || WaveHead->endtime < WaveHead->starttime
         )
      {
         if ( 2 <= Gparm.Debug )
         {
            logit( ""
                 , "%s %s message error: WaveHead->endtime = %lf, WaveHead->starttime = %lf\n"
                 , WaveHead->sta
                 , WaveHead->chan
                 , WaveHead->endtime
                 , WaveHead->starttime
                 );
         }
         continue;
      }


      /* Determine data latency (present time - mid-time of packet)
      *************************************************************/

      /*
      ** 20020930 dbh -- replaced this block
      */

/*
      if      ( (double)now-( (WaveHead->endtime + WaveHead->starttime)/2.) <=  60.0 ) iLate[i][1] = 0;
      else if ( (double)now-( (WaveHead->endtime + WaveHead->starttime)/2.) <= 120.0 ) iLate[i][1] = 1;
      else if ( (double)now-( (WaveHead->endtime + WaveHead->starttime)/2.) <= 180.0 ) iLate[i][1] = 2;
      else if ( (double)now-( (WaveHead->endtime + WaveHead->starttime)/2.) <= 300.0 ) iLate[i][1] = 3;
      else if ( (double)now-( (WaveHead->endtime + WaveHead->starttime)/2.)  > 300.0 ) iLate[i][1] = 4;
*/

      test_intrv = (double)now - ( ( WaveHead->endtime + WaveHead->starttime ) / 2.0 );

      if ( 5 <= Gparm.Debug )
      {
         logit( ""
              , "%s %s,  test interval: %d - (%lf - %lf) / 2 = %lf\n"
              , Sta->szStation
              , Sta->szChannel
              , now
              , WaveHead->endtime
              , WaveHead->starttime
              , test_intrv
              );
      }

      ThisLatency = 0;
      for ( i = 0 ; i < LATENCY_CUTOFF_COUNT ; i++ )
      {
         if ( test_intrv <= LATENCY_CUTOFFS[i] )
         {
            break;
         }
         /*
         ** Putting this here ensures that ThisLatency will be assigned
         ** a value  zero - LATENCY_CUTOFF_COUNT (0 - 4).
         ** For example, on the first pass through, if it did not meet the cutoff
         ** for latency 0, then ThisLatency will be increment to 1.
         */
         ThisLatency++;
      }

      if ( 4 == Gparm.Debug )
      {
         logit( ""
              , "%s %s latn: %d\n"
              , Sta->szStation
              , Sta->szChannel
              , ThisLatency
              );
      }

      /* Do this the first time we get a message with this SCN
      *******************************************************/
      if ( Sta->LastPeriod.stt_time == 0.0 )
      {
         LATENCY_PERIOD     writePeriod;

         if ( ( WaveHead->starttime - 1.0 / WaveHead->samprate ) < 0.0 )
         {
            // Catch certain errors
            logit( "t"
                 , "%s %s error: WaveHead->starttime: %lf ; WaveHead->samprate: %lf\n"
                 , Sta->szStation
                 , Sta->szChannel
                 , WaveHead->starttime
                 , WaveHead->samprate
                 );
            continue;
         }

         if ( 2 <= Gparm.Debug )
         {
            logit( "t"
                 , "Init %s %s\n"
                 , Sta->szStation
                 , Sta->szChannel
                 );
         }

         if ( (hFile = fopen( Sta->StoreFileName, "r+b" )) == NULL )
         {
            logit( "t", "failed opening latency file %s\n", Sta->StoreFileName );
         }
         else
         {
            writePeriod.stt_time = WaveHead->starttime;
            writePeriod.end_time = WaveHead->endtime;
            writePeriod.latency  = ThisLatency;

            if ( 3 <= Gparm.Debug )
            {
               logit( "t"
                    , "writing %s %s init period: %lf - %lf = %d\n"
                    , Sta->szStation
                    , Sta->szChannel
                    , writePeriod.stt_time
                    , writePeriod.end_time
                    , writePeriod.latency
                    );
            }

            WritePeriod2File( hFile, &writePeriod, Sta->StoreFileName );

            fclose( hFile );
         }

         Sta->LastPeriod.stt_time = WaveHead->endtime;
         Sta->LastPeriod.end_time = WaveHead->starttime - 1.0 / WaveHead->samprate;
         Sta->LastPeriod.latency  = ThisLatency;

      } /* first time for this SCN */


      /* If data is not in order, throw it out
      ***************************************/
      if ( Sta->LastPeriod.end_time >= WaveHead->starttime )
      {
         if ( 3 <= Gparm.Debug )
         {
            logit( "t", "%s %s out of order\n", Sta->szStation, Sta->szChannel );
         }
         continue;
      }

      /* Compute the number of samples since the end of the previous message.
      ** If (lGapSize == 1), no data has been lost between messages.
      ** If (1 < lGapSize <= 2), go ahead anyway.
      ** If (lGapSize > 2), call it an outage.
      **********************************************************************/
      lGapSize = (long) (WaveHead->samprate *
                        (WaveHead->starttime - Sta->LastPeriod.end_time) + 0.5);

      if ( 5 <= Gparm.Debug )
      {
         logit( ""
              , "%s %s,  gap size: %ld\n"
              , Sta->szStation
              , Sta->szChannel
              , lGapSize
              );
      }

      /* Log gaps or changes in latency (update file hourly if no changes)
      *****************************************************************/
      if (   lGapSize > 2
          || Sta->LastPeriod.latency  != ThisLatency
          || (Sta->LastPeriod.stt_time + 3600.0) < Sta->LastPeriod.end_time
         )
      {
         if ( 4 <= Gparm.Debug )
         {
            logit( "t"
                 , "%s %s gap (%ld) or latency (%d != %d) [%f - %f]\n"
                 , Sta->szStation
                 , Sta->szChannel
                 , lGapSize
                 , Sta->LastPeriod.latency
                 , ThisLatency
                 , Sta->LastPeriod.stt_time
                 , Sta->LastPeriod.end_time
                 );
         }

         if ( (hFile = fopen( Sta->StoreFileName, "r+b" )) == NULL )
         {
            logit( "t", "failed opening latency file %s\n", Sta->StoreFileName );
         }
         else
         {
            if ( 3 <= Gparm.Debug )
            {
               logit( "t"
                    , "writing %s %s period: %lf - %lf : %d\n"
                    , Sta->szStation
                    , Sta->szChannel
                    , Sta->LastPeriod.stt_time
                    , Sta->LastPeriod.end_time
                    , Sta->LastPeriod.latency
                    );
            }

            WritePeriod2File( hFile, &Sta->LastPeriod, Sta->StoreFileName );

            fclose( hFile );
         }

         if ( Sta->LastPeriod.end_time > Sta->LastPeriod.stt_time + 3600.0 )
         {
            /* If hourly update */
            Sta->LastPeriod.stt_time = Sta->LastPeriod.end_time;
         }
         else
         {
            Sta->LastPeriod.stt_time = WaveHead->starttime;
         }
      }

      /* Save time of the end of the current message and station's latency flag
      ************************************************************************/
      Sta->LastPeriod.end_time = WaveHead->endtime;
      Sta->LastPeriod.latency  = ThisLatency;

      _flag = tport_getflag( &Gparm.InRegion );
   }

   PostMessage( hwndWndProc, WM_DESTROY, 0, 0 );
}
