
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: errexit.c 1878 2005-07-15 18:20:22Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2005/07/15 18:20:19  friberg
 *     Unix version of libsrc for POSIX systems
 *
 *     Revision 1.2  2004/04/12 22:30:02  dietz
 *     included stdlib.h
 *
 *     Revision 1.1  2000/02/14 18:46:17  lucky
 *     Initial revision
 *
 *
 */
#include <stdlib.h>

void ErrExit( int returnCode )
{
   exit( returnCode );
}
