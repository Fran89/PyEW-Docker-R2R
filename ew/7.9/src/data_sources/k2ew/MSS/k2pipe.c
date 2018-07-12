/*******************************************************************
 *
 *    K2pipe.c
 *
 *  TCP server running under PUC for K2 comms. Provides a two-way
 *  pipe between a K2 on the serial port and a computer running k2ew
 *  on the TCP port. Expects traffic from k2ew_tcp periodically; closes
 *  and reopens socket if nothing heard within timeout interval.
 *  Code is based on Stevens' tcpcliserv/tcpserv01.c
 *  This code is very experimental; use at your own risk!
 *  Pete Lombard, 9/1/00
 *
 * BE SURE TO DO "CHANGE ACCESS DYNAMIC" OR YOU WON'T READ FROM SERIAL
 *
 *******************************************************************/

#include "unp.h"
#include "k2pipe.h"

/* automatically include needed c files in PUC */
#ifndef NO_PUC
#include "wrapper.c"
#endif

/* valid return values from your serIn or netIn functions */
#define RET_OK		0
#define RET_STOPSOCK	1
#define RET_STOPSER	-2
#define RET_STOPNET	-3
#define RET_STARTSER	2
#define RET_STARTNET	3	

/* Timeout for trying to write to network or serial ports, milliseconds */
#define WRITETIMEOUT 1000

/* function prototypes */
int do_socket(int sockfd);
int serIn(char *buf, int nbytes);
int netIn(char *buf, int nbytes);
int serOut(const char *buf, int nbytes);
int netOut(const char *buf, int nbytes);
int writeOut(const char *buf, int nbytes, char *outBuf, int *outEnd,
	     int outfd);

/* private variables */
static int 	_serfd, _netfd, _serOutEnd, _netOutEnd;
static char 	_serOutBuf[RECSIZE], _netOutBuf[RECSIZE];

/* Pending-output controls */
static int net_pend = 0, ser_pend = 0;

/* Send data to serial port.  Will write all or none of the
   requested bytes, returns 0 on success. */

int main()
{
  int        listenfd, connfd;
  int        rv=0;
  socklen_t  clilen;
  SA_in	   cliaddr, servaddr;

  /* so we don't have to worry about flushing stdout */
  setbuf(stdout, NULL);

  /* set up listening socket */
  listenfd = Socket(AF_INET, SOCK_STREAM, 0);
  /* zero out structure */
  bzero(&servaddr, sizeof(servaddr));
  /* fill in structure */
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port        = htons(K2EW_PORT);
  /* bind and listen to port */
  Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
  Listen(listenfd, LISTENQ);

  /* look for clients */
  clilen = sizeof(cliaddr);
  while (rv == 0) 
  {
    if (DEBUG)
      printf("waiting for connection\n\r");

    connfd = Accept(listenfd, &cliaddr, &clilen);
    if (DEBUG)
      printf("accepted socket\n\r");

    /* process the request */
    rv = do_socket(connfd);

    if (DEBUG)
      printf("closing socket\n\r");

    Close(connfd);
  }
  /* close the listening socket */
  Close(listenfd);
  return rv;
}

int do_socket(int sockfd)
{
  ssize_t    n;
  int        resp;
  char       serInBuf[RECSIZE], netInBuf[RECSIZE]; 
  char       *stop_ptr;
  int        loop_on = 1, ser_on = 1, net_on = 1; 
  int	       serInEnd = 0, netInEnd = 0;
  ulong      transferred = 0;
  clock_t    clock_now, clock_last;
  int newset;
  
  /* make sure we have a valid socket fd */
  if (sockfd < 1)
    return -2;

  /* set up nonblocking serial i/o */
  _serfd = open("tt0:", O_RDWR, 0);
  if (_serfd < 0)
    return -2;
  fcntl(_serfd, F_SETFL, O_NONBLOCK);

  /* Maybe we need to set the erial port to hardware flow control? */
  /* Set speed 19200, 8 bits, no parity (def), one stop (def), RTSCTS flow */
  newset = B57600 | CS8 | CRTSCTS;
  ioctl(_serfd, IO_STTY, &newset);
  
  /* set network socket to nonblocking mode */
  fcntl(sockfd, F_SETFL, O_NONBLOCK);
  _netfd = sockfd;

  _serOutEnd = 0;
  if (DEBUG)
    printf("ser %d sock %d\n\r", _serfd, sockfd);
    
  clock_last = clock();
  while(loop_on) 
  {
    if ( (DEBUG > 1 && (serInEnd || netInEnd )) || 
         (DEBUG &&  (_serOutEnd || _netOutEnd )))
      printf("\t\t\t%d %d %d %d\n\r", serInEnd, netInEnd,
             _serOutEnd, _netOutEnd);
    
    /* read from serial */
    n = 0;
    if (ser_on == 1) 
    {
      if ((serInEnd = read(_serfd, serInBuf, RECSIZE)) > 0) 
      {
        transferred += serInEnd;
        n = serInEnd;
        clock_last = clock();
      }
      else if (serInEnd < 0) 
      {
        if (DEBUG)
          printf("serial read error\n\r");
        loop_on = 0;
        break;
      }
    }
    else if (ser_on == -1)
    {
      /* If we ever stopped, we will re-send the latest
         data when restarted */
      ser_on = 1;
      n = serInEnd;
    }
    /* Process serial input: send it to the net */
    resp = serIn(serInBuf, n);
    switch (resp) 
    {
    case RET_STOPSOCK: 
      {   /* Error sending to network, so quit */
        if (DEBUG)
          printf("net output error; stopping loop\n\r");
        loop_on = 0;
        break;
      }
    case RET_STOPSER: 
      {
        /* Can't write to socket, so stop reading from serial */
        if (DEBUG)
          printf("serial blocked\n\r");
        /* See what serial flow control settings are doing */
        if (DEBUG)
        {
          int ret;
          
          ret = ioctl(_serfd, IO_GSTATUS, &newset);
          printf("ser stat: %d\n\r", newset);
        }
        ser_on = 0;
        break;
      }
    case RET_STARTSER: 
      {
        /* Network OK, resume serial if it was blocked */
        if (!ser_on) 
        {
          if (DEBUG)
            printf("serial unblocked\n\r");
          
          ser_on = -1;
        }
        break;
      }
    default:     /* RET_OK */
      break;
    }
    
    if (!loop_on)
      break;
    
    /* read network data */
    n = 0;
    if (net_on == 1) 
    {
      /* read new data */
      if ((netInEnd = recv(sockfd, netInBuf, RECSIZE, 0)) > 0) 
      {
        transferred += netInEnd;
        n = netInEnd;
      }
      else if (netInEnd < 0) 
      {
        /* recv error probably means client disconnected, 
           so end loop */
        if (DEBUG)
          printf("net read error; stopping loop\n\r");
        loop_on = 0;
        break;
      }
    }
    else if (net_on == -1) 
    {
      /* if we are ever stopped, we will re-send the latest
         data when restarted. */
      net_on = 1;
      n = netInEnd;
    }
    
    /* process network input: send to serial ouput */
    resp = netIn(netInBuf, n);
    switch (resp) 
    {
    case RET_STOPSOCK:   /* Error writing to serial; quit */
      {
        if (DEBUG)
          printf("serial output error; stopping loop\n\r");
        loop_on = 0;
        break;
      }
    case RET_STOPNET:    /* serial port blocked, so block network */
      {
        if (DEBUG)
          printf("network blocked\n\r");
        
        net_on = 0;
        break;
      }
    case RET_STARTNET:   /* serial port OK, so unblock network */
      {
        if (!net_on) 
        {
          if (DEBUG)
            printf("network unblocked\n\r");
          
          net_on = -1;
        }
        break;
      }
    default:   /* RET_OK */
      break;
    }
  }
    
  if (DEBUG)
    printf("Transferred %u bytes\n\r", transferred);
  
  close (_serfd);
  return 0;
}


/* 
   Handle incoming serial data: send it out the net
*/
int serIn(char *buf, int nbytes)
{
  int resp;
  static ulong pend_time;
  ulong current_time;

  if (nbytes)
    if (DEBUG > 1)
      printf("serial  %d bytes\n\r", nbytes);

  /* send everything to the network socket */
  resp = netOut(buf, nbytes);
  if (resp == -4) 
  {        /* Socket reported data pending to send */
    if (DEBUG)
      printf("socket output pending \n\r");
    if (net_pend)
    {  /* Pending flag set previously; has our timer run out? */
      current_time = time(NULL);
      if (DEBUG)
        printf("socket pend timer %d\n\r", current_time - pend_time);
      if (current_time - pend_time > MAX_PEND_SEC)
        return RET_STOPSOCK;  /* Timer expired; close up shop */
      sleepMS(500);   /* The pause that refreshes */
    }
    else
    {  /* pending flag not set previously; set it now */
      net_pend = 1;
      pend_time = time(NULL);
    }
    return RET_STOPSER;
  }

  if (resp == -3) 
  {
    if (DEBUG)
      printf("network out write error\n\r");

    return RET_STOPSOCK;
  }
  net_pend = 0; /* nothing pending in socket, so clear the flag */

  if (resp < 0)
    /* netOut couldn't write all our data to the network port, so 
       let's stop reading serial data until some of the network buffer
       has cleared. */
    return RET_STOPSER;
  if (resp == 2)
    /* When we return a RET_STARTSER, we'll next be called with the
       original data that caused a RET_STOPSER */
    return RET_STARTSER;
    
  return RET_OK;
}


/* 
   Handle incoming network data: send it to serial output.
*/
int netIn(char *buf, int nbytes)
{
  int resp;
  static ulong pend_time;
  ulong current_time;

  if (nbytes) 
    if (DEBUG > 1)
      printf("network %d bytes\n\r", nbytes);
    
  /* send everything to the serial port */
  resp = serOut(buf, nbytes);
  if (resp == -4) 
  {        /* Serial device reported data pending to send */
    if (DEBUG)
      printf("serial output pending \n\r");
    if (ser_pend)
    {  /* Pending flag set previously; has our timer run out? */
      current_time = time(NULL);
      if (DEBUG)
        printf("serial pend timer %d\n\r", current_time - pend_time);
      if (current_time - pend_time > MAX_PEND_SEC)
        return RET_STOPSOCK;  /* Timer expired; close up shop */
      sleepMS(500);  /* The pause that refreshes */
    }
    else
    {  /* pending flag not set previously; set it now */
      ser_pend = 1;
      pend_time = time(NULL);
    }
    return RET_STOPNET;
  }

  if (resp == -3) 
  {
    if (DEBUG)
      printf("serial out write error\n\r");

    return RET_STOPSOCK;
  }
  ser_pend = 0; /* nothing pending in serial device, so clear the flag */
  
  if (resp < 0)
    /* serOut couldn't write all our data out the serial line, so 
       let's stop reading network data until some of the serial buffer
       has cleared. */
    return RET_STOPNET;
  if (resp == 2)
    /* When we return a RET_STARTNET, we'll next be called with the
       original data that caused a RET_STOPNET */
    return RET_STARTNET;
    
  return RET_OK;
}

/* Send data to serial port.  Will write all or none of the
   requested bytes, returns 0 on success. */
int serOut(const char *buf, int nbytes)
{
  int rv;

  /* if we blocked but wrote some data, try again. */
  while ((rv = writeOut(buf, nbytes, _serOutBuf, &_serOutEnd, _serfd)) == -2)
  {}
  return rv;
} 


/* Send data to network port.  Will write all or none of the
   requested bytes, returns 0 on success. */
int netOut(const char *buf, int nbytes)
{
  int rv;

  /* if we blocked but wrote some data, try again. */
  while ((rv = writeOut(buf, nbytes, _netOutBuf, &_netOutEnd, _netfd)) == -2)
  {}
  return rv;
} 


/* Add data to output buffer, and try to send some of the data to
   the filedescriptor */
int writeOut(const char *buf, int nbytes, char *outBuf, int *outEnd, int outfd)
{
  int outpend, rv = 1;
    
  if ((*outEnd + nbytes) == 0)
    return 0;

  if (DEBUG > 1)
    printf("req %d ", nbytes);

  /* make sure there's enough buffer room */
  if ((*outEnd + nbytes) > RECSIZE)
    rv = -1;
  /* append onto buffer */
  else if (nbytes) 
  {
    memcpy(outBuf + *outEnd, buf, nbytes);
    *outEnd += nbytes;
    rv = 0;
  }

  /* See if there is anything pending in the output device */
  outpend = ioctl(outfd, IO_OUTPEND, NULL);
  if (DEBUG && outpend > 0)
    printf("%d pending %d\n\r", outfd, outpend);
  if (outpend > PENDMAX )
    return -4;
  
  /* service the buffer */
  if ((nbytes = write(outfd, outBuf, *outEnd)) < 0) 
    return -3;

  /* move any remaining data to the beginning of the buffer */
  else if (nbytes) 
  {
    *outEnd -= nbytes;
    memcpy(outBuf, outBuf + nbytes, *outEnd);
    rv *= 2;
  }
    
  if (DEBUG > 1)
    printf("sent %d rv=%d\n\r", nbytes, rv); 

  /* return values:
     0 = bytes requested have been buffered
     -1= request rejected - buffer too full, no buffer sent
     -2= request rejected - buffer too full, some buffer sent
     -3= write error
     -4= output device has data pending
     1 = no bytes requested, no buffer sent
     2 = no bytes requested, some buffer sent
  */
  return rv;
}


