
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rb_sol.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.3  2004/06/25 18:27:27  dietz
 *     modified to work with TYPE_TRACEBUF2 and location code
 *
 *     Revision 1.2  2001/04/27 00:54:54  kohler
 *     Implemented option for MSS serial port logout.
 *
 *     Revision 1.1  2001/04/26 17:48:56  kohler
 *     Initial revision
 *
 *
 *
 */
       /****************************************************
        *                     rb_sol.c                     *
        *                                                  *
        *  This file contains the SpawnRebootMSS and       *
        *  TestChild functions                             *
	*  Valid for both Linux and Solaris                *
        ****************************************************/


#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <earthworm.h>
#include "reboot_mss_ew.h"


      /*****************************************************
       *                   SpawnRebootMSS()                *
       *            Spawn the reboot_mss command.          *
       *                                                   *
       *  Returns  0 if successful                         *
       *          -1 if the process couldn't be spawned    *
       *****************************************************/

int SpawnRebootMSS( char ProgName[], SCNL *scnlp, int Logout ) {
  pid_t pid = fork();
  
  switch( pid ){
  case -1:    /* fork failed */
    logit( "t", "Error spawning reboot process: %s\n", strerror(errno) );
    return -1;
    
  case 0:     /* in new child process */
    if ( Logout == 0 )
      execlp(ProgName,ProgName,scnlp->mss_ip,scnlp->mss_apwd,scnlp->mss_ppwd,"-q",(char *)0);
    else if ( Logout == 1 )
      execlp(ProgName,ProgName,scnlp->mss_ip,scnlp->mss_apwd,scnlp->mss_ppwd,"-q","-l",(char *)0);
    logit( "t", "execlp() failed: %s\n", strerror(errno) );
    return -1;
    
  default:    /* in parent, pid is PID of child */
    break;
  }
  
  scnlp->pid = pid;       /* Save process id */
  return 0;              /* Reboot succeeded */
}


      /**********************************************************
       *                       TestChild()                      *
       *          See if a child process has completed          *
       *                                                        *
       *  Returns  2 if child completed and exitCode is set.    *
       *           1 if child completed and exitCode not set.   *
       *           0 if child process hasn't completed          *
       *          -1 if an error occured.                       *
       **********************************************************/

int TestChild( SCNL *scnlp, int *exCode )
{
   int   status;
   pid_t rc;

   rc = waitpid( scnlp->pid, &status, WNOHANG  );

   if ( rc == -1 )              /* We shouldn't see this */
   {
      logit( "t", "reboot_mss_ew: waitpid() error: %s\n", strerror(errno) );
      return -1;
   }
   else if ( rc == 0 )          /* Child process is still alive */
      return 0;

   if ( WIFEXITED( status ) )
   {
      *exCode = WEXITSTATUS( status );
      return 2;
   }
   else
      return 1;
}

