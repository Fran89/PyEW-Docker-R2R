
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arcfile2ring.c 553 2001-04-17 22:13:35Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2001/04/17 22:13:35  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 18:56:41  lucky
 *     Initial revision
 *
 *
 */

/*
 *  arcfile2ring:  reads from stdin the names of single-event or
 *              multiple-event archive files.  Reads one event
 *              at a time from the archive file, writes it to the
 *              given transport ring (argv[1]) as module the 
 *              given moduleid (argv[2]), as msg type TYPE_HYP2000ARC.
 *              This allows one to test modules such as menlo_report
 *              without running the entire system.
 *
 *  Example startup command (processes all .arc files in cwd):
 *     ls *.arc | arcfile2ring HYPO_RING MOD_EQPROC  
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>

#define BUF_SIZE MAX_BYTES_PER_EQ

SHM_INFO      Region1;       /* main shared memory region   */
static long   RingKey;       /* key to transport ring       */
MSG_LOGO      Logo;          /* array of module,type,instid */

static unsigned char MyModId;        /* module id to use      */
static unsigned char InstId;         /* local installation id */
static unsigned char TypeHyp2000Arc; /* message type          */

main( int argc, char *argv[] )
{
   static char   msg[BUF_SIZE];    /* message from arcfile   */
   char          line[200];        /* one line at a time     */
   char          fname[50];        /* archive file name      */
   char          junk;
   int           n;                /* working pointer to msg */
   int           nline;
   int           length;
   int           endoffile;
   long          size;
   FILE         *fparc;

/* Check command line arguments
   ****************************/
   if ( argc != 3 )
   {
        fprintf( stderr, "Usage:   arcfile2ring <ring_name> <module_id>\n" );
        fprintf( stderr, "Example: arcfile2ring HYPO_RING MOD_EQPROC\n" );
        exit( 0 );
   }

/* Look up info from earthworm.h tables
   ************************************/
   if( (RingKey = GetKey(argv[1])) == -1 ) {
        fprintf( stderr,
                "arcfile2ring: Invalid ring name: %s; exiting!\n",
                 argv[1] );
        exit( -1 );
   }
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr,
              "arcfile2ring: error getting local installation id; exiting!\n" );
      exit( -1 );
   }
   if ( GetModId( argv[2], &MyModId ) != 0 ) {
      fprintf( stderr,
              "arcfile2ring: Invalid module name: %s; exiting!\n",
               argv[2] );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      fprintf( stderr,
              "arcfile2ring: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }

/* Attach to public shared memory ring
 *************************************/
   tport_attach( &Region1, RingKey );
   Logo.instid = InstId;
   Logo.mod    = MyModId;
   Logo.type   = TypeHyp2000Arc;

/* Loop over all .arc file names fed to stdin
 ********************************************/
   while( scanf( "%s", fname ) != EOF )
   {
   /* Open next archive file
    ************************/
      if( (fparc=fopen( fname, "r")) == (FILE *) NULL )
      {
         printf( "arcfile2ring: error opening file <%s>\n", fname );
         continue;
      }

      endoffile = 0;
      while( !endoffile )
      {
      /* Read next archive message from file
       *************************************/
         n=0;
         nline = 0;
         while( n < BUF_SIZE-1 )
         {
            if( fscanf( fparc, "%[^\n]", line ) == EOF )  {
                 endoffile = 1;
                 break;
            }
            fscanf( fparc, "%c", &junk ); /*read newline*/
            length = strlen(line);
            strncpy( msg+n, line, length );
            n += length;
            msg[n++] = '\n';
            nline++;
            if( length<75 && line[0]=='$' && nline>2)
                break;      /*found terminator line*/
         }
         msg[n] = '\0';
         if( endoffile ) continue;
         /*printf("Next event:\n%s", msg );*/ /*DEBUG*/

   /* Send archive message to transport ring
    ****************************************/
        size = strlen( msg );   /* don't include the null byte in the message */
        if( tport_putmsg( &Region1, &Logo, size, msg ) != PUT_OK )
            printf( "arcfile2ring: Error writing msg to transport ring.\n");

         sleep_ew(100);
      }
      fclose(fparc);
   }

   tport_detach( &Region1 );
   return(0);
}
