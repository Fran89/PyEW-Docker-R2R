
  /**********************************************************************
   *                             display.c                              *
   *                                                                    *
   * This set of functions draws traces, time lines, stations names,    *
   * etc. to a window.  Windows API calls are used throughout.          *
   *                                                                    *
   **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <kom.h>
#include <transport.h>
#include <earthworm.h>
#include "earlybirdlib.h"

#ifdef _WINNT
 /********************************************************************
  *                  DisplayAlignPTime()                             *
  *                                                                  *
  * This function displays a blue line at the expected Ptime of      *
  * the signal.                                                      *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to write to                    *
  *  cxScreen          Horizontal pixel height of window             *
  *  cyScreen          Vertical pixel height of window               *
  *  dScreenTime       Number of seconds to show on screen           *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *                                                                  *
  ********************************************************************/
  
void DisplayAlignPTime( HDC hdc, int cxScreen, int cyScreen, double dScreenTime,
                        int iChanIDOffset, double dOffset )
{
   int     iChanIDOffset2;       /* Used to change trace offset from left of 
                                    screen necessary for eb4 */
   HPEN    hNPen, hOPen;         /* Pen handles */
   HFONT   hOFont, hNFont;       /* Font handles */
   POINT   pt[2];

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Create green pen and fonts */
   hNPen = CreatePen( PS_SOLID, 1, RGB( 0, 255, 0 ) );
   hOPen = SelectObject( hdc, hNPen );
   SetTextColor (hdc, RGB( 0, 0, 255 ) );                             /* Blue */
/* Change font here */
   hNFont = CreateFont( cxScreen/56, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial" );
   hOFont = SelectObject( hdc, hNFont );

/* Display expected P line */
   pt[0].x = iChanIDOffset2 + (long) (((3.*(dScreenTime/8.)-dOffset) *
             (double) (cxScreen-iChanIDOffset2)) / dScreenTime);
   pt[1].x = pt[0].x;
   pt[0].y = 0;
   pt[1].y = cyScreen;
   MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
   LineTo(   hdc, pt[1].x, pt[1].y );
   TextOut(  hdc, pt[1].x+cxScreen/100, pt[1].y-cyScreen/40, "Expected P-time", 
             (int)strlen( "Expected P-time" ) );

   DeleteObject( SelectObject( hdc, hOPen ) );             /* Reset Pen color */
   DeleteObject( SelectObject( hdc, hOFont ) );
}

 /********************************************************************
  *                DisplayBodyPhase()                                *
  *                                                                  *
  * This function displays the arrival time of chosen body           *
  * wave phases.  The phase arrival times are computed using         *
  * NEIC's Tau/P program (through GetPhase).                         *
  *                                                                  *
  * April, 2012: Added ability to align display on P-times           *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to write to                    *
  *  Sta               Array of STATION structures                   *
  *  iNumStas          Number of stations in Sta                     *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  dScreenTime       Number of seconds to show on screen           *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTraces        Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  iAlign            1->Align screen output on expected P time     *
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *                                                                  *
  ********************************************************************/      

void DisplayBodyPhase( HDC hdc, STATION Sta[], int iNumStas, 
                       int iScreenToDisplay, double dScreenTime, 
                       long lTitleFHt, long lTitleFWd, int cxScreen, 
                       int cyScreen, int iNumTraces, int iVScrollOffset, 
                       int iTitleOffset, int iChanIDOffset, double dScreenEnd,
                       int iAlign, double dOffset )
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   int     iChanIDOffset2;       /* Used to change trace offset from left of 
                                    screen necessary for eb4 */
   int     iPKP;                 /* 1 if PKP branch or depth branch */
   int     iPKKP;                /* 1 if PKKP branch or depth branch */
   int     iPKS;                 /* 1 if PKS branch or depth branch */
   int     iPPprime;             /* 1 if P'P' branch or depth branch */
   int     iSKP;                 /* 1 if SKP branch or depth branch */
   int     iSKKP;                /* 1 if SKKP branch or depth branch */
   int     iSKS;                 /* 1 if SKS branch or depth branch */
   HPEN    hNPen, hOPen;         /* Pen handles */
   int     i, j, iCnt, k;
   long    lOffsetY;             /* Midpoint to draw line */
   POINT   pt[2];
   char    *ptr;

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Create font and pens */
   hNPen = CreatePen( PS_SOLID, 2, RGB( 0, 127, 0 ) );/* Create green line */
   hOPen = SelectObject( hdc, hNPen );
   
   iCnt = 0;
/* Place green line at phase arrival time for each station */
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] )
      {
         for ( j=0; j<Sta[i].iNumPhases; j++ )
/* Filter out unwanted phases */
         if ( strstr( Sta[i].szPhaseIasp[j], "PKiKP" ) == NULL )
         {
/* See if this phase is already plotted */
            for ( k=0; k<j; k++ )
               if ( !strcmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k] ) )
                  goto ForEnd;
/* Only show the PKPdf branch (this is the PKP or PKIKP).
   It comes in first almost always */
            iPKP = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "PKP" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 3 ) )
                     goto ForEnd;
               iPKP = 1;
            }
/* Only show the first SKS branch (this is the SKS or SKIKS). It is either ac
   branch or df */
            iSKS = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "SKS" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 3 ) )
                     goto ForEnd;
               iSKS = 1;
            }
/* Only show the first SKP branch */
            iSKP = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "SKP" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 3 ) )
                     goto ForEnd;
               iSKP = 1;
            }
/* Only show the first PKS branch */
            iPKS = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "PKS" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 3 ) )
                     goto ForEnd;
               iPKS = 1;
            }
/* Only show the first PKKP branch */
            iPKKP = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "PKKP" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 4 ) )
                     goto ForEnd;
               iPKKP = 1;
            }
/* Only show the first SKKP branch */
            iSKKP = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "SKKP" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 4 ) )
                     goto ForEnd;
               iSKKP = 1;
            }
/* Only show the first P'P' branch */
            iPPprime = 0;
            if ( (ptr = strstr( Sta[i].szPhaseIasp[j], "P'P'" )) != NULL )
            {
               for ( k=0; k<j; k++ )
                  if ( !strncmp( Sta[i].szPhaseIasp[j], Sta[i].szPhaseIasp[k],
                        ptr - Sta[i].szPhaseIasp[j] + 4 ) )
                     goto ForEnd;
               iPPprime = 1;
            }
            if ( iAlign == 1 )    /* Align screen on P */ 
               pt[0].x = iChanIDOffset2 + (long) (((Sta[i].dPhaseTimeIasp[j] - 
                         Sta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                         * (double) (cxScreen-iChanIDOffset2)) / dScreenTime);
            else                  /* Align screen on time */
               pt[0].x = iChanIDOffset2 + (long) (((Sta[i].dPhaseTimeIasp[j] - 
                         dScreenStart) * (double) (cxScreen-iChanIDOffset2)) / 
                         dScreenTime);
            pt[1].x = pt[0].x;
            lOffsetY = iCnt*(cyScreen-iTitleOffset)/iNumTraces +
                       iVScrollOffset + iTitleOffset;
		
/* Mark pick on screen, if it really shows up on screen */			
            if ( pt[1].x >= iChanIDOffset2 && pt[1].x <= cxScreen )
            {
               pt[0].y = lOffsetY - lTitleFHt/4;
               pt[1].y = lOffsetY + lTitleFHt/4;
               MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
               LineTo ( hdc, pt[1].x, pt[1].y );
               if ( iPKP  == 1 || iSKS  == 1 || iSKP     == 1 || iPKS == 1 ||
                    iPKKP == 1 || iSKKP == 1 || iPPprime == 1 )
                  TextOut( hdc, pt[0].x, pt[1].y, Sta[i].szPhaseIasp[j], 
                           (int)strlen (Sta[i].szPhaseIasp[j])-2 );
               else
                  TextOut( hdc, pt[0].x, pt[1].y, Sta[i].szPhaseIasp[j], 
                           (int)strlen( Sta[i].szPhaseIasp[j] ) );
            }
            ForEnd:;
         }
         iCnt++;
      }
   DeleteObject( SelectObject( hdc, hOPen ) );             /* Reset Pen color */
}

 /********************************************************************
  *                  DisplayChannel()                                *
  *                                                                  *
  * This function displays the channel next to                       *
  *  the station name next to its trace in the window.               *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iLPScreen         Screen on which LP data is shown (from 0)     *
  *                                                                  *
  ********************************************************************/
  
void DisplayChannel( HDC hdc, STATION Sta[], int iNumStas,
                     int iScreenToDisplay, long lTitleFHt, long lTitleFWd,
                     int cyScreen, int iNumTracePerScreen,
                     int iVScrollOffset, int iTitleOffset, int iLPScreen )
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i, iFont_Height, iFont_Width;
   int     iCnt;                 /* Channel names counter */
   long    lOffset;              /* Offset to center of trace from top */
   POINT   pt;                   /* Screen location for trace name */

/* Compute font size using a ramp function; this change was necessary for
   the increase in pixels on eb4;  dln 10/11/06 */
   iFont_Height = 40*lTitleFHt/iNumTracePerScreen;
   if ( iFont_Height < 6 ) iFont_Height = 6;
   if ( iFont_Height > 30 ) iFont_Height = 30;
   iFont_Width = lTitleFWd;
   if ( iFont_Width > 8 ) iFont_Width = 8;
   hNFont = CreateFont( iFont_Height, iFont_Width, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
             "Arial" );		
   hOFont = SelectObject( hdc, hNFont );

/* Write Title */
   SetTextColor( hdc, RGB( 0, 0, 0 ) );
   pt.x = 9 * iFont_Width; 
   pt.y = lTitleFHt / 10;
   if ( iVScrollOffset == 0 )
      TextOut( hdc, pt.x, pt.y, "CHN", (int)strlen( "CHN" ) );
   
/* Loop through all stations and see which should be shown now */   
   SetTextColor( hdc, RGB( 255, 0, 0 ) );
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] ||
           iScreenToDisplay == iLPScreen )
      {
         lOffset = iCnt*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset;
         pt.y = lOffset - 3*lTitleFHt/8;
         TextOut( hdc, pt.x, pt.y, Sta[i].szChannel,
                  (int)strlen( Sta[i].szChannel ) );
         iCnt++;
      }
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                 DisplayChannelID()                               *
  *                                                                  *
  * This function displays the station name next to its trace in the *
  * window.                                                          *
  *                                                                  *
  * June, 2008: Combined with ANALZYE.                               *
  * September, 2004: Color name green if data ahead of present time. *
  * March, 2002: Color name blue if pager alarms on.                 *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iLPScreen         Screen on which LP data is shown (from 0)     *
  *                                                                  *
  ********************************************************************/
  
void DisplayChannelID( HDC hdc, STATION Sta[], int iNumStas,
                       int iScreenToDisplay, long lTitleFHt, long lTitleFWd,
                       int cxScreen, int cyScreen, int iNumTracePerScreen,
                       int iVScrollOffset, int iTitleOffset, int iLPScreen )
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i, iFont_Height, iFont_Width;
   int     iCnt;                 /* Channel names counter */
   long    lOffset;              /* Offset to center of trace from top */
   POINT   pt;                   /* Screen location for trace name */

/* Compute font size using a ramp function; this change was necessary for
   the increase in pixels on eb4;  dln 10/11/06 */
   iFont_Height = 40*lTitleFHt/iNumTracePerScreen;
   if ( iFont_Height < 6 )  iFont_Height = 6;
   if ( iFont_Height > 30 ) iFont_Height = 30;
   iFont_Width = lTitleFWd;
   if ( iFont_Width > 8 ) iFont_Width = 8;
   hNFont = CreateFont( iFont_Height, iFont_Width, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
             "Arial" );		
   hOFont = SelectObject( hdc, hNFont );

/* Write Title */
   SetTextColor( hdc, RGB( 0, 0, 0 ) );
   pt.x = cxScreen / 500;
   pt.y = lTitleFHt / 10;
   if ( iVScrollOffset == 0 )
      TextOut( hdc, pt.x, pt.y, "STN", (int)strlen( "STN" ) );

/* Loop through all stations and see which should be shown now */   
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] ||
           iScreenToDisplay == iLPScreen )
      {
         if      ( Sta[i].iAhead > 0 ) SetTextColor( hdc, RGB( 0, 128, 0 ) );
         else if ( Sta[i].iAlarm > 0 ) SetTextColor( hdc, RGB( 255, 0, 0 ) );
         else                          SetTextColor( hdc, RGB( 0, 0, 128 ) );
         lOffset = iCnt*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset;
         pt.y = lOffset - 3*lTitleFHt/8;
         TextOut( hdc, pt.x, pt.y, Sta[i].szStation, (int)strlen( Sta[i].szStation ) );
         iCnt++;
      }
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                  DisplayDCOffset()                               *
  *                                                                  *
  * This function displays the average DC offset in counts next to   *
  *  the station name next to its trace in the window.               *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iLPScreen         Screen on which LP data is shown (from 0)     *
  *                                                                  *
  ********************************************************************/
  
void DisplayDCOffset( HDC hdc, STATION Sta[], int iNumStas,
                      int iScreenToDisplay, long lTitleFHt, long lTitleFWd,
                      int cyScreen, int iNumTracePerScreen,
                      int iVScrollOffset, int iTitleOffset, int iLPScreen )
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i, iFont_Height, iFont_Width;
   int     iCnt;                 /* Channel names counter */
   long    lOffset;              /* Offset to center of trace from top */
   POINT   pt;                   /* Screen location for trace name */
   char    szTemp[32];           /* DC Offset in counts */

/* Compute font size using a ramp function; this change was necessary for
   the increase in pixels on eb4;  dln 10/11/06 */
   iFont_Height = 40*lTitleFHt/iNumTracePerScreen;
   if ( iFont_Height < 6 ) iFont_Height = 6;
   if ( iFont_Height > 30 ) iFont_Height = 30;
   iFont_Width = lTitleFWd;
   if ( iFont_Width > 8 ) iFont_Width = 8;
   hNFont = CreateFont( iFont_Height, iFont_Width, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
             "Arial" );		
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 0, 0, 0 ) );

/* Write Title */
   pt.x = 9 * iFont_Width; 
   pt.y = lTitleFHt / 10;
   if ( iVScrollOffset == 0 )
      TextOut( hdc, pt.x, pt.y, "DC", (int)strlen( "DC" ) );
   
/* Loop through all stations and see which should be shown now */   
   pt.x = 9*iFont_Width; 
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] ||
           iScreenToDisplay == iLPScreen )
      {
         lOffset = iCnt*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset;
         pt.y = lOffset - 3*lTitleFHt/8;
         itoaX( (int) fabs( Sta[i].dAveLDCRaw ), szTemp );
         TextOut( hdc, pt.x, pt.y, szTemp, (int)strlen( szTemp ) );
         iCnt++;
      }
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                  DisplayEpiDist()                                *
  *                                                                  *
  * This function displays the epicentral distance in degrees next to*
  *  the station name.                                               *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iLPScreen         Screen on which LP data is shown (from 0)     *
  *                                                                  *
  ********************************************************************/
  
void DisplayEpiDist( HDC hdc, STATION Sta[], int iNumStas,
                     int iScreenToDisplay, long lTitleFHt, long lTitleFWd,
                     int cyScreen, int iNumTracePerScreen,
                     int iVScrollOffset, int iTitleOffset, int iLPScreen ) 
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i, iFont_Height, iFont_Width;
   int     iCnt;                 /* Channel names counter */
   long    lOffset;              /* Offset to center of trace from top */
   POINT   pt;                   /* Screen location for trace name */
   char    szTemp[32];           /* DC Offset in counts */

/* Compute font size using a ramp function; this change was necessary for
   the increase in pixels on eb4;  dln 10/11/06 */
   iFont_Height = 40*lTitleFHt/iNumTracePerScreen;
   if ( iFont_Height < 6 ) iFont_Height = 6;
   if ( iFont_Height > 30 ) iFont_Height = 30;
   iFont_Width = lTitleFWd;
   if ( iFont_Width > 8 ) iFont_Width = 8;
   hNFont = CreateFont( iFont_Height, iFont_Width, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
             "Arial" );		
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 0, 0, 0 ) );

/* Write Title */
   pt.x = 9 * iFont_Width; 
   pt.y = lTitleFHt / 10;
   if ( iVScrollOffset == 0 )
      TextOut( hdc, pt.x, pt.y, "DIST.", (int)strlen( "DIST." ) );
   
/* Loop through all stations and see which should be shown now */   
   pt.x = 9*iFont_Width; 
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] ||
           iScreenToDisplay == iLPScreen )
      {
         lOffset = iCnt*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset;
         pt.y = lOffset - 3*lTitleFHt/8;
         _gcvt( Sta[i].dDelta, 4, szTemp);
         TextOut( hdc, pt.x, pt.y, szTemp, (int)strlen( szTemp ) );
         iCnt++;
      }
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                     DisplayKOPs()                                *
  *                                                                  *
  * This function displays an X when traces have been removed from   *
  * further locations.                                               *
  *                                                                  *
  * June, 2008: Combined with ANALZYE.                               *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of screen display information per trace *
  *  iNumStas          Number of stations in Sta and Trace arrays    *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  cyScreen          Vertical pixel height of window               *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *                                                                  *
  ********************************************************************/
  
void DisplayKOPs( HDC hdc, STATION Sta[], int iNumStas, int iScreenToDisplay,
                  int cyScreen, int iVScrollOffset, int iTitleOffset, 
                  long lTitleFHt, long lTitleFWd, int iNumTracePerScreen )
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i, iCnt;              /* Counters */
   int     iFont_Height, iFont_Width; /* height and width of X */
   POINT   pt;                   /* Screen location for X */

/* Compute font size using a ramp function; this change was necessary for
   the increase in pixels on eb4;  dln 10/11/06 */

   iFont_Height = 40*lTitleFHt/iNumTracePerScreen;
   if ( iFont_Height < 6 ) iFont_Height = 6;
   if ( iFont_Height > 30 ) iFont_Height = 30;
   iFont_Width = lTitleFWd;
   if ( iFont_Width > 8 )
   iFont_Width = 8;
   hNFont = CreateFont( iFont_Height, iFont_Width*2, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN,
             "Times New Roman"  );		
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 0, 0, 128 ) );         /* X's in Blue */
   iCnt = 0; 

/* Show all X's */
   pt.x = 6*iFont_Width; 
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] )
      {
         if ( Sta[i].iUseMe < 0 )
         {
            pt.y = iCnt*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset - lTitleFHt*1/2;
            TextOut( hdc, pt.x, pt.y, "X", 1 );
         }
         iCnt++;
      }      
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                      DisplayMagCycle()                           *
  *                                                                  *
  * This function draws a box around the cycle used for the mb, Ml,  *
  * or Ms magnitude computation. The box is a full                   *
  * cycle but the Mb, Ml are computed on the half cycles so          *
  * there may be some error in the box display.                      *
  *                                                                  *
  * April, 2012: Added ability to align display on P-times           *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  iNumStas          Number of stations in Sta array               *
  *  iNumTrace         Number of traces shown per screen             *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       # seconds to show on screen                   *
  *  lTitleFHt         Title font height                             *
  *  pSta              Array of Station structures                   *
  *  iLP               1->LP Screenl 0->SP Screen                    *
  *  iAlign            1->Align screen output on expected P time     *
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *                                                                  *
  ********************************************************************/
  
void DisplayMagCycle( HDC hdc, int iNumStas, int iNumTrace, int cxScreen,
                      int cyScreen, int iVScrollOffset, int iTitleOffset,
                      int iChanIDOffset, double dScreenEnd, double dScreenTime,
                      long lTitleFHt, STATION pSta[], int iLP, 
                      int iScreenToDisplay, int iAlign, double dOffset ) 
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   HPEN    hBPen, hOPen;         /* Pen handles */
   int     i, iCnt;              /* Counters */
   long    lOffsetY;             /* Dist to skip on top of screen */
   POINT   pt[2];

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Create font and pens */
   hBPen = CreatePen( PS_SOLID, 2, RGB( 0, 0, 127 ) );   /* Lines Blue */
   hOPen = SelectObject( hdc, hBPen );

/* Show which cycle Ms was computed on */   
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( pSta[i].iStationDisp[iScreenToDisplay] )
      {
         lOffsetY = iCnt*(cyScreen-iTitleOffset)/iNumTrace +
                    iVScrollOffset + iTitleOffset;
         if ( iLP == 1 )          /* Show Ms Cycle */
         {
            if ( iAlign == 1 )    /* Align screen on P */ 
               pt[0].x = iChanIDOffset + (long) (((pSta[i].dMSTime - 
                         pSta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                         * (double) (cxScreen-iChanIDOffset)) / dScreenTime);
            else                  /* Align screen on time */
               pt[0].x = iChanIDOffset + (long) (((pSta[i].dMSTime - 
                         dScreenStart) * (double) (cxScreen-iChanIDOffset)) / 
                         dScreenTime);
            pt[1].x = pt[0].x;
         }
         else if ( iLP == 0 )     /* Show Mb Cycle */
         {
            if ( iAlign == 1 )    /* Align screen on P */ 
               pt[0].x = iChanIDOffset + (long) (((pSta[i].dMbTime - 
                         pSta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                         * (double) (cxScreen-iChanIDOffset)) / dScreenTime);
            else                  /* Align screen on time */
               pt[0].x = iChanIDOffset + (long) (((pSta[i].dMbTime - 
                         dScreenStart) * (double) (cxScreen-iChanIDOffset)) / 
                         dScreenTime);
            pt[1].x = pt[0].x;
         }
		
/* Mark spot on screen, if it really shows up on screen */			
         if ( pt[1].x >= iChanIDOffset && pt[1].x <= cxScreen )
         {
            pt[0].y = lOffsetY - lTitleFHt/2;
            pt[1].y = lOffsetY + lTitleFHt/2;
            MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
            LineTo ( hdc, pt[1].x, pt[1].y );
 
/* Then make cross */	 
            pt[0].x -= lTitleFHt/2; 
            pt[1].x = pt[0].x + lTitleFHt;
            pt[0].y = lOffsetY;
            pt[1].y = lOffsetY;
            MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
            LineTo ( hdc, pt[1].x, pt[1].y );
         }

/* Draw second cross for Ml cycle if this is SP screen */
         if ( iLP == 0 )          /* Show Ml Cycle */
         {
            if ( iAlign == 1 )    /* Align screen on P */ 
               pt[0].x = iChanIDOffset + (long) (((pSta[i].dMlTime - 
                         pSta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                         * (double) (cxScreen-iChanIDOffset)) / dScreenTime);
            else                  /* Align screen on time */
               pt[0].x = iChanIDOffset + (long) (((pSta[i].dMlTime - 
                         dScreenStart) * (double) (cxScreen-iChanIDOffset)) / 
                         dScreenTime);
            pt[1].x = pt[0].x;
/* Mark spot on screen, if it really shows up on screen */			
            if ( pt[1].x >= iChanIDOffset && pt[1].x <= cxScreen )
            {
               pt[0].y = lOffsetY - lTitleFHt/2;
               pt[1].y = lOffsetY + lTitleFHt/2;
               MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
               LineTo ( hdc, pt[1].x, pt[1].y );
 
/* Then make cross */	 
               pt[0].x -= lTitleFHt/2; 
               pt[1].x = pt[0].x + lTitleFHt;
               pt[0].y = lOffsetY;
               pt[1].y = lOffsetY;
               MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
               LineTo ( hdc, pt[1].x, pt[1].y );
            }
         }
         iCnt++;
      }		
   DeleteObject( SelectObject( hdc, hOPen ) );   /* Reset Pen color */
}

 /********************************************************************
  *                     DisplayMwp()                                 *
  *                                                                  *
  * This function displays a line over the trace where Mwp           *
  * computation took place.                                          * 
  *                                                                  *
  * April, 2012: Added ability to align display on P-times           *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of screen display information per trace *
  *  iNumTrace         Number of traces to show on screen            *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       # seconds to show on screen                   *
  *  iAlign            1->Align screen output on expected P time     *
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *                                                                  *
  ********************************************************************/
   
void DisplayMwp( HDC hdc, STATION Sta[], int iNumTrace, int iNumStas,
                 int iScreenToDisplay, int cxScreen, int cyScreen,
                 int iVScrollOffset, int iTitleOffset, int iChanIDOffset,
                 double dScreenEnd, double dScreenTime, int iAlign, 
                 double dOffset ) 
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   HPEN    hOPen, hYPen;         /* Pen handles */
   int     i, iCnt;              /* Counters */
   POINT   pt[2];                /* Line start and end */
   int     iChanIDOffset2; /* used to change trace offset from left of screen
                              necessary for eb4 */

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Create pens */
   hYPen = CreatePen( PS_SOLID, 2, RGB( 100, 100, 0 ) );/*Create yellow lines */
   hOPen = SelectObject (hdc, hYPen);
   iCnt = 0; 

/* Show all Mwp time on each trace which Mwp was computed */
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] )
      {
	  
/* Draw Mwp yellow line over integrated part of P */
         if ( Sta[i].dMwpIntDisp > 0. )
         {
            if ( iAlign == 1 )    /* Align screen on P */ 
               pt[0].x = iChanIDOffset2 + (long) (((Sta[i].dPTime - 
                         Sta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                         * (double) (cxScreen-iChanIDOffset2)) / dScreenTime);
            else                  /* Align screen on time */
               pt[0].x = iChanIDOffset2 + (long) (((Sta[i].dPTime - 
                         dScreenStart) * (double) (cxScreen-iChanIDOffset2)) / 
                         dScreenTime);
            pt[1].x = pt[0].x + (long) (Sta[i].dMwpTime * 
                      (double) (cxScreen-iChanIDOffset2) / dScreenTime);
            pt[0].y = iCnt*(cyScreen-iTitleOffset)/iNumTrace +
                      iVScrollOffset + iTitleOffset;
            pt[1].y = pt[0].y;
            if ( pt[0].x < iChanIDOffset2 && pt[1].x > iChanIDOffset2 )
               pt[0].x = iChanIDOffset2;
            if ( pt[0].x < cxScreen && pt[1].x > cxScreen )
               pt[1].x = cxScreen;
            if ( pt[0].x >= iChanIDOffset2 && pt[1].x <= cxScreen )
            {
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               Polyline( hdc, pt, 2 );
            }
         }
         iCnt++;
      }   
   SelectObject( hdc, hOPen );               /* Reset Pen color */
   DeleteObject( hYPen );

}

 /********************************************************************
  *                  DisplayNetwork()                                *
  *                                                                  *
  * This function displays the Network ID next to                    *
  *  the station name next to its trace in the window.               *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cyScreen          Vertical pixel height of window               *
  *  iNumTracePerScreen Number of traces to put on screen            *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iLPScreen         Screen on which LP data is shown (from 0)     *
  *                                                                  *
  ********************************************************************/
  
void DisplayNetwork( HDC hdc, STATION Sta[], int iNumStas,
                     int iScreenToDisplay, long lTitleFHt, long lTitleFWd,
                     int cyScreen, int iNumTracePerScreen,
                     int iVScrollOffset, int iTitleOffset, int iLPScreen )
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     i, iFont_Height, iFont_Width;
   int     iCnt;                 /* Channel names counter */
   long    lOffset;              /* Offset to center of trace from top */
   POINT   pt;                   /* Screen location for trace name */

/* Compute font size using a ramp function; this change was necessary for
   the increase in pixels on eb4;  dln 10/11/06 */
   iFont_Height = 40*lTitleFHt/iNumTracePerScreen;
   if ( iFont_Height < 6 ) iFont_Height = 6;
   if ( iFont_Height > 30 ) iFont_Height = 30;
   iFont_Width = lTitleFWd;
   if ( iFont_Width > 8 ) iFont_Width = 8;
   hNFont = CreateFont( iFont_Height, iFont_Width, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
             "Arial" );		
   hOFont = SelectObject( hdc, hNFont );

/* Write Title */
   SetTextColor( hdc, RGB( 0, 0, 0 ) );
   pt.x = 9 * iFont_Width; 
   pt.y = lTitleFHt / 10;
   if ( iVScrollOffset == 0 )
      TextOut( hdc, pt.x, pt.y, "NET", (int)strlen( "NET" ) );
   
/* Loop through all stations and see which should be shown now */   
   SetTextColor( hdc, RGB( 255, 0, 0 ) );
   pt.x = 9*iFont_Width; 
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] ||
           iScreenToDisplay == iLPScreen )
      {
         lOffset = iCnt*(cyScreen-iTitleOffset)/iNumTracePerScreen +
                   iVScrollOffset + iTitleOffset;
         pt.y = lOffset - 3*lTitleFHt/8;
         TextOut( hdc, pt.x, pt.y, Sta[i].szNetID,
                  (int)strlen( Sta[i].szNetID ) );
         iCnt++;
      }            
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}                                      

 /********************************************************************
  *                  DisplayOriginTime()                             *
  *                                                                  *
  * This function displays a blue line at the Origin time of         *
  * the earthquake.                                                  *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to write to                    *
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       Number of seconds to show on screen           *
  *  cxScreen          Horizontal pixel height of window             *
  *  cyScreen          Vertical pixel height of window               *
  *  pszFile           Dummy File name                               *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *                                                                  *
  ********************************************************************/
  
void DisplayOriginTime( HDC hdc, double dScreenEnd, double dScreenTime,
                        int cxScreen, int cyScreen, HYPO *pHypo, 
                        int iChanIDOffset )
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   int     iChanIDOffset2;       /* Used to change trace offset from left of 
                                    screen necessary for eb4 */
   HPEN    hNPen, hOPen;         /* Pen handles */
   HFONT   hOFont, hNFont;       /* Font handles */
   POINT   pt[2];

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Create Blue pen and fonts */
   hNPen = CreatePen( PS_SOLID, 2, RGB( 0, 0, 255 ) );
   hOPen = SelectObject( hdc, hNPen );
   SetTextColor (hdc, RGB( 0, 0, 255 ) );                             /* Blue */
/* Change font here */
   hNFont = CreateFont( cxScreen/56, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial" );
   hOFont = SelectObject( hdc, hNFont );

/* Is OriginTime on this screen? */
   if ( pHypo->dOriginTime >= dScreenStart &&
        pHypo->dOriginTime <= dScreenStart+dScreenTime )
   {
      pt[0].x = iChanIDOffset2 + (long) (((pHypo->dOriginTime-dScreenStart)
                * (double) (cxScreen - iChanIDOffset2)) / dScreenTime + 0.001);
      pt[1].x = pt[0].x;
      pt[0].y = 0;
      pt[1].y = cyScreen;
      MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
      LineTo(   hdc, pt[1].x, pt[1].y );
      TextOut(  hdc, pt[1].x+cxScreen/100, pt[1].y-cyScreen/40, "O-time", 6 );
   }
   DeleteObject( SelectObject( hdc, hOPen ) );             /* Reset Pen color */
   DeleteObject( SelectObject( hdc, hOFont ) );
}

 /********************************************************************
  *                     DisplayPPicks()                              *
  *                                                                  *
  * This function displays all P arrivals in the STATION array.      *
  *                                                                  *
  * April, 2012: Added ability to align display on P-times           *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of screen display information per trace *
  *  iNumTrace         Number of traces to show on screen            *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       # seconds to show on screen                   *
  *  lTitleFHt         Title font height                             *
  *  iAlign            1->Align screen output on expected P time     *
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *                                                                  *
  ********************************************************************/
  
void DisplayPPicks( HDC hdc, STATION Sta[], int iNumTrace, int iNumStas,
                    int iScreenToDisplay, int cxScreen, int cyScreen,
                    int iVScrollOffset, int iTitleOffset, int iChanIDOffset,
                    double dScreenEnd, double dScreenTime, long lTitleFHt, 
                    int iAlign, double dOffset )
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   HPEN    hRPen, hOPen;         /* Pen handles */
   int     i, iCnt;              /* Counters */
   long    lOffsetY;             /* Dist to skip on top of screen */
   POINT   pt[2];
   int     iChanIDOffset2; /* used to change trace offset from left of screen
                              necessary for eb4 */

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Create font and pens */
   hRPen = CreatePen( PS_SOLID, 2, RGB( 255, 0, 127 ) );         /* Lines Red */
   hOPen = SelectObject( hdc, hRPen );

/* Show picks if on screen */   
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( Sta[i].iStationDisp[iScreenToDisplay] )
      {
         if ( Sta[i].dPTime > 0. )
         {
            if ( iAlign == 1 )                           /* Align screen on P */ 
               pt[0].x = iChanIDOffset2 + (long) (((Sta[i].dPTime - 
                         Sta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                         * (double) (cxScreen-iChanIDOffset2)) / dScreenTime);
            else                                      /* Align screen on time */
               pt[0].x = iChanIDOffset2 + (long) (((Sta[i].dPTime-dScreenStart)
                         * (double) (cxScreen-iChanIDOffset2)) / dScreenTime);
            pt[1].x  = pt[0].x;
            lOffsetY = iCnt*(cyScreen-iTitleOffset)/iNumTrace +
                       iVScrollOffset + iTitleOffset;
			
/* Mark pick on screen, if it really shows up on screen */			
            if ( pt[1].x >= iChanIDOffset2 && pt[1].x <= cxScreen )
            {
               pt[0].y = lOffsetY - lTitleFHt/2;
               pt[1].y = lOffsetY + lTitleFHt/2;
               MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
               LineTo ( hdc, pt[1].x, pt[1].y );
            }
			
/* If pick is off screen, display arrow showing where to go. */
            if ( pt[1].x > cxScreen )                /* It is right of screen */
            {
               pt[0].y = lOffsetY;
               pt[0].x = 97*cxScreen/100;
               pt[1].y = lOffsetY;
               pt[1].x = 99*cxScreen/100;
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               LineTo( hdc, pt[1].x, pt[1].y );
               pt[0].y = lOffsetY - lTitleFHt/2;
               pt[0].x = 98*cxScreen/100;
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               LineTo( hdc, pt[1].x, pt[1].y );
               pt[0].y = lOffsetY + lTitleFHt/2;
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               LineTo( hdc, pt[1].x, pt[1].y );
            }
            if ( pt[1].x < iChanIDOffset2 )           /* It is left of screen */
            {
               pt[0].y = lOffsetY;
               pt[0].x = iChanIDOffset2 + 3*cxScreen/100;
               pt[1].y = lOffsetY;
               pt[1].x = iChanIDOffset2 + 1*cxScreen/100;
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               LineTo( hdc, pt[1].x, pt[1].y );
               pt[0].y = lOffsetY - lTitleFHt/2;
               pt[0].x = iChanIDOffset2 + 2*cxScreen/100;
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               LineTo( hdc, pt[1].x, pt[1].y );
               pt[0].y = lOffsetY + lTitleFHt/2;
               MoveToEx( hdc, pt[0].x, pt[0].y, NULL );
               LineTo( hdc, pt[1].x, pt[1].y );
            }
         }		
         iCnt++;
      }
   DeleteObject( SelectObject( hdc, hOPen ) );             /* Reset Pen color */
}

 /********************************************************************
  *                     DisplayRTimes()                              *
  *                                                                  *
  * This function displays Rayleigh wave arrival times.              *
  *                                                                  *
  * April, 2012: Added ability to align display on P-times           *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  iNumStas          Number of stations in Sta array               *
  *  iNumTrace         Number of traces shown per screen             *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       # seconds to show on screen                   *
  *  lTitleFHt         Title font height                             *
  *  pSta              Rayleigh wave arrival times in structure      *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  iAlign            1->Align screen output on expected P time     *
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *                                                                  *
  ********************************************************************/
  
void DisplayRTimes( HDC hdc, int iNumStas, int iNumTrace, int cxScreen,
                    int cyScreen, int iVScrollOffset, int iTitleOffset,
                    int iChanIDOffset, double dScreenEnd, double dScreenTime,
                    long lTitleFHt, STATION pSta[], int iScreenToDisplay,
                    int iAlign, double dOffset ) 
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   HPEN    hGPen, hOPen;         /* Pen handles */
   int     i, iCnt;              /* Counters */
   long    lOffsetY;             /* Dist to skip on top of screen */
   POINT   pt[2];

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Create font and pens */
   hGPen = CreatePen( PS_SOLID, 2, RGB( 0, 127, 0 ) );   /* Lines Green */
   hOPen = SelectObject( hdc, hGPen );

/* Show Rayleigh wave start times on screen */   
   iCnt = 0;
   for ( i=0; i<iNumStas; i++ )
      if ( pSta[i].iStationDisp[iScreenToDisplay] )
      {
         if ( iAlign == 1 )    /* Align screen on P */ 
            pt[0].x = iChanIDOffset + (long) (((pSta[i].dRStartTime - 
                      pSta[i].dPStartTime-dOffset+(3.*dScreenTime/8.))
                      * (double) (cxScreen-iChanIDOffset)) / dScreenTime);
         else                  /* Align screen on time */
            pt[0].x = iChanIDOffset + (long) (((pSta[i].dRStartTime - 
                      dScreenStart) * (double) (cxScreen-iChanIDOffset)) / 
                      dScreenTime);
         pt[1].x = pt[0].x;
         lOffsetY = iCnt*(cyScreen-iTitleOffset)/iNumTrace +
                    iVScrollOffset + iTitleOffset;
			
/* Mark pick on screen, if it really shows up on screen */			
         if ( pt[1].x >= iChanIDOffset && pt[1].x <= cxScreen )
         {
            pt[0].y = lOffsetY - lTitleFHt/4;
            pt[1].y = lOffsetY + lTitleFHt/4;
            MoveToEx ( hdc, pt[0].x, pt[0].y, NULL );
            LineTo ( hdc, pt[1].x, pt[1].y );
            TextOut( hdc, pt[0].x, pt[1].y, "R", 1 );
         }
         iCnt++;
      }		
   DeleteObject( SelectObject( hdc, hOPen ) );   /* Reset Pen color */
}

 /********************************************************************
  *                 DisplayTimeLines()                               *
  *                                                                  *
  * This function displays timing lines and times on the display.    *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       # seconds to show on screen                   *
  *  dInc              Increment to display time lines               *
  *  iTimes            1->Show UTC times on minute; 0-don't          *
  *                                                                  *
  ********************************************************************/
  
void DisplayTimeLines( HDC hdc, long lTitleFHt, long lTitleFWd,
                       int cxScreen, int cyScreen, int iChanIDOffset, 
                       double dScreenEnd, double dScreenTime, double dInc, 
                       int iTimes ) 
{
   double  dScreenStart;         /* 1/1/70 (sec) time at left of screen */
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   HPEN    hGPen, hRPen, hOPen;  /* Pen handles */
   long    lTime;                /* 1/1/70 time in seconds in dInc's */
   POINT   pt, pt2;              /* Screen location for outputs */
   char    szTemp[8], szBuffer[16]; /* Time rounded to minutes */
   static  struct  tm *tm;       /* time structure */
   int     iChanIDOffset2; /* used to change trace offset from left of screen
                              necessary for eb4 */

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Create font and pens */
   hNFont = CreateFont( lTitleFHt, lTitleFWd, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, "Elite" );
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 0, 0, 128 ) );                /* Color times blue */
   hGPen = CreatePen( PS_SOLID, 1, RGB( 0, 128, 0 ) );   /* Lines Green */
   hRPen = CreatePen( PS_SOLID, 1, RGB( 128, 0, 0 ) );   /* Lines Red */
   hOPen = SelectObject( hdc, hRPen );

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;
   
/* Get first time after dScreenStart which is an even increment of dInc */   
   if ( iTimes == 1 )
      lTime = (long) ((dScreenStart+dInc-0.001) / dInc) * (long) dInc;
   else
      lTime = (long) (dScreenStart);
   
/* Loop through dScreenTime, and draw time lines and times whereever needed */   
   while ( (double) lTime < dScreenEnd )
   {
      tm = TWCgmtime( lTime );
      if ( iTimes == 1 )
      {
         if ( tm->tm_sec == 0 )       /* Even minute; list time at top */
         {
            SelectObject( hdc, hRPen );
            strcpy( szBuffer, "\0" );
            itoaX( tm->tm_hour, szTemp );
            PadZeroes( 2, szTemp );
            strcpy( szBuffer, szTemp );                            
            strcat( szBuffer, " " );
            itoaX( tm->tm_min, szTemp );
            PadZeroes( 2, szTemp );
            strcat( szBuffer, szTemp );
            if ( dInc < 30. || tm->tm_min%5 == 0 )
            {                            
               strcat( szBuffer, "-" );
               itoaX( tm->tm_mon+1, szTemp );
               PadZeroes( 2, szTemp );
               strcat( szBuffer, szTemp );                            
               strcat( szBuffer, "/" );
               itoaX( tm->tm_mday, szTemp );
               PadZeroes( 2, szTemp );
               strcat( szBuffer, szTemp );                            
            }                            
            pt2.x = iChanIDOffset2 + (long) ((((double) lTime-dScreenStart)/
                    dScreenTime) * (double) (cxScreen-iChanIDOffset2)) -
                    (long) ((double) lTitleFWd*2.5);
            pt2.y = lTitleFHt/10;
            TextOut( hdc, pt2.x, pt2.y, szBuffer, (int)strlen( szBuffer ) );
         }
         else SelectObject( hdc, hGPen );
      }
      pt.x = iChanIDOffset2 + (long) ((((double) lTime-dScreenStart)/
             dScreenTime) * (double) (cxScreen-iChanIDOffset2));
      pt.y = 0;
      MoveToEx( hdc, pt.x, pt.y, NULL );
      pt.y = cyScreen;
      LineTo( hdc, pt.x, pt.y );
      lTime += (long) dInc;
   }   
   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
   SelectObject( hdc, hOPen );
   DeleteObject( hGPen );
   DeleteObject( hRPen );  
}

 /********************************************************************
  *                 DisplayTitle()                                   *
  *                                                                  *
  * This function displays earthquake parameters at the top of SWD.  *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  lTitleFHt         Title font height                             *
  *  lTitleFWd         Title font width                              *
  *  pHypo             Hypocenter structure                          *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *                                                                  *
  ********************************************************************/
  
void DisplayTitle( HDC hdc, long lTitleFHt, long lTitleFWd, HYPO *pHypo, 
                   int cxScreen, int cyScreen, int iChanIDOffset ) 
{
   HFONT   hOFont, hNFont;       /* Old and new font handles */
   int     iChanIDOffset2; /* used to change trace offset from left of screen
                              necessary for eb4 */
   POINT   pt;                   /* Screen location for outputs */
   char    szTemp[32],szTemp2[32];/* Output strings */

/* Setting a variable offset to accomodate the greater # pixels on eb4 */
   iChanIDOffset2 = 18*iChanIDOffset/140;
   if ( iChanIDOffset2 > 12 ) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11*iChanIDOffset2;

/* Create font and pens */
   hNFont = CreateFont( lTitleFHt, lTitleFWd, 
             0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial" );		
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 0, 0, 128 ) );                 /* Color times blue */

/* Output string to top of screen */
   sprintf( szTemp, "O-Time=%d/%d %s\n", pHypo->stOTime.wMonth, 
            pHypo->stOTime.wDay, pHypo->szOTimeRnd );          /* OTime first */
   pt.x = iChanIDOffset2;
   pt.y = lTitleFHt/10;
   TextOut( hdc, pt.x, pt.y, szTemp, ((int)strlen( szTemp )-1) );

   pt.x = iChanIDOffset2 + 6*(cxScreen-iChanIDOffset2)/40;    /* Lat/lon Next */
   TextOut( hdc, pt.x, pt.y, pHypo->szLat, (int)strlen( pHypo->szLat ) );
   pt.x = iChanIDOffset2 + 9*(cxScreen-iChanIDOffset2)/40;
   TextOut( hdc, pt.x, pt.y, pHypo->szLon, (int)strlen( pHypo->szLon ) );

   sprintf( szTemp2, "M%s = ", pHypo->szPMagType );         /* Then magnitude */
   _gcvt( pHypo->dPreferredMag, 2, szTemp );
   if ( pHypo->dPreferredMag-(int)pHypo->dPreferredMag <= 0.05 || 
        pHypo->dPreferredMag-(int)pHypo->dPreferredMag >= 0.95 )szTemp[2] = '0';
   strcat( szTemp2, szTemp );
   pt.x = iChanIDOffset2 + 13*(cxScreen-iChanIDOffset2)/40;
   TextOut( hdc, pt.x, pt.y, szTemp2, (int)strlen( szTemp2 ) );

   DeleteObject( SelectObject( hdc, hOFont ) );  /* Reset font */
}

 /********************************************************************
  *                     DisplayTraces()                              *
  *                                                                  *
  * This function displays seismic traces to the screen and clips    *
  * them if ClipIt is set.                                           *
  *                                                                  *
  * Aug., 2014: Modified Auto-scaling to provide better response     *
  * July, 2014: Added Acc./Disp notice at bottom of screen           *
  * Apr., 2014: Adjusted to reduce trace overlap                     *
  * Jan., 2012: Added option to highlight trace based on iFind index *
  * April, 2012: Added ability to align display on P-times           *
  * Nov., 2011: Added option for automatic scaling based on RMS noise*
  * May, 2011: Added option to individually unclip stations          *
  *                                                                  *
  * Arguments:                                                       *
  *  hdc               Device context to use                         *
  *  Sta               Array of all station data structures          *
  *  iNumTrace         Number of traces to show on screen            *
  *  iNumStas          Number of stations in Sta array               *
  *  iScreenToDisplay  Screen index (subset of stations) to display  *
  *  iFiltDisplay      1=display filtered data, 0=display broadband  *
  *  iClipIt           1=amplitude limit display, 0=don't limit      *
  *  cxScreen          Horizontal pixel width of window              *
  *  cyScreen          Vertical pixel height of window               *
  *  iVScrollOffset    Vertical scroll setting                       *
  *  iTitleOffset      Vertical offset of traces to give room at top *
  *  iChanIDOffset     Horizontal offset of traces to give room at lt*
  *  dScreenEnd        1/1/70 (seconds) time at end of display       *
  *  dScreenTime       # seconds to show on screen                   *
  *  piNumShown        # traces displayed on screen                  *
  *  iLPScreen         Screen on which LP data is shown (from 0)     *
  *  iDisplay          1=Velocity; 2=Acceleration; 3= Displacement   *
  *  iRT               1=Realtime data; 0=Analyze data               *
  *  iAutoScale        1=Determine scale values here based on noise  *
  *  iAlign            1->Align screen output on expected P time     *
  *  dOffset           Horizontal offset (seconds) when in Align mode*
  *  iFind             Index of trace to show in a different color   *
  *                                                                  *
  ********************************************************************/
  
void DisplayTraces(HDC hdc, STATION Sta[], int iNumTrace,
	int iNumStas, int iScreenToDisplay, int iFiltDisplay,
	int iClipIt, int cxScreen, int cyScreen,
	int iVScrollOffset, int iTitleOffset, int iChanIDOffset,
	double dScreenEnd, double dScreenTime, int *piNumShown,
	int iLPScreen, int iDisplay, int iRT, int iAutoScale,
	int iAlign, double dOffset, int iFind)
{
   double  dAutoScale = 0.0;         /* Scaling factor based on noise level */
   double  dOldestTime;              /* Time of oldest data in buffer */
   double  dScreenStart;             /* 1/1/70 (sec) time at left of screen */
   double  dSum, dRMS = 0.0;    /* Summation variables to determine RMS noise */
   static  double  dTemp, dTemp2, dScale;    /* Temporary values */
   HPEN    hBPen, hGPen, hRPen, hOPen;       /* Pen handles */
   HFONT   hOFont, hNFont;           /* Old and new font handles */
   int     iPlotThis;                /* 1 if there is any data not = zero */
   static  long    i, ii, iii, j, iCnt, lTemp, lTempN;  /* Loop counters */
   long    lNum;                     /* # of data points to draw to screen */
   long    lNumNoise;                /* # of data points used for noise */
   long    lOffsetX;                 /* Dist to skip from left side of screen */
   long    lOffsetY;                 /* Dist to skip on top of screen */
   long    lStart;                   /* Starting buffer index */
   long    lStartNoise;              /* Starting buffer index for bkgd noise */
   long    lVertRange;               /* Max signal deflection when iClipIt=1 */
   int     iChanIDOffset2;  /* Used to change trace offset from left of screen
                               necessary for eb4; dln 10/13/06 */
   POINT   pt;
   POINT   ptData[DISPLAY_BUFFER_SIZE];  /* Trace display buffer */
   char    szText[16];

/* Note on screen if this display is Acceleration or Displacement */
   if      ( iDisplay == 1 ) sprintf( szText, "Velocity" );
   else if ( iDisplay == 2 ) sprintf( szText, "Acceleration" );
   else if ( iDisplay == 3 ) sprintf( szText, "Displacement" );
   hNFont = CreateFont( cyScreen/15, cxScreen/45,
             0, 0, FW_THIN, FALSE, FALSE, FALSE, ANSI_CHARSET,
             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
             DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
             "Arial" );
   hOFont = SelectObject( hdc, hNFont );
   SetTextColor( hdc, RGB( 180, 180, 180 ) );
   SetBkMode( hdc, TRANSPARENT );
   pt.x = cxScreen/8;
   pt.y = 9*cyScreen/10;
   if ( iDisplay == 2 || iDisplay == 3 )
      TextOut( hdc, pt.x, pt.y, szText, (int)strlen( szText ) );

/* Note on screen if AutoScale is on */
   if ( iAutoScale == 1 ) 
   {   
      sprintf( szText, "AutoScale On" );
      pt.x = 5*cxScreen/8;
      pt.y = 9*cyScreen/10;
      TextOut( hdc, pt.x, pt.y, szText, (int)strlen( szText ) );
   }
   SetBkMode( hdc, OPAQUE );

/* Setting a variable offset to accomodate the greater # pixels on eb4
   dln 10/13/06 */
   iChanIDOffset2 = 18 * iChanIDOffset / 140;
   if (iChanIDOffset2 > 12) iChanIDOffset2 = 12;
   iChanIDOffset2 = 11 * iChanIDOffset2;

/* Create pen and select into device context */
   hBPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));         /* Black */
   hRPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));         /* Red */
   hGPen = CreatePen(PS_SOLID, 1, RGB(0, 128, 0));       /* Green */
   hOPen = SelectObject(hdc, hBPen);
   *piNumShown = 0;

/* Compute time at left side of screen */
   dScreenStart = dScreenEnd - dScreenTime;

/* Loop through all stations */
   for ( j=0; j<iNumStas; j++ )
      if ( Sta[j].iStationDisp[iScreenToDisplay] ||
	      iScreenToDisplay == iLPScreen )       /* On this screen? */
         if ( Sta[j].dSampRate > 0. )               /* Prevent /0. */
         {
            iPlotThis = 0;
/* Compute time of oldest data in buffer */
            if ( iRT == 1 )
               dOldestTime = Sta[j].dEndTime -
                ((double)Sta[j].lRawCircSize/Sta[j].dSampRate) +
                1./Sta[j].dSampRate;
            else
               dOldestTime = Sta[j].dStartTime;

/* Does the buffer data fit in this screen's time */
            if ( dOldestTime >= dScreenEnd || 
                 Sta[j].dEndTime <= dScreenStart )
               goto EndIf;

/* Find where on screen we should start plotting */
            if ( dOldestTime <= dScreenStart || iAlign == 1 )
               lOffsetX = iChanIDOffset2;
            else lOffsetX = (long)(((double)(cxScreen - iChanIDOffset2) *
                            (dOldestTime - dScreenStart)) / dScreenTime) +
                             iChanIDOffset2;

/* Where in the buffer should we start plotting from? */
            if ( dOldestTime < dScreenStart )
            {
               if ( iAlign == 1 )                   /* Align screen on P */
               {
                  lTemp = (long) ((Sta[j].dEndTime-Sta[j].dPStartTime- 
                   dOffset+(3.*dScreenTime/8.)) * Sta[j].dSampRate + 
                   0.00001);
                  lTempN = (long) ((Sta[j].dEndTime-dOldestTime+1.) * 
                   Sta[j].dSampRate + 0.00001);
/* Is this looking for data ahead of that already read in? */
                  if ( lTemp  <= 0 )                   goto EndIf;
                  if ( lTemp  >= Sta[j].lRawCircSize ) goto EndIf;
                  if ( lTempN <= 0 )                   goto EndIf;
                  if ( lTempN >= Sta[j].lRawCircSize ) goto EndIf;
                  lStart      = Sta[j].lSampIndexR - 1 - lTemp;
                  lStartNoise = Sta[j].lSampIndexR - 1 - lTempN;
               }
               else                               /* Align screen on time */
               {
                  lStart = Sta[j].lSampIndexR - 1 -
                   (long) ((Sta[j].dEndTime-dScreenStart)*Sta[j].dSampRate + 
                   0.00001);
                  lStartNoise = Sta[j].lSampIndexR - 1 -
                   (long) ((Sta[j].dEndTime-dOldestTime+1.)*Sta[j].dSampRate + 
                   0.00001);
               }
            }
            else                             /* Take index of oldest data */
            {
               if ( iAlign == 1 )                    /* Align screen on P */
               {
                  lTemp = (long) ((Sta[j].dEndTime-Sta[j].dPStartTime- 
                   dOffset+(3.*dScreenTime/8.)) * Sta[j].dSampRate + 
                   0.00001);
                  if ( lTemp >= Sta[j].lRawCircSize ) goto EndIf;
                  lStart = Sta[j].lSampIndexR - 1 - lTemp;
               }
               else                              /* Align screen on time */
                  lStart = Sta[j].lSampIndexR - 1 - (long) 
                   ((Sta[j].dEndTime-dOldestTime)*Sta[j].dSampRate + 
                   0.00001);
               lStartNoise = lStart;
            }
            while ( lStart < 0 ) lStart += Sta[j].lRawCircSize;
            while ( lStart >= Sta[j].lRawCircSize) 
               lStart -= Sta[j].lRawCircSize;
            while ( lStartNoise < 0 ) lStartNoise += Sta[j].lRawCircSize;
            while ( lStartNoise >= Sta[j].lRawCircSize) 
               lStartNoise -= Sta[j].lRawCircSize;

/* How many points should we plot? */
            lNum = (long)(dScreenTime*Sta[j].dSampRate + 0.0001);
            if ( iScreenToDisplay == iLPScreen ) 
               lNumNoise = (long) (150.*Sta[j].dSampRate + 0.0001);
			else
               lNumNoise = (long) (30.*Sta[j].dSampRate + 0.0001);

/* Adjust lNum so we don't wrap into older data */
            if ( dScreenEnd > Sta[j].dEndTime ) 
               lNum -= (long) ((dScreenEnd-Sta[j].dEndTime)*
                       Sta[j].dSampRate + 0.0001);
            if ( lNum > Sta[j].lRawCircSize ) lNum = Sta[j].lRawCircSize;
            if ( lNum <= 0 ) goto EndIf;

/* If we are trying to get more than will fit in DISPLAY_BUFFER, adjust
   lStart to get the latest data */
            if ( lNum > DISPLAY_BUFFER_SIZE )
            {
               lStart += lNum - DISPLAY_BUFFER_SIZE;
               lOffsetX += (long) (((double) (cxScreen-iChanIDOffset2)*(double)
                (lNum-DISPLAY_BUFFER_SIZE)/Sta[j].dSampRate) /
                dScreenTime );
               if ( lStart >= Sta[j].lRawCircSize ) lStart -= Sta[j].lRawCircSize;
               lNum = DISPLAY_BUFFER_SIZE;
            }

/* Compute Y offset from top to center of trace and max signal range */
            lOffsetY = *piNumShown*(cyScreen - iTitleOffset) / iNumTrace +
                       iVScrollOffset + iTitleOffset;
            lVertRange = (cyScreen-iTitleOffset) / (2*iNumTrace + 1);

/* If Auto-scaling, get scale factors */
            if ( iAutoScale == 1 )
            {
               if ( iFiltDisplay == 0 && iDisplay == 1 && iRT == 1 )
               {                                    /* RT BB Velocity screen */
                  if ( Sta[j].dAveRawNoise > 0. )
                     dAutoScale = ((double)(lVertRange) / 10.) / Sta[j].dAveRawNoise;
                  else
                     dAutoScale = Sta[j].dVScale / BROADBAND_SCALE;
               }
               else if ( iDisplay == 1 && iRT == 1 )
               {                                  /* RT Filt Velocity screen */
                  if ( Sta[j].dAveFiltNoise > 0. )
                     dAutoScale = ((double)(lVertRange) / 10.) / Sta[j].dAveFiltNoise;
                  else
                     dAutoScale = Sta[j].dVScale;
               }
               else if ( iRT == 0 || iDisplay == 2 || iDisplay == 3 )
               {                                /* Acc. and Disp. and non RT */
                  iCnt  = 0;
                  dSum  = 0.;
                  dTemp = 0.;
                  for ( i=lStartNoise; i<lStartNoise+lNumNoise; i++ )
                  {
                     lTemp = i-lStartNoise;   /* lTemp is display buffer ctr */
                     if ( lTemp >= DISPLAY_BUFFER_SIZE ) break;
                     ii = i;                     /* Use ii to stay in bounds */
                     if ( ii >= Sta[j].lRawCircSize ) ii -= Sta[j].lRawCircSize;
                     iii = ii+1;
                     if ( iii == Sta[j].lRawCircSize ) iii = 0;
/* Check to see if we are about to go into older data */
                     if ( iii == Sta[j].lSampIndexR ) break;
/* Compute RMS on the y array */
                     if ( iFiltDisplay == 0 )
                     {                                     /* Broadband data */
                        if ( iDisplay == 2 )                 /* Acceleration */
                           dTemp = (((double)Sta[j].plRawCircBuff[iii] -
                            Sta[j].dAveLDCRaw) - ((double)Sta[j].plRawCircBuff[ii] -
                            Sta[j].dAveLDCRaw)) * Sta[j].dSampRate;
                        else if ( iDisplay == 3 )            /* Displacement */
                        {
                           dTemp2 = (((double)Sta[j].plRawCircBuff[ii] -
                            Sta[j].dAveLDCRaw) + ((double)Sta[j].plRawCircBuff[iii] -
                            Sta[j].dAveLDCRaw)) / (Sta[j].dSampRate*2.);
                           dTemp += dTemp2;
                        }
                        else if ( iDisplay == 1 )               /* Velocity */
                           dTemp = ((double)Sta[j].plRawCircBuff[ii] -
                                    Sta[j].dAveLDCRaw);
                     }
                     else
                     {                                     /* Filtered data */
                        if ( iDisplay == 2 )                /* Acceleration */
                           dTemp = (((double)Sta[j].plFiltCircBuff[iii] -
                            Sta[j].dAveLDC) - ((double)Sta[j].plFiltCircBuff[ii] -
                            Sta[j].dAveLDC)) * Sta[j].dSampRate;
                        else if ( iDisplay == 3 )           /* Displacement */
                        {
                           dTemp2 = (((double)Sta[j].plFiltCircBuff[ii] -
                            Sta[j].dAveLDC) + ((double)Sta[j].plFiltCircBuff[iii] -
                            Sta[j].dAveLDC)) / (Sta[j].dSampRate*2.);
                           dTemp += dTemp2;
                        }
                        else if ( iDisplay == 1 )               /* Velocity */
                           dTemp = ((double)Sta[j].plFiltCircBuff[ii] -
                                    Sta[j].dAveLDC);
                     }
                     dSum += dTemp*dTemp;
                     iCnt++;
                  }
                  if ( iCnt > 1  ) dRMS = sqrt( dSum/(double) iCnt );
                  if ( dRMS > 0. ) dAutoScale = ((double) (lVertRange)/8.) / dRMS;
                  else             dAutoScale = Sta[j].dVScale;
               }
            }

/* Fill display buffer with data */
            if ( iFiltDisplay == 0 ) dScale = Sta[j].dVScale/BROADBAND_SCALE;
            else                     dScale = Sta[j].dVScale;
            if      ( iDisplay == 2 ) dScale /= ACCELERATION_SCALE;
            else if ( iDisplay == 3 ) dScale *= DISPLACEMENT_SCALE;
            if ( iAutoScale == 1 ) dScale = dAutoScale;
            dTemp = 0.;
            if ( lStart >= 0 && lStart < Sta[j].lRawCircSize )
            {
               for ( i=lStart; i<lStart+lNum; i++ )
               {
                  lTemp = i-lStart;          /* lTemp is display buffer ctr */
                  if ( lTemp >= DISPLAY_BUFFER_SIZE ) break;
                  ii = i;                       /* Use ii to stay in bounds */
                  if ( ii>=Sta[j].lRawCircSize ) ii -= Sta[j].lRawCircSize;
                  iii = ii+1;
                  if ( iii == Sta[j].lRawCircSize ) iii = 0;

/* Check to see if we are about to go into older data and load x array */
                  if ( lTemp > 0 && iii == Sta[j].lSampIndexR ) break;
                  ptData[lTemp].x = lOffsetX + (long) (((cxScreen - iChanIDOffset2)*
                   (lTemp / Sta[j].dSampRate))/dScreenTime);

/* Load y array */
                  if ( iFiltDisplay == 0 )
                  {                                       /* Broadband data */
                     if ( iDisplay == 1 )                       /* Velocity */
                        dTemp = (double)Sta[j].plRawCircBuff[ii] - Sta[j].dAveLDCRaw;
                     else if ( iDisplay == 2 )              /* Acceleration */
                        dTemp = (((double)Sta[j].plRawCircBuff[iii] -
                         Sta[j].dAveLDCRaw) - ((double)Sta[j].plRawCircBuff[ii] -
                         Sta[j].dAveLDCRaw)) * Sta[j].dSampRate;
                     else if ( iDisplay == 3 )              /* Displacement */
                     {
                        dTemp2 = (((double)Sta[j].plRawCircBuff[ii] -
                         Sta[j].dAveLDCRaw) + ((double)Sta[j].plRawCircBuff[iii] -
                         Sta[j].dAveLDCRaw)) / (Sta[j].dSampRate*2.);
                        dTemp += dTemp2;
                     }
                     if ( Sta[j].plRawCircBuff[ii] != 0 ) iPlotThis = 1;
                  }
                  else
                  {                                        /* Filtered data */
                     if ( iDisplay == 1 )                       /* Velocity */
                        dTemp = (double)Sta[j].plFiltCircBuff[ii] - Sta[j].dAveLDC;
                     else if ( iDisplay == 2 )              /* Acceleration */
                        dTemp = (((double)Sta[j].plFiltCircBuff[iii] -
                         Sta[j].dAveLDC) - ((double)Sta[j].plFiltCircBuff[ii] -
                         Sta[j].dAveLDC)) * Sta[j].dSampRate;
                     else if ( iDisplay == 3 )              /* Displacement */
                     {
                        dTemp2 = (((double)Sta[j].plFiltCircBuff[ii] -
                         Sta[j].dAveLDC) + ((double)Sta[j].plFiltCircBuff[iii] -
                         Sta[j].dAveLDC)) / (Sta[j].dSampRate*2.);
                        dTemp += dTemp2;
                     }
                     if ( Sta[j].plFiltCircBuff[ii] != 0 ) iPlotThis = 1;
                  }
                  ptData[lTemp].y = (long)(dTemp*dScale)*(-1) + lOffsetY;

/* Limit trace deflection if ClipIt was chosen */
                  if ( iClipIt == 1 && Sta[j].iClipIt == 1 )
                  {
                     if ( ptData[lTemp].y - lOffsetY > lVertRange )
                        ptData[lTemp].y = lOffsetY + lVertRange;
                     if ( ptData[lTemp].y - lOffsetY < lVertRange*(-1) )
                        ptData[lTemp].y = lOffsetY - lVertRange;
                  }
               }

/* Should this trace be plotted? Is it turned off? If it is alarmed, draw it
   in red. */
               if ( Sta[j].iDisplayStatus == 1 )
               {
                  if ( Sta[j].iAlarmStatus == 3 ) SelectObject( hdc, hRPen );
                  else if ( iFind == j )          SelectObject( hdc, hRPen );
                  else                            SelectObject( hdc, hBPen );
/* Don't plot if no values greater than 0 */
                  if ( iPlotThis == 1 )
                  {
                     MoveToEx( hdc, ptData[0].x, ptData[0].y, NULL );
                     Polyline( hdc, &ptData[0], lTemp );
                  }
               }
            }
EndIf:;  *piNumShown = *piNumShown+1;
         }

   SelectObject( hdc, hOPen );
   DeleteObject( SelectObject( hdc, hOFont ) );               /* Reset font */
   DeleteObject( hBPen );
   DeleteObject( hGPen );
   DeleteObject( hRPen );
}

      /******************************************************************
       *                      ReadScreenIni()                           *
       *                                                                *
       * This function reads the screen display initialization file.    *
       * This file allows screen display which show just a sub-set of   *
       * stations.                                                      *
       *                                                                *
       * October, 2011: Updated with horizontal comps.                  *
       * June, 2008: Combined with ANALYZE and added geographic coords. *
       *                                                                *
       *  Arguments:                                                    *
       *   piNumScreens     Number station screen subsets               *
       *   iNumSta          Numberof stations in Sta                    *
       *   iLPScreen        Screen index of LP screen                   *
       *   szScreenName     Name of screen sub-sets                     *
       *   Sta              Array of station information                *
       *   pszFile          Screen display file name                    *
       *                                                                *
       *  Return:           1 if read ok, 0 if problem                  *
       *                                                                *
       ******************************************************************/
	   
int ReadScreenIni( int *piNumScreens, int iNumSta, int iLPScreen,
                   char szScreenName[][32], STATION Sta[], char *pszFile )
{
   int     i, j, jj;
   int     iScreen[MAX_SCREENS];   /* Temp flags for screen display */
   int     *iScreenSet;            /* Flag showing if scn was in ini file */
   FILE    *hFile;                 /* File handle */
   LATLON  llGG;                   /* Geographic station location */
   char    szSta[6], szChn[6], szNet[4];   /* Input SCN */
   char    szLocation[6];   /* Input SCN */
   char *paramdir;
   char  FullTablePath[512];

   paramdir = getenv( "EW_PARAMS" ); 
   if ( paramdir == (char *) NULL ) return(-1);
   strcpy( FullTablePath, paramdir  );
   strcat( FullTablePath, "\\");
   strcat( FullTablePath, pszFile );

/* Try to open the ini file */
   if ( (hFile = (FILE *) fopen( FullTablePath, "r" )) == NULL )
   {
      logit ("t", "Couldn't open screen ini file: %s\n", pszFile);
      return 0;
   }
   
/* Read screen initialization values */
   fscanf( hFile, "Number of Station Screens: %d\n", piNumScreens );
   if ( *piNumScreens > MAX_SCREENS || *piNumScreens < 1 )
   {
      logit( "", "Incorrect number of screen subsets - %d\n", *piNumScreens );
      fclose( hFile );
      return 0;
   }
   
   for ( i=0; i<*piNumScreens; i++ )
   {
      fscanf( hFile, "Screen Name: " );
      fgets( szScreenName[i], sizeof (szScreenName[i]), hFile );
      fscanf( hFile, "\n" );
      szScreenName[i][strlen( szScreenName[i] )-1] = '\0';  /* End string */
      fscanf( hFile, "S. Lat. (+/-): %lf\n", &Sta[0].dScreenLat[i][0] );
      fscanf( hFile, "N. Lat. (+/-): %lf\n", &Sta[0].dScreenLat[i][1] );
      fscanf( hFile, "W. Lon. (+): %lf\n",   &Sta[0].dScreenLon[i][0] );
      fscanf( hFile, "E. Lon. (+): %lf\n",   &Sta[0].dScreenLon[i][1] );
   }

/* Allocate some local memory */
   iScreenSet = (int *) calloc( iNumSta, sizeof( int ) );
   if ( iScreenSet == NULL )
   {
      logit( "et", "ReadScreenIni: iScreenSet calloc failed\n" );
      fclose( hFile );
      return 0;
   }

/* Set ScreenDisp to 0 */
   for ( i=0; i<iNumSta; i++ )
      for ( j=1; j<*piNumScreens; j++ ) Sta[i].iStationDisp[j]=0;
   
/* Assign screen display flags to known stations */
   while ( fscanf( hFile, "%s %s %s %s ", szSta, szChn, szNet, szLocation ) != EOF )
   {
      for ( i=0; i<*piNumScreens; i++ )
         fscanf( hFile, "%d ", &iScreen[i] );
      fscanf( hFile, "\n" );
      for ( i=0; i<iNumSta; i++ )
         if ( !strcmp(  szSta, Sta[i].szStation ) &&
              !strncmp( szChn, Sta[i].szChannel, 3 ) &&
              !strcmp(  szNet, Sta[i].szNetID ) )
         {
            for ( j=0; j<*piNumScreens; j++ )
               Sta[i].iStationDisp[j] = iScreen[j];
            iScreenSet[i] = 1;
            break;
         }
      if ( i == iNumSta ) logit( "", "No match for %s %s\n", szSta, szChn );
   }
   fclose( hFile ); 
   
/* Flag unmatched stations in log file */
   for ( i=0; i<iNumSta; i++ )
   {
      for ( j=0; j<*piNumScreens; j++ )
         if ( Sta[i].iStationDisp[j] == 1 ) break;
      if ( j == *piNumScreens )
      {
         logit( "", "%s %s %s %s not in ini file\n", Sta[i].szStation,
                Sta[i].szChannel, Sta[i].szNetID, Sta[i].szLocation );
         Sta[i].iStationDisp[0] = 1;            /* Force on in screen 1 */
      }
   }
   
/* Set unknown stations to certain screens based on the .ini file */
   for ( i=0; i<iNumSta; i++ )
      if ( iScreenSet[i] == 0 )
      {
         llGG.dLat = Sta[i].dLat;
         llGG.dLon = Sta[i].dLon;
/* First, see if there is a low gain screen */
         for ( j=0; j<*piNumScreens; j++ )
            if ( !strcmp( "Low Gain Stations", szScreenName[j] ) ) break;
         if ( j < *piNumScreens )      /* Then, there is a low gain screen */
            if ( Sta[i].szChannel[1] == 'L' || // PW 10/12/11
                 Sta[i].szChannel[1] == 'N' )  
            {
               Sta[i].iStationDisp[j] = 1;    /* Show on low gain only */
               continue;
            }
/* Then, see if there is a horizontal screen - PW 10/12/11 */
         for ( j=0; j<*piNumScreens; j++ )
            if ( !strncmp( "Horizontal", szScreenName[j], 10 ) ) break;
         if ( j < *piNumScreens )    /* Then, there is a low gain screen */
            if ( Sta[i].szChannel[2] != 'Z' ) 
            {
               Sta[i].iStationDisp[j] = 1;    /* Show on horizontal only */
               continue;
            }
/* Now go through the rest of screens and decide where to show this channel */
         for ( j=0; j<*piNumScreens; j++ )
/* Check lat/lon and channel type */
            if ( llGG.dLat > Sta[0].dScreenLat[j][0] &&
                 llGG.dLat < Sta[0].dScreenLat[j][1] &&
                 llGG.dLon > Sta[0].dScreenLon[j][0] &&
                 llGG.dLon < Sta[0].dScreenLon[j][1] &&
                 Sta[i].szChannel[0] != 'L' &&
                 Sta[i].szChannel[0] != 'M' &&
                 Sta[i].szChannel[1] != 'L' &&   // PW 10/12/11
                 Sta[i].szChannel[1] != 'N' &&
                 Sta[i].szChannel[2] == 'Z' )
            {
               for ( jj=0; jj<*piNumScreens; jj++ )
                  if ( Sta[i].iStationDisp[jj] == 1 )   /* Only show on one */
                     goto EndFor;
               Sta[i].iStationDisp[j] = 1;              /* Set flag */
EndFor:;    }
/* Lastly, show on LP screen if it has LP */
         if ( (Sta[i].szChannel[0] == 'L' || // PW 10/12/11
               Sta[i].szChannel[0] == 'B' ||
               Sta[i].szChannel[0] == 'H' ||
               Sta[i].szChannel[0] == 'M') &&
              (Sta[i].szChannel[1] == 'H' || 
               Sta[i].szChannel[1] == 'L') && 
               Sta[i].szChannel[2] == 'Z' ) 
            Sta[i].iStationDisp[iLPScreen] = 1;   /* Show on LP screen */
      }	    
   free( iScreenSet );
   return 1;
}
#endif

      /*************************************************************************
      *              struct_cmp_by_EpicentralDistance                          *
      *                                                                        *
      * This function sorts the StaArray by the distance from the epicenter    *
      *                                                                        *
      *************************************************************************/

int struct_cmp_by_EpicentralDistance( const void *a, const void *b )
{
   STATION *ia = (STATION *) a;
   STATION *ib = (STATION *) b;
   return (int) (100.f*ia->dDelta - 100.f*ib->dDelta);
}

      /*************************************************************************
      *              struct_cmp_by_ID                                          *
      *                                                                        *
      * This function sorts the StaArray by Network ID                         *
      *                                                                        *
      *************************************************************************/

int struct_cmp_by_ID( const void *a, const void *b )
{
   STATION *ia = (STATION *) a;
   STATION *ib = (STATION *) b;
   return ( strcmp( ia->szNetID, ib->szNetID ) );
}

      /*************************************************************************
      *              struct_cmp_by_name                                        *
      *                                                                        *
      * This function sorts the StaArray by name                               *
      *                                                                        *
      *************************************************************************/

int struct_cmp_by_name( const void *a, const void *b )
{
   STATION *ia = (STATION *) a;
   STATION *ib = (STATION *) b;
   return ( strcmp( ia->szStation, ib->szStation ) );
}

      /*************************************************************************
      *              struct_cmp_by_StationSortIndex                            *
      *                                                                        *
      * This function sorts the StaArray by station index                      *
      *                                                                        *
      *************************************************************************/

int struct_cmp_by_StationSortIndex( const void *a, const void *b )
{
   STATION *ia = (STATION *) a;
   STATION *ib = (STATION *) b;
   return (int) (ia->iStationSortIndex - ib->iStationSortIndex);
}
