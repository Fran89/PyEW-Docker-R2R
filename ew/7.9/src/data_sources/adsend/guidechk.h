/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: guidechk.h 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */


/***************************************************************************
   guidechk.h   header file for the triangle wave detector (see guidechk.c)

   This function assumes triangle waves as its input.  The function
   returns one of these "status" values in gStat:
     gStat = GUIDE_OK    Everything is in synch.
           = GUIDE_FLAT  no triangle wave; flat trace; dead mux
           = GUIDE_NOISY Signal does not look like a triangle wave.
***************************************************************************/


/* Constants for reporting the status of guide channels
   ****************************************************/
#define GUIDE_OK        0
#define GUIDE_FLAT     -1
#define GUIDE_NOISY    -2

/* Possible values of combined guide status
   ****************************************/
#define BAD             0
#define OK              1
#define LOCKED_ON       2
#define RESTART_DAQ     3
