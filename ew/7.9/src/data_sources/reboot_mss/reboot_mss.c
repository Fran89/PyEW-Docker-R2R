
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: reboot_mss.c 3762 2010-01-02 00:39:51Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2010/01/02 00:39:51  kress
 *     bookeeping on gcf2ew.  reboot_mss and reboot_mss_ew now both work in linux.
 *
 *     Revision 1.2  2001/04/26 22:32:25  kohler
 *     Added -l option to logout MSS100 serial port.
 *
 *     Revision 1.1  2001/04/25 23:44:18  kohler
 *     Initial revision
 *
 *
 *
 */
       /**********************************************************
        *                       reboot_mss                       *
        *                                                        *
        *              Program to reboot an MSS100               *
        *             or to log off its serial port.             *
        *                                                        *
        *  Usage: reboot_mss [IP address] [password] [-q] [-l]   *
        **********************************************************/

#include <stdio.h>
#include <string.h>

int  RebootMSS100( char [], char [], char [] );
void SocketSysInit( void );

int Quiet  = 0;                  /* If 0, don't print anything to stdout */
int Logout = 0;                  /* If 0, reboot MSS100.  If 1, logout port 1 */

void printUsage() {
  printf( "\nUsage: reboot_mss <IP address> [<apassword>] <ppassword> [-q] [-l]\n\n" );
  printf( "apassword is an optional access password (defaults to 'access')\n" );
  printf( "ppassword is required priviledged password.\n");
  printf( "The -q (quiet) and -l (logout) arguments are optional, and,\n" );
  printf( "if present, they must appear at the end of the command line.\n" );
  printf( "If -q is specified, nothing is written to stdout.\n" );
  printf( "If -l is specified, the MSS serial port is logged out,\n" );
  printf( "and no reboot occurs.\n" );
  return;
}

int main( int argc, char *argv[] ){
  int  i,npwd;
  char ServerIP[20];            /* IP address of the MSS to reboot */
  char aPassword[20];           /* Primary access password */
  char pPassword[20];           /* Password of priviledged MSS user */
  
  /* Get command line arguments
**************************/
  if ( argc < 3 ) {
    printUsage();
    return -1;
  }
  
  strcpy( ServerIP, argv[1] );
  
  for ( i = 3; i < argc; i++ ) {
    if ( !strcmp(argv[i],"-q") ) Quiet  = 1;
    if ( !strcmp(argv[i],"-l") ) Logout = 1;
  }
  npwd=argc-Quiet-Logout-2;
  if (npwd>=2) {
    strcpy( aPassword, argv[2] );
    strcpy( pPassword, argv[3] );
  } else if (npwd==1) {
    strcpy( aPassword, "access" );
    strcpy( pPassword, argv[2] );
  } else {
    printUsage();
    return -1;
  }


  /* Initialize the socket system
****************************/
  SocketSysInit();
  
  /* Invoke the reboot function
**************************/
  if ( RebootMSS100( ServerIP, aPassword, pPassword ) < 0 ) {
    if ( !Quiet )
      if ( Logout == 0 )
	printf( "Error rebooting the MSS100.\n" );
      else if ( Logout == 1 )
	printf( "Error logging off the MSS100 serial port.\n" );
    return -1;
  }
  
  /* Success!
********/
  if ( !Quiet )
    if ( Logout == 0 )
      printf( "The MSS100 was successfully rebooted.\n" );
    else if ( Logout == 1 )
      printf( "The MSS100 serial port was successfully logged off.\n" );
  
  return 0;
}
