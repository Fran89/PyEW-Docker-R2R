//---------------------------------------------------------------------------
#include "serverbase.h"


#include <worm_signal.h>
#include <worm_exceptions.h>
#include <logger.h>
#include <timefuncs.h> // MSecSleep()


// A Borland pragma, ignored by other compilers
#pragma package(smart_init)


#define ACCEPTABLE_SEC_TO_WAIT_FOR_SERVICE_THREAD_TO_START 6

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

bool WormServerBase::DoSCNLsMatch( const char * p_s1
                                 , const char * p_c1
                                 , const char * p_n1
                                 , const char * p_l1
                                 , const char * p_s2
                                 , const char * p_c2
                                 , const char * p_n2
                                 , const char * p_l2
                                 )
{
   // For each descriptor type (S, C, N, L)
   // a match is: either string starts with '*'
   //             an exact string match
   //
   if ( ! (   p_s1[0] == '*'
           || p_s2[0] == '*'
           || strcmp( p_s1, p_s2 ) == 0
      )   )
   {
      return false;
   }

   if ( ! (   p_c1[0] == '*'
           || p_c2[0] == '*'
           || strcmp( p_c1, p_c2 ) == 0
      )   )
   {
      return false;
   }

   if ( ! (   p_n1[0] == '*'
           || p_n2[0] == '*'
           || strcmp( p_n1, p_n2 ) == 0
      )   )
   {
      return false;
   }

   if ( ! (   p_l1[0] == '-'
           || p_l2[0] == '-'
           || strcmp( p_l1, p_l2 ) == 0
      )   )
   {
      return false;
   }

   return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
WormServerBase::WormServerBase()
{
   Running = false;
   CommandRingKey = WORM_RING_INVALID;
   CommandRegion.addr = NULL;
   strcpy( ServerIPAddr, "" );
   ServerPort = -1;
   SocketDebug = false;
   PassiveSocket = INVALID_SOCKET;
   MaxServiceThreads = 10;
   SendTimeoutMS = -2;
   RecvTimeoutMS = -1;

   try
   {
      if ( TGlobalUtils::LookupMessageTypeId("TYPE_HEARTBEAT") == WORM_MSGTYPE_INVALID )
      {
         throw worm_exception("message type <TYPE_HEARTBEAT> not defined");
      }

      if ( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR") == WORM_MSGTYPE_INVALID )
      {
         throw worm_exception("message type <TYPE_ERROR> not defined");
      }

      SocketSysInit();
   }
   catch( worm_exception _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase(): error: %s\n"
                    , _we.what()
                    );
   }
}
//---------------------------------------------------------------------------
WormServerBase::~WormServerBase()
{
   // tell all threads to quit
   Running = false;
   // give accept_ew a chance to return before closing socketing system
   TTimeFuncs::MSecSleep(1000);
   WSACleanup();
}
//---------------------------------------------------------------------------
int WormServerBase::ListenForMsg( SOCKET  p_descriptor
                                , char *  p_rcv_buffer
                                , int *   p_length // in = max read ; out = actually read
                                , int     p_timeoutms
                                )
{
//   if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
//   {
//      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
//                    , "WormServerBase.ListenForMsg(): listening at the socket %d...\n"
//                    , p_descriptor
//                    );
//   }

   int r_status = 0;

   int r_readcount = 0;

   int _readmax = *p_length;

   // if p_timeoutms greater than zero, then use it for the
   // timeout, otherwise use RecvTimeoutMS.  (In the method
   // declaration, p_timeoutms is defaulted to -1 -- so it
   // will be ignored).
   //
   int _timeoutms = ( p_timeoutms < 1 ? RecvTimeoutMS : p_timeoutms );

   // report actual number read
   *p_length = 0;

   bool _stillreading = true;

   try
   {
      if ( _timeoutms == -1 )
      {
         // tell the main thread that we're entering a potentially
         // blocked state so it will not kill this thread during a
         // long block.
         ThreadsInfo[p_descriptor].status = THREAD_BLOCKINGSOCKET;
      }

      // Listen to the socket for incoming characters until our buffer is full
      // or '\n' encountered....
      //
      do
      {

         if ( r_readcount == _readmax )
         {
            // buffer is maxed-out, can't read anymore.
            r_status = -4;
            throw worm_exception("message buffer overflow");
         }

         // tell the main thread this thread still alive
         ThreadsInfo[p_descriptor].lastpulse = time(NULL); // "I'm still alive"

         // although this single-char read is inefficient, messages from the clients
         // tend to be relatively rare and this method allows us to identify
         // the end-of-messge char easily.
         //
         // In addition, the handling for a socket time-out is optimistic,
         // specifically, it only cleanly handles the case of a timeout between
         // messages, not in the middle of one.
         //
         // Ultimately, this should be supplemented with an additional buffering
         // area that receives a larger amount from the socket, and can be manipulated
         // to identify message termination.
         //
//         if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
//         {
//            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDOUT|WORM_LOG_TIMESTAMP
//                          , "WormServerBase.ListenForMsg(): calling recv_ew( %d , %d )\n"
//                          , (int)p_descriptor
//                          , _timeoutms
//                          );
//         }

         int _err;
         switch( recv_ew( p_descriptor, &p_rcv_buffer[r_readcount], 1, 0, _timeoutms ) )
         {
           case 1:
                if ( p_rcv_buffer[r_readcount] == '\n' )
                {
                   /* If we got a newline, we're done gathering */
                   /* replace the \n with a zero */
                   p_rcv_buffer[r_readcount] = '\0';
                   _stillreading = false;
                }
                (*p_length)++;
                r_readcount++;
                break;

           case 0:
                r_status = -2;
                if ( WORM_LOG_DETAILS <= TGlobalUtils::GetLoggingLevel() )
                {
                   TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                 , "WormServerBase.ListenForMsg(): Client closed socket %d\n"
                                 , p_descriptor
                                 );
                }
                _stillreading = false;
                break;

           case SOCKET_ERROR:
#if defined(_WINNT) || defined(_Windows)
                switch( (_err = WSAGetLastError()) )
                {
                  case WSAEWOULDBLOCK:
                       // timed out -- nothing now available on non-blocking socket
                       r_status = -1;
                       _stillreading = false;
                       break;

                  case WSAECONNRESET:
                       r_status = -2;
                       if ( WORM_LOG_DETAILS <= TGlobalUtils::GetLoggingLevel() )
                       {
                          TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                        , "WormServerBase.ListenForMsg(): Other end closed socket %d\n"
                                        , p_descriptor
                                        );
                       }
                       _stillreading = false;
                       break;

                  default:
                       r_status = -3;
                       throw worm_socket_exception( WSF_RECV
                                                  , _err
                                                  , "Socket error from recv()"
                                                  );
                }
                break;
#endif
/* TODO : CHECK FOR TIMED-OUT ON NON_WINDOWS OS */

         } // switch recv_ew()

      } while( _stillreading );  // for 0 --> (p_maxlen - 1)
   }
   catch( worm_socket_exception & _se )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::ListenForMsg(): socket exception:\n%s\n"
                    , _se.DecodeError()
                    );
   }
   catch( worm_exception & _we )
   {
      if ( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                       , "WormServerBase.ListenForMsg(): Error - %s\n"
                       , _we.what()
                       );
      }
   }

   // tell the main thread this thread still alive
   ThreadsInfo[p_descriptor].lastpulse = time(NULL); // "I'm still alive"
   if ( _timeoutms == -1 )
   {
      // tell the main thread that we're entering a potentially
      // blocked state so it will not kill this thread during a
      // long block.
      ThreadsInfo[p_descriptor].status = THREAD_PROCESSING;
   }

   return r_status;
}
//---------------------------------------------------------------------------
WORM_STATUS_CODE WormServerBase::SendMessage( const SOCKET  p_descriptor
                                            , const char *  p_msg
                                            , int        *  p_length
                                            )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase.SendMessage(): sending message to %s  (timeout %d):\n%s<\n"
                    , ThreadsInfo[p_descriptor].ipaddr
                    , SendTimeoutMS
                    , p_msg
                    );
   }


   try
   {
      if ( p_msg == NULL )
      {
         throw worm_exception( "WormServerBase.SendMessage(): message string is NULL" );
      }

      int _sendcount = *p_length;
      if ( (*p_length = send_ew(p_descriptor, p_msg, _sendcount, 0, SendTimeoutMS)) == SOCKET_ERROR )
      {
         throw worm_socket_exception( WSF_SEND
                                    , socketGetError_ew()
                                    , "send_ew returned error"
                                    );
      }

      if ( _sendcount != *p_length )
      {
         throw worm_exception("bytes sent does not match message size");
      }
   }
   catch( worm_socket_exception& _se )
   {
      if ( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                       , "WormServerBase.SendMessage(): failed sending message to %s:\n  %s\n"
                       , ThreadsInfo[p_descriptor].ipaddr
                       , _se.DecodeError()
                       );
      }
      r_status = WORM_STAT_FAILURE;
   }
   catch( worm_exception _we )
   {
      if ( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                       , "WormServerBase.SendMessage(): Error - %s\n"
                       , _we.what()
                       );
      }
      r_status = WORM_STAT_FAILURE;
   }
   if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase.SendMessage(): exiting\n"
                    );
   }
   return r_status;
}
//---------------------------------------------------------------------------
void WormServerBase::CheckConfig()
{

   if ( TGlobalUtils::GetHeartbeatInt() < 1 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::CheckConfig(): No <HeartBeatInt> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( RecvTimeoutMS < MIN_RECV_TIMEOUT_MS )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::CheckConfig(): <RecvTimeoutMSecs> too low in config file, using %d\n"
                    , MIN_RECV_TIMEOUT_MS
                    );
      RecvTimeoutMS = MIN_RECV_TIMEOUT_MS;
   }

   if ( CommandRingKey == WORM_RING_INVALID )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::CheckConfig(): No <CmdRingName> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( strlen(ServerIPAddr) == 0 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::CheckConfig(): No <ServerIPAddr> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( ServerPort == -1 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::CheckConfig(): No <ServerPort> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( SendTimeoutMS == -2 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::CheckConfig(): No <SendTimeoutMSecs> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }
}
//---------------------------------------------------------------------------
HANDLE_STATUS WormServerBase::HandleConfigLine( ConfigSource * p_parser )
{
   // Do not manipulate ConfigState herein.
   //
   HANDLE_STATUS r_handled = HANDLER_USED;

   try
   {
      char * _token;

      do
      {

         if ( p_parser->Its("CmdRingName") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <CmdRingName> value");
            }
            strncpy( CommandRingName, _token, MAX_RINGNAME_LEN );

            if ( (CommandRingKey = TGlobalUtils::LookupRingKey(CommandRingName)) == WORM_RING_INVALID )
            {
               throw worm_exception("invalid <CmdRingName> value");
            }

            continue;
         }

         if( p_parser->Its( "ServerIPAddr" ) )
         {
            _token = p_parser->String();

            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <ServerIPAddr> value");
            }

            if ( 20 <= strlen(_token) )
            {
               throw worm_exception("<ServerIPAddr> name too long");
            }

            strcpy( ServerIPAddr, _token );
            continue;
         }

         if ( p_parser->Its("ServerPort") )
         {
            if ( (ServerPort = p_parser->Int()) == ConfigSource::INVALID_INT )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::HandleConfigLine(): Invalid <ServerPort> value in line\n%s\n"
                             , p_parser->GetCurrentLine()
                             );
            }
            continue;
         }

         if( p_parser->Its("SendTimeoutMSecs") )
         {
            // convert to milliseconds
            if ( (SendTimeoutMS = p_parser->Int()) < 0 )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::HandleConfigLine(): Invalid <SendTimeoutMSecs> line (must be > 0), reverting to 10000 ms\n"
                             );
               SendTimeoutMS = 10000;
            }
            continue;
         }

         if( p_parser->Its("RecvTimeoutMSecs") )
         {
            RecvTimeoutMS = p_parser->Int();
            if ( RecvTimeoutMS < -1 )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::HandleConfigLine(): Invalid <RecvTimeoutSecs> line, reverting to 200 ms\n"
                             );
               RecvTimeoutMS = 200;
            }
            continue;
         }


         if ( p_parser->Its("MaxServerThreads") )
         {
            MaxServiceThreads = p_parser->Int();
            if ( MaxServiceThreads <= 0 || SERVE_MAX_THREADS < MaxServiceThreads )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::HandleConfigLine(): <MaxServiceThreads> (%d) out of range (0 - %d)\n%s %d\n"
                             , MaxServiceThreads
                             , SERVE_MAX_THREADS
                             , "                                    setting to "
                             , SERVE_MAX_THREADS
                             );
               MaxServiceThreads = SERVE_MAX_THREADS;
            }
            continue;
         }

         if( p_parser->Its("SocketDebug") )
         {
            SocketDebug = true;
            continue;
         }

         r_handled = HANDLER_UNUSED;

      } while ( false );
   }
   catch( worm_exception _we )
   {
      r_handled = HANDLER_INVALID;
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                   , "WormServerBase::HandleConfigLine(): configuration error:\n%s%s\n"
                   , "                                    "
                   , _we.what()
                   );
   }

   return r_handled;
}
//---------------------------------------------------------------------------
/******************************************************************************
* SendStatus() builds a heartbeat or error message & puts it into    *
*                     shared memory.  Writes errors to log file & screen.    *
******************************************************************************/
void WormServerBase::SendStatus( unsigned char type, short p_ierr, char * p_text )
{
   if ( CommandRingKey != WORM_RING_INVALID )
   {
      MSG_LOGO    logo;
      char        msg[256];
      long        size;
      long        t;
	
      /* Build the message
      *******************/
      logo.instid = TGlobalUtils::GetThisInstallationId();
      logo.mod    = TGlobalUtils::GetThisModuleId();
      logo.type   = type;
	
      time( &t );

      strcpy( msg, "" );
   
      if( type == TGlobalUtils::LookupMessageTypeId("TYPE_HEARTBEAT") )
      {
         sprintf( msg, "%ld %ld\n\0", t, TGlobalUtils::GetPID());
      }
      else if( type == TGlobalUtils::LookupMessageTypeId("TYPE_ERROR") )
      {
         sprintf( msg, "%ld %hd %s\n\0", t, p_ierr, p_text);
      }
      else
      {
         return;
      }
	
      size = strlen( msg );   /* don't include the null byte in the message */

      /* Write the message to shared memory
      ************************************/

      if( tport_putmsg( &CommandRegion, &logo, size, msg ) != PUT_OK )
      {
         if( type == TGlobalUtils::LookupMessageTypeId("TYPE_HEARTBEAT") )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                          , "WormServerBase::SendStatus(): Error sending heartbeat.\n"
                          );
         }
         else // if( type == TGlobalUtils::LookupMessageTypeId("TYPE_ERROR") )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                          , "WormServerBase::SendStatus(): Error sending error: %d.\n"
                          , p_ierr
                          );
         }
      }
   }
}
//---------------------------------------------------------------------------
WORM_STATUS_CODE WormServerBase::Run()
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   static long CurrentTime
             , LastBeatTime
             ;

   try
   {
      if ( ConfigState != WORM_STAT_SUCCESS )
      {
         throw worm_exception("WormServerBase::Run() Server not configured");
      }

      if ( SocketDebug )
      {
         // Turn Socket level debugging On/Off
         setSocket_ewDebug(1);
      }

      tport_attach( &CommandRegion, CommandRingKey );

#ifdef _SOLARIS
      // Ignore broken socket signals
      (void)sigignore(SIGPIPE);
#endif

      // Set up the signal handler so we can shut down gracefully
	   //
	   signal(SIGINT, (SIG_HANDLR_PTR)SignalHandler);     /* <Ctrl-C> interrupt */
	   signal(SIGTERM, (SIG_HANDLR_PTR)SignalHandler);    /* program termination request */
	   signal(SIGABRT, (SIG_HANDLR_PTR)SignalHandler);    /* abnormal termination */
#ifdef SIGBREAK
	   signal(SIGBREAK, (SIG_HANDLR_PTR)SignalHandler);   /* keyboard break */
#endif

      if ( ! PrepareToRun() )
      {
         throw worm_exception("PrepareToRun() reported not ready");
      }


      // Start the Listener Thread
      //
      // alternative:     MyThreadFunction = &WormServerBase::Listener;
      LastListenerPulse = time(NULL) + 5; // allow an extra 5 seconds to start
      if ( StartThread( THREAD_STACK, &ListenerThreadId ) == WORM_STAT_FAILURE )
      {
         throw worm_exception("failed starting listener thread");
      }


      int flag;

      // initialize the Times
      //
      time(&CurrentTime);
      LastBeatTime = CurrentTime;

      Running = true;
      do
      {
         // Check for and respond to stop flags
         flag = tport_getflag(&CommandRegion);
         if( flag == TERMINATE  ||  flag == (int)TGlobalUtils::GetPID() )
         {
            Running = false; // signal all threads to return
            continue;
         }

         // +2 is to allow sleeps for same time
         if ( (LastListenerPulse + ACCEPTABLE_SEC_TO_WAIT_FOR_SERVICE_THREAD_TO_START + 2) < time(NULL) )
         {
            SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), 0, "listener thread stopped" );
            throw worm_exception("Listener thread stopped pulsing");
         }

         // Send server's heartbeat if it is time
         if( TGlobalUtils::GetHeartbeatInt() <= (time(&CurrentTime) - LastBeatTime) )
         {
            LastBeatTime = CurrentTime;
            SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_HEARTBEAT"), 0, "" );
         }

         // Handle logging, etc, in MainThreadActions()....
         // so, don't throw an error here
         //
         if ( MainThreadActions() == WORM_STAT_SUCCESS )
         {
            //
            TTimeFuncs::MSecSleep(500);
         }
         else
         {
            Running = false;
         }

      } while( Running );

   }
   catch( worm_exception _we )
   {
      r_status = WORM_STAT_FAILURE;
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::Run(): error: %s\n"
                    , _we.what()
                    );
   }

   TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDOUT|WORM_LOG_TIMESTAMP
                 , "WormServerBase::Run(): calling FinishedRunning()\n"
                 );

   // Tell the derivative classes to finish up
   FinishedRunning();

   if ( CommandRegion.addr != NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "WormServerBase::Listener(): detaching from command ring\n"
                    );
      tport_detach( &CommandRegion );
      CommandRegion.addr = NULL;
   }

   TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDOUT|WORM_LOG_TIMESTAMP
                 , "WormServerBase::Run(): returning.\n"
                 );

   return r_status;
}
//---------------------------------------------------------------------------
// THREAD_RETURN is some sort of void
//
// The sockaddr structure varies depending on the the protocol selected.
// The structure below is used with TCP/IP. Other protocols use similar structures.
//
// struct sockaddr_in {
//         short   sin_family;
//         u_short sin_port;
//         struct  in_addr sin_addr;
//         char    sin_zero[8];
// };
//
// struct hostent {
//     char FAR *       h_name;
//     char FAR * FAR * h_aliases;
//     short            h_addrtype;
//     short            h_length;
//     char FAR * FAR * h_addr_list;
// };
//
// Members
//h_name -- Official name of the host (PC).If using the DNS or similar resolution system,
//          it is the Fully Qualified Domain Name (FQDN) that caused the server to return a reply.
//          If using a local "hosts" file, it is the first entry after the IP address.
//h_aliases -- A NULL-terminated array of alternate names.
//h_addrtype -- The type of address being returned.
//h_length -- The length, in bytes, of each address.
//h_addr_list -- A NULL-terminated list of addresses for the host.
//               Addresses are returned in network byte order.
//               The macro h_addr is defined to be h_addr_list[0] for compatibility with older software.
//
THREAD_RETURN WormServerBase::Listener( void * p_dummy )
{

   if ( WORM_LOG_TRACKING <= TGlobalUtils::GetLoggingLevel() )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDOUT|WORM_LOG_TIMESTAMP
                    , "WormServerBase::Listener(): Starting\n"
                    );
   }

   try
   {
      int              _clientaddrlength;

/*
      // Fill in the hostent structure, given dot address as a char string
      //
//      unsigned long    ServerNBOAddr;
//      struct hostent * HostEnt;
//      struct  in_addr  hostentaddr;
//      short            hostentaddrlen;

      // TODO : CHECK inet_addr() ON UNIX
      if ( (ServerNBOAddr = inet_addr(ServerIPAddr)) == INADDR_NONE)
      {
         char _msg[40];
         sprintf( _msg, "inet_addr(%s) failed", ServerIPAddr );
         throw worm_exception(_msg);
      }
      // TODO : CHECK gethostbyaddr() ON UNIX
      HostEnt = gethostbyaddr((char *)&ServerNBOAddr, sizeof(ServerNBOAddr), AF_INET);
      if (HostEnt == NULL)
      {
         char _msg[80];
         sprintf( _msg
                , "gethostbyaddr() for %s failed"
                , ServerIPAddr
                );
         throw worm_exception(_msg);
      }

      memcpy( (char *)&hostentaddr, HostEnt->h_addr, HostEnt->h_length );
      hostentaddrlen = HostEnt->h_length;
*/

      while( Running )
      {
         LastListenerPulse = time(NULL);


         if ( PassiveSocket == INVALID_SOCKET )
         {
            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::Listener(): calling socket_ew()\n"
                             );
            }

            if ( ( PassiveSocket = socket_ew(AF_INET, SOCK_STREAM, 0) ) == INVALID_SOCKET )
            {
               throw worm_exception("socket_ew() returned error");
            }

            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::Listener(): calling setsockopt(SO_REUSEADDR)\n"
                             );
            }

            // Allows the server to reuse the address if problems (stop, crash, etc.)
            // kept the socket bound to a previous process
            //
            char _value = 1;
            if( setsockopt( PassiveSocket, SOL_SOCKET, SO_REUSEADDR, &_value, sizeof(char *) ) != 0 )
            {
               if ( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
               {
                  TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                , "setsockopt() failed setting SO_REUSEADDR, can't reuse socket if crash.\n"
                                );
               }
            }


/*
            struct sockaddr_in _serverstruct;

            // Fill in server's socket address structure
            //*******************************************
            memset( (char *)&_serverstruct, '\0', sizeof(_serverstruct) );
            memcpy( (char *)&_serverstruct.sin_addr, &hostentaddr, hostentaddrlen );
            _serverstruct.sin_family = AF_INET;
            _serverstruct.sin_port   = htons( (short)ServerPort );
*/
            struct sockaddr_in _serverstruct;

            // Fill in server's socket address structure
            //*******************************************
            memset( (char *)&_serverstruct, '\0', sizeof(_serverstruct) );

            _serverstruct.sin_family = AF_INET;
            _serverstruct.sin_port   = htons( (short)ServerPort );

            if ((int)(_serverstruct.sin_addr.S_un.S_addr = inet_addr(ServerIPAddr)) == -1)
            {
               worm_exception _expt( "inet_addr(" );
                              _expt += ServerIPAddr;
                              _expt += ") failed";
               throw _expt;
            }



            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::Listener(): calling bind_ew()\n"
                             );
            }

            // Bind socket to a name
            if ( bind_ew( PassiveSocket, (struct sockaddr *)&_serverstruct, sizeof(_serverstruct)) )
            {
               closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
               PassiveSocket = INVALID_SOCKET;
               throw worm_exception("bind_ew() returned error");
            }

            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::Listener(): calling listen_ew(%d, %d)\n"
                             , PassiveSocket
                             , MaxServiceThreads
                             );
            }

            // start listening
            if ( listen_ew( PassiveSocket, MaxServiceThreads ) )
            {
               closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
               PassiveSocket = INVALID_SOCKET;
               throw worm_exception("listen_ew() returned error");
            }

            LastListenerPulse = time(NULL);

         } // need to initialize server socket


         // =================================================================
         // Clean up the thread info structures
         //
         // Remove thread tracking structures for any threads in the
         // ERROR or COMPLETED state
         //
         // That is, assume that any thread in another state is still
         // talking successfully with its socket.
         //
         SERVICETHREADID_VECTOR _vec;      // list of items to be removed from list
         _vec.reserve(ThreadsInfo.size()); // adjust capacity

         SERVICETHREAD_MAP_ITERATOR _sttitr = ThreadsInfo.begin()
                                  , _enditr = ThreadsInfo.end()
                                  ;
         while ( _sttitr != _enditr )
         {
//            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
//            {
//               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
//                             , "WormServerBase::Listener(): checking thread %d (d: %d)\n"
//                             , _sttitr->second.threadid
//                             , _sttitr->second.descriptor
//                             );
//            }

            switch ( _sttitr->second.status )
            {
              case THREAD_ERROR:
                   TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                 , "WormServerBase::Listener(): service thread %d reported error\n"
                                 , _sttitr->second.threadid
                                 );
              case THREAD_COMPLETED:
              case THREAD_DISCONNECTED:
                   _vec.push_back( _sttitr->second.descriptor );
                   break;

              default:
                   if (   (_sttitr->second.lastpulse + SERVICE_THREAD_PULSE_MAX) < time(NULL)
                       && (_sttitr->second.status != THREAD_BLOCKINGSOCKET)
                      )
                   {
                      // Service thread found without a pulse for too long, and it is not
                      // making a possibly blocking socket call.
                      // Close socket and kill the thread
                      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                    , "WormServerBase::Listener(): service thread %d found hung, killing it\n"
                                    , _sttitr->second.threadid
                                    );
                      try
                      {
                         closesocket_ew( _sttitr->second.descriptor, SOCKET_CLOSE_SIMPLY_EW );
                      }
                      catch( ... ) { } // ensure that the thread is killed despite any closure error
                      KillThread( _sttitr->second.threadid );
                      _vec.push_back( _sttitr->second.descriptor );
                   }
                   break;
            }


            _sttitr++;
         }

         for ( SOCKET _t = 0 ; _t < _vec.size() ; _t++ )
         {
            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::Listener(): erasing thread %d\n"
                             , _vec[_t]
                             );
            }
            ThreadsInfo.erase(_vec[_t]);
         }


         // =================================================================
         // Try to accept new client
         //
         if ( ThreadsInfo.size() == MaxServiceThreads )
         {
            if ( WORM_LOG_STATUS <= TGlobalUtils::GetLoggingLevel() )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "WormServerBase::Listener(): maxed out on service threads, cannot service client\n"
                             );
            }
            TTimeFuncs::MSecSleep(WAIT_FOR_SERVICE_THREAD);
         }
         else
         {
//            if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
//            {
//               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
//                             , "WormServerBase::Listener(): waiting for new connection\n"
//                             );
//            }


            _ServiceThreadInfoStruct _newconn_info;
            _newconn_info.status = THREAD_STARTING;

            _clientaddrlength = sizeof(_newconn_info.sock_addr);

            // Don't use a blocking socket here (-1), so the loop can clean up
            // dead threads if needed.
            _newconn_info.descriptor = accept_ew( PassiveSocket
                                                , (struct sockaddr*)&_newconn_info.sock_addr
                                                , &_clientaddrlength
                                                , 100 // do not make this -1
                                                );

            if( _newconn_info.descriptor == INVALID_SOCKET )
            {
#if defined(_WINNT) || defined(_Windows)

               int _err = socketGetError_ew();
               switch( _err )
               {
                 case WSAEMFILE:
                      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                    , "WormServerBase::Listener(): accept_ew() returned error: no descriptors available\n"
                                    );
                      // queue not empty, no descriptors available (can try again later)
                      break;
                 case WSAEWOULDBLOCK:
//                      if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
//                      {
//                         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
//                                       , "WormServerBase::Listener(): accept_ew() returned error, no connection attempt\n"
//                                       );
//                      }
                      TTimeFuncs::MSecSleep(500);
                      break;
                 default:
                      throw worm_socket_exception( WSF_ACCEPT
                                                 , _err
                                                 , "Socket error returned from accept_ew()"
                                                 );
               }
#else
               /* TODO : FINISH NON-WINDOWS ERROR HANDLING */
               closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
               PassiveSocket = INVALID_SOCKET;
               continue;
#endif
            }
            else
            {
               strcpy( _newconn_info.ipaddr, inet_ntoa(_newconn_info.sock_addr.sin_addr) );

               // Start the Service thread
               //
               if ( WORM_LOG_STATUS <= TGlobalUtils::GetLoggingLevel() )
               {
                  TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                , "WormServerBase::Listener(): Connection accepted from IP address %s\n"
                                , _newconn_info.ipaddr
                                );
               }

               ThreadsInfo[_newconn_info.descriptor] = _newconn_info;

               TO_THREAD_ID _newthreadid;

               if ( StartThreadWithArg( (unsigned int)THREAD_STACK
                                      , &_newthreadid
                                      , &_newconn_info.descriptor
                                      )  == WORM_STAT_FAILURE )
               {
                  closesocket_ew( _newconn_info.descriptor, SOCKET_CLOSE_IMMEDIATELY_EW );
                  ThreadsInfo.erase(_newconn_info.descriptor);
                  if ( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
                  {
                     TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                   , "WormServerBase::Listener(): failed to start service thread\n"
                                   , _newconn_info.ipaddr
                                   );
                  }
                  continue;
               }

               ThreadsInfo[_newconn_info.descriptor].threadid = _newthreadid;

               // Check to see if the thread has awakened
               if ( WORM_LOG_DETAILS <= TGlobalUtils::GetLoggingLevel() )
               {
                  TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                , "WormServerBase::Listener(): waiting for thread %d to come alive\n"
                                , _newthreadid
                                );
               }

               LastListenerPulse = time(NULL);
               TTimeFuncs::MSecSleep(ACCEPTABLE_SEC_TO_WAIT_FOR_SERVICE_THREAD_TO_START * 1000);

               if (   0 < ThreadsInfo.count(_newconn_info.descriptor)
                   && ThreadsInfo[_newconn_info.descriptor].status == THREAD_STARTING
                  )
               {
                  if ( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
                  {
                     TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                                   , "WormServerBase::Listener(): Thread %d never went to busy, killing it\n"
                                   , _newthreadid
                                   );
                  }
                  // A confused thread running around out there,
                  // clean up the socket and kill it
                  //
                  KillThread( _newthreadid );
                  closesocket_ew( _newconn_info.descriptor, SOCKET_CLOSE_IMMEDIATELY_EW );
                  ThreadsInfo.erase(_newconn_info.descriptor);
               }

            } // accept_ew() successful

         } // thread available (not maxed out)

      } // Running == true
   }
   catch( worm_socket_exception& _se )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::Listener(): socket exception:\n%s\n"
                    , _se.DecodeError()
                    );
   }
   catch( worm_exception& _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "WormServerBase::Listener(): %s\n"
                    , _we.what()
                    );
   }


   if ( PassiveSocket != INVALID_SOCKET )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "WormServerBase::Listener(): closing passive socket\n"
                    );
      closesocket_ew( PassiveSocket, SOCKET_CLOSE_IMMEDIATELY_EW );
      PassiveSocket = INVALID_SOCKET;
   }

   TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                 , "WormServerBase::Listener(): returning\n"
                 );

}
