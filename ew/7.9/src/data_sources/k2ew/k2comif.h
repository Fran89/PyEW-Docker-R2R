/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: k2comif.h 6094 2014-05-26 01:56:04Z baker $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/05/04 23:48:03  lombard
 *     Initial revision
 *
 *
 *
 */
/* k2comif.h:  Header file for general IO routines */

#ifndef K2COMIF_H            /* compile file only once */
#define K2COMIF_H 1          /* indicate file has been compiled */

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
  char k2_address[KC_MAX_ADDR_LEN];  /* domain name or IP address */
  int k2_port;                       /* tcp port number           */
} s_tcp_io;

/* All the modes of communication with the K2 */
typedef enum 
{
  IO_NONE = 0,
  IO_TCP,
  IO_COM_NT,
  IO_TTY_UN,
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
 * k2c_init_io:  tcp port initialization routine                        *
 *      gen_io: generalize IO parameter structure                       *
 *      toutms: timeout in milliseconds for connect()                   *
 *      return value:  returns K2R_NO_ERROR if successful               *
 *                     returns K2R_ERROR on error                       *
 *                     or K2R_TIMEOUT                                   *
 ************************************************************************/
int k2c_init_io(s_gen_io *gen_io, int toutms);

/************************************************************************
 * k2c_close_io:  closes tcp socket opened by 'k2_init_io()'            *
 *                sets the socket to INVALID_SOCKET                     *
 *         return value: returns K2R_NO_ERROR on success,               *
 *                        K2R_ERROR on error                            *
 ************************************************************************/
int k2c_close_io( void );

/**************************************************************************
 * k2c_tran_buff:  transmits buffer of data (up to 65535 bytes) out TCP   *
 *      port; with optional timeout value                                 *
 *      If socket is re-opened, then entire buffer is sent again          *
 *         buff     - address of buffer containing data bytes             *
 *         datalen   - number of bytes from buffer to transmit            *
 *         toutms   - timeout value in milliseconds; (note that the       *
 *                    timeout is reset after each tcp packet is sent)     *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value:  returns the number of bytes successfully           *
 *                     transmitted or K2R_ERROR on error,                 *
 *                     K2R_TIMEOUT on timeout                             *
 **************************************************************************/
int k2c_tran_buff(const unsigned char *buff, int datalen, int toutms, 
                  int redo);

/**************************************************************************
 * k2c_rcvflg_tout:  waits for data to be received into TCP port          *
 *         toutms   - a timeout value in milliseconds                     *
 *      return value:  returns K2R_NO_ERROR if data is ready to read      *
 *                returns K2R_TIMEOUT if no data to read within timeout   *
 *                     returns K2R_ERROR on error                         *
 **************************************************************************/
int k2c_rcvflg_tout(int toutms);

/**************************************************************************
 * k2c_rcvbt_tout:  returns next byte received into TCP port; waits for   *
 *      data ready with timeout                                           *
 *         toutms   - if non-zero then a timeout value in milliseconds;   *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value: returns the received byte (as an int) if successful *
 *                    returns K2R_TIMEOUT if timeout                      *
 *                    returns K2R_ERROR if an error detected              *
 **************************************************************************/
int k2c_rcvbt_tout(int toutms, int redo);

/* ************************************************************************
 * k2c_recv_buff:  fills buffer with received data; waits for given       *
 *      number of bytes to be received; with optional timeout             *
 *         rbuff    - address of buffer to receive data bytes             *
 *         datalen  - number of data bytes to receive                     *
 *         toutms   - timeout value in milliseconds;                      *
 *         redo     - number of times to re-open the socket on timeouts   *
 *      return value: returns the number of data bytes received and       *
 *                    placed into 'rbuff[]'                               *
 *                    returns K2R_ERROR on error                          *
 *                    returns K2R_TIMEOUT if no data read before timeout  *
 **************************************************************************/
int k2c_recv_buff(unsigned char *rbuff, int datalen, int toutms, int redo);

/**************************************************************************
 * k2c_flush_recv:  flushes out the receive buffer by reading in and      *
 *      discarding incoming data (up to 'K2C_MAX_FLUSH' bytes); with      *
 *      optional timeout                                                  *
 *         toutms - timeout value in milliseconds                         *
 *                  during which any incoming data will be flushed;       *
 *                  if zero then no timeout is used and only the          *
 *                  data already received is flushed                      *
 *      return value:  returns 0 if no error; -1 on error                 *
 **************************************************************************/
int k2c_flush_recv(int toutms);


#endif
