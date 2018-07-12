/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: raw2ew.c 4193 2011-04-27 17:04:59Z luetgert $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2003/10/03 23:24:59  dietz
 *     Fixed bug which caused file2ew to exit when optional 3rd argument
 *     to SuffixType command was ommitted.
 *
 *     Revision 1.4  2002/12/06 00:31:59  dietz
 *     added optional instid argument to SuffixType command
 *     changed lots of fprintf(stderr...) calls to logit("e",...)
 *
 *     Revision 1.3  2002/06/05 16:19:44  lucky
 *     I don't remember
 *
 *     Revision 1.2  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *     Currently file2ewfilter_hbeat() is a dummy function.
 *
 *     Revision 1.1  2001/03/14 21:01:52  dietz
 *     Initial revision
 *
 *
 */

/*  raw2ew.c
 *  Reads the contents of a file into a message buffer "as is", with
 *  no attempt at reformatting.  Places the message into the configured
 *  transport ring, assigning a message type and installation id based 
 *  on the suffix of the filename.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <chron3.h>
#include <kom.h>
#include <earthworm.h>
#include <k2evt2ew.h>
#include "file2ew.h"

/* Globals from file2ew.c
 ************************/
extern int Debug;

/* Local globals & definitions
 *****************************/
# define SUFFIX_LEN 15

typedef struct _SUFFIXTYPE_ {
  char suffix[SUFFIX_LEN+1];       /* suffix of filename (after last .)     */
  unsigned char msgtype;           /* message type associated with suffix   */
  unsigned char instid;            /* installation id associated with suffix*/
} SUFFIXTYPE;

# define MAX_SUFFIX  5             /* default value for MaxSuffix           */
static int MaxSuffix = MAX_SUFFIX; /* configured max# suffix/type pairs     */
static int NSuffix   = 0;          /* # of suffix pairs in this config file */
static SUFFIXTYPE *FileSfx = NULL; /* table of suffix/messag type pairs     */

//#define BUFLEN MAX_BYTES_PER_EQ    /* define maximum size for an ew msg     */
#define BUFLEN DB_MAX_BYTES_PER_EQ    /* define maximum size for an ew msg     */
static char MsgBuf[BUFLEN+1];      /* char string to hold output message    */

static int Init = 0;               /* initialization flag                   */

/******************************************************************************
 * file2ewfilter_com() processes config-file commands.                        *
 * Returns  1 if the command was recognized & processed                       *
 *          0 if the command was not recognized                               *
 * Note: this function may exit the process if it finds serious errors in     *
 *       any commands                                                         *
 ******************************************************************************/
int file2ewfilter_com( )
{
   char   *str;

/* Reset the maximum number of suffix/type pairs
 ***********************************************/
   if( k_its("MaxSuffixType") ) { /*optional*/
      MaxSuffix = k_int();
      if( FileSfx != NULL ) {
         logit( "e", "raw2ew_com: Must specify <MaxSuffixType> before"
                " any <SuffixType> commands; exiting!\n" );
         exit( -1 );
      }
      return( 1 );
   }

/* Get the mappings from filename suffix to EW msg type
 ******************************************************/
   if( k_its("SuffixType") ) { 
   /* First one; allocate FileSfx struct */
      if( FileSfx == NULL ) {
         FileSfx = (SUFFIXTYPE *) calloc( (size_t)MaxSuffix,
                                           sizeof(SUFFIXTYPE) );
         if( FileSfx == NULL ) {
            logit( "e", "raw2ew_com: Error allocating %ld bytes for"
                   " %d suffix/type pairs; exiting!\n",
                   MaxSuffix*sizeof(SUFFIXTYPE), MaxSuffix );
            exit( -1 );
         }
      }
   /* See if we have room for another pair */
      if( NSuffix >= MaxSuffix ) {
         logit( "e",
                "raw2ew_com: More than %d <SuffixType> cmds in configfile!\n"
                "      Use <MaxSuffixType> cmd increase limit; exiting!\n",
                (int) MaxSuffix );
         exit( -1 );
      }
   /* Get the file suffix */
      if( ( str=k_str() ) ) {
         if( str[0]=='.' ) str++;  /* ignore any period */
         if( strlen(str) > SUFFIX_LEN ) { 
            logit( "e", "raw2ew_com: suffix <%s> too long"
                   " in <SuffixType> cmd; max=%d char; exiting!\n", 
                   str, SUFFIX_LEN );
            exit( -1 );
         }
         strcpy(FileSfx[NSuffix].suffix, str);
      }
   /* Get the message type */
      if( ( str=k_str() ) ) {
         if( GetType( str, &(FileSfx[NSuffix].msgtype) ) != 0 ) {
            logit( "e", "raw2ew_com: Invalid message type <%s>"
                   " in <SuffixType> cmd; exiting!\n", str );
            exit( -1 );
         }
      }
   /* Get the installation id (optional) */
      if( ( str=k_str() ) ) {
         if( str[0] != '#'  &&    /* str is not a comment */
             GetInst( str, &(FileSfx[NSuffix].instid) ) != 0 ) {
            logit( "e", "raw2ew_com: Invalid installation id <%s>"
                   " in <SuffixType> cmd; exiting!\n", str );
            exit( -1 );
         }
      }
      if( FileSfx[NSuffix].instid == 0 ) {  /* instid was not given */
         k_err();  /* clear the error if instid was omitted */
         if( GetLocalInst( &(FileSfx[NSuffix].instid) ) != 0 ) {
            logit( "e", "raw2ew_com: error getting local "
                   "installation id; exiting!\n" );
            exit( -1 );
         }
      }
      NSuffix++;
      return( 1 );
   } 		

   return( 0 );
}

/******************************************************************************
 * file2ewfilter()   Do all the work to submit files to Earthworm msgs        *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
   char  *dot;
   size_t nread;
   int    ist;

/* Initialize variables
 **********************/
   if(!Init) file2ewfilter_init();


/* Close and reopen file for binary read
 ***************************************/
   fclose( fp );
   if( (fp=fopen( fname, "rb")) == (FILE *) NULL )
   {
      logit("e", "raw2ew: Error opening file <%s>.\n", 
             fname );
      return( -1 );
   }

/* Point to suffix of filename
 *****************************/
   dot = strrchr( fname, '.' );
   if( dot == NULL ) {
      logit("e","raw2ew: Cannot process filename <%s>; "
            "it has no suffix.\n", fname );
      return( -1 );
   }
   dot++; /* move beyond the period */

/* Find suffix in suffix/msgtype list
 ************************************/
   for( ist=0; ist<NSuffix; ist++ ) {
      if( strcmp( dot, FileSfx[ist].suffix ) == 0 ) break;
   }
   if( ist==NSuffix ) { /* no match found */
      logit("e","raw2ew: Cannot process filename <%s>; "
            "unknown suffix.\n", fname );
      return( 0 );
   }

/* Read file into msg buffer & write to ring
 *******************************************/
   nread = fread( MsgBuf, (size_t) 1, (size_t)BUFLEN, fp );
   MsgBuf[nread] = '\0';
   if( !feof(fp) ) {
      logit("e","raw2ew: Error processing filename <%s>; ", fname );
      if( nread==BUFLEN ) {
         logit("e","file longer than msgbuffer[%d]\n", BUFLEN );
      } else {
         logit("e","trouble reading file\n" );
      }
      return( -1 );
   }

   if( file2ew_ship( FileSfx[ist].msgtype, FileSfx[ist].instid,
                     MsgBuf, nread ) != 0 ) return( -1 );

   return( 1 );
}


/******************************************************************************
 * file2ewfilter_init()  check arguments                                      *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int i;
   int ret=0;

/* Log configured params
 ***********************/
   if( NSuffix==0 ) {
      logit("e","raw2ew_init: no <SuffixType> commands in configfile; "
             "at least 1 required!\n");
      ret = -1;
   } else {
      logit("","Configured to process:\n" );
      for(i=0;i<NSuffix;i++) {
         logit("","  File Suffix: %10s   MsgType: %3d  InstID: %3d\n", 
               FileSfx[i].suffix, (int)FileSfx[i].msgtype, 
               (int)FileSfx[i].instid );
      }
   }

   Init = 1;
   return( ret );
}

/******************************************************************************
 * file2ewfilter_hbeat()  read heartbeat file, check for trouble              *
 ******************************************************************************/
int file2ewfilter_hbeat( FILE *fp, char *fname, char *system )
{
   return( 0 );
}

/******************************************************************************
 * file2ewfilter_shutdown()  free memory, other cleanup tasks                 *
 ******************************************************************************/
void file2ewfilter_shutdown( void )
{
   free( FileSfx );
   return;
}

