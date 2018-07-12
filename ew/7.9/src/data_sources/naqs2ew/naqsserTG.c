/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: naqsserTG.c 4513 2011-08-09 18:33:19Z paulf $
 *
 *    Revision history:
 *
 *     Revision 1.x  2007/08/06 22:35:07  whitmore
 *     Uses MySQL data base for tide station information
 *     Revision 1.1  2003/02/10 22:35:07  whitmore
 *     Initial revision (based on naqs2ew)
 *
 */

/*
 *   naqsserTG.c:  Program to receive transparent serial packets from a NaqsServer
 *                 via socket and log them to a disk file and send to RING.
 *                 Based on naqs2ew by dietz.
 *                 Serial packets expected here are ASCII output from the
 *                 real-time Sutron tide gage units at NOS gages.  One sample
 *                 is expected every 15 seconds.  This data is logged to disk
 *                 and RING for future display in the TIDE display programs or
 *                 export or processing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include <kom.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include "naqsser.h"
#ifdef ATWC
#include "..\..\..\..\atwc\src\libsrc\TideDBLib.h"
#endif
// ---- JMC 4.25.2008 ------

#include <windows.h>
#include <winsock.h>
#include <errno.h>

// ---- JMC 4.25.2008 ------

/* Functions used in this source file
 ************************************/
int  naqsser_shipdata( NMX_SERIAL_DATA * );
void PadZeroes( int, char * );
#ifdef ATWC
int  RetrieveTideDataFromFile( TGFILE_HEADER *, int *, char * );
#endif

#define BUFSIZE 1024                                     // JMC 4.25.2008
#define END_OF_RECORD 0x1E                               // JMC 4.25.2008

SOCKET getConnection(void);                              // JMC 4.25.2008
#ifdef ATWC
int  writeTideDataNGDC(char *sWr, int CalledFromline);    // JMC 4.25.2008
#endif
SOCKET mySocket = INVALID_SOCKET;                 // JMC 4.25.2008

/* Globals declared in naqschassis.c
 ***********************************/
extern char          RingName[];  /* name of transport ring for output        */
extern char          ProgName[];  /* name of program - used in logging        */
extern int           Debug;       /* Debug logging flag (optionally configured) */
extern unsigned char MyInstId;    /* local installation id                    */
extern unsigned char MyModId;     /* Module Id for this program               */
extern SHM_INFO      Region;      /* shared memory region to use for output   */

/* Globals specific to this naqsclient
 *************************************/
static int       MaxSamplePerMsg = 0; /* max # samples to expect in serial data */
static char      DataPath[64] = "\0"; /* Path to send data files to           */
static char      DataPath2[64] = "\0";/* Backup Path to send data files to    */
static int       MaxReqSER       = 0; /* working limit on max # scn's in ReqSCN */ 
static int       nReqSER = 0;         /* # of scn's found in config file      */
static SER_INFO *ReqSER  = NULL;      /* Serial channels requested in config  */
static TGFILE_HEADER TGIn[MAX_SCN_DUMP];/* Stations to convert and output     */
static TGFILE_HEADER TGH[MAX_TG_DATA];/* Tide gage data base information      */
static NMX_SERIAL_DATA NaqsSer;       /* struct containing one Naqs serial data packet */
static char      TideGageFile[64] = "\0"; /* Tide gage station control data   */
static char      TGDBAdd[64];         /* Tide gage data base server address */
static char      TGDBUsr[64];         /* Tide gage data base user name */
static char      TGDBPwd[64];         /* Tide gage data base password */
static char      TGDBDbn[64];         /* Tide gage data base name */
static char      TGDBTbl[64];         /* Tide gage data base table */
static int       iNumRec = 0;         /* Number of tide stations in data base */
static unsigned char    TypeSerialTG; /* Serial Tide gage message type        */

static NMX_CHANNEL_INFO *ChanInf = NULL;  /* channel info structures from NAQS*/
static int                nChan = 0;      /* actual # channels in ChanInf array */


/******************************************************************************
 * naqsclient_com() read config file commands specific to this client         *
 *   Return 1 if the command was processed, 0 if it wasn't                    *
 ******************************************************************************/

int startNGDC(void)
{
STARTUPINFO          si = { sizeof(si) };
PROCESS_INFORMATION  pi;
char                 szExe[256];
void *pMsg;
char tMsg[256];

   strcpy(szExe, "c:\\ngdc\\ngdc.bat"); // todo move to config file

   if(CreateProcess(0, szExe, 0, 0, FALSE, 0, 0, 0, &si, &pi))
    {
//      WaitForSingleObject(pi.hProcess, 60000);  // Wait for the child process to finish

      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
  else
    {
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  0,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPTSTR)&pMsg,
                  0,
                  NULL);
     sprintf(tMsg, "File: %s Error: %s\n", szExe, pMsg);
//
// Open dialog with error message
//
//      MessageBox(0, tMsg,
//                "Error", MB_OK|MB_ICONEXCLAMATION );
      logit ("t", tMsg);
      return -1;
    }
  return 0;
}

// -----------------------------------------------------------------------------

int naqsclient_com( char *configfile )
{
   int   i;
   char *str;

/* Add an entry to the request-list
 **********************************/
   if(k_its("RequestChannel") ) {
      int badarg = 0;
      char *argname[] = {"","sta","comp","net",
                            "pinno","delay","sendbuffer","dtexp","doffset",
                            "format","gain","tgabbrev","tgabbfile"};
      if( nReqSER >= MaxReqSER )
      {
         size_t size;
         MaxReqSER += 10;
         size     = MaxReqSER * sizeof( SER_INFO );
         ReqSER   = (SER_INFO *) realloc( ReqSER, size );
         if( ReqSER == NULL )
         {
            logit( "et",
                   "%s: Error allocating %d bytes"
                   " for requested channel list; exiting!\n", ProgName, size );
            exit( -1 );
         }
      }
      memset( &ReqSER[nReqSER], 0, sizeof(SER_INFO) );

      str=k_str();
      if( !str || strlen(str)>(size_t)STATION_LEN ) {
         badarg = 1; goto EndRequestChannel;
      }
      strcpy(ReqSER[nReqSER].sta,str);

      str=k_str();
      if( !str || strlen(str)>(size_t)CHAN_LEN) {
         badarg = 2; goto EndRequestChannel;
      }
      strcpy(ReqSER[nReqSER].chan,str);

      str=k_str();
      if( !str || strlen(str)>(size_t)NETWORK_LEN) {
         badarg = 3; goto EndRequestChannel;
      }
      strcpy(ReqSER[nReqSER].net,str);

      ReqSER[nReqSER].pinno = k_int();
      if( ReqSER[nReqSER].pinno <= 0 || ReqSER[nReqSER].pinno > 32767 ) {
         badarg = 4; goto EndRequestChannel;
      }

      ReqSER[nReqSER].delay = k_int();
      if( ReqSER[nReqSER].delay < -1 || ReqSER[nReqSER].delay > 300 ) {
         badarg = 5; goto EndRequestChannel;
      }

      ReqSER[nReqSER].sendbuffer = k_int();
      if( ReqSER[nReqSER].sendbuffer!=0 && ReqSER[nReqSER].sendbuffer!=1 ) {
         badarg = 6; goto EndRequestChannel;
      }

      ReqSER[nReqSER].dtexp = k_val();
      if( ReqSER[nReqSER].dtexp < 0. ) {
         badarg = 7; goto EndRequestChannel;
      }

      ReqSER[nReqSER].doffset = k_val();
      if( fabs( ReqSER[nReqSER].doffset ) > 100. ) {
         badarg = 8; goto EndRequestChannel;
      }

      ReqSER[nReqSER].format = k_int();
      if( ReqSER[nReqSER].format != 0 && ReqSER[nReqSER].format != 1 ) {
         badarg = 9; goto EndRequestChannel;
      }

      ReqSER[nReqSER].dgain = k_val();
      if( fabs( ReqSER[nReqSER].dgain ) < 0. ) {
         badarg = 10; goto EndRequestChannel;
      }
	  
      str=k_str();
      if( !str || strlen(str)>(size_t)ABBREV_LEN) {
         badarg = 11; goto EndRequestChannel;
      }
      
      strcpy(ReqSER[nReqSER].stnAbbrev,str);
      

/* Associate tide gage data base information with requested stations
********************************************************************/
      for ( i=0; i<iNumRec; i++ )
         if ( !strcmp( ReqSER[nReqSER].stnAbbrev, TGH[i].szSiteAbbrev ) )
         {
            logit( "", "Found %s in data file\n", TGH[i].szSiteAbbrev );
            strcpy( TGIn[nReqSER].szSiteAbbrev, TGH[i].szSiteAbbrev );
            strcpy( TGIn[nReqSER].szSiteName, TGH[i].szSiteName );
            strcpy( TGIn[nReqSER].szPlatformID, TGH[i].szPlatformID );
            strcpy( TGIn[nReqSER].szWMOCode, TGH[i].szWMOCode );
            strcpy( TGIn[nReqSER].szGOESID, TGH[i].szGOESID );
            strcpy( TGIn[nReqSER].szSiteOperator, TGH[i].szSiteOperator );
            strcpy( TGIn[nReqSER].szSiteOAbbrev, TGH[i].szSiteOAbbrev );
            strcpy( TGIn[nReqSER].szSiteLat, TGH[i].szSiteLat );
            strcpy( TGIn[nReqSER].szSiteLon, TGH[i].szSiteLon );
            strcpy( TGIn[nReqSER].szSensorType, TGH[i].szSensorType );
            strcpy( TGIn[nReqSER].szWaterLevelUnits, TGH[i].szWaterLevelUnits );
            strcpy( TGIn[nReqSER].szSampleRate, TGH[i].szSampleRate );
            strcpy( TGIn[nReqSER].szSampleRateUnits, TGH[i].szSampleRateUnits );
            strcpy( TGIn[nReqSER].szTransmissionType, TGH[i].szTransmissionType );
            strcpy( TGIn[nReqSER].szReferenceDatum, TGH[i].szReferenceDatum );
            strcpy( TGIn[nReqSER].szRecordingAgency, TGH[i].szRecordingAgency );
            strcpy( TGIn[nReqSER].szOcean, TGH[i].szOcean );
            strcpy( TGIn[nReqSER].szComments, TGH[i].szComments );
            strcpy( TGIn[nReqSER].szTimeZone, TGH[i].szTimeZone );
            strcpy( TGIn[nReqSER].szProcessingInfo, TGH[i].szProcessingInfo );
            strcpy( TGIn[nReqSER].szDataHeader, TGH[i].szDataHeader );
         }
      if ( i == nReqSER ) 
      {
         logit( "", "%s not found in data base\n", ReqSER[nReqSER].stnAbbrev );
         exit( -1 );
      }

      ReqSER[nReqSER].first = 0;

   EndRequestChannel:
      if( badarg ) {
         logit( "et", "%s: Argument %d (%s) bad in <RequestChannel> "
                "command (too long, missing, or invalid value):\n"
                "   \"%s\"\n", ProgName, badarg, argname[badarg], k_com() );
         logit( "et", "%s: exiting!\n", ProgName );
         free( ReqSER );
         exit( -1 );
      }
      nReqSER++;
      if ( nReqSER > MAX_SCN_DUMP )
      {
         logit( "et", "%s: Too many stations (%ld) - increase MAX_SCN_DUMP\n",
                ProgName, nReqSER );
         logit( "et", "%s: exiting!\n", ProgName );
         free( ReqSER );
         exit( -1 );
      }
      return( 1 );

   } /* end of RequestChannel */

   if(k_its("MaxSamplePerMsg") ) {
      MaxSamplePerMsg = k_int();
      if( MaxSamplePerMsg <= 0 )
      {
         logit( "e", "%s: <MaxSamplePerMsg %d> must be positive; exiting!\n",
                ProgName, MaxSamplePerMsg );
         free( ReqSER );
         exit( -1 );
      }
      return( 1 );
   }

   if(k_its("DataPath") ) {
      str = k_str();
      strcpy( DataPath, str );
      return( 1 );
   }

   if(k_its("DataPath2") ) {
      str = k_str();
      strcpy( DataPath2, str );
      return( 1 );
   }

#ifdef ATWC
   if(k_its("TideGageFile") ) {
      str = k_str();
      strcpy( TideGageFile, str );
/* Read tide gage data base 	   
***************************/
      if ( !RetrieveTideDataFromFile( TGH, &iNumRec, TideGageFile ) )
      {
         logit( "t", "Tide gage data base read failed; iNumRec=%ld\n", iNumRec);
         exit( -1 );
      } 
      logit( "", "Tide Gage Data read in from file - %ld stns\n", iNumRec );
      for (i=0; i<iNumRec; i++ )      /* Fix some values */
      {
         strcpy( TGH[i].szTimeZone, "UTC" );
         strcpy( TGH[i].szProcessingInfo, "Unfiltered" );
         strcpy( TGH[i].szDataHeader, "Data Format: SampleTime(epochal 1/1/1970)  WaterLevel  SampleTime(yyymmddhhmmss)" );
      }   
      return( 1 );
   }
#endif

   if(k_its("TGDBAdd") ) {
      str = k_str();
      strcpy( TGDBAdd, str );
      return( 1 );
   }

   if(k_its("TGDBUsr") ) {
      str = k_str();
      strcpy( TGDBUsr, str );
      return( 1 );
   }

   if(k_its("TGDBPwd") ) {
      str = k_str();
      strcpy( TGDBPwd, str );
      return( 1 );
   }

   if(k_its("TGDBDbn") ) {
      str = k_str();
      strcpy( TGDBDbn, str );
      return( 1 );
   }
// -----------------------------------------------------------------------------------------
// start the sockets receiver for NGDC  JMC 7.29.2008
//
    if (startNGDC()<0)
      logit( "t", "%s - %d startNGDC() failed - data not being sent to NGDC\n", __FILE__, __LINE__ );

// -----------------------------------------------------------------------------------------
#ifdef ATWC
   if(k_its("TGDBTbl") ) {
      str = k_str();
      strcpy( TGDBTbl, str );
/* Read tide gage data base 	   
***************************/
      if ( strlen( TideGageFile ) <= 0 )/* We are using data base for tg info */
      {
         logit( "", "%s %s %s %s %s\n", TGDBAdd, TGDBUsr, TGDBPwd, TGDBDbn, TGDBTbl );
         if ( !RetrieveTideStationData( TGH, &iNumRec, TGDBAdd, TGDBUsr, TGDBPwd,
                                        TGDBDbn, TGDBTbl ) )
         {
            logit( "t", "Tide gage data base read failed; iNumRec=%ld\n", iNumRec );
            exit( -1 );
         } 
         if ( iNumRec > MAX_TG_DATA )
         {
            logit( "t", "Too many stns in TGDB-%ld;increase MAX_TG_DATA\n", iNumRec );
            return( -1 );
         } 
         logit( "", "Tide Gage Data read in from data base - %ld stns\n", iNumRec );
         for (i=0; i<iNumRec; i++ )      /* Fix some values */
         {
            strcpy( TGH[i].szTimeZone, "UTC" );
            strcpy( TGH[i].szProcessingInfo, "Unfiltered" );
            strcpy( TGH[i].szDataHeader, "Data Format: SampleTime(epochal 1/1/1970)  WaterLevel  SampleTime(yyymmddhhmmss)" );
         }
      }
      return( 1 );
   }
#endif
   return( 0 );
}

/*******************************************************************************
 * naqsclient_init() initialize client stuff                                   *
 *******************************************************************************/
int naqsclient_init( void )
{
/* Check that all commands were read from config file
 ****************************************************/
   if( nReqSER == 0 )
   {
     logit("et", "%s: No <RequestChannel> commands in config file!\n",
            ProgName );
     return( -1 );
   }
   if( MaxSamplePerMsg == 0 )
   {
     logit("et", "%s: No <MaxSamplePerMsg> command in config file!\n",
            ProgName );
     return( -1 );
   }
   if( !DataPath || strlen( DataPath ) <= 0 )
   {
      logit( "et", "%s: No <DataPath> command in config file!\n",
             ProgName );
      return( -1 );
   }
   if( !DataPath2 || strlen( DataPath2 ) <= 0 )
   {
      logit( "et", "%s: No <DataPath2> command in config file!\n",
             ProgName );
      return( -1 );
   }
   if( !TideGageFile || strlen( TideGageFile ) <= 0 )
      logit( "et", "%s: No <TideGageFile> command in config file; Use data base\n",
             ProgName );
   else
      logit( "et", "%s: <TideGageFile> specified in config file - %s\n",
             ProgName, TideGageFile );

/* Look up message types in earthworm.d tables
 *********************************************/
   if ( GetType( "TYPE_SERTGTWC", &TypeSerialTG ) != 0 ) {
      logit("et","%s: Invalid message type <TYPE_SERTGTWC>!\n",
             ProgName );
      return( -1 );
   }

/* Allocate memory 
 *****************/
   NaqsSer.maxdatalen = MaxSamplePerMsg; 
   if ( ( NaqsSer.data = (char *) malloc( NaqsSer.maxdatalen ) ) == NULL )
   {
     logit( "et", "%s: Cannot allocate %ld bytes for serial data!\n",
            ProgName, NaqsSer.maxdatalen );
     return( -1 );
   }
   
   mySocket = getConnection();                                  // JMC 4.25.2008
   return( 0 );
} 


/*******************************************************************************
 * naqsclient_process() handle interesting packets from NaqsServer             *
 *  Returns 1 if packet was processed successfully                             *
 *          0 if packet was NOT processed                                      *
 *         -1 if there was trouble processing the packet                       *
 *******************************************************************************/
int naqsclient_process( long rctype, NMXHDR *pnaqshd, 
                        char **pbuf, int *pbuflen )
{
   char *sockbuf = *pbuf;  /* working pointer into socket buffer */
   int   rdrc;
   int   rc;

/* Process serial packets 
 ************************/
   if( rctype == NMXMSG_COMPRESSED_DATA )  
   {
      if ( Debug ) logit( "et","%s: got a %d-byte data message\n",
                          ProgName, pnaqshd->msglen );

   Read_Waveform:
      /* read the message */
      rdrc = nmx_rd_serial_data( pnaqshd, sockbuf, &NaqsSer );
      if( rdrc < 0 )
      {
         logit("et","%s: trouble reading waveform message\n", ProgName );
         return( -1 );
      }

   /* data buffer wasn't big enough, realloc and try again */
      else if( rdrc > 0 )
      {
         char *ptmp;
         if( (ptmp=(char *)realloc( NaqsSer.data, rdrc)) == NULL )
         {
            logit("et","%s: error reallocing NaqsSer.data from %d to %d "
                    "bytes\n", ProgName, NaqsSer.maxdatalen, rdrc );
            return( -1 );
         }
         logit( "et","%s: realloc'd NaqsSer.data from %d to %d "
                "bytes\n", ProgName, NaqsSer.maxdatalen, rdrc );
         NaqsSer.data       = ptmp;
         NaqsSer.maxdatalen = rdrc;
         goto Read_Waveform;
      }

      if(Debug>=2)
      {
         int id;
         logit("et","chankey:%d  starttime:%.2lf  nbyte:%d\n",
                NaqsSer.chankey, NaqsSer.starttime, NaqsSer.nbyte );
         for( id=0; id<NaqsSer.nbyte; id++ ) {
            logit("e", "%c", NaqsSer.data[id] );
            if((id+1)%SAMPLE_LENGTH2==0) logit("e", "\n");
         }
      }

   /* write it to the the disk file */
      rc = naqsser_shipdata( &NaqsSer );
	  
   } /* end if serial data */

/* got a list of all possible channels, continue startup protocol
 *  2) recv "channel list" message
 *  3) send "add channel" message(s)
 ****************************************************************/
   else if( rctype == NMXMSG_CHANNEL_LIST )
   {
      int msglen = 0;
      int nadd   = 0;
      int ir;

      nChan = 0;
      free( ChanInf );

   /* read the channel list message */
      if( nmx_rd_channel_list( pnaqshd, sockbuf, &ChanInf, &nChan ) < 0 )
      {
         logit("e", "%s: trouble reading channel list message\n", ProgName );
         return( -1 );
      }

   /* compare the Naqs channel list against the requested channels */

      rc = SelectSerChannels( ChanInf, nChan, ReqSER, nReqSER );

   /* build and send "add channel" message for each available channel */
          
      for( ir=0; ir<nReqSER; ir++ )
      {
         if( ReqSER[ir].flag == SER_NOTAVAILABLE ) continue;
         if( nmx_bld_add_serial( pbuf, pbuflen, &msglen,
                  &(ReqSER[ir].info.chankey), 1, ReqSER[ir].delay,
                  ReqSER[ir].sendbuffer ) != 0 )
         {
            logit("et","%s: trouble building add_serial message\n", ProgName);
            continue;
         }
         nmxsrv_sendto( sockbuf, msglen );
         if(Debug) logit("et","sent add_serial message %d\n", ++nadd );
      }
      
   } /* end if channel list msg */

/* Not a message type we're interested in
 ****************************************/
   else {
      return( 0 );
   }

   return( 1 );

} /* end of naqsclient_process */


/*******************************************************************************
 * naqsclient_shutdown()  frees malloc'd memory, etc                           *
 *******************************************************************************/
void naqsclient_shutdown( void )
{
/* Free all allocated memory
 ***************************/
   free( ReqSER );
   free( NaqsSer.data );
   free( ChanInf );

}

/*****************************************************************************
 * naqsser_shipdata() takes a buffer of Naqs serial data, translates it into *
 *   TYPE_SERTGTWC message(s) and writes it to the transport region and      *
 *   to a disk file.                                                         * 
 *****************************************************************************/
int naqsser_shipdata( NMX_SERIAL_DATA *nbuf )
{
   double       dTemp;
   char         FileName[64];             /* Path/file to output data to    */
   FILE        *hFile;                    /* Data File handle               */
   static int   i, j;
   int          iLen;                     /* Station Abbreviation length */
   int          iyDay, imDay, iYear, iMon, iHour, iMin, iSec;/* converted time*/
   char         line[MAX_SERTGTWC_SIZE];  /* buffer for outgoing message    */
   int          lineLen;                  /* length of line in bytes        */
   MSG_LOGO     logo;
   double       dt=0.0;   
   int          numsamps;                 /* number of samples in this packet*/
   struct tm*   PacketTime;               /* Integer form time structure     */
   char         samp[MAX_SAMPS][SAMPLE_LENGTH2+1]; /* serial data             */
   SER_INFO    *scn;                      /* pointer to channel information  */
   long         StartTime;                /* Data point time                 */
   char         szBuffer[8];              /* Temporary buffer for file name  */
   char         Temp[16];                 /* Temporary character string      */
   char         Time[16];                 /* Temporary character string      */
   char         tData[500];               // JMC 4.25.2008
   
   logo.instid = MyInstId;
   logo.mod    = MyModId;
   logo.type   = TypeSerialTG;

/* Find this channel in the subscription list
 ********************************************/
   scn = FindSerChannel( ReqSER, nReqSER, nbuf->chankey );
   if( scn == NULL )
   {
      logit("et","%s: Received unknown chankey:%d "
            "(not in subscription list)\n", ProgName, nbuf->chankey );
      /* probably should report this to statmgr */
      return( NMX_SUCCESS );
   }
   if( Debug ) logit("e","%s.%s.%s  new data starttime: %.3lf nsamp:%d\n",
                      scn->sta,scn->chan,scn->net,nbuf->starttime,
                      nbuf->nbyte );
					  
/* First data for this channel.  
 ******************************/
   if( scn->first == 0 ) 
   {
      scn->texp     = nbuf->starttime;
      scn->first    = 1;
   }
   
/* Check for time tears (overlaps/abnormal forward jumps). 
 *********************************************************/
   dt = nbuf->starttime - scn->texp;
   if( ABS(dt) > scn->dtexp ) 
   {
      if ( dt > 0.0 )                             /* time gap */
      {
         logit("e","%s: %s.%s.%s %.3lf %.3lf s gap detected\n",
                ProgName, scn->sta, scn->chan, scn->net, scn->texp, dt );
      }
      else                                        /* time overlap */ 
      {     
         logit("e","%s: %s.%s.%s %.3lf %.3lf s overlap detected\n",
               ProgName, scn->sta, scn->chan, scn->net, scn->texp, ABS(dt) );
      }
   }

/* Split msg in case there is more than one sample
 *************************************************/
   numsamps = 0;
   for ( i=0; i<nbuf->nbyte; i++ )
   {
      if ( nbuf->data[i] == '$')            /* Start of new NOS data */
      {
         i += 4;
         for ( j=0; j<SAMPLE_LENGTH2-5; j++ )/* Expect 9 chars. after $1AO */
         {                                  /* Sutron format $1AO+00002.34 */
            i += 1;
            samp[numsamps][j] = nbuf->data[i];
         }
         samp[numsamps][SAMPLE_LENGTH2-5] = '\0';
         numsamps++;                            
         if ( numsamps >= MAX_SAMPS )
         {
            logit( "", "Too many different channels in incoming data - increase MAX_SAMPS\n" );
            break;
         }
      }
      else if ( nbuf->data[i] == '@')        /* Start of new WCATWC data */
      {
         i += 25;
         for ( j=0; j<SAMPLE_LENGTH3-28; j++ )/* Look for amplitude field */
         {        /* Ohmart format @2005/12/02 16:00:23=001# 528       #cm */
            i += 1;
            samp[numsamps][j] = nbuf->data[i];
         }
         samp[numsamps][SAMPLE_LENGTH3-28] = '\0';
         numsamps++;                            
         if ( numsamps >= MAX_SAMPS )
         {
            logit( "", "Too many different channels in incoming data - increase MAX_SAMPS-2\n" );
            break;
         }
      }
   }

/* Write samples to disk and ring.
 *********************************/
   for ( i=0; i<numsamps; i++ )
   {
      /* Match SCN with station abbreviation */
      for ( j=0; j<nReqSER; j++ )
         if ( !strcmp( scn->sta, ReqSER[j].sta ) ) break;
      if ( j == nReqSER )
      {
         logit( "", "Incoming station not listed in .d file - %s\n", scn->sta );
         break;
      }
   
      /* Create TYPE_SERTGTWC message */
      dTemp = atof( samp[i] );
      dTemp *= scn->dgain;                   /* Account for station gain */
      dTemp -= scn->doffset;                 /* Adjust to reference datum */
      if (scn->format == 0) dTemp *= 100.;   /* Convert m to cm */
      sprintf( line, "%s %s %s %lf %lf\0",
               scn->sta, scn->chan, scn->net,
               nbuf->starttime + (double) i*scn->dtexp, dTemp );
      lineLen = strlen( line ) + 1;
      if ( Debug ) logit( "e","%s - to RING\n", line );
      /* Send message to ring */	  
      if( tport_putmsg( &Region, &logo, lineLen, line ) != PUT_OK )
      {
         logit( "et", "%s: Error putting %d-byte SERTGTWC msg in %s\n",
                ProgName, lineLen, RingName );
      }
	  
      /* Create file name and open the data file (old format) */
      strcpy( FileName, DataPath );        
      strcat( FileName, "T" );
      strcat( FileName, scn->sta );
      StartTime = (long) (floor( nbuf->starttime + (double) i*scn->dtexp ));
      PacketTime = gmtime( (time_t*) &StartTime );
      iyDay = (int) PacketTime->tm_yday;
      imDay = (int) PacketTime->tm_mday;
      iYear = (int) PacketTime->tm_year;
      iMon  = (int) PacketTime->tm_mon;
      iHour = (int) PacketTime->tm_hour;
      iMin  = (int) PacketTime->tm_min;
      iSec  = (int) PacketTime->tm_sec;
      itoa( iYear+1900, Temp, 10 );
      strcat( FileName, Temp );
      strcat( FileName, "." );
      itoa( iyDay+1, Temp, 10 );
      PadZeroes( 3, Temp );
      strcat( FileName, Temp );
      if ( (hFile = fopen( FileName, "a" )) != NULL )
      {             /* Convert time to Modified Julian Time */
         dTemp = atof( samp[i] );
         dTemp *= scn->dgain;      /* Account for station gain */
         dTemp -= scn->doffset;    /* Adjust to reference datum */
         if (scn->format == 0) dTemp *= 100.;   /* Convert m to cm */
         fprintf( hFile, "%.1lf %.0lf\n", dTemp, (StartTime+3506630400.)*100. );
	     fclose( hFile );
      }
      else
         logit( "", "Could not open file for append - %s (errno=%d)\n", FileName, errno ); // JMC 
		 
      /* Create file name and open the data file (new format) */
      strcpy( FileName, DataPath );        
      iLen = strlen( TGIn[j].szSiteAbbrev );
      strncpy( szBuffer, TGIn[j].szSiteAbbrev, iLen-1 );
      szBuffer[iLen-1] = '\0';
      strcat( szBuffer, "_" );
      szBuffer[iLen] = TGIn[j].szSiteAbbrev[iLen-1];
      szBuffer[iLen+1] = '\0';
      strcat( FileName, szBuffer );
      strcat( FileName, "_" );
      itoa( iYear+1900, Temp, 10 );
      strcat( FileName, Temp );
      strcat( FileName, "." );
      itoa( iyDay+1, Temp, 10 );
      PadZeroes( 3, Temp );
      strcat( FileName, Temp );
      if ( (hFile = fopen( FileName, "r" )) != NULL )
      {             /* File exists so just append data */
         fclose( hFile );
         if ( (hFile = fopen( FileName, "a" )) != NULL )
         {
            dTemp = atof( samp[i] );
            dTemp *= scn->dgain;         /* Account for station gain */
            dTemp -= scn->doffset;       /* Adjust to reference datum */
            if (scn->format == 0) dTemp *= 100.;    /* Convert m to cm */
            /* Put data in proper time format */
            itoa( iYear+1900, Temp, 10 );
            strcpy( Time, Temp );
            itoa( iMon+1, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( imDay, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iHour, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iMin, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iSec, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            fprintf( hFile, "%ld %.1lf %s\n", StartTime, dTemp, Time );
	    
            fclose( hFile );                                  // JMC 

         }
         else
         {  logit( "t", "Could not open file for append (2) - %s (errno = %d)\n",
                        FileName, errno ); }          // JMC
      }
      else          /* File doesn't exist so create with header */
      {
         logit( "t", "Create new file - %s\n", FileName );
         /* Put data in proper time format */
         itoa( iYear+1900, Temp, 10 );
         strcpy( Time, Temp );
         itoa( iMon+1, Temp, 10 );
         PadZeroes( 2, Temp );
         strcat( Time, Temp );
         itoa( imDay, Temp, 10 );
         PadZeroes( 2, Temp );
         strcat( Time, Temp );
         if ( (hFile = fopen( FileName, "w" )) != NULL )
         {
            fprintf( hFile, "%s %s %s %s %s %s %s\n", TGIn[j].szSiteName, 
             TGIn[j].szPlatformID, TGIn[j].szSiteOperator,
             TGIn[j].szTransmissionType, TGIn[j].szSiteLat, TGIn[j].szSiteLon,
             Time );
            fprintf( hFile, "%s %s %s %s %s %s %s %s\n", TGIn[j].szSensorType, 
             TGIn[j].szWaterLevelUnits, TGIn[j].szTimeZone, TGIn[j].szSampleRate, 
             TGIn[j].szSampleRateUnits, TGIn[j].szReferenceDatum,
             TGIn[j].szRecordingAgency, TGIn[j].szProcessingInfo );
            fprintf( hFile, "%s\n", TGIn[j].szComments ); 
            fprintf( hFile, "%s\n", TGIn[j].szDataHeader ); 
            dTemp = atof( samp[i] );
            dTemp *= scn->dgain;      /* Account for station gain */
            dTemp -= scn->doffset;    /* Adjust to reference datum */
            if (scn->format == 0) dTemp *= 100.;    /* Convert m to cm */
            itoa( iHour, Temp, 10 );  /* Finish Time */
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iMin, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iSec, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            fprintf( hFile, "%ld %.1lf %s\n", StartTime, dTemp, Time );
            sprintf(tData,  "%s|%.1lf|%s\n", TGIn[j].szSiteName, 
	             dTemp, Time );                           // JMC 4.25.2008
            writeTideDataNGDC(tData, __LINE__);               // JMC 4.25.2008		    
            fclose( hFile );                                  // JMC 
         }
         else
         {  logit( "t", "Could not open file for write - %s (errno = %d)\n",
                        FileName, errno ); }  // JMC
      }
		 
      /* Create backup file name and open the data file (new format) */
      strcpy( FileName, DataPath2 );        
      iLen = strlen( TGIn[j].szSiteAbbrev );
      strncpy( szBuffer, TGIn[j].szSiteAbbrev, iLen-1 );
      szBuffer[iLen-1] = '\0';
      strcat( szBuffer, "_" );
      szBuffer[iLen] = TGIn[j].szSiteAbbrev[iLen-1];
      szBuffer[iLen+1] = '\0';
      strcat( FileName, szBuffer );
      strcat( FileName, "_" );
      itoa( iYear+1900, Temp, 10 );
      strcat( FileName, Temp );
      strcat( FileName, "." );
      itoa( iyDay+1, Temp, 10 );
      PadZeroes( 3, Temp );
      strcat( FileName, Temp );
      if ( (hFile = fopen( FileName, "r" )) != NULL )
      {             /* File exists so just append data */
         fclose( hFile );
         if ( (hFile = fopen( FileName, "a" )) != NULL )
         {
            dTemp = atof( samp[i] );
            dTemp *= scn->dgain;         /* Account for station gain */
            dTemp -= scn->doffset;       /* Adjust to reference datum */
            if (scn->format == 0) dTemp *= 100.;    /* Convert m to cm */
            /* Put data in proper time format */
            itoa( iYear+1900, Temp, 10 );
            strcpy( Time, Temp );
            itoa( iMon+1, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( imDay, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iHour, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iMin, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iSec, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            fprintf( hFile, "%ld %.1lf %s\n", StartTime, dTemp, Time );
            sprintf(tData,  "%s|%.1lf|%s\n", TGIn[j].szSiteName, 
	             dTemp, Time );                           // JMC 4.25.2008
            writeTideDataNGDC(tData, __LINE__);               // JMC 4.25.2008		
            fclose( hFile );                                  // JMC 
         }
         else
         {  logit( "t", "Could not open file for append (2) - %s (errno = %d)\n",
                        FileName, errno ); }            // JMC
      }
      else          /* File doesn't exist so create with header */
      {
         logit( "t", "Create new file - %s\n", FileName );
         /* Put data in proper time format */
         itoa( iYear+1900, Temp, 10 );
         strcpy( Time, Temp );
         itoa( iMon+1, Temp, 10 );
         PadZeroes( 2, Temp );
         strcat( Time, Temp );
         itoa( imDay, Temp, 10 );
         PadZeroes( 2, Temp );
         strcat( Time, Temp );
         if ( (hFile = fopen( FileName, "w" )) != NULL )
         {
            fprintf( hFile, "%s %s %s %s %s %s %s\n", TGIn[j].szSiteName, 
             TGIn[j].szPlatformID, TGIn[j].szSiteOperator,
             TGIn[j].szTransmissionType, TGIn[j].szSiteLat, TGIn[j].szSiteLon,
             Time );
            fprintf( hFile, "%s %s %s %s %s %s %s %s\n", TGIn[j].szSensorType, 
             TGIn[j].szWaterLevelUnits, TGIn[j].szTimeZone, TGIn[j].szSampleRate, 
             TGIn[j].szSampleRateUnits, TGIn[j].szReferenceDatum,
             TGIn[j].szRecordingAgency, TGIn[j].szProcessingInfo );
            fprintf( hFile, "%s\n", TGIn[j].szComments ); 
            fprintf( hFile, "%s\n", TGIn[j].szDataHeader ); 
            dTemp = atof( samp[i] );
            dTemp *= scn->dgain;      /* Account for station gain */
            dTemp -= scn->doffset;    /* Adjust to reference datum */
            if (scn->format == 0) dTemp *= 100.;    /* Convert m to cm */
            itoa( iHour, Temp, 10 );  /* Finish Time */
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iMin, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            itoa( iSec, Temp, 10 );
            PadZeroes( 2, Temp );
            strcat( Time, Temp );
            fprintf( hFile, "%ld %.1lf %s\n", StartTime, dTemp, Time );
            sprintf(tData,  "%s|%.1lf|%s\n", TGIn[j].szSiteName, 
	             dTemp, Time );                           // JMC 4.25.2008
            writeTideDataNGDC(tData, __LINE__);               // JMC 4.25.2008		    
            fclose( hFile );                                  // JMC 
         }
         else
         {  logit( "t", "Could not open file for write - %s (errno = %d)\n",
                        FileName, errno ); }


      }
   }
   
/* Compute next expected sample time
   *********************************/
   if ( numsamps > 0 )
   {   
      scn->texp = nbuf->starttime + (double)numsamps*scn->dtexp;
      if ( Debug ) logit( "e","%s.%s.%s texp=%.3lf after data shipped\n",
                          scn->sta,scn->chan,scn->net,scn->texp );
   }
   
   return NMX_SUCCESS;
}

      /******************************************************************
       *                           PadZeroes()                          *
       *                                                                *
       *  This function adds leading zeroes to a numeric character      *
       *  string. (10 characters maximum in string).                    *
       *                                                                *
       *  Arguments:                                                    *
       *   iNumDigits       # digits desired in output                  *
       *   pszString        String to be padded with zeroes             *
       *                                                                *
       ******************************************************************/
	   
void PadZeroes( int iNumDigits, char *pszString )
{
   int     i, iStringLen;
   char    szTemp1[10], szTemp2[10];

   strcpy( szTemp1, pszString );
   strcpy( szTemp2, szTemp1 );
   iStringLen = strlen( szTemp1 );
   if ( iNumDigits-iStringLen > 0 )     /* Number of zeroes to add */
   {
      for ( i=0; i<iNumDigits-iStringLen; i++ ) szTemp1[i] = '0';
      for ( i=iNumDigits-iStringLen; i<iNumDigits; i++ ) 
         szTemp1[i] = szTemp2[i-(iNumDigits-iStringLen)];
      szTemp1[iNumDigits] = '\0';       /* Add NULL character to end */
      strcpy( pszString, szTemp1 );
   }
}

      /******************************************************************
       *                RetrieveTideDataFromFile()                      *
       *                                                                *
       *  This function fills in tide gage header information for each  *
       *  station.                                                      *
       *                                                                *
       *  Arguments:                                                    *
       *   pTGH             Structure to fill with tide header info     *
       *   piNSta           Number of stations in pTGH                  *
       *   pszFileName      Name of file containing station information *
       *                                                                *
       *  Returns:                                                      *
       *   int              1 -> Data filled OK                         *
       *                    0 -> Problem                                *
       *                                                                *
       ******************************************************************/  
int RetrieveTideDataFromFile( TGFILE_HEADER *pTGH, int *piNSta,
                              char *pszFileName )
{
   FILE    *hFile;

   *piNSta = 0;
   if ( (hFile = fopen( pszFileName, "r" )) == NULL )
   {
      fprintf( stderr, "Tide station file (%s) not opened\n", pszFileName );
      return( 0 );
   }
   else
   {
      while( !feof( hFile ) )
      {
         fscanf( hFile, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
          pTGH[*piNSta].szSiteName, pTGH[*piNSta].szSiteAbbrev,
	  pTGH[*piNSta].szPlatformID, pTGH[*piNSta].szWMOCode,
          pTGH[*piNSta].szGOESID, pTGH[*piNSta].szSiteOperator,
	  pTGH[*piNSta].szSiteOAbbrev, pTGH[*piNSta].szSiteLat,
          pTGH[*piNSta].szSiteLon, pTGH[*piNSta].szWaterLevelUnits,
	  pTGH[*piNSta].szTransmissionType, pTGH[*piNSta].szSampleRate,
	  pTGH[*piNSta].szSampleRateUnits, pTGH[*piNSta].szReferenceDatum,
          pTGH[*piNSta].szRecordingAgency, pTGH[*piNSta].szSensorType,
	  pTGH[*piNSta].szOcean, pTGH[*piNSta].szComments );
         *piNSta = *piNSta + 1;
         if ( *piNSta >= MAX_TG_DATA )
         {
            logit( "t", "Too many stns in file (%ld);increase MAX_TG_DATA\n",
                   *piNSta );
            fclose( hFile );
            return( 0 );
         } 
      }   
      fclose( hFile );
   }
   return( 1 );
}

//----JMC 4.25.2008------------------------------------------------------
//
// ----------------------------------------------------------------
// Function: initPipe()
// This function creates a named pipe to ship data to the NGDC receiver
//
// ----------------------------------------------------------------                   
//

int  writeTideDataNGDC(char *sWr, int CalledFromline)
{
 int i, len;
 char szUpdate[9600];
 
    if (mySocket == INVALID_SOCKET)
      return -1;
      
    i = sprintf(szUpdate,"%s%c", sWr, END_OF_RECORD);
    len = send(mySocket, szUpdate, i, 0);
//    logit( "e", "%s@%d: send() %s (inv %d)\n", __FILE__, __LINE__, szUpdate, CalledFromline);        
    if (len < i || len == INVALID_SOCKET)
      logit( "t", "%s@%d: send() error: %ld  \n", __FILE__, __LINE__, WSAGetLastError());    
   
  return 0;   
}
   
// --------------------------------------------------------------------

SOCKET getConnection(void)
{
  SOCKET theSocket;
  SOCKADDR_IN serverInfo;
  WSADATA wsadata;
  int rVal=0; 
  
  if (WSAStartup(MAKEWORD(1,1), &wsadata) < 0)
    {
     logit( "t", "%s@%d: WSAStartup() error: %ld \n", __FILE__, __LINE__, WSAGetLastError());
     return INVALID_SOCKET;
    }

//
// ---------------------------------------------------------
// 
   theSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	
   if (theSocket == INVALID_SOCKET) 
     {
     logit( "t", "%s@%d: socket() error: %ld  \n", __FILE__, __LINE__, WSAGetLastError());
     return INVALID_SOCKET;
     }

//
// ---------------------------------------------------------
// 

   serverInfo.sin_family = PF_INET;
   serverInfo.sin_addr.s_addr = inet_addr("127.0.0.1"); // loop back host
   serverInfo.sin_port = htons(22302);

   if (connect(theSocket, (LPSOCKADDR)&serverInfo, sizeof(struct sockaddr))<0)
    {	 
         logit( "t", "%s@%d: connect() error: %ld \n", __FILE__, __LINE__, WSAGetLastError());
         closesocket(theSocket);
	 theSocket = INVALID_SOCKET;
         return INVALID_SOCKET;
    }
    
  logit( "t", "%s@%d: connected OK (%s - %s)\n", __FILE__, __LINE__, __DATE__, __TIME__);
    
  return theSocket;

}
