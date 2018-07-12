
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: feedeqcoda.c 1489 2004-05-18 20:21:00Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/18 20:21:00  dietz
 *     Modified to use TYPE_EVENT_SCNL as input and to include location
 *     codes in the TYPE_HYP2000ARC output msg.
 *
 *     Revision 1.1  2001/12/12 19:19:22  dietz
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 17:13:57  lucky
 *     Initial revision
 *
 *
 */


/*
 *  feedeqcoda:  Stub program for testing eqcoda.
 *    Reads from stdin the names files containing a single 
 *    TYPE_EVENT_SCNL message each.  Reads the contents
 *    of the file and pipes it to the command "eqcoda eqcoda.d"
 *
 *  Example startup command:
 *     ls *.event | feedeqcoda    (processes all .event files in cwd)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>

int main( int argc, char *argv[] )
{
   static char   msg[MAX_BYTES_PER_EQ]; /* message from arcfile   */
   char          line[256];             /* one line at a time     */
   char          fname[50];             /* archive file name      */
   char          junk; 
   int           n;                     /* working pointer to msg */
   int           length;
   FILE         *fp;
   unsigned char TypeEventSCNL;
   unsigned char TypeKill;

/* Look up message types in earthworm.h tables
 *********************************************/
   GetType( "TYPE_EVENT_SCNL", &TypeEventSCNL );
   GetType( "TYPE_KILL",       &TypeKill );

/* Spawn the next process
 ************************/
   if ( pipe_init( "eqcoda eqcoda.d", (unsigned long)0 ) == -1 )
   {
      printf( "feedeqcoda: Error starting eqcoda; exiting!\n" );
      exit ( -1 );
   }
   printf( "feedeqcoda: piping output to eqcoda\n" );

/* Loop over all file names fed to stdin
 ***************************************/
   while( scanf( "%s", fname ) != EOF )
   {
   /* Open next file
    ************************/
      if( (fp=fopen( fname, "r")) == (FILE *) NULL ) 
      {
         printf( "feedeqcoda: error opening file <%s>\n", fname );
         continue;
      }

   /* Read event message from file
    *******************************/
      n=0;
      while( n < MAX_BYTES_PER_EQ-1 )
      {
         if( fscanf( fp, "%[^\n]", line ) == EOF )  {
              break;
         }
         fscanf( fp, "%c", &junk ); /*read newline*/
         length = strlen(line);
         strncpy( msg+n, line, length );
         n += length;
         msg[n++] = '\n';
      } 
      msg[n] = '\0';
      /*printf("Next event:\n%s", msg );*/ /*DEBUG*/

   /* Send msg down pipe
    ********************/
      if ( pipe_put( msg, TypeEventSCNL ) != 0 )
         printf( "feedeqcoda: Error writing msg to pipe.\n"); 

      sleep_ew(1000);
      fclose(fp);
   }

   if ( pipe_put( "", TypeKill ) != 0 )
       printf( "feedeqcoda: Error writing kill msg to pipe.\n");

   exit( 0 );
}
