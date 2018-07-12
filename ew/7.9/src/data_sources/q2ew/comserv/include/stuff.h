/*   General purpose utility routines header file
     Copyright 1994-1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 30 Mar 94 WHO Hacked from various other files.
    1 30 May 94 WHO Missing "void" on str_right added (DSN).
    2 27 Feb 95 WHO Start of conversion to run on OS9.
    3  3 Nov 97 WHO Add c++ conditionals.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifndef _OSK

#include <fcntl.h>
#include <sys/types.h>
#endif

#ifndef cs_dpstruc_h
#include "dpstruc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Return seconds (and parts of a second) since 1970 */
  double dtime (void) ;

/* Convert C string to longinteger */
  long str_long (pchar name) ;

/* Convert longinteger to C string */
  pchar long_str (long name) ;

/* Convert pascal string to C string */
  void strpcopy (pchar outstring, pchar instring) ;

/* Convert C string to Pascal string */
  void strpas (pchar outstring, pchar instring) ;

/* Set the bit in the mask pointed to by the first parameter */
  void set_bit (long *mask, short bit) ;

/* Clear the bit in the mask pointed to by the first parameter */
  void clr_bit (long *mask, short bit) ;

/* Returns TRUE if the bit in set in the mask */
  boolean test_bit (long mask, short bit) ;

/* remove trailing spaces and control characters from a C string */
  void untrail (pchar s) ;
   
/* upshift a C string */
  void upshift (pchar s) ;

/* add a directory separator slash to the end of a C string if there isn't one */
  void cs_addslash (pchar s) ;

/* Start at ptr+1 and copy characters into dest up to and including the
   terminator */
  void str_right (pchar dest, pchar ptr) ;

/* Return longinteger representation of a byte, making sure it is not sign extended */
  long longhex (byte b) ;

/* Return integer representation of a byte, making sure it is not sign extended */
  short ord (byte b) ;

#ifdef __cplusplus
}
#endif
