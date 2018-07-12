/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: console.c 1147 2002-11-25 22:55:49Z alex $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/11/25 22:55:49  alex
 *     Initial revision
 *
 *     Revision 1.2  2002/03/06 01:30:46  kohler
 *     Added include <stdio> and prototype for PrintGmtime().
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

     /***************************************************************
      *                  Control the console display                *
      ***************************************************************/

#include <stdio.h>
#include <time.h>
#include <earthworm.h>

static HANDLE outHandle;           // The console file handle

void PrintGmtime( double, int );   // Function prototype


void SetCurPos( int x, int y )
{
   COORD     coord;

   coord.X = x;
   coord.Y = y;
   SetConsoleCursorPosition( outHandle, coord );
   return;
}


void InitCon( void )
{
   time_t current_time;
   WORD   color;
   COORD  coord;
   DWORD  numWritten;

/* Get the console handle
   **********************/
   outHandle = GetStdHandle( STD_OUTPUT_HANDLE );

/* Set foreground and background colors
   ************************************/
   color = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
   coord.X= coord.Y= 0;
   FillConsoleOutputAttribute( outHandle, color, 2000, coord, &numWritten );
   SetConsoleTextAttribute( outHandle, color );

/* Fill in the labels
   ******************/
   SetCurPos( 30, 1 );
   printf( "TRACE GENERATOR" );

   
   SetCurPos( 4, 10 );
   printf( "Scan:" );

   SetCurPos( 4, 12 );
   printf( "Samples/sec:" );

   SetCurPos( 4, 14 );
   printf( "Packets/sec:" );

   SetCurPos( 4, 21 );
   printf( "Last Error/Warning:" );

   /*SetCurPos( 46, 4 );
   printf( "Guide status    Signal  Noise" );
   SetCurPos( 49, 5 );
   printf( "All:" );
   SetCurPos( 49, 6 );
   printf( "1:  " );
   SetCurPos( 49, 7 );
   printf( "2:  " );
   SetCurPos( 49, 8 );
   printf( "3:  " );
   SetCurPos( 49, 9 );
   printf( "4:  " );

   SetCurPos( 47, 12 );
   printf( "DAQ Restarts (UTC)" );
   SetCurPos( 53, 13 );
   printf( "None" );

   SetCurPos( 47, 18 );
   printf( "Not sending..." );

   SetCurPos( 0, 0 ); */
   return;
}
