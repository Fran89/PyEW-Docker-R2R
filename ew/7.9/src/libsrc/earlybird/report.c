  /**********************************************************************
   *                              report.c                              *
   *                                                                    *
   *  The functions in this file report picks, alarms, and locations    *
   *  to the appropriate rings.  They are shared by all Earlybird       *
   *  programs.                                                         *
   **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>          
#include <time.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "earlybirdlib.h"

     /**************************************************************
      *                         ReportAlarm()                      *
      *                                                            *
      *                 Fill in TYPE_ALARM format                  *
      **************************************************************/

void ReportAlarm( STATION *Sta, unsigned char ucMyModID, SHM_INFO siAlarmRegion,
                  unsigned char ucEWHTypeAlarm, unsigned char ucEWHMyInstID,
                  int iType, char *pszAlarmString, int iRespond )
{
   MSG_LOGO     logo;      /* Logo of message to send to the output ring */
   int          lineLen;
   char         line[256]; /* TYPE_PICKALARM format message */

/* Create TYPE_ALARM message (1->activate Respond Thread)
   ******************************************************/
   if ( iType == 1 )        /* Strong motion SP alarm */
   {
      sprintf( line, "%5d %5d %5d %5d     0 %s %s SP ALARM",
       (int) ucEWHTypeAlarm, (int) ucMyModID, (int) ucEWHMyInstID,
       Sta->iAlarm, Sta->szStation, Sta->szChannel );
/* Send the SP alarm to the earthvu from here. */
#ifdef _WINNT
      SendAlarm2Earthvu( Sta->szStation, Sta->szChannel, Sta->szNetID );
#endif
   }
   else if ( iType == 2 )   /* Multi-station regional alarm */
      sprintf( line,    "%5d %5d %5d     1     0 %s REGIONAL ALARM",
           (int) ucEWHTypeAlarm, (int) ucMyModID, (int) ucEWHMyInstID,
           pszAlarmString );
   else if ( iType == 3 )   /* Strong motion LP alarm */
   {
      sprintf( line, "%5d %5d %5d %5d     0 %s %s LP ALARM",
           (int) ucEWHTypeAlarm, (int) ucMyModID, (int) ucEWHMyInstID,
           Sta->iAlarm, Sta->szStation, Sta->szChannel );
/* Send the LP alarm to the earthvu from here. */
#ifdef _WINNT
      SendAlarm2Earthvu( Sta->szStation, Sta->szChannel, Sta->szNetID );
#endif
   }
   else if ( iType == 4 )   /* Location alarm */
      sprintf( line,    "%5d %5d %5d     0 %5d %s",
           (int) ucEWHTypeAlarm, (int) ucMyModID, (int) ucEWHMyInstID,
           iRespond, pszAlarmString );
   else if ( iType == 5 )   /* No data alarm */
      sprintf( line,    "%5d %5d %5d     1     0 Earthworm Data Outage",
           (int) ucEWHTypeAlarm, (int) ucMyModID, (int) ucEWHMyInstID );
   lineLen = strlen( line ) + 1;

/* log alarm
   *********/
   logit( "t", "%s\n", line );

/* Send the alarm to the output ring
   *********************************/
   logo.type   = ucEWHTypeAlarm;
   logo.mod    = ucMyModID;
   logo.instid = ucEWHMyInstID;

   if ( tport_putmsg( &siAlarmRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "t", "Error sending alarm to output ring.\n" );
}

     /**************************************************************
      *                         ReportHypo()                       *
      *                                                            *
      *   Report type H712K and type TWCLOC messages.              *
      **************************************************************/

void ReportHypo( char *pszPageMsg, char *pszTWCMsg, unsigned char ucMyModID,
                 SHM_INFO siHRegion, unsigned char ucEWHTypeHypoTWC,
                 unsigned char ucEWHTypeHypoH71, unsigned char ucEWHMyInstID,
                 int iLoc, int iNumPs )
{
   MSG_LOGO     logo;      /* Logo of message to send to the output ring */
   int          lineLen;

/* log hypo
   ********/
   if ( iLoc == 1 ) logit( "et", "HYPO - %s\n", pszPageMsg );
   logit( "et", "HYPO - %s\n", pszTWCMsg );

/* Send the hypo (TWC format) to the output ring
   *********************************************/
   logo.type   = ucEWHTypeHypoTWC;
   logo.mod    = ucMyModID;
   logo.instid = ucEWHMyInstID;
   lineLen     = strlen( pszTWCMsg ) + 1;

   if ( tport_putmsg( &siHRegion, &logo, lineLen, pszTWCMsg ) !=PUT_OK )
      logit( "t", "Error sending TWChypo to output ring - TWC.\n" );
	  
/* Send the hypo (H71 format) to the output ring
   *********************************************/
   if ( iLoc == 1 && iNumPs >= 8 )
   {
      logo.type   = ucEWHTypeHypoH71;
      lineLen     = strlen( pszPageMsg );

      if ( tport_putmsg( &siHRegion, &logo, lineLen, pszPageMsg ) !=
           PUT_OK )
         logit( "t", "Error sending H71hypo to output ring - H71.\n" );
   }
}

     /**************************************************************
      *                         ReportPick()                       *
      *                                                            *
      *                 Fill in TYPE_PICKTWC format                *
      **************************************************************/

void ReportPick( STATION *Sta, unsigned char ucMyModID,
                 SHM_INFO siPRegion, unsigned char ucEWHTypePickTWC,
                 unsigned char ucEWHMyInstID, int iType )
{
   MSG_LOGO     logo;      /* Logo of message to send to the output ring */
   int          lineLen;
   char         line[512]; /* TYPE_PICKTWC format message */
   long         lTemp;

/* Create TYPE_PICKTWC message
   ***************************/
   if ( iType == 1 )        /* Data from lpproc */
   {
      sprintf( line, "%d %d %d %s %s %s %s %ld %d %lf %c %s %lf %lf %lf %lf "
                     "%lf %lf %lf %lf %lf %lE %lf %d %lf %lf",
       (int) ucEWHTypePickTWC, (int) ucMyModID, (int) ucEWHMyInstID,
       Sta->szStation, Sta->szChannel, Sta->szNetID, Sta->szLocation,
       Sta->lPickIndex, Sta->iUseMe, Sta->dPTime, Sta->cFirstMotion,
       Sta->szPhase, Sta->dMbAmpGM, Sta->dMbPer, Sta->dMbTime, Sta->dMlAmpGM,
       Sta->dMlPer, Sta->dMlTime, Sta->dMSAmpGM, Sta->dMSPer, Sta->dMSTime,
       Sta->dMwpIntDisp, Sta->dMwpTime, 1, Sta->dPStrength, Sta->dFreq );
   }
   if ( iType == 2 )        /* Data from develo */
   {
      sprintf( line, "%d %d %d %s %s %s %s %ld %d %lf %c %s %lf %lf %lf %lf "
                     "%lf %lf %lf %lf %lf %lE %lf %s %lf %lf",
       (int) ucEWHTypePickTWC, (int) ucMyModID, (int) ucEWHMyInstID,
       Sta->szStation, Sta->szChannel, Sta->szNetID, Sta->szLocation,
       Sta->lPickIndex, Sta->iUseMe, Sta->dPTime, Sta->cFirstMotion,
       Sta->szPhase, Sta->dMbAmpGM, Sta->dMbPer, Sta->dMbTime, Sta->dMlAmpGM,
       Sta->dMlPer, Sta->dMlTime, 0., 0., 0., Sta->dMwpIntDisp, Sta->dMwpTime,
       Sta->szHypoID, Sta->dPStrength, Sta->dFreq );
   }
   else if ( iType == 3 )   /* Data from hypo_display */
   {
      sprintf( line, "%d %d %d %s %s %s %s %ld %d %lf %c %s %lf %lf %lf %lf "
                     "%lf %lf %lf %lf %lf %lE %lf %s %lf %lf",
       (int) ucEWHTypePickTWC, (int) ucMyModID, (int) ucEWHMyInstID,
       Sta->szStation, Sta->szChannel, Sta->szNetID, Sta->szLocation,
       Sta->lPickIndex, Sta->iUseMe, Sta->dPTime, Sta->cFirstMotion,
       Sta->szPhase, Sta->dMbAmpGM, Sta->dMbPer, Sta->dMbTime, Sta->dMlAmpGM,
       Sta->dMlPer, Sta->dMlTime, Sta->dMSAmpGM, Sta->dMSPer, Sta->dMSTime,
       Sta->dMwpIntDisp, Sta->dMwpTime, Sta->szHypoID, Sta->dPStrength,
       Sta->dFreq );
   }
   if ( iType == 4 )        /* Data from pick_wcatwc */
   {
      sprintf( line, "%d %d %d %s %s %s %s %ld %d %lf %c %s %lf %lf %lf %lf "
                     "%lf %lf %lf %lf %lf %lE %lf %d %lf %lf",
       (int) ucEWHTypePickTWC, (int) ucMyModID, (int) ucEWHMyInstID,
       Sta->szStation, Sta->szChannel, Sta->szNetID, Sta->szLocation,
       Sta->lPickIndex, 1, Sta->dTrigTime-Sta->dTimeCorrection,
       Sta->cFirstMotion, Sta->szPhase, Sta->dMbAmpGM, (double) Sta->lMbPer/10.,
       Sta->dMbTime, Sta->dMlAmpGM, (double) Sta->lMlPer/10., Sta->dMlTime,
       0., 0., 0., Sta->dMwpIntDisp, Sta->dMwpTime, -1, Sta->dPStrength,
       Sta->dFreq );

/* Now let us send it to the Earthvu for display */
/* ********************************************* */
#ifdef _WINNT
      SendPPick2Earthvu( line );
#endif
   }
   lineLen = strlen( line ) + 1;

/* log pick
   ********/
   lTemp = (long) (Sta->dPTime);
   if ( lTemp < 0 )
   {
      logit( "", "Bad pick time lTemp=%ld on %s\n", lTemp, Sta->szStation );
      return;
   }
   if ( iType == 1 )        /* Data from lpproc */
      logit( "et", "%s %s %lf, MSamp=%lf, MSPer=%lf, MSTime=%lf\n",
       Sta->szStation, asctime( TWCgmtime( lTemp ) ), Sta->dPTime, Sta->dMSAmpGM,
       Sta->dMSPer, Sta->dMSTime );
   else if ( iType == 2 || iType == 3)   /* Data from develo or hypo_display */
      logit( "et", "%s, P-time=%s %c P=%lf, Mbamp=%lf, MbPer=%lf, MbTime=%lf,"
       " Mlamp=%lf, MlPer=%lf, MlTime=%lf, Mwptime=%lf, MwpID=%le\n",
       Sta->szStation, asctime( TWCgmtime( lTemp ) ), Sta->cFirstMotion, Sta->dPTime,
       Sta->dMbAmpGM, Sta->dMbPer, Sta->dMbTime, Sta->dMlAmpGM, Sta->dMlPer, Sta->dMlTime,
       Sta->dMwpTime, Sta->dMwpIntDisp );
   else if ( iType == 4 )   /* Data from pick_wcatwc */
   {
      lTemp = (long) (Sta->dTrigTime-Sta->dTimeCorrection);    
      logit( "et", "%s, %c, %lf, %lf P-time=%s, %lf\n", Sta->szStation,
       Sta->cFirstMotion, Sta->dFreq, Sta->dPStrength,
       asctime( TWCgmtime( lTemp ) ), Sta->dTrigTime-Sta->dTimeCorrection );
   }

/* Send the pick to the output ring
   ********************************/
   logo.type   = ucEWHTypePickTWC;
   logo.mod    = ucMyModID;
   logo.instid = ucEWHMyInstID;

   if ( tport_putmsg( &siPRegion, &logo, lineLen, line ) != PUT_OK )
      logit( "t", "Error sending pick to output ring.\n" );
}

#ifdef _WINNT
/* **************************************************************
*                      SendAlarm2Earthvu()
*
*  INPUT:  name -- Station Name.
*          channel -- Station Channel.
*          netid -- Station Net ID.
*
*  Send a message to earthvu that an LP or SP alarm went off.
*
***************************************************************** */
void SendAlarm2Earthvu( char *name, char *channel, char *netid )
{
   TCHAR   chReadBuf[512]; 
   BOOL    fSuccess = FALSE; 
   DWORD   cbRead; 
   char    lpszPipename[64];
   HANDLE  hPipe=NULL;

   strcpy( lpszPipename, "\\\\.\\pipe\\earthvu.Seiemos" );
   hPipe = CreateFile( lpszPipename, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING,0,NULL );

   if ( hPipe == INVALID_HANDLE_VALUE )
   {
      logit("", "Could not open pipe: %s \n", lpszPipename );
      return;
   }
   sprintf( chReadBuf, " %s %s %s ", name, channel, netid );

   fSuccess = WriteFile( hPipe, chReadBuf, strlen( chReadBuf ), &cbRead, NULL );
   if ( !fSuccess )
   {
      logit("", "Could not open pipe: %s \n", lpszPipename );
      CloseHandle( hPipe );
      return;
   }
   CloseHandle( hPipe );
}

/* **************************************************************
*                      SendPPick2Earthvu()
*
*  INPUT:  Pick information
*
*
*  Send a pick message to earthvu.
*
*
***************************************************************** */
void SendPPick2Earthvu( char *line )
{
   TCHAR   chReadBuf[512]; 
   BOOL    fSuccess = FALSE; 
   DWORD   cbRead; 
   char    lpszPipename[64];
   HANDLE  hPipe=NULL;

   strcpy( lpszPipename, "\\\\.\\pipe\\earthvu.pPick" );
   hPipe = CreateFile( lpszPipename, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING,0,NULL );

   if ( hPipe == INVALID_HANDLE_VALUE )
   {
      logit("", "Could not open pipe: %s \n", lpszPipename );
      return;
   }
   sprintf( chReadBuf, "%s", line );

   fSuccess = WriteFile( hPipe, chReadBuf, strlen( chReadBuf ), &cbRead, NULL );
   if ( !fSuccess )
   {
      logit("", "Could not open pipe: %s \n", lpszPipename );
      CloseHandle( hPipe );
      return;
   }
   CloseHandle( hPipe );
}
#endif

