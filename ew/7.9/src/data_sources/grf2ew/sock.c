/* $Id: sock.c 5714 2013-08-05 19:18:08Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

    Make WinSock2 look something like Berkeley Sockets and visa versa.

-----------------------------------------------------------------------*/

#include "sock.h"
#include "mem.h"

#if defined WIN32

#define MAX_LENGTH 128

typedef struct TAG_WINSOCK_ERRORS {
	INT32 error;
	CHAR *string;
} WINSOCK_ERRORS;

static WINSOCK_ERRORS winsock_errors[] = {
	{10004, "Interrupted function call."},
	{10013, "Permission denied."},
	{10014, "Bad address."},
	{10022, "Invalid argument."},
	{10024, "Too many open files."},
	{10035, "Resource temporarily unavailable."},
	{10036, "Operation now in progress."},
	{10037, "Operation already in progress."},
	{10038, "Socket operation on non-socket."},
	{10039, "Destination address required."},
	{10040, "Message too long."},
	{10041, "Protocol wrong type for socket."},
	{10042, "Bad protocol option."},
	{10043, "Protocol not supported."},
	{10044, "Socket type not supported."},
	{10045, "Operation not supported."},
	{10046, "Protocol family not supported."},
	{10047, "Address family not supported by protocol family."},
	{10048, "Address already in use."},
	{10049, "Cannot assign requested address."},
	{10050, "Network is down."},
	{10051, "Network is unreachable."},
	{10052, "Network dropped connection on reset."},
	{10053, "Software caused connection abort."},
	{10054, "Connection reset by peer."},
	{10055, "No buffer space available."},
	{10056, "Socket is already connected."},
	{10057, "Socket is not connected."},
	{10058, "Cannot send after socket shutdown."},
	{10060, "Connection timed out."},
	{10061, "Connection refused."},
	{10064, "Host is down."},
	{10065, "No route to host."},
	{10067, "Too many processes."},
	{10091, "Network subsystem is unavailable."},
	{10092, "WINSOCK.DLL version out of range."},
	{10093, "Successful WSAStartup not yet performed."},
	{10094, "Graceful shutdown in progress."},
	{10109, "Class type not found."},
	{11001, "Host not found."},
	{11002, "Non-authoritative host not found."},
	{11003, "This is a non-recoverable error."},
	{11004, "Valid name, no data record of requested type."},
	{0, "Unknown WinSock error code"}
};
#endif										/* defined WIN32 */

/*---------------------------------------------------------------------*/
BOOL SocketsInit(void)
{
#if defined WIN32
	WSADATA wsa_info;

	if (WSAStartup(MAKEWORD(2, 2), &wsa_info) != 0) {
		logit("et", "grf2ew: ERROR: WSAStartup: %s\n", SOCK_ERRSTRING(SOCK_ERRNO()));
		return FALSE;
	}

#if 0
	logit("ot", "grf2ew:   Description:     %s\n", wsa_info.szDescription);
	logit("ot", "grf2ew:   Status:          %s\n", wsa_info.szSystemStatus);
	logit("ot", "grf2ew:   Running version: %u.%u\n", wsa_info.wVersion & 0x00FF, (wsa_info.wVersion & 0xFF00) >> 8);
#endif
#endif

	return TRUE;
}

#if defined WIN32
/*---------------------------------------------------------------------*/
const CHAR *WinSockErrorString(INT32 error)
{
	UINT32 i;

	i = 0;
	while (TRUE) {
		if (winsock_errors[i].error == 0 || winsock_errors[i].error == error)
			break;
		i++;
	}

	return winsock_errors[i].string;
}

/*---------------------------------------------------------------------*/
CHAR *inet_ntop(int family, const void *address, char *string, size_t length)
{
	struct in_addr in;

	ASSERT(address != NULL);
	ASSERT(string != NULL);

	in.S_un.S_addr = *(UINT32 *)address;

	strncpy(string, inet_ntoa(in), length);

	return string;
}
#endif

/*---------------------------------------------------------------------*/
CHAR *FormatEndpoint(ENDPOINT * addr, CHAR *string)
{

	if (addr->sin_addr.s_addr == INADDR_ANY)
		sprintf(string, "*");
	else
		sprintf(string, "%s", inet_ntoa(addr->sin_addr));

	if (ntohs(addr->sin_port) == 0)
		sprintf(string, "%s:*", string);
	else
		sprintf(string, "%s:%d", string, ntohs(addr->sin_port));

	return string;
}

/*---------------------------------------------------------------------*/
BOOL ParseEndpoint(ENDPOINT *addr, CHAR *string, UINT16 port)
{
	CHAR *ptr;
	struct hostent *hptr;

	ASSERT(addr != NULL);
	ASSERT(string != NULL);

	if ((ptr = strchr(string, ':')) == NULL)
		addr->sin_port = htons(port);
	else {
		addr->sin_port = htons((UINT16)atoi(ptr + 1));
		*ptr = '\0';
	}

	if (string[0] == '*')
		addr->sin_addr.s_addr = INADDR_ANY;
	else {
		/* Is this an address string (dotted decimal address)? */
		if (isdigit((int)string[0])) {
			if ((addr->sin_addr.s_addr = inet_addr(string)) == INADDR_NONE) {
				logit("et", "Invalid address string: %s\n", string);
				return FALSE;
			}
		}
		/* Assume it's a name */
		else if ((hptr = gethostbyname(string)) != NULL) {
			addr->sin_addr.s_addr = *(UINT32 *)hptr->h_addr_list[0];
		}
		else {
			logit("et", "Unable to resolve name: %s\n", string);
			return FALSE;
		}
	}

	return TRUE;
}
