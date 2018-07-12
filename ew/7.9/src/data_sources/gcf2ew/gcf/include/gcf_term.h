#ifndef _GCF_TERM_H
#define _GCF_TERM_H
/* COPYRIGHT 1997. Paul Friberg and Sid Hellman, RPSC */

#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <memory.h>

#define DEFAULT_BAUD B9600
#define GCF_SOCKET_READ_TIMEOUT -10

#ifdef LINUX
#define DEFAULT_TTY "/dev/ttyS0"
#endif

#ifdef SOLARIS2
#define DEFAULT_TTY "/dev/term/a"
#endif

#ifdef SUNOS
#define DEFAULT_TTY "/dev/ttya"
#endif

#define BRP_MAX_TRIES 3

#ifdef STDC
int check_speed(speed_t *speed);
int gcf_open(char * term_dev, speed_t speed);
int gcf_read(int fd, char **buf);
int gcf_read_s(int fd, char **buf, int *size);
int gcf_serial_simread(int fd, char **buf, int *size);
int gcf_sendNACK_BRP(int fd, unsigned char * serial_buf, unsigned char block_requested);
int get_equiv_com_port(char * str);
int gcf_socket_read(int fd, char **buf, int *size, int timeout_msecs);
#else
int check_speed();
int gcf_open();
int gcf_read();
int gcf_read_s();
int gcf_sendNACK_BRP();
int get_equiv_com_port();
int gcf_socket_read();
#endif
#endif /*_GCF_TERM_H*/

