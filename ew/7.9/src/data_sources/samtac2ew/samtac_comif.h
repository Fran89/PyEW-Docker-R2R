/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac_comif.h 3536 2009-01-15 22:09:51Z tim $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *     Revision 1.3  2009/01/13 17:12:53  tim
 *     Clean up source
 *
 *     Revision 1.2  2009/01/13 15:41:27  tim
 *     Removed more k2 references
 *
 *     Revision 1.1  2009/01/12 20:52:32  tim
 *     Removing K2 references
 *
 *     Revision 1.2  2008/10/17 19:59:55  tim
 *     remove refrences to STN_ID_LENGTH as it is used for reading station IDs from K2
 *
 *     Revision 1.1  2008/10/17 19:38:59  tim
 *     adding k2comif, this may later be moved to samtac_comif
 *
 */

#ifndef SAMTAC_COMIF_H            /* compile file only once */
#define SAMTAC_COMIF_H 1          /* indicate file has been compiled */

#define KC_MAX_ADDR_LEN 80
#define KC_MAX_TTY_LEN 80
#define RB_LEN 256

typedef struct _s_com_io     /* Parameters for NT COM port */ 
{
  int speed;
  int commsel;
} s_com_io;

typedef struct _s_tty_io     /* Parameters for Unix TTY port */
{
  int speed;
  char ttyname[KC_MAX_TTY_LEN];
} s_tty_io;

typedef struct _s_tcp_io      /* Parameters for TCP scoket comms */ 
{
  char samtac_address[KC_MAX_ADDR_LEN];  /* domain name or IP address */
  int samtac_port;                       /* tcp port number           */
} s_tcp_io;

/* All the modes of communication with the K2 */
typedef enum 
{
  IO_TCP = 0,
  IO_COM_NT = 1,
  IO_TTY_UN = 2,
  IO_NUM_MODES
} io_mode;

typedef struct _s_gen_io
{
  io_mode mode;
  s_com_io com_io;
  s_tty_io tty_io;
  s_tcp_io tcp_io;
} s_gen_io;

/* Function prototypes for all the communication rotuines */
/************************************************************************
 * samtac_init_io:  tcp port initialization routine                        *
 *      gen_io: generalize IO parameter structure                       *
 *      toutms: timeout in milliseconds for connect()                   *
 *      return value:  returns SAMTAC2R_NO_ERROR if successful               *
 *                     returns SAMTAC2R_ERROR on error                       *
 *                     or SAMTAC2R_TIMEOUT                                   *
 ************************************************************************/
int samtac_init_io(s_gen_io *gen_io, int toutms);

/************************************************************************
 * samtac_close_io:  closes tcp socket opened by 'samtac_init_io()'            *
 *                sets the socket to INVALID_SOCKET                     *
 *         return value: returns SAMTAC2R_NO_ERROR on success,               *
 *                        SAMTAC2R_ERROR on error                            *
 ************************************************************************/
int samtac_close_io( void );

/* ************************************************************************
 * samtac_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value: returns the number of data bytes received and       *
 *                    placed into 'rbuff[]'                               *
 *                    returns SAMTAC2R_ERROR on error                          *
 *                    returns SAMTAC2R_TIMEOUT if no data read before timeout  *
 **************************************************************************/
int samtac_recv_buff(unsigned char *rbuff, int datalen, int toutms, int redo);


#endif
