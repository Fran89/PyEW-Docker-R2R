/*! \file
 *
 * \brief Function for Windows OS
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_win.c 3898 2010-03-25 13:25:46Z quintiliani $
 *
 */

/* Code is based on guide and example from:
 * http://tangentsoft.net/wskfaq/articles/bsd-compatibility.html
 * http://tangentsoft.net/wskfaq/examples/basics/index.html
 * http://tangentsoft.net/wskfaq/examples/basics/ws-util.cpp
 */

#include "config.h"
#include "nmxp_win.h"
#include "nmxp_log.h"

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


#ifdef HAVE_WINDOWS_H
void nmxp_initWinsock() {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "WSAStartup\n");
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	/* Initialize Winsock */
	int iResult = WSAStartup( wVersionRequested, &wsaData );
	if( iResult != 0 ) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error at WSAStartup\n");
	}
	if( LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion != 2) ) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "winsock library\n");
		WSACleanup();
		exit(1);
	}
}
#endif


#ifdef HAVE_WINDOWS_H
/* \brief List of Winsock error constants
 *
 * List of Winsock error constants mapped to an interpretation string.
 * Note that this list must remain sorted by the error constants' values,
 * because we do a binary search on the list when looking up items.
*/
static struct ErrorEntry {
	int nID;
	const char* pcMessage;
} gaErrorList[] = {
	{0,                  "No error"},
	{WSAEINTR,           "Interrupted system call"},
	{WSAEBADF,           "Bad file number"},
	{WSAEACCES,          "Permission denied"},
	{WSAEFAULT,          "Bad address"},
	{WSAEINVAL,          "Invalid argument"},
	{WSAEMFILE,          "Too many open sockets"},
	{WSAEWOULDBLOCK,     "Operation would block"},
	{WSAEINPROGRESS,     "Operation now in progress"},
	{WSAEALREADY,        "Operation already in progress"},
	{WSAENOTSOCK,        "Socket operation on non-socket"},
	{WSAEDESTADDRREQ,    "Destination address required"},
	{WSAEMSGSIZE,        "Message too long"},
	{WSAEPROTOTYPE,      "Protocol wrong type for socket"},
	{WSAENOPROTOOPT,     "Bad protocol option"},
	{WSAEPROTONOSUPPORT, "Protocol not supported"},
	{WSAESOCKTNOSUPPORT, "Socket type not supported"},
	{WSAEOPNOTSUPP,      "Operation not supported on socket"},
	{WSAEPFNOSUPPORT,    "Protocol family not supported"},
	{WSAEAFNOSUPPORT,    "Address family not supported"},
	{WSAEADDRINUSE,      "Address already in use"},
	{WSAEADDRNOTAVAIL,   "Can't assign requested address"},
	{WSAENETDOWN,        "Network is down"},
	{WSAENETUNREACH,     "Network is unreachable"},
	{WSAENETRESET,       "Net connection reset"},
	{WSAECONNABORTED,    "Software caused connection abort"},
	{WSAECONNRESET,      "Connection reset by peer"},
	{WSAENOBUFS,         "No buffer space available"},
	{WSAEISCONN,         "Socket is already connected"},
	{WSAENOTCONN,        "Socket is not connected"},
	{WSAESHUTDOWN,       "Can't send after socket shutdown"},
	{WSAETOOMANYREFS,    "Too many references, can't splice"},
	{WSAETIMEDOUT,       "Connection timed out"},
	{WSAECONNREFUSED,    "Connection refused"},
	{WSAELOOP,           "Too many levels of symbolic links"},
	{WSAENAMETOOLONG,    "File name too long"},
	{WSAEHOSTDOWN,       "Host is down"},
	{WSAEHOSTUNREACH,    "No route to host"},
	{WSAENOTEMPTY,       "Directory not empty"},
	{WSAEPROCLIM,        "Too many processes"},
	{WSAEUSERS,          "Too many users"},
	{WSAEDQUOT,          "Disc quota exceeded"},
	{WSAESTALE,          "Stale NFS file handle"},
	{WSAEREMOTE,         "Too many levels of remote in path"},
	{WSASYSNOTREADY,     "Network system is unavailable"},
	{WSAVERNOTSUPPORTED, "Winsock version out of range"},
	{WSANOTINITIALISED,  "WSAStartup not yet called"},
	{WSAEDISCON,         "Graceful shutdown in progress"},
	{WSAHOST_NOT_FOUND,  "Host not found"},
	{WSANO_DATA,         "No host data of that type was found"}
};
const int kNumMessages = sizeof(gaErrorList) / sizeof(struct ErrorEntry);


#define MAX_SIZE_acErrorBuffer 2048
char* WSAGetLastErrorMessage(int nErrorID)
{
	static char acErrorBuffer[MAX_SIZE_acErrorBuffer];
	int i = 0;
	while(i < kNumMessages  &&  gaErrorList[i].nID != nErrorID) {
		i++;
	}
	if(i < kNumMessages) {
		strncpy(acErrorBuffer, gaErrorList[i].pcMessage, MAX_SIZE_acErrorBuffer);
	} else {
		strncpy(acErrorBuffer, "Unknown error", MAX_SIZE_acErrorBuffer);
	}
	return acErrorBuffer;
}

#endif


