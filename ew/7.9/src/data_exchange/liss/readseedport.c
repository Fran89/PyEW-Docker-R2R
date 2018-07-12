/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readseedport.c 23 2000-03-05 21:49:40Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:46:07  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * client program to catch data from dpnetport
 */
#ifdef _WINNT
#include <windows.h>
#include <winsock.h>               /* Socket stuff */
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#endif

#include <dcc_std.h>
#include "dumpseed_proto.h"

int seed_size;

unsigned char buf[8192];
int bufpop=0,jettison=0;

void process_record(char *inseed);

_SUB void procdata()
{

  for (;;) {
    if (bufpop>=seed_size) {
      process_record((char *) buf);
      memcpy(buf,&buf[seed_size],seed_size);
      bufpop -= seed_size;
      continue;
    }
    return;
  }
}


_SUB void process_telemetry(char *host, int port, int bufsize)
{
  struct sockaddr_in sin;
  struct hostent *hp;
#ifndef _WINNT
  struct hostent *gethostbyname();
#endif
  int fd, count;

  seed_size = bufsize;

#ifdef _WINNT
  {
    int     status;
    WSADATA Data;
    
    status = WSAStartup( MAKEWORD(2,2), &Data );
    if ( status != 0 )
      {
	fprintf(stderr, "WSAStartup failed. Exitting.\n" );
	exit( -1 );
      }
  }
#endif

    printf("[Connecting to remote host: %s (port %d) blocksize %d]\n",
	 host,port,bufsize);

  /*
   * Translate the hostname into an internet address
   */
  hp = gethostbyname(host);
  if(hp == 0) {
    fprintf(stderr, "%s: unknown host\n",host);
    exit(1);
  }
  /*
   * We now create a socket which will be used for our connection.
   */
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0) {
    perror("socket");
    exit(1);
  }

  /*
   *  Before we attempt a connect, we fill up the "sockaddr_in" structure
   */
  memcpy((char *)&sin.sin_addr, hp->h_addr , hp->h_length);
  sin.sin_family      = AF_INET;
  sin.sin_port        = htons((unsigned short)port);
  /*
   *  Now attempt to connect to the remote host .....
   */
  if(connect(fd, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
    perror("connect");
    exit(1);
  }

  fprintf(stderr,"Connected to remote host %s port %d\n",host,port);
  fflush(stderr);

  bufpop = 0;

  printf("Connected to %s\n",hp->h_name);
  for (;;) {

    /*
     * Read from the network
     */
    count = recv(fd, &buf[bufpop], 256, 0);
    if(count < 0) {
      /* Unable to read from the terminal */
      perror("network read");
      exit(1);
    }
    if (count==0) {
      printf("End of file\n");
      break;
    }
	
    bufpop += count;

    if (bufpop>=seed_size) procdata();

    /*
     * Continue the loop
     */
  }                  

  return;
}





