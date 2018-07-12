
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: truetime.c 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

  /*******************************************************************
   *                            truetime.c                           *
   *                                                                 *
   *       Function to get time from the True-Time PC-SG board,      *
   *     using the device driver written by Tod Morcott, Mar 1996.   *
   *     Runs on OS/2 version 3.                                     *
   *******************************************************************/

#define INCL_DOSFILEMGR                       
#define INCL_DOSPROCESS
#define INCL_DOSDEVICES
#define INCL_DOSPROFILE
#include <os2.h>
#include <bsedos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <stddef.h>
#include <math.h>
#include <truetime.h>

#define TB_CATEGORY  0x80
#define TB_FREEZE    0x41
#define TB_RELEASE   0x42
#define TB_GET_TIME  0X43
#define READ_READY   0x80

static int   MaxPassCnt;
static int   MaxPass = 0;
static char  buffer[32];
static char  *pszFileName;
static ULONG ulActionTaken, ulFileAttribute, ulOpenFlag, ulOpenMode, ulFileSize;
static HFILE hFileHandle;
static ULONG ulCategory;
static ULONG ulFunction;
static VOID  *pParmList;
static ULONG ulParmLengthMax;
static ULONG ulParmLengthInOut;
static ULONG ulDataLengthMax;
static ULONG ulDataLengthInOut;
static APIRET rc;



  /***************************************************************
   *                         OpenTrueTime()                      *
   *                                                             *
   *              Open the True-Time PC-SG clock card            *
   *                                                             *
   *  Argument:                                                  *
   *     MaxPassCount = Number of times the data ready bit is    *
   *     polled before an error is reported.  MaxPassCount must  *
   *     be larger for faster processors, and for TrueTime       *
   *     PC-SG boards with different characteristics.  A value   *
   *     of 100 was large enough, but 20 was not large enough    *
   *     (7/11/96)                                               *
   ***************************************************************/

int OpenTrueTime( int MaxPassCount )
{
   MaxPassCnt = MaxPassCount;

   ulCategory      = TB_CATEGORY;
   ulFileAttribute = FILE_NORMAL;
   ulOpenFlag      = OPEN_ACTION_FAIL_IF_NEW |
                     OPEN_ACTION_OPEN_IF_EXISTS;
   ulOpenMode      = OPEN_FLAGS_FAIL_ON_ERROR |
                     OPEN_FLAGS_NO_CACHE |
                     OPEN_SHARE_DENYNONE |
                     OPEN_ACCESS_READWRITE;
   pszFileName     = "TimeBas$";

   rc = DosOpen( pszFileName,
                 &hFileHandle, 
                 &ulActionTaken,
                 ulFileSize,
                 ulFileAttribute,
                 ulOpenFlag, 
                 ulOpenMode,
                 NULL );
   if ( rc != 0 )
   {
      printf( "Could not open %s\n", pszFileName );
      return -1;
   }
   return 0;
}


int CloseTrueTime( void )
{
   DosClose( hFileHandle );
   return MaxPass;
}


BOOL FreezeTime( void )
{
   ulFunction        = TB_FREEZE;
   pParmList         = NULL;
   ulParmLengthMax   = 0;
   ulParmLengthInOut = 0;
   ulDataLengthMax   = sizeof(int);
   ulDataLengthInOut = sizeof(int);

   rc = DosDevIOCtl( hFileHandle,
                     ulCategory,
                     ulFunction,
                     pParmList,
                     ulParmLengthMax,
                     &ulParmLengthInOut,
                     buffer,
                     ulDataLengthMax,
                     &ulDataLengthInOut );
   if ( rc != 0 )
   {
      printf( "%s DosDevIOCtl = %d\n", __FUNCTION__, rc );
      DosClose( hFileHandle );
      return FALSE;
   }
   return TRUE;
}



BOOL ReleaseTime( void )
{
   ulFunction = TB_RELEASE;

   rc = DosDevIOCtl( hFileHandle,
                     ulCategory,
                     ulFunction,
                     pParmList,
                     ulParmLengthMax,
                     &ulParmLengthInOut,
                     buffer,
                     ulDataLengthMax,
                     &ulDataLengthInOut );
   if ( rc != 0 )
   {
      printf( "%s DosDevIOCtl = %d\n", __FUNCTION__, rc );
      DosClose( hFileHandle );
      return FALSE;
   }
   return TRUE;
}



int GetTime( TrueTimeStruct *pTrueTime )
{
   int PassCount = 0;
   UCHAR data_ready;

   ulFunction        = TB_GET_TIME;
   pParmList         = NULL;
   ulParmLengthMax   = 0;
   ulParmLengthInOut = 0;
   ulDataLengthMax   = sizeof( TrueTimeStruct );
   ulDataLengthInOut = sizeof( TrueTimeStruct );

   pTrueTime->UnitMillisecs     = (UCHAR)0xff;
   pTrueTime->TensMillisecs     = (UCHAR)0xff;
   pTrueTime->HundredsMillisecs = (UCHAR)0xff;
   pTrueTime->UnitSeconds       = (UCHAR)0xff;
   pTrueTime->TensSeconds       = (UCHAR)0xff;
   pTrueTime->UnitMinutes       = (UCHAR)0xff;
   pTrueTime->TensMinutes       = (UCHAR)0xff;
   pTrueTime->UnitHours         = (UCHAR)0xff;  
   pTrueTime->TensHours         = (UCHAR)0xff;  
   pTrueTime->UnitDays          = (UCHAR)0xff;   
   pTrueTime->TensDays          = (UCHAR)0xff;   
   pTrueTime->HundredsDays      = (UCHAR)0xff;
   pTrueTime->Status            = (UCHAR)0xff;

/* Make several attempts to get the time from the True-Time clock
   **************************************************************/
   do
   {
      rc = DosDevIOCtl( hFileHandle,
                        ulCategory,
                        ulFunction,
                        pParmList,
                        ulParmLengthMax,
                        &ulParmLengthInOut,
                        pTrueTime,
                        ulDataLengthMax,
                        &ulDataLengthInOut );
      if ( rc != 0 )
      {
         printf( "%s DosDevIOCtl = %d\n", __FUNCTION__, rc );
         return -1;
      }

      if ( ++PassCount == MaxPassCnt )     /* Error. Too many passes. */
         return -2;

      data_ready = pTrueTime->DataReady & READ_READY;

   } while ( !data_ready );

   MaxPass = (PassCount > MaxPass) ? PassCount : MaxPass;
   return 0;
}


int GetTrueTime( TrueTimeStruct *pTrueTime )
{
   int rc;

   FreezeTime();
   rc = GetTime( pTrueTime );
   ReleaseTime();
   return rc;
}
