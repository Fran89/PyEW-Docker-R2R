
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arcfeeder.c 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:40:56  lucky
 *     Initial revision
 *
 *
 */

/*
 *  arcfeeder:  reads from stdin the names of single-event or
 *              multiple-event archive files.  Reads one event
 *              at a time from the archive file, pipes it to 
 *              command "hyp2000_mgr hyp2000_mgr.d ncal2000.hyp"
 *
 *  Example startup command:
 *     ls *.arc | arcfeeder    (processes all .arc files in cwd)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>

main( int argc, char *argv[] )
{
   static char   msg[MAX_BYTES_PER_EQ];  /* message from arcfile   */
   char          line[200];              /* one line at a time     */
   char          fname[50];              /* archive file name      */
   char          junk; 
   int           n;                      /* working pointer to msg */
   int           nline;
   int           length;
   int           endoffile;
   FILE         *fparc;
   unsigned char TypeHyp2000Arc;
   unsigned char TypeKill;

/* Spawn the next process
 ************************/
   if ( pipe_init( "hyp2000_mgr hyp2000_mgr.d ncal2000.hyp",
        (unsigned long)0 ) == -1 )
   {
      printf( "arcfeeder: Error starting hyp2000_mgr; exiting!\n" );
      exit ( -1 );
   }
   printf( "arcfeeder: piping output to hyp2000_mgr\n" );

/* Look up message types in earthworm.h tables
 *********************************************/
   GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc );
   GetType( "TYPE_KILL",   &TypeKill );

/* Loop over all .arc file names fed to stdin
 ********************************************/
   while( scanf( "%s", fname ) != EOF )
   {
   /* Open next archive file
    ************************/
      if( (fparc=fopen( fname, "r")) == (FILE *) NULL ) 
      {
         printf( "arcfeeder: error opening file <%s>\n", fname );
         continue;
      }

      endoffile = 0;
      while( !endoffile )
      {
      /* Read next archive message from file
       *************************************/
         n=0;
         nline = 0;
         while( n < MAX_BYTES_PER_EQ-1 )
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

   /* Send msg down pipe
    ********************/
         if ( pipe_put( msg, TypeHyp2000Arc ) != 0 )
            printf( "arcfeeder: Error writing msg to pipe.\n"); 

         sleep_ew(100);
      }
      fclose(fparc);
   }

   if ( pipe_put( "", TypeKill ) != 0 )
       printf( "arcfeeder: Error writing kill msg to pipe.\n");

   exit( 0 );
}
