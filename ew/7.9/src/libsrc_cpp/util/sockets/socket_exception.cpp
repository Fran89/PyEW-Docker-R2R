//---------------------------------------------------------------------------
#include <socket_exception.h>

#if  defined(_WINNT) || defined(_Windows)

#include <winsock2.h>

#elif defined(_SOLARIS)

#include <sys/types.h>
#include <sys/socket.h>




#else //-----------------------------------

#error socket_exception.h not completed for this O/S

#endif

//---------------------------------------------------------------------------
#pragma package(smart_init)

//---------------------------------------------------------------------------
const char * worm_socket_exception::DecodeError() const
{
   return DecodeError(FunctionId, ErrorCode);
}
//---------------------------------------------------------------------------
const char * worm_socket_exception::DecodeError( WS_FUNCTION_ID p_functionid, int p_errcode )
{
   char * r_message = "Unidentified socket error.";

   switch( p_errcode )
   {
#if  defined(_WINNT) || defined(_Windows)

     case WSANOTINITIALISED:
          r_message = "A successful WSAStartup must occur before using this function.";
          break;
     case WSAEINPROGRESS:
          r_message = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
          break;
     case WSAENOTSOCK:
          r_message = "The descriptor is not a socket.";
          break;
     case WSAEACCES:
          switch( p_functionid )
          {
            case WSF_SEND:
                 r_message = "The requested address is a broadcast address, but the appropriate flag was not set.";
                 break;
            case WSF_CONNECT:
                 r_message = "Attempt to connect datagram socket to broadcast address failed because setsockopt SO_BROADCAST is not enabled.";
                 break;
          }
          break;
     case WSAEADDRINUSE: // bind()
          r_message = "The specified address is already in use. (See the SO_REUSEADDR socket option under setsockopt.)";
          break;
     case WSAEADDRNOTAVAIL: // WSF_CONNECT
          r_message = "The specified address is not available from the local machine.";
          break;
     case WSAEAFNOSUPPORT:
          switch( p_functionid )
          {
            case WSF_SOCKET:
                 r_message = "The specified address family is not supported.";
                 break;
            case WSF_CONNECT:
                 r_message = "Addresses in the specified family cannot be used with this socket.";
                 break;
          }
          break;
     case WSAEALREADY: // WSF_CONNECT
          r_message = "A nonblocking connect call is in progress on the specified socket.";
          break;
     case WSAECONNABORTED:   // WSF_SEND, WSF_RECV, WSF_RECVFROM
          r_message = "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
          break;
     case WSAECONNRESET:
          switch( p_functionid )
          {
            case WSF_CONNECT:
                 r_message = "The attempt to connect was forcefully rejected.";
                 break;
            case WSF_SEND:
                 r_message = "The virtual circuit was reset by the remote side executing a 'hard' or 'abortive' close. For UPD sockets, the remote host was unable to deliver a previously sent UDP datagram and responded with a 'Port Unreachable' ICMP packet. The application should close the socket as it is no longer usable";
                 break;
            case WSF_RECV:
            case WSF_RECVFROM:
                 r_message = "The virtual circuit was reset by the remote side executing a 'hard' or 'abortive' close. The application should close the socket as it is no longer usable. On a UDP datagram socket this error would indicate that a previous send operation resulted in an ICMP 'Port Unreachable' message.";
                 break;
          }
          break;
     case WSAEFAULT:
          switch( p_functionid )
          {
            case WSF_ACCEPT:
                 r_message = "The addrlen argument is too small or addr is not a valid part of the user address space.";
                 break;
            case WSF_BIND:
                 r_message = "The name or the namelen argument is not a valid part of the user address space, the namelen argument is too small, the name argument contains incorrect address format for the associated address family, or the first two bytes of the memory block specified by name does not match the address family associated with the socket descriptor s.";
                 break;
            case WSF_CONNECT:
                 r_message = "The name or the namelen argument is not a valid part of the user address space, the namelen argument is too small, or the name argument contains incorrect address format for the associated address family.";
                 break;
            case WSF_IOCTLSOCK:
                 r_message = "The argp argument is not a valid part of the user address space";
                 break;
            case WSF_SETSOCKOPT:
                 r_message = "optval is not in a valid part of the process address space or optlen argument is too small";
                 break;
            case WSF_GETSOCKOPT:
                 r_message = "One of the optval or the optlen arguments is not a valid part of the user address space, or the optlen argument is too small.";
                 break;
            case WSF_SELECT:
                 r_message = "The Windows Sockets implementation was unable to allocated needed resources for its internal operations, or the readfds, writefds, exceptfds, or timeval parameters are not part of the user address space.";
                 break;
            case WSF_SEND:
                 r_message = "The buf argument is not totally contained in a valid part of the user address space.";
                 break;
            case WSF_RECV:
                 r_message = "The buf argument is not totally contained in a valid part of the user address space.";
                 break;
            case WSF_RECVFROM:
                 r_message = "The buf or from parameters are not part of the user address space, or the fromlen argument is too small to accommodate the peer address.";
                 break;
          }
          break;
     case WSAEHOSTUNREACH:
          r_message = "The remote host cannot be reached from this host at this time";
          break;
     case WSAEINTR: // WSF_SEND, WSF_RECV
          r_message = "The (blocking) call was canceled through WSACancelBlockingCall.";
          break;
     case WSAEINVAL:
          switch( p_functionid )
          {
            case WSF_ACCEPT:
                 r_message = "listen was not invoked prior to accept.";
                 break;
            case WSF_BIND:
                 r_message = "The socket is already bound to an address.";
                 break;
            case WSF_CONNECT:
                 r_message = "The parameter s is a listening socket, or the destination address specified is not consistent with that of the constrained group the socket belongs to.";
                 break;
            case WSF_SETSOCKOPT:
                 r_message = "level is not valid, or the information in optval is not valid";
                 break;
            case WSF_GETSOCKOPT:
                 r_message = "level is unknown or invalid";
                 break;
            case WSF_SELECT:
                 r_message = "The timeout value is not valid, or all three descriptor parameters were NULL.";
                 break;
            case WSF_SEND:
                 r_message = "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled.";
                 break;
            case WSF_RECV:
            case WSF_RECVFROM:
                 r_message = "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.";
                 break;
          }
          break;
     case WSAEISCONN: // WSF_CONNECT
          r_message = "The socket is already connected (connection-oriented sockets only).";
          break;
     case WSAEMFILE: // WSF_SOCKET, WSF_ACCEPT
          r_message = "No more socket descriptors are available";
          break;
     case WSAEMSGSIZE:
          switch( p_functionid )
          {
            case WSF_SEND:
                 r_message = "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport.";
                 break;
            case WSF_RECV:
            case WSF_RECVFROM:
                 r_message = "The message was too large to fit into the specified buffer and was truncated.";
                 break;
          }
          break;
     case WSAENETDOWN: // WSF_SOCKET, WSF_SEND
          r_message = "The network subsystem or the associated service provider has failed.";
          break;
     case WSAENETRESET:
          switch( p_functionid )
          {
            case WSF_SETSOCKOPT:
                 r_message = "Connection has timed out when SO_KEEPALIVE is set";
                 break;
            case WSF_SEND:
            case WSF_RECV:
            case WSF_RECVFROM:
                 r_message = "The connection has been broken due to the remote host resetting.";
                 break;
          }
          break;
     case WSAENETUNREACH: // WSF_CONNECT
          r_message = "The network cannot be reached from this host at this time.";
          break;
     case WSAENOBUFS:
          switch( p_functionid )
          {
            case WSF_ACCEPT:
                 r_message = "No buffer space available.";
                 break;
            case WSF_BIND:
                 r_message = "Not enough buffers available, too many connections.";
                 break;
            case WSF_SOCKET:
                 r_message = "No buffer space is available. The socket cannot be created.";
                 break;
            case WSF_CONNECT:
                 r_message = "No buffer space is available. The socket cannot be connected.";
                 break;
            case WSF_IOCTLSOCK:
            case WSF_SEND:
                 r_message = "No buffer space is available.";
                 break;
          }
          break;
     case WSAENOPROTOOPT:
          switch( p_functionid )
          {
            case WSF_SETSOCKOPT:
                 r_message = "The option is unknown or unsupported for the specified provider.";
                 break;
            case WSF_GETSOCKOPT:
                 r_message = "The option is unknown or unsupported by the indicated protocol family.";
                 break;
            case WSF_SOCKET:
                 r_message = "The specified protocol is the wrong type for this socket.";
                 break;
          }
          break;
     case WSAENOTCONN:
          switch( p_functionid )
          {
            case WSF_SETSOCKOPT:
                 r_message = "Connection has been reset when SO_KEEPALIVE is set";
                 break;
            case WSF_SEND:
            case WSF_RECV:
                 r_message = "The socket is not connected.";
                 break;
            case WSF_RECVFROM:
                 r_message = "The socket is not connected (connection-oriented sockets only).";
                 break;
          }
          break;
     case WSAEOPNOTSUPP:
          switch( p_functionid )
          {
            case WSF_ACCEPT:
                 r_message = "The referenced socket is not a type that supports connection-oriented service.";
                 break;
            case WSF_SEND:
                 r_message = "MSG_OOB was specified, but the socket is not stream style such as type SOCK_STREAM, out-of-band data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only receive operations.";
                 break;
            case WSF_RECV:
            case WSF_RECVFROM:
                 r_message = "MSG_OOB was specified, but the socket is not stream style such as type SOCK_STREAM, out-of-band data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.";
                 break;
          }
          break;
     case WSAEPROTONOSUPPORT: // WSF_SOCKET
          r_message = "The specified protocol is not supported";
          break;
     case WSAEPROTOTYPE: // WSF_SOCKET
          r_message = "The specified protocol is the wrong type for this socket";
          break;
     case WSAESHUTDOWN:
          switch( p_functionid )
          {
            case WSF_SEND:
                 r_message = "The socket has been shut down; it is not possible to send on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH";
                 break;
            case WSF_RECV:
                 r_message = "The socket has been shut down; it is not possible to recv on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
                 break;
            case WSF_RECVFROM:
                 r_message = "The socket has been shut down; it is not possible to recvfrom on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
                 break;
          }
          break;
     case WSAESOCKTNOSUPPORT: // WSF_SOCKET
          r_message = "The specified socket type is not supported in this address family";
          break;
     case WSAETIMEDOUT:
          switch( p_functionid )
          {
            case WSF_CONNECT:
                 r_message = "Attempt to connect timed out without establishing a connection.";
                 break;
            case WSF_SEND:
                 r_message = "The connection has been dropped because of a network failure or because the system on the other end went down without notice";
                 break;
            case WSF_RECV:
                 r_message = "The connection has been dropped because of a network failure or because the peer system failed to respond.";
                 break;
            case WSF_RECVFROM:
                 r_message = "The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";
                 break;
          }
          break;
     case WSAEWOULDBLOCK:
          switch( p_functionid )
          {
            case WSF_ACCEPT:
                 r_message = "The socket is marked as nonblocking and no connections are present to be accepted.";
                 break;
            case WSF_CONNECT:
                 r_message = "The socket is marked as nonblocking and the connection cannot be completed immediately. It is possible to select the socket while it is connecting by selecting it for writing.";
                 break;
            case WSF_SEND:
                 r_message = "The socket is marked as nonblocking and the requested operation would block.";
                 break;
            case WSF_RECV:
                 r_message = "The socket is marked as nonblocking and the receive operation would block.";
                 break;
            case WSF_RECVFROM:
                 r_message = "The socket is marked as nonblocking and the recvfrom operation would block.";
                 break;
          }
          break;

#elif defined(_SOLARIS)

     case EACCES:
          switch( p_functionid )
          {
            case WSF_SOCKET:
                 r_message = "Permission to create a  socket  of  the  specified type and/or protocol is denied.";
                 break;

            case WSF_BIND:
                 r_message = "The requested address is protected and the current user has inadequate permission to access it.";
                 break;

            case WSF_CONNECT:
                 r_message = "Search permission is denied for a component of the path prefix of the pathname in name.";
                 break;
          }
          break;


     case EADDRINUSE:
          switch( p_functionid )
          {
            case WSF_CONNECT:
                 r_message = "The address is already in use.";
                 break;

            case WSF_BIND:
                 r_message = "The specified address is already in use.";
                 break;
          }
          break;

     case EADDRNOTAVAIL:
          switch( p_functionid )
          {
            case WSF_CONNECT:
                 r_message = "The specified address is not available on the remote machine.";
                 break;

            case WSF_BIND:
                 r_message = "The specified address is not available on the local machine.";
                 break;
          }
          break;

     case EAFNOSUPPORT:  // WSF_CONNECT
          r_message = "Addresses in the specified address family cannot be used with this socket.";
          break;

     case EALREADY:  // WSF_CONNECT
          r_message = "The socket is non-blocking and a previous connection attempt has not yet been completed.";
          break;

     case EBADF:
          switch( p_functionid )
          {
            case WSF_SEND:
            case WSF_RECV:
                 r_message = "s is an invalid file descriptor.";
                 break;

            case WSF_ACCEPT:
                 r_message = "The descriptor is invalid.";
                 break;

            case WSF_CONNECT:
            case WSF_BIND:
                 r_message = "s is not a valid descriptor.";
                 break;

            case WSF_LISTEN:
                 r_message = "The argument s is not a valid file descriptor.";
                 break;
          }
          break;

     case ECONNREFUSED:  // WSF_CONNECT
          r_message = "The attempt to connect was forcefully rejected.  The calling  program  should close(2) the socket descriptor, and issue another socket(3N) call to obtain a new descriptor before attempting another connect() call.";
          break;

     case EINPROGRESS:  // WSF_CONNECT
          r_message = "The socket is non-blocking and the connection cannot be completed immediately.  It is possible to select(3C) for completion by selecting the  socket for writing.  However, this is only possible if the socket STREAMS module is the topmost module on the protocol  stack  with  a  write service procedure.  This will be the normal case.";
          break;

     case EINTR:
          switch( p_functionid )
          {
            case WSF_SEND:
                 r_message = "The operation was interrupted by delivery of a signal before any data could be buffered to be sent.";
                 break;

            case WSF_RECV:
                 r_message = "The operation was interrupted by delivery of a signal before any data was available to be received.";
                 break;

            case WSF_ACCEPT:
                 r_message = "The accept attempt was interrupted by the delivery of a signal.";
                 break;

            case WSF_CONNECT:
                 r_message = "The connection attempt was interrupted before any data arrived by the delivery of a signal.";
                 break;
          }
          break;

     case EINVAL:
          switch( p_functionid )
          {
            case WSF_SEND:
                 r_message = "tolen is not the size of a valid address for the specified address family.";
                 break;

            case WSF_BIND:
                 r_message = "The socket is already bound to an address. or namelen is not the size of a valid address for the specified address family.";
                 break;

            case WSF_CONNECT:
                 r_message = "namelen is not the size of a valid address for the specified address family.";
                 break;
          }
          break;

     case EIO:
          switch( p_functionid )
          {
            case WSF_CONNECT:
            case WSF_RECV:
                 r_message = "An I/O error occurred while reading from or writing to the file system.";
                 break;
          }

     case EISCONN:  // WSF_CONNECT
          r_message = "The socket is already connected.";
          break;

     case ELOOP:  // WSF_CONNECT
          r_message = "Too many symbolic links were encountered in translating the pathname in name.";
          break;

     case EMFILE:
          switch( p_functionid )
          {
            case WSF_SOCKET:
            case WSF_ACCEPT:
                 r_message = "The per-process descriptor table is full."
                 break;
          }
          break;

     case EMSGSIZE:  // WSF_SEND
          r_message = "The socket requires that message be sent atomically, and the message was too long.";
          break;

     case ENETUNREACH:  // WSF_CONNECT
          r_message = "The network is not reachable from this host.";
          break;

     case ENODEV:  // WSF_ACCEPT
          r_message = "The protocol family and type corresponding to s could not be found in the netconfig file.";
          break;

     case ENOENT:  // WSF_CONNECT
          r_message = "The socket referred to by the pathname in name does not exist.";
          break;

     case ENOMEM:
          switch( p_functionid )
          {
            case WSF_SEND:
                 r_message = "There was insufficient memory available to complete the operation.";
                 break;

            case WSF_RECV:
                 r_message = "There was insufficient user memory available for the operation to complete.";
                 break;

            case WSF_SOCKET:
                 r_message = "Insufficient user memory is available.";
                 break;

            case WSF_ACCEPT:
                 r_message = "There was insufficient user memory available to complete the operation.";
                 break;
          }
          break;

     case ENOSR:
          switch( p_functionid )
          {
            case WSF_SEND:
            case WSF_RECV:
                 r_message = "There were insufficient STREAMS resources available for the operation to complete.";
                 break;

            case WSF_ACCEPT:
            case WSF_CONNECT:
            case WSF_SOCKET:
                 r_message = "There were insufficient STREAMS resources available to complete the operation.";
                 break;

            case WSF_BIND:
                 r_message = "There were insufficient STREAMS resources for the operation to complete.";
                 break;
          }
          break;

     case ENOTSOCK:  // WSF_BIND
          switch( p_functionid )
          {
            case WSF_SEND:
            case WSF_RECV:
                 r_message = "s is not a socket.";
                 break;

            case WSF_ACCEPT:
                 r_message = "The descriptor does not reference a socket.";
                 break;

            case WSF_BIND:
                 r_message = "s is a descriptor for a file, not a socket.";
                 break;

            case WSF_LISTEN:
                 r_message = "The argument s is not a socket.";
                 break;
          }
          break;

     case ENXIO:  // WSF_CONNECT
          r_message = "The server exited before the connection was complete.";
          break;

     case EOPNOTSUPP:
          switch( p_functionid )
          {
            case WSF_ACCEPT:
                 r_message = "The referenced socket is not of type SOCK_STREAM.";
                 break;

            case WSF_LISTEN:
                 r_message = "The socket is not of a type that supports the operation listen().";
                 break;
          }
          break;

     case EPROTO:  // WSF_ACCEPT
          r_message = "A protocol error has occurred; for example, the STREAMS protocol stack has not been initialized or the connection has already been released.";
          break;

     case EPROTONOSUPPORT:  // WSF_SOCKET
          r_message = "The protocol type or the specified protocol is not supported within this domain.";
          break;

     case ESTALE:  // WSF_RECV
          r_message = "A stale NFS file handle exists.";
          break;

     case ETIMEDOUT:  // WSF_CONNECT
          r_message = "Connection establishment timed out without establishing a connection.";
          break;

     case EWOULDBLOCK:
          switch( p_functionid )
          {
            case WSF_SEND:
            case WSF_RECV:
            case WSF_CONNECT:
                 r_message = "The socket is marked as non-blocking, and the requested operation would block.";
                 break;

            case WSF_ACCEPT:
                 r_message = "The socket is marked as non-blocking and no connections are present to be accepted.";
                 break;
          }
          break;
#else
#error socket_exception.cpp not complete for this O/S
#endif

   }
   return r_message;
}

