/* Definitions for k2pipe program */

#ifndef __K2PIPE_H__
#define __K2PIPE_H__

/* Be sure to place MSS access mode in DYNAMIC or LOCAL; if left in *
 * REMOTE, this code will never read anything from the serial port  */

/* The TCP port for network data: DO NOT use ports 3001, 7000 or any ports *
 * below 1024; they are already reserved. */
#define K2EW_PORT 14011

/* Size of largest K2 packet in bytes: */
#define RECSIZE  3006

/* Maximum number of bytes allowed pending in output device */
#define PENDMAX 256

/* Maximum number of seconds to wait with output pending */
#define MAX_PEND_SEC 15

/* Set to 1 to turn on debugging; zero otherwise */
#define DEBUG 0

#endif  /* __K2PIPE_H__ */
