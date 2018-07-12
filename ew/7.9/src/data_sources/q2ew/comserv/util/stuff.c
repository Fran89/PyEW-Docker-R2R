/*$Id: stuff.c 5818 2013-08-14 20:47:04Z paulf $*/
/*   General purpose utility routines
     Copyright 1994-1996 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 30 Mar 94 WHO Hacked from various other files.
    1 30 May 94 WHO str_long and long_str changed to handle right
                    justified name instead of left justified (DSN). Add
                    downshift procedure (DSN). Add "void" to str_right (DSN).
    2  9 Jun 94 WHO Cleanup to avoid warnings.
    3 28 Feb 95 WHO Start of conversion to run on OS9.
    4 17 Oct 97 WHO Add VER_STUFF
    5  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
      12 Nov 99 IGD Add up byte-swapping calls fip2 and flip4 required for
                    i386 architecture porting (calls are derived from version
                    3 of timeutil.c modified for Linux by Hanka et al.) 
      17 Nov 99 IGD Modified flip2() and flip4() so that flipX() does it
                    flipping job if LINUX flag is defined and returns input
                    unchanged otherwise. This is done in order not to mess up
                    the rest of the code with #ifdefs   
      29 Nov 99 IGD Modified str_long (if LINUX flag is defined)    
       9 Dec 99 IGD Add up  flip4array(long *in, short bytes) to swap long and int 
		    data arrays 4 byte data array in case of NUXI problem
    6   5 Mar 01 IGD Add float flip_float(float) byte-swapping routine
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifndef _OSK

#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <time.h>
#endif
#include "dpstruc.h"

short VER_STUFF = 6 ;

/* Return seconds (and parts of a second) since 1970 */
  double dtime (void) 
    {
#ifndef _OSK
    struct timeval tp;
    struct timezone tzp;
      
      gettimeofday (&tp, &tzp) ;
      return ((double) tp.tv_sec + ((double) tp.tv_usec / 1000000.0)) ;
#else
#define TIMEDIFF 2440587 /* difference between 1970 and dorky OS9 julian time */
      u_int32 time, date, ticks ;
      u_int16 day ;
      
      _os9_getime(3, &time, &date, &day, &ticks) ;
      date = date - TIMEDIFF ; /* change from 4513BC to 1980 */
      return (double) date * 86400.0 + (double) time + 
           (double) (ticks & 0xffff) / (double) (ticks >> 16) ;
#endif
    }

/* Convert C string to longinteger */
  long str_long (pchar name)
    {
      short i ;
      complong temp ;
      
      temp.l = 0x20202020 ; /* all spaces */
      for (i = 0 ; i < 4 ; i++)
        if (i < strlen(name))
            { /* move characters left, add on right */
#if defined (LINUX)
              temp.l = temp.l >> 8 ; /*IGD - that how it should work for little-endian machines */
#else
              temp.l = temp.l << 8 ;
#endif
              temp.b[3] = toupper(name[i]) ;
            }
      return temp.l ;
    }

/* Convert longinteger to C string */
  pchar long_str (long name)
    {
      short i, j ;
      complong temp ;
      static char out[5] ;
      
      temp.l = name ;
      j = 0 ;
      for (i = 0 ; i < 4 ; i++)
        if (temp.b[i] != ' ')
            out[j++] = temp.b[i] ;
      out[j] = '\0' ;
      return (pchar) &out ;
    }
    
/* Convert pascal string to C string */
  void strpcopy (pchar outstring, pchar instring)
    {
      short i ;
      
      for (i = 0 ; i < instring[0] ; i++)
        outstring[i] = instring[i + 1] ;
      outstring[i] = '\0' ;
    }

/* Convert C string to Pascal string */
  void strpas (pchar outstring, pchar instring)
    {
      short i ;
      
      i = 0 ;
      while (instring[i])
        outstring[i + 1] = instring[i++] ;
      outstring[0] = i ;
    }

/* Set the bit in the mask pointed to by the first parameter */
  void set_bit (long *mask, short bit)
    {
      *mask = *mask | (1 << (long)bit) ;
    }

/* Clear the bit in the mask pointed to by the first parameter */
  void clr_bit (long *mask, short bit)
    {
      *mask = *mask & (~(1 << (long)bit)) ;
    }

/* Returns TRUE if the bit in set in the mask */
  boolean test_bit (long mask, short bit)
    {
      return ((mask & (1 << (long)bit)) != 0) ;
    }

/* remove trailing spaces & control characters from a C string */
  void untrail (pchar s)
    {
      while ((s[0] != '\0') && (s[strlen(s)-1] <= ' '))
        s[strlen(s)-1] = '\0' ;
    }
    
/* upshift a C string */
  void upshift (pchar s)
    {
      short i ;
      
      for (i = 0 ; i < strlen(s) ; i++)
        s[i] = toupper (s[i]) ;
    }

/* downshift a C string */
  void downshift (pchar s)
    {
      short i ;
      
      for (i = 0 ; i < strlen(s) ; i++)
        s[i] = tolower (s[i]) ;
    }

/* add a directory separator slash to the end of a C string if there isn't one */
  void comserv_addslash (pchar s)
    {
      if ((s[0] != '\0') && (s[strlen(s)-1] != '/'))
          strcat(s, "/") ;
    }

/* Start at ptr+1 and copy characters into dest up to and including the
   terminator */
  void str_right (pchar dest, pchar ptr)
    {
      do
        *(dest++) = *(++ptr) ;
      while (*ptr != '\0') ;
    }

/* Return longinteger representation of a byte, making sure it is not sign extended */
  long longhex (byte b)
    {
      return ((long) b) & 255 ;
    }

/* Return integer representation of a byte, making sure it is not sign extended */
  short ord (byte b)
    {
      return ((short) b) & 255 ;
    }


/* flip short and integer */


short flip2( short shToFlip ) {

#if defined (LINUX)
 short shSave1, shSave2;
 
 	shSave1 = ((shToFlip & 0xFF00) >> 8);
 	shSave2 = ((shToFlip & 0x00FF) << 8); 	
 	return( shSave1 | shSave2 );
#else   /*if it is not LINUX-ported version just return input*/
       return (shToFlip);
#endif
}

int flip4( int iToFlip ) {
#if defined (LINUX)
int iSave1, iSave2, iSave3, iSave4;

	iSave1 = ((iToFlip & 0xFF000000) >> 24);
	iSave2 = ((iToFlip & 0x00FF0000) >> 8);
	iSave3 = ((iToFlip & 0x0000FF00) << 8);
	iSave4 = ((iToFlip & 0x000000FF) << 24); 

	return( iSave1 | iSave2 | iSave3 | iSave4 );
#else    /*if it is not LINUX-ported version just return input*/
       return (iToFlip);
#endif
}

/* 
 * IGD 03/05/01 : new functtion to byte-swap 4-byte float 
 * Modified on 03/09/01
 */
float flip_float(float fToFlip)	{
#if defined (LINUX)
    union {
		float fl;		
		char p[4];
	  } u;
    unsigned char tmp; 
    u.fl = fToFlip;
    tmp = *u.p;
    *u.p = *(u.p+3);
    *(u.p+3) = tmp;
    tmp = *(u.p+1);
    *(u.p+1) = *(u.p+2);
    *(u.p+2) = tmp;	 
    fToFlip = u.fl;
#endif
return (fToFlip);        /*if it is not little-endian computer, just return input*/
}	
 
int flip4array (long *in, short bytes)	{
/*--------------------------------------------------------
/ Swaps "bytes" bytes of a long array *in: 12345678->43218765 etc. 
/ returns 0 in case of success and negative value otherwise. 
/ Directly modifies data stored in the memory pointed by *data,
/ therefore could be dangerous if used improperly.
/            Ilya Dricker ISTI (i.dricker@isti.com) 12/09/1999
/ Revisions 
/ 12/09/1999 Initial revision
/---------------------------------------------------------*/
unsigned char tmp;
unsigned char *p = (unsigned char *)in; 
short i;

if (sizeof(long) != 4) {
	fprintf (stderr, "Warning in flip4array: Sizeof long is not 4 as was assumed\n flip4array is aborted\n ");
	return (-1);
	}

for (i=0; i < bytes; i=i+4)	{
	tmp = *(p + i);		
	*(p + i) = *(p + 3 + i);    
	*(p + 3 +i) = tmp;
	tmp = *(p + 1 + i);
	*(p+1 + i) = *(p + 2 + i);
	*(p + 2 + i) = tmp;
	}
return (0);
}
 
