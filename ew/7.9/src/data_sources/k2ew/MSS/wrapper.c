/*******************************************************************
 *
 *		Copyright (c) 1999 Gordian
 *
 *	Module: wrapper.c
 *	Author: Nathan Rutman
 *	Date: Wed Aug 11 11:26:18 1999
 *	Description: 
 *         Wrapper functions, based on Stevens.
 *
 *******************************************************************/

/*
  We're doing something kind of ugly here - treating a source
  file as a header file.  Since we don't have makefiles or the
  ability to compile multiple source files from the command line,
  this is the easiest way to make sure this c file is included only
  once.
*/
#ifndef	__wrapper_c
#define	__wrapper_c

#include "unp.h"
#include <stdarg.h>

/* Error functions */
#define err_quit err_sys

void
err_sys(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fprintf(stderr, "Syserr (errno %d): %s\n\r", errno, strerror(errno));
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n\r");
    va_end(args);
    exit(1);
}

/* Socket wrappers */

/* changed to eliminate label
 * changed second argument type from sockaddr to sockaddr_in
 * proto: int accept (int fd, SA_in *addr, int *addrlen)
 */
int
Accept(int fd, SA_in *sa, socklen_t *salenptr)
{
    int		n;
    
    n = accept(fd, sa, salenptr);
    while ( (n < 0) &&  (errno == EPROTO || errno == ECONNABORTED) ) {
	n = accept(fd, sa, salenptr);
    }
    if ( n < 0) {
	err_sys("accept error");
    }
    return(n);
}

void
Bind(int fd, SA *sa, socklen_t salen)
{
	if (bind(fd, sa, (int)salen) < 0)
		err_sys("bind error");
}

void
Connect(int fd, SA *sa, socklen_t salen)
{
	if (connect(fd, sa, salen) < 0)
		err_sys("connect error");
}

void
Listen(int fd, int backlog)
{
	if (listen(fd, backlog) < 0)
		err_sys("listen error");
}

ssize_t
Recv(int fd, void *ptr, size_t nbytes, int flags)
{
	ssize_t		n;
	if ( (n = recv(fd, ptr, nbytes, flags)) < 0)
		err_sys("recv error");
	return(n);
}

/* changed SA* to SA_in* */
ssize_t
Recvfrom(int fd, void *ptr, size_t nbytes, int flags,
                 SA_in *sa, socklen_t *salenptr)
{
        ssize_t         n;

        if ( (n = recvfrom(fd, ptr, nbytes, flags, sa, salenptr)) < 0)
                err_sys("recvfrom error (%d)", n);
        return(n);
}

int
Send(int fd, void *ptr, size_t nbytes, int flags)
{
	if (send(fd, ptr, nbytes, flags) != nbytes)
	    err_sys("send error");
	return nbytes;
}

/* changed SA* to SA_in* */
void
Sendto(int fd, void *ptr, size_t nbytes, int flags,
           SA_in *sa, socklen_t salen)
{
        if (sendto(fd, ptr, nbytes, flags, sa, salen) != nbytes)
                err_sys("sendto error");
}

int
Socket(int family, int type, int protocol)
{
	int		n;
	if ( (n = socket(family, type, protocol)) < 0)
		err_sys("socket error");
	return(n);
}

struct hostent *
Gethostbyname(char *host)
{
    HOSTENT *hp;
    if ( (hp = gethostbyname(host)) == NULL )
	err_sys("gethostbyname error - host not found");
    if ( (struct in_addr *)hp->h_addr == NULL )
	err_sys("gethostbyname error - no address");
    return hp;
}


/* Unix wrappers */

void *
Calloc(size_t n, size_t size)
{
	void	*ptr;

	if ( (ptr = calloc(n, size)) == NULL)
		err_sys("calloc error");
	return(ptr);
}

/* we must use closesocket instead of close */
void
Close(int fd)
{
    if (close(fd) == -1)
	err_sys("close error");
    return;
}

void *
Malloc(size_t size)
{
	void	*ptr;

	if ( (ptr = malloc(size)) == NULL)
		err_sys("malloc error");
	return(ptr);
}

int
Open(const char *pathname, int oflag, mode_t mode)
{
	int		fd;

	if ( (fd = open(pathname, oflag, mode)) < 0)
		err_sys("open error for %s", pathname);
	return(fd);
}

ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
	ssize_t		n;

	if ( (n = read(fd, ptr, nbytes)) < 0)
		err_sys("read error");
	return(n);
}

int
Write(int fd, void *ptr, size_t nbytes)
{
	if (write(fd, ptr, nbytes) != nbytes)
		err_sys("write error");
	return(nbytes);
}


/* Stdio wrappers */

void
Fclose(FILE *fp)
{
	if (fclose(fp) != 0)
		err_sys("fclose error");
}

char *
Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		err_sys("fgets error");

	return (rptr);
}

FILE *Fopen(const char *filename, const char *mode)
{
    FILE	*fp;
    
    if ( (fp = fopen(filename, mode)) == NULL)
	err_sys("fopen error");
    
    return(fp);
}

void
Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		err_sys("fputs error");
}



#endif
