
/**************************************************************
 *                          thread.c                          *
 *          Thread function started by sendfilemt.            *
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include "sendfilemt.h"

int SendOneFile( unsigned int i );
int GetFileNameAndOpenFile( unsigned int i, char fname[], FILE **fp );
void EraseLocalFile( unsigned int i, char fname[] );

extern SERVER *srv;        // Structure of server info


void *ThreadFunction( void *thread_arg )
{

   while ( 1 )
   {
      unsigned int i = (unsigned int)thread_arg;
      extern int  KillThreadFlag;   // If 1, exit thread.
      extern int  RetryInterval;    // Retry after this many seconds
      int rc;

/* The main program wants this thread to exit
   ******************************************/
      if ( KillThreadFlag ) KillSelfThread();

/* If a file is available, send it
   *******************************/
      rc = SendOneFile( i );

      if ( rc == SENDFILE_NOFILE )    // No file in output directory
         sleep_ew( 200 );

      if ( rc == SENDFILE_SUCCESS )   // File found and sent to getfileII
         sleep_ew( 200 );

      if ( rc == SENDFILE_FAILURE )   // File found but could not be sent
      {
         if ( KillThreadFlag ) KillSelfThread();
//       logit( "et", "%-12s Will retry in %d sec...\n", srv[i].tag,
//              RetryInterval );
         sleep_ew( 1000 * RetryInterval );
      }
   }
}


int SendOneFile( unsigned int i )
{
   char fname[100];
   FILE *fp;
   int  send_failed = 0;
   int  rc;
   int  sd;                   // Socket descriptor

/* Get name of file to send and open the file.
   GetFileNameAndOpenFile isn't MT-safe, so it's
   protected with a mutex.  fname and fp are on
   the stack, so they don't need to be protected.
   *********************************************/
   RequestMutex();
   rc = GetFileNameAndOpenFile( i, fname, &fp );
   ReleaseMutex_ew();
   if ( rc == SENDFILE_NOFILE ) return SENDFILE_NOFILE;
// logit( "et", "%-12s Found file to send: %s\n", srv[i].tag, fname );

/* If it is a 'core' file, just delete it.
   These should never be sent.
   *****************************************/
   if(fname == "core") {
	   logit( "et", "%-12s Deleting file %s without sending.\n", srv[i].tag, fname );
/* Erase it.  EraseLocalFile()
   isn't MT-safe, so it's protected with a mutex.
   *********************************************/
	   RequestMutex();
	   EraseLocalFile( i, fname );
	   ReleaseMutex_ew();
	   return SENDFILE_SUCCESS;
   }

/* Connect to getfileII and retrieve socket descriptor.
   Socket descriptor stored as local variable here.
   ***************************************************/
   logit( "et", "%-12s Connecting to %s port %d\n", srv[i].tag, srv[i].ip,
          srv[i].port );
   if ( ConnectToGetfile(srv[i].ip, srv[i].port, &sd) == SENDFILE_FAILURE )
   {
      logit( "et", "%-12s Connection failed.\n", srv[i].tag );
      fclose( fp );
      return SENDFILE_FAILURE;
   }

/* Send file name to getfileII
   ***************************/
// logit( "et", "%-12s Sending file %s\n", srv[i].tag, fname );
   if ( SendBlockToSocket( sd, fname, strlen(fname) ) == SENDFILE_FAILURE )
   {
      logit( "et", "%-12s Can't send file name to getfileII.\n", srv[i].tag );
      fclose( fp );
      return SENDFILE_FAILURE;
   }

/* Send BUFLEN bytes at a time to getfileII
   ****************************************/
   while ( 1 )
   {
      char buf[BUFLEN];             // Work buffer
      int  nbytes = fread( buf, sizeof(char), BUFLEN, fp );

      if ( nbytes > 0 )
      {
         if ( SendBlockToSocket( sd, buf, nbytes ) == SENDFILE_FAILURE )
         {
            logit( "et", "%-12s Error sending block to socket.  nbytes = %d\n",
                   srv[i].tag, nbytes );
            send_failed = 1;
            break;
         }
      }

/* End-of-file encountered on read.
   Send a zero-length block to getfileII.
   *************************************/
      if ( feof( fp ) )
      {
         if ( SendBlockToSocket( sd, buf, 0 ) == SENDFILE_FAILURE )
         {
            logit( "et", "%-12s Error sending zero-length block to socket.\n",
                   srv[i].tag );
            send_failed = 1;
         }
         break;         // Success!
      }

/* If an fread error occurs, kill this thread
   ******************************************/
      if ( (nbytes == 0) && ferror( fp ) )
      {
         logit( "et", "%-12s fread() error on file %s.\n", srv[i].tag, fname );
         logit( "et", "%-12s File partially sent. Thread exiting.\n", srv[i].tag );
         KillSelfThread();
      }
   }

/* Wait for acknowledgement from receiver
   **************************************/
   if ( GetAckFromSocket(sd) == SENDFILE_FAILURE )
      send_failed = 1;

/* Finish up with this file
   ************************/
   CloseSocketConnection( sd );
   fclose( fp );

   if ( send_failed )             // Sleep a while
   {
      logit( "et", "%-12s Send of %s failed.\n", srv[i].tag, fname );
      return SENDFILE_FAILURE;
   }
   logit( "et", "%-12s File %s sent.\n", srv[i].tag, fname );   // Success

/* Erase local copy of file.  EraseLocalFile()
   isn't MT-safe, so it's protected with a mutex.
   *********************************************/
   RequestMutex();
   EraseLocalFile( i, fname );
   ReleaseMutex_ew();
   return SENDFILE_SUCCESS;
}


     /*********************************************************
      *                 GetFileNameAndOpenFile                *
      *                                                       *
      *  Change working directory to outDir, and get name of  *
      *  file to send to getfileII.  Then, open the file.     *
      *  If chdir fails, kill the entire process.             *
      *  fname and fp are returned to calling function.       *
      *********************************************************/

int GetFileNameAndOpenFile( unsigned int i, char fname[], FILE **fp )
{
   extern int SendPause;      // Pause before opening file (msec)

   if ( chdir_ew( srv[i].outDir ) == -1 )
   {
      logit( "et", "%-12s Unknown output directory %s\n", srv[i].tag,
              srv[i].outDir );
      logit( "et", "%-12s sendfilemt exiting.\n", srv[i].tag );
      exit( 0 );
   }
   if ( GetFileName( fname ) == 1 )   // No files available
      return SENDFILE_NOFILE;

/* Pause to allow file being sent to be completely written
   *******************************************************/
   if ( SendPause ) sleep_ew( SendPause );

/* Open the file for reading only.  Since the file name is in the
   directory we know the file exists, but it may be in use by another
   process.  If so, fopen_excl() will return NULL.  In this case,
   wait a second for the other process to finish and try opening the
   file again.  fopen_excl works in Windows, but in Solaris fopen_excl
   just does a regular non-exclusive fopen.
   *******************************************************************/
   *fp = fopen_excl( fname, "rb" );
   if ( *fp == NULL )                  // Can't open the file
      return SENDFILE_NOFILE;
   else
      return SENDFILE_SUCCESS;
}

     /*********************************************************
      *                     EraseLocalFile                    *
      *                                                       *
      *  Change working directory to outDir, and erase file   *
      *  sent to getfileII.                                   *
      *********************************************************/

void EraseLocalFile( unsigned int i, char fname[] )
{
   if ( chdir_ew( srv[i].outDir ) == -1 )
       logit( "et", "%-12s chdir_ew error to directory: %s\n",
              srv[i].tag, srv[i].outDir );
   if ( remove( fname ) == -1 )
       logit( "et", "%-12s Error erasing: %s\n", srv[i].tag, fname );
   return;
}
