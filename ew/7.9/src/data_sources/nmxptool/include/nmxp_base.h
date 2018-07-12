/*! \file
 *
 * \brief Base for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_base.h 4965 2012-07-22 07:12:34Z quintiliani $
 *
 */

#ifndef NMXP_BASE_H
#define NMXP_BASE_H 1

#include "nmxp_data.h"
#include "nmxp_chan.h"
#include "nmxp_log.h"

/*! Maximum time between connection attempts (seconds). */
#define NMXP_SLEEPMAX 30

/*! Return message for succes on socket. */
#define NMXP_SOCKET_OK          0

/*! Return message for error on socket.*/
#define NMXP_SOCKET_ERROR      -1

/*! Maximum time out for receiving data (seconds). */
#define NMXP_HIGHEST_TIMEOUT 30

/*! \brief Looks up target host, opens a socket and connects
 *
 *  \param hostname	hostname
 *  \param portNum	port number
 *  \param func_cond Pointer to function for exit condition from loop.
 *
 *  \retval sd A descriptor referencing the socket.
 *  \retval -1 "Empty host name", "Cannot lookup host", ...
 *
 */
int nmxp_openSocket(char *hostname, int portNum, int (*func_cond)(void));


/*! \brief Close a socket.
 *
 *  \param isock  A descriptor referencing the socket.
 *
 *  \retval  0 Success
 *  \retval -1 Error
 *
 */
int nmxp_closeSocket(int isock);


/*! \brief Sends a buffer on a socket.
 *
 * \param isock A descriptor referencing the socket.
 * \param buffer Data buffer.
 * \param length Length in bytes.
 *
 * \retval NMXP_SOCKET_OK on success
 * \retval NMXP_SOCKET_ERROR on error
 *
 */
int nmxp_send_ctrl(int isock, void *buffer, int length);


/*! \brief Receives length bytes in a buffer from a socket.
 *
 * \param isock A descriptor referencing the socket.
 * \param timeoutsec Time-out in seconds
 *
 * \return getsockopt() return value
 *
 */
int nmxp_setsockopt_RCVTIMEO(int isock, int timeoutsec);


/*! \brief Wrapper to strerror, strerror_r or WSAGetLastErrorMessage
 *
 * \return String message of errno_value. It is not static, need to be freed.
 *
 */
char *nmxp_strerror(int errno_value);


/*! \brief Receives length bytes in a buffer from a socket.
 *
 * \param isock A descriptor referencing the socket.
 * \param[out] buffer Data buffer.
 * \param length Length in bytes.
 * \param timeoutsec Time-out in seconds
 * \param[out] recv_errno errno value after recv()
 *
 * \warning Data buffer it has to be allocated before and big enough to contain length bytes!
 *
 * \retval NMXP_SOCKET_OK on success
 * \retval NMXP_SOCKET_ERROR on error
 *
 */
int nmxp_recv_ctrl(int isock, void *buffer, int length, int timeoutsec, int *recv_errno );


/*! \brief Sends header of a message.
 *
 * \param isock A descriptor referencing the socket.
 * \param type Type of message within \ref NMXP_MSG_CLIENT.
 * \param length Length in bytes.
 *
 * \retval NMXP_SOCKET_OK on success
 * \retval NMXP_SOCKET_ERROR on error
 *
 */
int nmxp_sendHeader(int isock, NMXP_MSG_CLIENT type, int32_t length);


/*! \brief Receives header of a message.
 * 
 * \param isock A descriptor referencing the socket.
 * \param[out] type Type of message within \ref NMXP_MSG_CLIENT.
 * \param[out] length Length in bytes.
 * \param timeoutsec Time-out in seconds
 * \param[out] recv_errno errno value after recv()
 *
 * \retval NMXP_SOCKET_OK on success
 * \retval NMXP_SOCKET_ERROR on error
 *
 */
int nmxp_receiveHeader(int isock, NMXP_MSG_SERVER *type, int32_t *length, int timeoutsec, int *recv_errno );


/*! \brief Sends header and body of a message.
 *
 * \param isock A descriptor referencing the socket.
 * \param type Type of message within \ref NMXP_MSG_CLIENT.
 * \param buffer Data buffer. It could be NULL.
 * \param length Length in bytes. It must be greater or equal to zero.
 *
 * \retval NMXP_SOCKET_OK on success
 * \retval NMXP_SOCKET_ERROR on error
 *
 */
int nmxp_sendMessage(int isock, NMXP_MSG_CLIENT type, void *buffer, int32_t length);


/*! \brief Receives header and body of a message.
 *
 * \param isock A descriptor referencing the socket.
 * \param[out] type Type of message within \ref NMXP_MSG_SERVER.
 * \param buffer Pointer to the Data buffer.
 * \param[out] length Length in bytes.
 * \param timeoutsec Time-out in seconds
 * \param[out] recv_errno errno value after recv()
 * \param buffer_length Max length of Data buffer.
 *
 * \retval NMXP_SOCKET_OK on success
 * \retval NMXP_SOCKET_ERROR on error
 *
 */
int nmxp_receiveMessage(int isock, NMXP_MSG_SERVER *type, void *buffer, int32_t *length, int timeoutsec, int *recv_errno, int buffer_length);


/*! \brief Process Compressed Data message by function func_processData().
 *
 * \param buffer_data Pointer to the data buffer containing Compressed Nanometrics packets.
 * \param length_data Buffer length in bytes.
 * \param channelList Pointer to the Channel List.
 * \param network_code_default Value of network code to assign returned structure. It should not be NULL.
 * \param location_code_default Value of location code to assign returned structure. It should not be NULL.
 *
 * \return Return a pointer to static struct NMXP_DATA_PROCESS.
 *
 */
NMXP_DATA_PROCESS *nmxp_processCompressedData(char* buffer_data, int length_data, NMXP_CHAN_LIST_NET *channelList, const char *network_code_default, const char *location_code_default);


/*! \brief Process decompressed Data message by function func_processData().
 *
 * \param buffer_data Pointer to the data buffer containing Decompressed Nanometrics packets.
 * \param length_data Buffer length in bytes.
 * \param channelList Pointer to the Channel List.
 * \param network_code_default Value of network code to assign returned structure. It should not be NULL.
 * \param location_code_default Value of location code to assign returned structure. It should not be NULL.
 *
 * \return Return a pointer to static struct NMXP_DATA_PROCESS.
 *
 */
NMXP_DATA_PROCESS *nmxp_processDecompressedData(char* buffer_data, int length_data, NMXP_CHAN_LIST_NET *channelList, const char *network_code_default, const char *location_code_default);


/*! \brief Wrapper for functions sleep on different platforms
 *
 *  \param sleep_time time in seconds
 *
 *  \retval ....
 *
 */
unsigned int nmxp_sleep(unsigned int sleep_time);

/*! \brief Wrapper for functions usleep on different platforms
 *
 *  \param usleep_time time in microseconds
 *
 *  \retval ....
 *
 */
unsigned int nmxp_usleep(unsigned int usleep_time);

#endif

