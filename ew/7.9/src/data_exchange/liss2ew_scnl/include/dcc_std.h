/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_std.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:55  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:26  mark
 *     Initial checkin
 *
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

/*----------------------------------------------------------------------*
 *	Albuquerque Seismological Laboratory - USGS - US Dept of Inter	* 
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *	dcc_std.h - Standards and constants definition for project	*
 *----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 *	Modification and history 					*
 *									*
 *-----Edit---Date----Who-------Description of changes------------------*
 *	001 24-Apr-86 SH	Set up standards file			*
 *	002  9-Jul-86 SH	Add mfree to free for unix29		*
 *	003 22-Dec-86 SH	Tailor VMS definitions 			*
 *	004 10-Jul-89 SH	Add ssdef.h to files loaded		*
 *	005 22-Aug-89 SH	Add math.h to files loaded		*
 *	006 13-Jun-91 SH	Make more unix oriented 		*
 *      007 29-May-92 SH        POSIX project                           *
 *      008 15-Sep-97 SH        GNU Autoconf compatibility              *
 *----------------------------------------------------------------------*/

/*-----------------------Determine System Dependancies------------------*/

#include <dcc_config.h>

#include <stdio.h>
#include <sys/types.h>

/* Include all standard header files for software */

#ifdef HAVE_MATH_H
# include <math.h>
#endif

#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif

#ifdef HAVE_ERRNO_H
# include <errno.h>

# define EXIT_NORMAL	0
# define EXIT_ABORT	1
# define EXIT_NOMEM	ENOMEM
# define EXIT_INSFMEM	ENOMEM
# define EXIT_BADPARAM	EINVAL
# define EXIT_NODEVAVL	EACCES
# define EXIT_NOSUCHDEV	ENODEV
# define EXIT_TIMEOUT	ETIME

# define DB_NORMAL   0
# define DB_RNF      100
# define DB_RLK	    101

#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_STDDEF_H
# include <stddef.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define shrrchr rindex
# endif
char *shrchr(), *strrchr();
# ifdef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy((s), (d), (n))
#  define memmove(d, s, n) bcopy((s), (d), (n))
# endif
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if SIZEOF_SHORT==2
# define _BIT16 short		/* What is the 16 bit quantity */
#else
# if SIZEOF_INT==2
#  define _BIT16 int
# endif
#endif

#if SIZEOF_INT==4
# define _BIT32 int
#else
# if SIZEOF_LONG==4
#  define _BIT32 int
# endif
#endif

#define QTICKS 100		/* times CPU times are 1/100th of a second */
#define _mag_type int        	/* Magtape uses file-descriptor # */

/* Earthworm conventions for byte order */
#ifdef _SPARC
#ifndef BIG_ENDIAN
#define	BIG_ENDIAN		/* 68000 word order */
#endif
#undef LITTLE_ENDIAN
#endif
#ifdef _INTEL
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#undef BIG_ENDIAN
#endif

/*--------------------------Define Data types------------------------------*/

#define STDFLT double		/* Standard floating */

#define VOID void     	/* For functions which return nothing */
#ifdef _WINDEF_
#else
typedef unsigned char BOOL;	/* Flag quantities */
#endif
typedef unsigned char TEXT;	/* For character strings */

	/*--------------------------------------------------*
	 *			    8-Bit	    16-Bit	    32-Bit	    *
	 *	Numbers:	DCC_BYTE	DCC_WORD	DCC_LONG	*
	 *	Unsigned:	UDCC_BYTE	UDCC_WORD	UDCC_LONG	*
	 *--------------------------------------------------*/

/*	8-Bit quantities	*/

typedef unsigned char UDCC_BYTE;	/* An 8-Bit definition */
typedef signed char DCC_BYTE;		/* Numeric 8-bit definition */

/*	16-Bit quantities	*/

typedef unsigned _BIT16 UDCC_WORD;	/* 16 bit unsigned */
typedef signed _BIT16 DCC_WORD;		/* 16 bit numeric quantity */

/*	32-Bit quantities	*/

typedef unsigned _BIT32 UDCC_LONG;	/* Definition of a 32 bit mask */
/*#ifndef LONG_t */  /* Added for earthworm compatibility: PNL 2/2/2000 */
/*#define LONG_t */
typedef signed _BIT32 DCC_LONG;	/* A 32 bit number */
/*#endif*/

/* 	Misc definitions */

typedef UDCC_LONG DSKREC;

typedef _mag_type MAG_ID;

#define LOCAL static
#define FAST register
#define PUBLIC
#define PRIVATE static 
#define _SUB			/* Key for subroutine definitions */

/*--------------Define some macros and contants we will use-----------------*/

#define FOREVER for(;;)		/* Infinite loop	*/


#ifndef NULL
#define NULL	(0)		/* Impossible pointer	*/
#endif

#ifndef TRUE
#define TRUE	1		/* if (TRUE)		*/
#endif

#ifndef FALSE
#define FALSE	0		/* if (!TRUE)		*/
#endif

#ifndef EOS
#define EOS	'\0'		/* End of string	*/
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define streq(a,b) (strcmp(a,b)==0)

#define IUBYTE(x) (((DCC_WORD) x)&0xFF)

#define bomb_memory(x) if ((x)==NULL) _bombout(EXIT_INSFMEM,	\
	"\nFile=%s (%d)\nLine=%d\n",__FILE__,__DATE__,__LINE__)

#define bombout _bombdesc(__FILE__,__DATE__,__LINE__), _bomb2

/* DCC Standard environment variables */

#define DCC_SETUP_ENV "DCC_SETUPS"
#define DCC_BIN_ENV "DCC_BIN"

/* End */
