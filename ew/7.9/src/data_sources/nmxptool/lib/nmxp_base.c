/*! \file
 *
 * \brief Base for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_base.c 4965 2012-07-22 07:12:34Z quintiliani $
 *
 */

#include "config.h"
#include "nmxp_base.h"
#include "nmxp_memory.h"
#ifdef HAVE_WINDOWS_H
#include "nmxp_win.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_WINDOWS_H
#include "winsock2.h"
#warning You are compiling on Windows MinGW...
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#endif

#define MAX_OUTDATA 4096

int nmxp_openSocket(char *hostname, int portNum, int (*func_cond)(void))
{
  int sleepTime = 1;
  int isock = -1;
  struct hostent *hostinfo = NULL;
  struct sockaddr_in psServAddr;
  struct in_addr hostaddr;
  time_t timeStart;

#ifdef HAVE_WINDOWS_H
  nmxp_initWinsock();
#endif

  if (!hostname)
  {
    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Empty host name?\n");
    return -1;
  }

  if ( (hostinfo = gethostbyname(hostname)) == NULL) {
    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Cannot lookup host: %s\n",
	    NMXP_LOG_STR(hostname));
    return -1;
  }

  while(!func_cond())
  {
    isock = socket (AF_INET, SOCK_STREAM, 0);

#ifdef HAVE_WINDOWS_H
    if (isock == INVALID_SOCKET) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW,"Error at socket()\n");
	    WSACleanup();
	    exit(1);
    }
#else
    if (isock < 0)
    {
      nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Can't open stream socket\n");
      exit(1);
    }
#endif

    /* Fill in the structure "psServAddr" with the address of server
       that we want to connect with */
    memset (&psServAddr, 0, sizeof(psServAddr));
    psServAddr.sin_family = AF_INET;
    psServAddr.sin_port = htons((unsigned short) portNum);
#ifdef HAVE_WINDOWS_H
    unsigned long address;
    memcpy(&address, hostinfo->h_addr, (size_t) hostinfo->h_length);
    psServAddr.sin_addr.s_addr = address;
#else
    psServAddr.sin_addr = *(struct in_addr *)hostinfo->h_addr_list[0];
#endif

    /* Report action and resolved address */
    memcpy(&hostaddr.s_addr, *hostinfo->h_addr_list,
	   sizeof (hostaddr.s_addr));
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Attempting to connect to %s port %d\n",
	    NMXP_LOG_STR(inet_ntoa(hostaddr)), portNum);

    if(connect(isock, (struct sockaddr *)&psServAddr, sizeof(psServAddr)) >= 0) {
	sleepTime = 1;
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Connection established: socket=%i,IP=%s,port=%d\n",
		isock, NMXP_LOG_STR(inet_ntoa(hostaddr)), portNum);
	return isock;
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Connecting to %s port %d. Trying again after %d seconds...\n",
		NMXP_LOG_STR(inet_ntoa(hostaddr)), portNum, sleepTime);
	nmxp_closeSocket(isock);
	isock = -1;
	timeStart = time(NULL);

	while(!func_cond()  &&  (time(NULL) - timeStart) < sleepTime) {
	  nmxp_sleep (1);
	}

	sleepTime *= 2;
	if (sleepTime > NMXP_SLEEPMAX) {
	  sleepTime = NMXP_SLEEPMAX;
	}
    }

  }
  return isock;
}


int nmxp_closeSocket(int isock) {
	int ret;
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Closed connection.\n");
#ifdef HAVE_WINDOWS_H
	ret = closesocket(isock);
	WSACleanup();
#else
	ret = close(isock);
#endif
	return ret;
}


int nmxp_send_ctrl(int isock, void* buffer, int length)
{
  int sendCount = send(isock, (char*) buffer, length, 0);
  
  if (sendCount != length)
    return NMXP_SOCKET_ERROR;

  return NMXP_SOCKET_OK;
}

#ifdef HAVE_BROKEN_SO_RCVTIMEO
#warning Managing non-blocking I/O using select()
int nmxp_recv_select_timeout(int s, char *buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;
    static int message_times = 0;

    /* set up the file descriptor set*/
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    /* set up the struct timeval for the timeout*/
    if(timeout == 0) {
	if(message_times == 0) {
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "nmxp_recv_select_timeout(): timeout = %d\n", NMXP_HIGHEST_TIMEOUT);
	    message_times++;
	}
	timeout = NMXP_HIGHEST_TIMEOUT;
    }
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    /* wait until timeout or data received*/
    errno = 0;
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; /* timeout!*/
    if(errno == EINTR) return -2; /* timeout! "Interrupted system call" */
    if (n == -1) return -1; /* error*/
 
    /* data must be here, so do a normal recv()*/
    return recv(s, buf, len, 0);
}
#endif

int nmxp_setsockopt_RCVTIMEO(int isock, int timeoutsec) {
    int ret = 0;
#ifdef HAVE_WINDOWS_H
    int timeos;
#else

#ifndef HAVE_BROKEN_SO_RCVTIMEO
    struct timeval timeo;
#endif

#endif

    if(timeoutsec == 0) {
	timeoutsec = NMXP_HIGHEST_TIMEOUT;
    }

    if(timeoutsec > 0) {
#ifdef HAVE_WINDOWS_H
	timeos  = timeoutsec * 1000;
	ret = setsockopt(isock, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeos, sizeof(timeos));
	if (ret < 0)
	{
	    perror("setsockopt SO_RCVTIMEO");
	}
#else

#ifndef HAVE_BROKEN_SO_RCVTIMEO
	timeo.tv_sec  = timeoutsec;
	timeo.tv_usec = 0;
	ret = setsockopt(isock, SOL_SOCKET, SO_RCVTIMEO, &timeo, sizeof(timeo));
	if (ret < 0) {
	    perror("setsockopt SO_RCVTIMEO");
	}
#else
#warning nmxp_setsockopt_RCVTIMEO() do nothing for your system.
#endif

#endif
    }

    return ret;
}


#define MAXLEN_RECV_ERRNO_STR 200

char *nmxp_strerror(int errno_value) {
    char * ret_recv_errno_str;
#ifdef HAVE_WINDOWS_H
    char *recv_errno_str;
#else

#ifdef HAVE_STRERROR_R
    char recv_errno_str[MAXLEN_RECV_ERRNO_STR]="";
#else
    char *recv_errno_str=NULL; 
#endif

#endif
    ret_recv_errno_str= (char *) NMXP_MEM_MALLOC (MAXLEN_RECV_ERRNO_STR * sizeof(char));
    ret_recv_errno_str[0] = 0;

#ifdef HAVE_WINDOWS_H
    recv_errno_str = WSAGetLastErrorMessage(errno_value);
#else

#ifdef HAVE_STRERROR_R
    strerror_r(errno_value, recv_errno_str, MAXLEN_RECV_ERRNO_STR);
#else
    recv_errno_str = strerror(errno_value);
#endif

#endif

#ifndef HAVE_STRERROR_R
    if(recv_errno_str) {
#endif
	strncpy(ret_recv_errno_str, recv_errno_str, MAXLEN_RECV_ERRNO_STR);
#ifndef HAVE_STRERROR_R
    }
#endif

    return ret_recv_errno_str;
}


int nmxp_recv_ctrl(int isock, void *buffer, int length, int timeoutsec, int *recv_errno )
{
  int recvCount;
  int cc;
  char *buffer_char = buffer;
  char *recv_errno_str = NULL;

  nmxp_setsockopt_RCVTIMEO(isock, timeoutsec);
  
  cc = 1;
  *recv_errno  = 0;
  recvCount = 0;
  while(cc > 0 && *recv_errno == 0  && recvCount < length) {

      /* TODO some operating system could not reset errno */
      errno = 0;

#ifdef HAVE_BROKEN_SO_RCVTIMEO
      cc = nmxp_recv_select_timeout(isock, buffer_char + recvCount, length - recvCount, timeoutsec);
#else
      cc = recv(isock, buffer_char + recvCount, length - recvCount, 0);
#endif

#ifdef HAVE_WINDOWS_H
      *recv_errno  = WSAGetLastError();
#else
      if(cc == -2) {
	  *recv_errno  = EWOULDBLOCK;
      } else {
	  *recv_errno  = errno;
      }
#endif
      if(cc <= 0) {
	  /*
	  nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "nmxp_recv_ctrl(): (cc=%d <= 0) errno=%d  recvCount=%d  length=%d\n",
	  cc, *recv_errno, recvCount, length);
	  */
      } else {
	  recvCount += cc;
      }
  }

  nmxp_setsockopt_RCVTIMEO(isock, 0);

  if (recvCount != length  ||  *recv_errno != 0  ||  cc <= 0) {

      recv_errno_str = nmxp_strerror(*recv_errno);

#ifdef HAVE_WINDOWS_H
      if(*recv_errno != WSAEWOULDBLOCK  &&  *recv_errno != WSAETIMEDOUT)
#else
      if(*recv_errno != EWOULDBLOCK)
#endif
      {
	  nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "nmxp_recv_ctrl(): %s (errno=%d recvCount=%d length=%d cc=%d)\n",
		  NMXP_LOG_STR(recv_errno_str), *recv_errno, recvCount, length, cc);
      }
       NMXP_MEM_FREE(recv_errno_str);
      /* TO IMPROVE 
       * Fixed bug receiving zero byte from recv() 'TCP FIN or EOF received'
       * */
      if(cc == 0  &&  *recv_errno == 0) {
	  *recv_errno = -100;
      }

#ifdef HAVE_WINDOWS_H
      if(recvCount != length || (*recv_errno != WSAEWOULDBLOCK  &&  *recv_errno != WSAETIMEDOUT))
#else
      if(recvCount != length || *recv_errno != EWOULDBLOCK)
#endif
      {
	  return NMXP_SOCKET_ERROR;
      }
  }
  
  return NMXP_SOCKET_OK;
}


int nmxp_sendHeader(int isock, NMXP_MSG_CLIENT type, int32_t length)
{  
    NMXP_MESSAGE_HEADER msg;

    msg.signature = htonl(NMX_SIGNATURE);
    msg.type      = htonl(type);
    msg.length    = htonl(length);

    return nmxp_send_ctrl(isock, &msg, sizeof(NMXP_MESSAGE_HEADER));
}


int nmxp_receiveHeader(int isock, NMXP_MSG_SERVER *type, int32_t *length, int timeoutsec, int *recv_errno )
{  
    int ret ;
    NMXP_MESSAGE_HEADER msg={0};

    ret = nmxp_recv_ctrl(isock, &msg, sizeof(NMXP_MESSAGE_HEADER), timeoutsec, recv_errno);

    *type = 0;
    *length = 0;

    if((ret == NMXP_SOCKET_OK) && (msg.type != 0)) {
	msg.signature = ntohl(msg.signature);
	msg.type      = ntohl(msg.type);
	msg.length    = ntohl(msg.length);

	if (msg.signature != NMX_SIGNATURE)
	{
	    ret = NMXP_SOCKET_ERROR;
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW,
		    "nmxp_receiveHeader(): signature mismatches. signature = %d, type = %d, length = %d\n",
		    msg.signature, msg.type, msg.length);
	} else {
	    *type = msg.type;
	    *length = msg.length;
	}
    }

    return ret;
}


int nmxp_sendMessage(int isock, NMXP_MSG_CLIENT type, void *buffer, int32_t length) {
    int ret;
    ret = nmxp_sendHeader(isock, type, length);
    if( ret == NMXP_SOCKET_OK) {
	if(buffer && length > 0) {
	    ret = nmxp_send_ctrl(isock, buffer, length);
	}
    }
    return ret;
}


int32_t nmxp_display_error_from_server(char *buffer, int32_t length) {
    /* NMXP_MSG_TERMINATESUBSCRIPTION */
    char *str_msg = NULL;
    int32_t reason;
    memcpy(&reason, buffer, sizeof(reason));
    reason = ntohl(reason);
    str_msg = buffer + sizeof(reason);
    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "%d %s shutdown: %s\n",
	    reason,
	    (reason == 0)? "Normal" : (reason == 1)? "Error" : (reason == 2)? "Timeout" : "Unknown",
	    str_msg);
    return reason;
}

int nmxp_receiveMessage(int isock, NMXP_MSG_SERVER *type, void *buffer, int32_t *length, int timeoutsec, int *recv_errno, int buffer_length) {
    int ret;
    *length = 0;

    ret = nmxp_receiveHeader(isock, type, length, timeoutsec, recv_errno);

    if( ret == NMXP_SOCKET_OK  ) {

	if(*length > buffer_length) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_receiveMessage(): size of received messagge is bigger than buffer. (%d > %d). \n",
		    *length, buffer_length);
	    ret = NMXP_SOCKET_ERROR;
	} else if (*length > 0) {
	    ret = nmxp_recv_ctrl(isock, buffer, *length, 0, recv_errno);

	    if(*type == NMXP_MSG_TERMINATESUBSCRIPTION) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Received TerminateSubscritption.\n");
		nmxp_display_error_from_server(buffer, *length);
	    } else if(*type == NMXP_MSG_ERROR) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Received ErrorMessage: %s\n", NMXP_LOG_STR(buffer));
	    } else {
		nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_PACKETMAN, "Received message type: %d  length=%d\n", *type, *length);
	    }

	}
    }

    if(*recv_errno != 0) {
#ifdef HAVE_WINDOWS_H
	if(*recv_errno == WSAEWOULDBLOCK  ||  *recv_errno == WSAETIMEDOUT) {
#else
	if(*recv_errno == EWOULDBLOCK) {
#endif
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_DOD, "Timeout receiving in nmxp_receiveMessage()\n");
	} else {
	    /* Log message is not necessary because managed by nmxp_recv_ctrl() */
	    /* nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error in nmxp_receiveMessage()\n"); */
	}
    }

    return ret;
}


NMXP_DATA_PROCESS *nmxp_processDecompressedData(char* buffer_data, int length_data, NMXP_CHAN_LIST_NET *channelList, const char *network_code_default, const char *location_code_default)
{
  int32_t   netInt    = 0;
  int32_t   pKey      = 0;
  double    pTime     = 0.0;
  int32_t   pNSamp    = 0;
  int32_t   pSampRate = 0;
  int32_t  *pDataPtr  = NULL;
  int       swap      = 0;
  int       idx;
  int32_t  *outdata   = NULL;


  char station_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];
  char channel_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];
  char network_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];
  char location_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];

  char *nmxp_channel_name = NULL;
  NMXP_DATA_PROCESS *pd   = NULL;

  pd= (NMXP_DATA_PROCESS *) NMXP_MEM_MALLOC(sizeof(NMXP_DATA_PROCESS));
  memset(pd,0,sizeof(NMXP_DATA_PROCESS));


  /* copy the header contents into local fields and swap */
  memcpy(&netInt, &buffer_data[0], 4);
  pKey = ntohl(netInt);
  if ( pKey != netInt ) { swap = 1; }

  nmxp_data_init(pd);
  outdata = (int32_t *) NMXP_MEM_MALLOC(MAX_OUTDATA*sizeof(int32_t));
  nmxp_channel_name = nmxp_chan_lookupName(pKey, channelList);

  if(nmxp_channel_name != NULL) {

  memcpy(&pTime, &buffer_data[4], 8);
  if ( swap ) { nmxp_data_swap_8b(&pTime); }

  memcpy(&netInt, &buffer_data[12], 4);
  pNSamp = ntohl(netInt);
  memcpy(&netInt, &buffer_data[16], 4);
  pSampRate = ntohl(netInt);

  /* There should be (length_data - 20) bytes of data as 32-bit ints here */
  memcpy(outdata , (int32_t *) &buffer_data[20], length_data - 20);
  pDataPtr = outdata;

  /* Swap the data samples to host order */
  for ( idx=0; idx < pNSamp; idx++ ) {
      netInt = ntohl(pDataPtr[idx]);
      pDataPtr[idx] = netInt;
  }

  if(!nmxp_chan_cpy_sta_chan(nmxp_channel_name, station_code, channel_code, network_code, location_code)) {
    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Channel name not in STA.CHAN format: %s\n",
	    NMXP_LOG_STR(nmxp_channel_name));
  }

  pd->key = pKey;
  if(network_code[0] != 0) {
      strncpy(pd->network, network_code, NMXP_DATA_NETWORK_LENGTH);
  } else {
      strncpy(pd->network, network_code_default, NMXP_DATA_NETWORK_LENGTH);
  }
  if(station_code[0] != 0) {
      strncpy(pd->station, station_code, NMXP_DATA_STATION_LENGTH);
  }
  if(channel_code[0] != 0) {
      strncpy(pd->channel, channel_code, NMXP_DATA_CHANNEL_LENGTH);
  }
  if(location_code[0] != 0) {
      strncpy(pd->location, location_code, NMXP_DATA_LOCATION_LENGTH);
  } else {
      strncpy(pd->location, location_code_default, NMXP_DATA_LOCATION_LENGTH);
  }
  pd->packet_type = NMXP_MSG_DECOMPRESSED;
  pd->x0 = -1;
  pd->xn = -1;
  pd->x0n_significant = 0;
  pd->time = pTime;
  pd->nSamp = pNSamp;
  pd->pDataPtr = pDataPtr;
  pd->sampRate = pSampRate;



  /* TODO*/
  /* pd.oldest_seq_no = ;*/
  /* pd.seq_no = ;*/

  NMXP_MEM_FREE(nmxp_channel_name);
  } else {
      nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Channel name not found for key %d\n", pKey);
  }

  return pd;
}


NMXP_DATA_PROCESS *nmxp_processCompressedData(char* buffer_data, int length_data, NMXP_CHAN_LIST_NET *channelList, const char *network_code_default, const char *location_code_default)
{
    int32_t   pKey      = 0;
    double    pTime     = 0.0;
    int32_t   pNSamp    = 0;
    int32_t   pSampRate = 0;
    int32_t  *pDataPtr  = NULL;

    char station_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];
    char channel_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];
    char network_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];
    char location_code[NMXP_CHAN_MAX_SIZE_STR_PATTERN];

    NMXP_DATA_PROCESS *pd = NULL;



    int32_t nmx_rate_code_to_sample_rate[32] = {
	0,1,2,5,10,20,40,50,
	80,100,125,200,250,500,1000,25,
	120,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0};

	int32_t nmx_oldest_sequence_number;
	char nmx_hdr[25];
	unsigned char nmx_ptype;
	int32_t nmx_seconds;
	double nmx_seconds_double;
	int16_t nmx_ticks, nmx_instr_id;
	int32_t nmx_seqno;
	unsigned char nmx_sample_rate;
	int32_t nmx_x0;
	int32_t rate_code, chan_code, this_sample_rate;

	int32_t comp_bytecount;
	unsigned char *indata;
        int32_t * outdata = NULL;

	int32_t nout, i, k;
	int32_t prev_xn;
	const uint32_t high_scale = 4096 * 2048;
	const uint32_t high_scale_p = 4096 * 4096;

	char *nmxp_channel_name = NULL;

        pd= (NMXP_DATA_PROCESS *) NMXP_MEM_MALLOC(sizeof(NMXP_DATA_PROCESS));
        memset(pd,0,sizeof(NMXP_DATA_PROCESS));

	/* TOREMOVE int my_order = get_my_wordorder();*/
	int my_host_is_bigendian = nmxp_data_bigendianhost();
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "my_host_is_bigendian %d\n", my_host_is_bigendian);

	memcpy(&nmx_oldest_sequence_number, buffer_data, 4);
	if (my_host_is_bigendian) {
	    nmxp_data_swap_4b (&nmx_oldest_sequence_number);
	}
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Oldest sequence number = %d\n", nmx_oldest_sequence_number);

	memcpy(nmx_hdr, buffer_data+4, 17);
	/* Decode the Nanometrics packet header bundle. */
	memcpy (&nmx_ptype, nmx_hdr+0, 1);
	if ( (nmx_ptype & 0xf) == 9) {
	    /* Filler packet.  Discard entire packet.   */
	    nmxp_log (NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Filler packet - discarding\n");
	    /*m continue;*/
	    exit(0);
	}

	nmx_x0 = 0;
	memcpy (&nmx_seconds, nmx_hdr+1, 4);
	memcpy (&nmx_ticks, nmx_hdr+5, 2);
	memcpy (&nmx_instr_id, nmx_hdr+7, 2);
	memcpy (&nmx_seqno, nmx_hdr+9, 4);
	memcpy (&nmx_sample_rate, nmx_hdr+13, 1);
	memcpy (&nmx_x0, nmx_hdr+14, 3);

	if (my_host_is_bigendian) {
	    nmxp_data_swap_4b ((int32_t *)&nmx_seconds);
	    nmxp_data_swap_2b (&nmx_ticks);
	    nmxp_data_swap_2b (&nmx_instr_id);
	    nmxp_data_swap_4b (&nmx_seqno);
	    nmxp_data_swap_4b (&nmx_x0);
	}

	/* check if nmx_x0 is negative like as signed 3-byte int */
	if( (nmx_x0 & high_scale) ==  high_scale) {
	    /* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "WARNING: changed nmx_x0, old value = %d\n",  nmx_x0);*/
	    nmx_x0 -= high_scale_p;
	}

	nmx_seconds_double = (double) nmx_seconds + ( (double) nmx_ticks / 10000.0 );
	rate_code = nmx_sample_rate>>3;
	chan_code = nmx_sample_rate&7;
	this_sample_rate = nmx_rate_code_to_sample_rate[rate_code];

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_ptype          = %d\n", nmx_ptype);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_seconds        = %d\n", nmx_seconds);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_ticks          = %d\n", nmx_ticks);

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_seconds_double = %f\n", nmx_seconds_double);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_x0             = %d\n", nmx_x0);

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_instr_id       = %d\n", nmx_instr_id);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_seqno          = %d\n", nmx_seqno);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmx_sample_rate    = %d\n", nmx_sample_rate);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "this_sample_rate   = %d\n", this_sample_rate);

	pKey = (nmx_instr_id << 16) | ( 1 << 8) | ( chan_code);

	pTime = nmx_seconds_double;

	pSampRate = this_sample_rate;

	nmxp_data_init(pd);
        outdata = (int32_t *) NMXP_MEM_MALLOC(MAX_OUTDATA*sizeof(int32_t));
	nmxp_channel_name = nmxp_chan_lookupName(pKey, channelList);

	if(nmxp_channel_name) {

	if(!nmxp_chan_cpy_sta_chan(nmxp_channel_name, station_code, channel_code, network_code, location_code)) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Channel name not in STA.CHAN format: %s\n",
		    NMXP_LOG_STR(nmxp_channel_name));
	}
  
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Channel key %d for %s.%s\n",
		pKey, NMXP_LOG_STR(station_code), NMXP_LOG_STR(channel_code));

	comp_bytecount = length_data-21;
	indata = (unsigned char *) buffer_data + 21;

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "comp_bytecount     = %d  (N = %.2f)\n", comp_bytecount, (double) comp_bytecount / 17.0);

	/* Unpack the data bundles, each 17 bytes long. */
	prev_xn = nmx_x0;
	outdata[0] = nmx_x0;
	nout = 1;
	for (i=0; i<comp_bytecount; i+=17) {
	    if (i+17>comp_bytecount) {
		nmxp_log (NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "comp_bytecount = %d, i+17 = %d\n",
			comp_bytecount, i+17);
		exit(1);
	    }
	    if (nout+16 > MAX_OUTDATA)  {
		nmxp_log (NMXP_LOG_ERR,  NMXP_LOG_D_PACKETMAN, "Output buffer size too small\n");
		exit(1);
	    }
	    k = nmxp_data_unpack_bundle (outdata+nout,indata+i,&prev_xn);
	    if (k < 0) nmxp_log (NMXP_LOG_WARN, NMXP_LOG_D_PACKETMAN, "Null bundle: %s.%s.%s (k=%d) %s %d\n",
		    NMXP_LOG_STR(network_code),
		    NMXP_LOG_STR(station_code),
		    NMXP_LOG_STR(channel_code), k,
		    __FILE__,  __LINE__);
	    if (k < 0) break;
	    nout += k;
	    /* prev_xn = outdata[nout-1]; */
	}
	nout--;

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Unpacked %d samples.\n", nout);

	pDataPtr = outdata;
	pNSamp = nout;

        pd->key = pKey;
	if(network_code[0] != 0) {
	    strncpy(pd->network, network_code, NMXP_DATA_NETWORK_LENGTH);
	} else {
	    strncpy(pd->network, network_code_default, NMXP_DATA_NETWORK_LENGTH);
	}
	if(station_code[0] != 0) {
	    strncpy(pd->station, station_code, NMXP_DATA_STATION_LENGTH);
	}
	if(channel_code[0] != 0) {
	    strncpy(pd->channel, channel_code, NMXP_DATA_CHANNEL_LENGTH);
	}
	if(location_code[0] != 0) {
	    strncpy(pd->location, location_code, NMXP_DATA_LOCATION_LENGTH);
	} else {
	    strncpy(pd->location, location_code_default, NMXP_DATA_LOCATION_LENGTH);
	}
	pd->packet_type = nmx_ptype;
	pd->x0 = nmx_x0;
	pd->xn = pDataPtr[nout];
	pd->x0n_significant = 1;
	pd->oldest_seq_no = nmx_oldest_sequence_number;
	pd->seq_no = nmx_seqno;
	pd->time = pTime;
	pd->nSamp = pNSamp;
	pd->pDataPtr = pDataPtr;
	pd->sampRate = pSampRate;

	NMXP_MEM_FREE(nmxp_channel_name);
	} else {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Channel name not found for key %d\n", pKey);
	}

	return pd;

}


unsigned int nmxp_sleep(unsigned int sleep_time) {
#ifdef HAVE_WINDOWS_H
    Sleep(sleep_time * 1000);
    return 0;
#else
    return sleep(sleep_time);
#endif
}

unsigned int nmxp_usleep(unsigned int usleep_time) {
#ifdef HAVE_WINDOWS_H
    Sleep((usleep_time+500)/1000);
    return 0;
#else
    return usleep(usleep_time);
#endif
}

