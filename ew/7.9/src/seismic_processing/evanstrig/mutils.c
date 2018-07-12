
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: mutils.c 6852 2016-10-20 21:01:59Z kevin $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2006/10/23 21:22:20  paulf
 *     made mutils more linux friendly
 *
 *     Revision 1.2  2000/07/09 18:23:07  lombard
 *     Corrected missing comment terminator
 *
 *     Revision 1.1  2000/02/14 17:17:36  lucky
 *     Initial revision
 *
 *
 */


/* FILE: mutils.c                               (D. Tottingham  03/24/91)

The following functions are included:

u_convert_time ()               convert a struct timeb into abs. time
u_timestamp ()                  get timestamp
er_abort ()                     display an error message then quit 
*/

/************************************************************************
 *                          INCLUDE FILES                               *
 ************************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/timeb.h>

#include <time.h>
#include "mconst.h"
#include "mutils.h"


/*************************************************************************
 *                              GLOBALS                                  *
 *************************************************************************/

static ER_MESSAGE em[] =  {
{ TEL_INVALID_SRATE     , "Invalid digitization rate in TEL_INITIALIZE_SYSTEM.", FALSE, TRUE },
{ TEL_EMPTY_BUFFER      , "Empty buffer in TEL_TRIGGERED_ON_CHANNEL."          , FALSE, TRUE },
{ TEL_NEED_FOR_DEC_BUF  , "Need decimated buffer in TEL_TRIGGERED_ON_CHANNEL." , FALSE, TRUE },
{ UNEXPECTED_ERROR      , "Unexpected error."                                  , FALSE, TRUE }};


/*=======================================================================*
 *                            u_convert_time()                           *
 *       Convert struct timeb into an absolute time.  Used below.        *
 *=======================================================================*/
double u_convert_time( struct timeb abs_timeb )
{
   return( (double)abs_timeb.time + (double)abs_timeb.millitm / 1000.0 );
}


/*=======================================================================*
 *                            u_timestamp()                              *
 *                Get timestamp.  Used by mteltrg.c.                     *
 *=======================================================================*/
double u_timestamp( void )
{
   struct timeb current_timeb;

   ftime( &current_timeb );
   return ( u_convert_time( current_timeb ) );
}


/*=======================================================================*
 *                             er_abort()                                *
 *                 Display an error message then quit.                   *
 *=======================================================================*/

void er_abort( unsigned message_id )
{
   static char message[80];
   int i;

   for ( i = 0; em[i].id != UNEXPECTED_ERROR && em[i].id != message_id; i++ );
   sprintf( message, "ERROR %3d: %s\n", message_id, em[i].text );
   printf( "%s", message );
   exit( 1 );
}
