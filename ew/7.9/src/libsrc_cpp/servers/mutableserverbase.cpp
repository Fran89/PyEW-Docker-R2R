// mutableserverbase.cpp: implementation of the MutableServerBase class.
//
//////////////////////////////////////////////////////////////////////


#include "mutableserverbase.h"


#define THR_KEY_LISTENER -1
#define THR_KEY_STACKER  -2
#define THR_KEY_HANDLER  -3

// this must follow the other includes
extern "C" {
#include <transport.h>
}

// Implementation of mode names,
// this array must match the enum MUTABLE_MODE_TYPE
// towards the top of mutableserverbase.h
//
const char * MutableServerBase::MSB_MODE_NAME[] =
{
  ""
, "Standalone"
, "Module"
, "Server"
, "Client"
};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MutableServerBase::MutableServerBase()
{
   MaxMessageLength = 0;
   MessageBuffer = NULL;

   LoggingOptions = WORM_LOG_TOFILE;

   Mode = MMT_NONE;

   ConnectFailureSec = -1;

   MessageMutex = NULL;
  
   ServeLogoCount = 0;

   MaxClients = 1;

   MaxServerTryLoopCount = 2;

   TransmitBuffer = NULL;
   TransmitBufferLength = 0;

   InputRingName = "";
   InputRingKey = WORM_RING_INVALID;
   InputRegionStruct.addr = NULL;
   InputRegion = &InputRegionStruct;
   
   OutputRingName = "";
   OutputRingKey = WORM_RING_INVALID;
   OutputRegionStruct.addr = NULL;
   OutputRegion = &OutputRegionStruct;
      
}
//
//-------------------------------------------------------------------
//
MutableServerBase::~MutableServerBase()
{
   if (   OutputRingKey != WORM_RING_INVALID
       && OutputRingKey != InputRingKey
       && OutputRingKey != CommandRingKey
      )
   {
      if ( OutputRegionStruct.addr != NULL )
      {
         TLogger::Logit( LoggingOptions
                       , "~MutableServerBase(): detaching from output ring\n"
                       );
         tport_detach( &OutputRegionStruct );
         OutputRegionStruct.addr = NULL;
      }

      OutputRingKey = WORM_RING_INVALID;
   }

   if (   InputRingKey != WORM_RING_INVALID
       && InputRingKey != CommandRingKey
      )
   {
      if ( InputRegionStruct.addr != NULL )
      {
         TLogger::Logit( LoggingOptions
                       , "~MutableServerBase(): detaching from input ring\n"
                       );
         tport_detach( &InputRegionStruct );
         InputRegionStruct.addr = NULL;
      }

      InputRingKey = WORM_RING_INVALID;
   }

   if ( CommandRegion.addr != NULL )
   {
      TLogger::Logit( LoggingOptions
                    , "~MutableServerBase(): detaching from command ring\n"
                    );
      tport_detach( &CommandRegion );
      CommandRegion.addr = NULL;
   }

   if ( MessageMutex != NULL )
   {
      delete( MessageMutex );
   }

   if ( MessageBuffer != NULL )
   {
      delete [] MessageBuffer;
   }

   if ( TransmitBuffer != NULL )
   {
      delete [] TransmitBuffer;
   }

}
//
//-------------------------------------------------------------------
//
HANDLE_STATUS MutableServerBase::HandleConfigLine( ConfigSource * p_parser )
{
   // Do not manipulate ConfigState herein.
   //
   HANDLE_STATUS r_handled = HANDLER_USED;

   try
   {
      char * _token;

      int _ival;

      do
      {
         if ( p_parser->Its("ImA") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("Incomplete <ImA> line");
            }

            if ( p_parser->Its("StandAlone") )
            {
               Mode = MMT_STANDALONE;
               continue;
            }

            if ( p_parser->Its("Module") )
            {
               Mode = MMT_MODULE;
               continue;
            }

            if ( p_parser->Its("Server") )
            {
               Mode = MMT_SERVER;
               continue;
            }

            if ( p_parser->Its("Client") )
            {
               Mode = MMT_CLIENT;
               continue;
            }
         }


         // - - - - - - - - - - - - - - - - - - - - - - - - -
         // Client
         //
         if ( p_parser->Its("Provider") )
         {
            PROVIDER_ADDR _provider;

            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("Incomplete <Provider> line");
            }
            _provider.IPAddr = _token;

            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("Incomplete <Provider> line");
            }
            _provider.Port = _token;

            Providers.push_back( _provider );

            continue;
         }

         if ( p_parser->Its("ConnectFailDelay") )
         {
            if ( (_ival = p_parser->Int()) == p_parser->INVALID_INT )
            {
               throw worm_exception("Invalid or incomplete <ConnectFailDelay> line");
            }
            ConnectFailureSec = _ival;

            continue;
         }


         // - - - - - - - - - - - - - - - - - - - - - - - - -
         // Server
         //
         // see WormServerBase::HandleConfigLine()


         // - - - - - - - - - - - - - - - - - - - - - - - - -
         // Module
         //
         if ( p_parser->Its("InputRing") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("Missing <InputRing> value");
            }
            InputRingName = _token;

            if ( (InputRingKey = TGlobalUtils::LookupRingKey(InputRingName.c_str())) == WORM_RING_INVALID )
            {
               throw worm_exception("invalid <InputRing> value");
            }

            continue;
         }

         if ( p_parser->Its("OutputRing") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <OutputRing> value");
            }
            OutputRingName = _token;

            if ( (OutputRingKey = TGlobalUtils::LookupRingKey(OutputRingName.c_str())) == WORM_RING_INVALID )
            {
               throw worm_exception("invalid <OutputRing> value");
            }

            continue;
         }

         if ( p_parser->Its("AcceptLogo") )
         {
            if ( ServeLogoCount == SERVE_MAX_LOGOS )
            {
               throw worm_exception("too many <AcceptLogo> lines");
            }

            // Institution

            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <AcceptLogo> institution value");
            }

            ServeLogo[ServeLogoCount].instid = (WORM_INSTALLATION_ID)TGlobalUtils::LookupInstallationId(_token);

            // Module

            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <AcceptLogo> module value");
            }

            ServeLogo[ServeLogoCount].mod = (WORM_MODULE_ID)TGlobalUtils::LookupModuleId(_token);

            // Type

            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <AcceptLogo> type value");
            }

            ServeLogo[ServeLogoCount].type = (WORM_MSGTYPE_ID)TGlobalUtils::LookupMessageTypeId(_token);

            ServeLogoCount++;

            continue;
         }


         // - - - - - - - - - - - - - - - - - - - - - - - - -
         // Module or Server
         //

         if ( p_parser->Its("MaxMessageLength") )
         {
            int _wrkint = p_parser->Int();
            if ( _wrkint == ConfigSource::INVALID_INT )
            {
               throw worm_exception("Missing or invalid <MaxMessageLength> value");
            }
            MaxMessageLength = _wrkint;

            if ( (MessageBuffer = new char[MaxMessageLength]) == NULL )
            {
               throw worm_exception("failed allocating MessageBuffer");
            }
         }


         // Give the base class a whack at it

         r_handled = WormServerBase::HandleConfigLine( p_parser );


      } while ( false );
   }
   catch( worm_exception _we )
   {
      r_handled = HANDLER_INVALID;
      ConfigState = WORM_STAT_BADSTATE;
      TLogger::Logit( LoggingOptions
                   , "MutableServerBase::HandleConfigLine(): configuration error:\n%s\n"
                   , _we.what()
                   );
   }

   return r_handled;
}
//
//-------------------------------------------------------------------
//
void MutableServerBase::CheckConfig()
{

   switch ( Mode )
   {
     case MMT_NONE:
          TLogger::Logit( LoggingOptions
                        , "MutableServerBase::CheckConfig(): <ImA> line is required\n"
                        );
          ConfigState = WORM_STAT_BADSTATE;

          break;

     case MMT_CLIENT:

          if ( Providers.size() == 0 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig(): client mode requires at least one <Provider> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          if ( ConnectFailureSec < 0 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() client mode requires <ConnectFailureSec> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }
          
          if ( SendTimeoutMS == -2 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() client mode requires <SendTimeoutMSecs> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          if ( RecvTimeoutMS < MIN_RECV_TIMEOUT_MS )
          {
             TLogger::Logit( LoggingOptions
                           , "WormServerBase::CheckConfig(): <RecvTimeoutMSecs> too low in config file, using %d\n"
                           , MIN_RECV_TIMEOUT_MS
                           );
             RecvTimeoutMS = MIN_RECV_TIMEOUT_MS;
          }

          break;

 
     case MMT_SERVER:

          if ( strlen(ServerIPAddr) == 0 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() server mode requires <ServerIPAddr> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          if ( ServerPort == 0 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() server mode requires <ServerPort> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          if ( CommandRingKey != WORM_RING_INVALID )
          {
             if ( MaxMessageLength == 0 )
             {
                TLogger::Logit( LoggingOptions
                              , "MutableServerBase::CheckConfig() Error: missing or invalid <MaxMessageLength> line\n"
                              );
                ConfigState = WORM_STAT_BADSTATE;
             }
          }

          break;
 
     case MMT_MODULE:

          if ( CommandRingKey == WORM_RING_INVALID )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() module mode requires <CmdRingName> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          if ( InputRingKey == WORM_RING_INVALID )
          {
             InputRingKey = CommandRingKey;
          }

          if ( OutputRingKey == WORM_RING_INVALID )
          {
             OutputRingKey = InputRingKey;
          }

          if ( MaxMessageLength == 0 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() Error: missing or invalid <MaxMessageLength> line\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          ResultMsgLogo.instid = TGlobalUtils::GetThisInstallationId();
          ResultMsgLogo.mod    = TGlobalUtils::GetThisModuleId();

          if ( ServeLogoCount == 0 )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() Error: no valid <AcceptLogo> lines found\n"
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          if ( (ResultMsgLogo.type = TGlobalUtils::LookupMessageTypeId(OutputMessageTypeKey())) == WORM_MSGTYPE_INVALID )
          {
             TLogger::Logit( LoggingOptions
                           , "MutableServerBase::CheckConfig() Error: id not found for output message type %s\n"
                           , OutputMessageTypeKey()
                           );
             ConfigState = WORM_STAT_BADSTATE;
          }

          break;
 
   }

   
   // - - - - - - - - - - - - - - - - - - - - - - - - -
   // Standalone
   //

//   TGlobalUtils::SetFileLoggingState( false );

//   Mode = MMT_STANDALONE;

}
//
//-------------------------------------------------------------------
//
bool MutableServerBase::CheckForThreadDeath()
{
   try
   {
      switch( Mode )
      {
        case MMT_MODULE:

             if ( (LastStackerPulse + 6) < time(NULL) )
             {
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), 0, "stacker thread stopped" );
                throw worm_exception("Stacker thread stopped pulsing");
             }

             if ( (LastHandlerPulse + 6) < time(NULL) )
             {
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), 0, "handler thread stopped" );
                throw worm_exception("Handler thread stopped pulsing");
             }
          
             break;

        case MMT_SERVER:

             // length of acceptable delay in listener pulse is tied to the
             // length of time it takes for a new service thread to start up,
             // (since the listener is incapacitated during that time).
             //
             if ( (LastListenerPulse + 10 + 3) < time(NULL) )
             {
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), 0, "listener thread stopped" );
                throw worm_exception("Listener thread stopped pulsing");
             }
          
             break;
      }
   }
   catch( worm_exception _we )
   {
      return true;
   }

   return false;
}
//
//-------------------------------------------------------------------
//
void MutableServerBase::StartThreadFunc( void * p_arg )
{
   switch ( *((int *)p_arg) )
   {
     case THR_KEY_LISTENER:
          Listener(p_arg);  // p_arg is ignored
          break;

     case THR_KEY_STACKER:
          Stacker();  // p_arg is ignored
          break;

     case THR_KEY_HANDLER:
          Handler();  // p_arg is ignored
          break;

     default:
          ClientServicer( p_arg ); // p_arg is socket descriptor
          break;
   }
}
//
//-------------------------------------------------------------------
//
// Server and Module
//
WORM_STATUS_CODE MutableServerBase::MainThreadActions()
{
   static long CurrentTime
             , LastBeatTime
             ;

      // set running state to keep worker threads from dying
      //
      Running = true;

      //
      // START WORKER THREADS
      //
      int _arg;

      switch( Mode )
      {
        case MMT_MODULE:

             if ( (MessageMutex = new TMutex("msbmbmtx")) == NULL )
             {
                throw worm_exception("Failed creating Message buffer mutex");
             }

             // Start processing thread
             //
             LastHandlerPulse = time(NULL) + 5; // allow an extra 5 seconds to start
             _arg = THR_KEY_HANDLER;
             if ( StartThreadWithArg( THREAD_STACK, &HandlerThreadId, &_arg ) == WORM_STAT_FAILURE )
             {
                throw worm_exception("Server failed starting handler thread");
             }
             // give the thread a chance to start
             TTimeFuncs::MSecSleep(300);

             // Start stacker thread
             //
             LastStackerPulse = time(NULL) + 5; // allow an extra 5 seconds to start
             _arg = THR_KEY_STACKER;
             if ( StartThreadWithArg( THREAD_STACK, &StackerThreadId, &_arg ) == WORM_STAT_FAILURE )
             {
                throw worm_exception("Server failed starting stacker thread");
             }
             // give the thread a chance to start
             TTimeFuncs::MSecSleep(250);
          
             break;

        case MMT_SERVER:

             if ( SocketDebug )
             {
                // Turn Socket level debugging On/Off
                setSocket_ewDebug(1);
             }

             // Start the Listener Thread
             //
             LastListenerPulse = time(NULL) + 5; // allow an extra 5 seconds to start
             _arg = THR_KEY_LISTENER;
             if ( StartThreadWithArg( THREAD_STACK, &ListenerThreadId, &_arg ) == WORM_STAT_FAILURE )
             {
                throw worm_exception("Server failed starting listener thread");
             }
             // give the thread a chance to start
             TTimeFuncs::MSecSleep(250);

             break;
      }


      int shutdown_flag;

      // initialize main heartbeat times
      //
      time(&CurrentTime);
      LastBeatTime = CurrentTime;

      
      while ( Running )
      {
         // Module and Server check for shutdown commands
         //
         if ( CommandRingKey != WORM_RING_INVALID )
         {
            // Check for and respond to stop flags
            shutdown_flag = tport_getflag(&CommandRegion);
            if( shutdown_flag == TERMINATE  ||  shutdown_flag  == (int)TGlobalUtils::GetPID() )
            {
               Running = false; // signal all threads to return
               continue;
            }

            // Send heartbeat if it is time
            if( TGlobalUtils::GetHeartbeatInt() <= (time(&CurrentTime) - LastBeatTime) )
            {
               LastBeatTime = CurrentTime;
               SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_HEARTBEAT"), 0, "" );
            }
         }


         // signal handler sets global utils terminate flag, check that
         //
         if ( TGlobalUtils::GetTerminateFlag() )
         {
            Running = false; // signal all threads to return
            continue;
         }


         if ( CheckForThreadDeath() )
         {
            Running = false;
         }
         else
         {
            TTimeFuncs::MSecSleep(500);
         }

      } // Running


   Running = false;

   // All errors are thrown
   return WORM_STAT_SUCCESS;
}
//
//-------------------------------------------------------------------
//
WORM_STATUS_CODE MutableServerBase::Run( int argc, char* argv[] )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   MutableServerRequest * _requestcontainer = NULL;

   MutableServerResult * _resultcontainer  = NULL;

   try
   {
      if ( ConfigState != WORM_STAT_SUCCESS )
      {
         throw worm_exception("MutableServerBase::Run() not properly configured");
      }

      // Set up the signal handler so we can shut down gracefully
	   //
      signal(SIGINT, (SIG_HANDLR_PTR)SignalHandler);     /* <Ctrl-C> interrupt */
      signal(SIGTERM, (SIG_HANDLR_PTR)SignalHandler);    /* program termination request */
      signal(SIGABRT, (SIG_HANDLR_PTR)SignalHandler);    /* abnormal termination */
#ifdef SIGBREAK
      signal(SIGBREAK, (SIG_HANDLR_PTR)SignalHandler);   /* keyboard break */
#endif

#ifdef _SOLARIS
      // Ignore broken socket signals
      (void)sigignore(SIGPIPE);
#endif


      // Call base class's PrepareToRun() to init logging level
      // and to perform other initialization needed by derivative
      // classes.
      //
      if ( ! PrepareToRun() )
      {
         throw worm_exception("PrepareToRun() returned not ready");
      }


      switch( Mode )
      {
        case MMT_CLIENT:
   
             LoggingOptions = WORM_LOG_TOFILE;

             if ( (_requestcontainer = GetRequestContainer()) == NULL )
             {
                throw worm_exception("Failed creating request container");
             }

             if ( GetRequestFromInput( argc, argv, _requestcontainer ) != WORM_STAT_SUCCESS )
             {
                throw worm_exception("GetRequestFromInput() returned error");
             }

             if ( (_resultcontainer = GetResultContainer()) == NULL )
             {
                throw worm_exception("Failed creating result container");
             }

             if ( TransmitRequest( _requestcontainer, _resultcontainer ) != WORM_STAT_SUCCESS )
             {
                throw worm_exception("TransmitRequest() returned error");
             }

             // Handle special case of client that will return 
             // WORM_STAT_SUCCESS  = got good result
             // WORM_STAT_BADSTATE = did not get good result
             // WORM_STAT_FAILURE  = system failure
             //
             if ( (r_status = HandleResult( _resultcontainer )) == WORM_STAT_FAILURE )
             {
                throw worm_exception("HandleResult() returned error");
             }
             
             break;


        case MMT_STANDALONE:

             LoggingOptions = WORM_LOG_TOFILE;

             if ( (_requestcontainer = GetRequestContainer()) == NULL )
             {
                throw worm_exception("Failed creating request container");
             }

             if ( GetRequestFromInput( argc, argv, _requestcontainer ) != WORM_STAT_SUCCESS )
             {
                throw worm_exception("GetRequestFromInput() returned error");
             }

             if ( (_resultcontainer = GetResultContainer()) == NULL )
             {
                throw worm_exception("Failed creating result container");
             }

             if ( (r_status = ProcessRequest(_requestcontainer, _resultcontainer)) == WORM_STAT_SUCCESS )
             {
                if ( HandleResult( _resultcontainer ) != WORM_STAT_SUCCESS )
                {
                   throw worm_exception("HandleResult() returned error");
                }
             }

             break;


        case MMT_MODULE:

             LoggingOptions = WORM_LOG_TOFILE|WORM_LOG_TOSTDERR;

             tport_attach( &CommandRegion, CommandRingKey );
             
             MainThreadActions();

             tport_detach( &CommandRegion );

             break;


        case MMT_SERVER:

             LoggingOptions = WORM_LOG_TOFILE|WORM_LOG_TOSTDERR;

             if ( CommandRingKey != WORM_RING_INVALID )
             {
                tport_attach( &CommandRegion, CommandRingKey );
             }
             
             MainThreadActions();

             if ( CommandRingKey != WORM_RING_INVALID )
             {
                tport_detach( &CommandRegion );
             }

             break;
      }

   }
   catch( worm_exception & _we )
   {
      Running = false;
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::Run() Error: %s\n"
                    , _we.what()
                    );
      r_status = WORM_STAT_FAILURE;
   }
   catch( ... )
   {
      Running = false;
      r_status = WORM_STAT_FAILURE;
   }

   if ( _requestcontainer != NULL )
   {
      delete( _requestcontainer );
   }

   if ( _resultcontainer != NULL )
   {
      delete( _resultcontainer );
   }

   return r_status;
}
//
//-------------------------------------------------------------------
//
WORM_STATUS_CODE MutableServerBase::TransmitRequest( MutableServerRequest * p_request
                                                   , MutableServerResult  * r_result
                                                   )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   SOCKET _clientsocket = INVALID_SOCKET;

   char * _buffer = NULL;

   try
   {
      if ( p_request == NULL || r_result == NULL )
      {
         throw worm_exception("NULL parameter pointer");
      }

      switch( Mode )
      {
        case MMT_CLIENT:
             // send via socket
             {
                int                 _lastprovider = 0
                  ,                 _currprovider = 0
                  ;
                PROVIDER_ADDR       _provider_addr;
                struct sockaddr_in  _socket_addr;
                

                bool _needresult = true;

   /* TODO: get last provider id from persistence store  */

                _currprovider = _lastprovider;

                // Willing to try each provider up to 2 times
                //
                for ( int _pass = 0 ; _needresult && _pass < MaxServerTryLoopCount ; _pass++ )
                {

                   if ( _pass != 0 )
                   {
                      // sleep before trying to connect again
                      TTimeFuncs::MSecSleep(250);
                   }


                   do
                   {
                      _currprovider++;


                      if ( Providers.size() <= _currprovider )
                      {
                         _currprovider = 0;
                      }

                      _provider_addr = Providers[_currprovider];

                      // Fill socket address structure with  address and port
                      memset( (char *)&_socket_addr, 0, sizeof(_socket_addr) );
                      _socket_addr.sin_family = AF_INET;
                      _socket_addr.sin_port = htons( (short)atoi(_provider_addr.Port.c_str()) );

                      if ( (int)(_socket_addr.sin_addr.S_un.S_addr = inet_addr(_provider_addr.IPAddr.c_str())) == INADDR_NONE )
                      {
                         worm_exception _expt("inet_addr failed for ");
                                        _expt += _provider_addr.IPAddr;
                                        _expt += ":";
                                        _expt += _provider_addr.Port;
                         throw _expt;
                      }

                      if( WORM_LOG_DEBUG <= TGlobalUtils::GetLoggingLevel() )
                      {
                         TLogger::Logit( LoggingOptions
                                       , "Attempting connection to provider %d: %s:%s\n"
                                       , _currprovider
                                       , _provider_addr.IPAddr.c_str()
                                       , _provider_addr.Port.c_str()
                                       );
                      }


                      if ( ( _clientsocket = socket_ew( AF_INET, SOCK_STREAM, 0) ) == INVALID_SOCKET )
                      {
                         throw worm_exception("socket_ew() failed");
                      }

                      if ( _clientsocket == SOCKET_ERROR )
                      {
                         _clientsocket = INVALID_SOCKET;
                         throw worm_exception("socket_ew() failed");
                      }


                      if ( connect_ew( _clientsocket
                                     , (struct sockaddr * )&_socket_addr
                                     , sizeof(_socket_addr)
                                     , ConnectFailureSec
                                     ) != 0 )
                      {
                         if( WORM_LOG_DETAILS <= TGlobalUtils::GetLoggingLevel() )
                         {
                            TLogger::Logit( LoggingOptions
                                         , "connect_ew(): failed for %s:%s\n"
                                         , _provider_addr.IPAddr.c_str()
                                         , _provider_addr.Port.c_str()
                                         );
                         }
                      }
                      else
                      {

                         // have a connection to a server
                         //

                         if( WORM_LOG_DEBUG <= TGlobalUtils::GetLoggingLevel() )
                         {
                            TLogger::Logit( LoggingOptions
                                          , "Connected to %s:%s\n"
                                          , _provider_addr.IPAddr.c_str()
                                          , _provider_addr.Port.c_str()
                                          );
                         }


                         if ( (_buffer = new char[GetMaxSocketBufferSize()]) == NULL )
                         {
                            throw worm_exception("failed to allocate socket buffer");
                         }


                         // Set up thread info for base class handling
                         //
                         ServiceThreadInfoStruct _thr_info;

                         _thr_info.descriptor = _clientsocket;
                         strcpy ( _thr_info.ipaddr , _provider_addr.IPAddr.c_str() );
                
                         ThreadsInfo[_clientsocket] = _thr_info;

                               
                         p_request->FormatBuffer();

                         int _msglen = p_request->GetBufferLength();

                         strcpy( _buffer, p_request->GetBuffer() );

                         // wait a quarter second for the server socket to prepare
                         //
                         TTimeFuncs::MSecSleep(250);


                         if( WORM_LOG_DEBUG <= TGlobalUtils::GetLoggingLevel() )
                         {
                            TLogger::Logit( LoggingOptions
                                          , "sending to descriptor %d (%d); message [%d]:\n%s<\n"
                                          , (int)_clientsocket
                                          , SendTimeoutMS
                                          , _msglen
                                          , _buffer
                                          );
                         }


                         if ( SendMessage(  _clientsocket
                                         ,  _buffer
                                         , &_msglen
                                         )
                              == WORM_STAT_FAILURE )
                         {
                            throw worm_exception("Failed sending request");
                         }


                         // willing to wait at least 5 seconds for 
                         time_t _quit_time = time(NULL)
                                           + ( RecvTimeoutMS / 1000 < 5 ? 5 : RecvTimeoutMS / 1000 )
                                           ;


                         bool _readfinished = false;

                         do
                         {
                            // Get message line from socket
               
                            _msglen = GetMaxSocketBufferSize();

                            switch( ListenForMsg(  _clientsocket
                                                ,  _buffer
                                                , &_msglen // in = max read ; out = actually read
                                                ,   500   // 500 ms for this try
                                                ) )
                            {
                              case  0:  // got message line
                                   _readfinished = r_result->ParseMessageLine( _buffer );
                                   break;
                              case -1:  // timed out
                                   if ( _quit_time < time(NULL) )
                                   {
                                      throw worm_exception("timed out waiting for server result");
                                   }
                                   break;
                              case -2:  // socket closed on other end
                                    throw worm_exception("Server closed socket");
                              case -3:  // socket error
                                    throw worm_exception("Socket error");
                              case -4:  // message too large for buffer
                                    throw worm_exception("Server response too large for buffer");
                            }
        

                         } while( ! _readfinished );


                         // Leave the loop unless the server responded that it
                         // was busy, which case try another server
                         //
                         if ( r_result->GetStatus() != MSB_RESULT_BUSY )
                         {
                            _needresult = false;
                         }

                      } // obtained a server connection

                      // Loop until connected or all providers tried once
                   } while(   _needresult
                           && _currprovider != _lastprovider 
                          );

                } // try all providers for n passes


                if ( _needresult )
                {
                   if(   WORM_LOG_STATUS <= TGlobalUtils::GetLoggingLevel()
                      &&                    TGlobalUtils::GetLoggingLevel() < WORM_LOG_DETAILS
                     )
                   {
                         TLogger::Logit( LoggingOptions
                                       , "Unable to establish connection to providers:\n"
                                       );
                      for ( int _p = 0 ; _p < Providers.size() ; _p )
                      {
                         TLogger::Logit( LoggingOptions
                                       , "   %s:%s\n"
                                       , Providers[_p].IPAddr.c_str()
                                       , Providers[_p].Port.c_str()
                                       );
                      }
                   }
                   r_status = WORM_STAT_DISCONNECT;
                }


             } // client Mode
             break;

        default:
             // other modes invalid
             {
                worm_exception _expt("Invalid call for ");
                               _expt +=                MSB_MODE_NAME[Mode];
                               _expt +=                                   " mode.";
                throw _expt;
             }
      }
   }
   catch( worm_exception & _we )
   {
      r_status = WORM_STAT_FAILURE;

      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "MutableServerBase::TransmitRequest(): %s\n"
                      , _we.what()
                      );
      }
   }
   catch( ... )
   {
      r_status = WORM_STAT_FAILURE;

      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "MutableServerBase::TransmitRequest(): Unknown error\n"
                      );
      }
   }


   if ( _clientsocket != INVALID_SOCKET )
   {
      closesocket_ew( _clientsocket, SOCKET_CLOSE_IMMEDIATELY_EW );
   }

   return r_status;
}
//
//-------------------------------------------------------------------
//
WORM_STATUS_CODE MutableServerBase::TransmitResult(       MutableServerResult * p_result
                                                  , const SOCKET              * p_socketdescriptor /* = NULL */
                                                  )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   MutableServerResult * _workresult = p_result;
   bool _myresult = false;

   try
   {
      int _neededlength;

      switch( Mode )
      {
        case MMT_SERVER:
             // send via socket
             if ( p_socketdescriptor == NULL )
             {
                throw worm_exception("SOCKET pointer NULL");
             }
             else
             {
                int    _msglen;

                SOCKET _socket = (SOCKET)*p_socketdescriptor;

                if ( _workresult == NULL )
                {
                   if ( WORM_LOG_ERRORS <= LoggingLevel )
                   {
                      TLogger::Logit( LoggingOptions
                                    , "NULL result container pointer\n"
                                    );
                   }
                   if ( (_workresult = GetResultContainer()) == NULL )
                   {
                      throw worm_exception("Failed to create work result buffer for error");
                   }
                   _myresult = true;
                   _workresult->SetStatus( MSB_RESULT_ERROR );
                }

                _workresult->FormatBuffer();
                _msglen = p_result->GetBufferLength();


TLogger::Logit( LoggingOptions
              , "DEBUG [%d] returning status to client at %d; message [%d]:\n%s<\n"
              , SendTimeoutMS
              , (int)_socket
              , _msglen
              , p_result->GetBuffer()
              );

                if ( SendMessage(  _socket
                                ,  p_result->GetBuffer()
                                , &_msglen
                                ) != WORM_STAT_SUCCESS
                   )
                {
                   throw worm_exception("Failed sending result across socket");
                }
             }
             break;

        case MMT_MODULE:

             // send via ring

             if ( p_result == NULL )
             {
                // this represents an error condition ("-1\n\n")
                _neededlength = 4;
             }
             else
             {
                p_result->FormatBuffer();
                _neededlength = p_result->GetBufferLength();
             }

             if ( TransmitBufferLength < _neededlength )
             {
                if ( TransmitBuffer != NULL )
                {
                   delete [] TransmitBuffer;
                }
                TransmitBuffer = NULL;
             }

             if ( TransmitBuffer == NULL )
             {
                if ( (TransmitBuffer = new char[p_result->GetBufferLength()+100]) == NULL )
                {
                   throw worm_exception("Failed allocating transmit buffer");
                }
                TransmitBufferLength = p_result->GetBufferLength()+100;
             }


             TransmitBuffer[0] = '\0';

             if ( p_result == NULL )
             {
                strcpy( TransmitBuffer , "-1\n\n" );
             }
             else
             {
                strcpy( TransmitBuffer , p_result->GetBuffer() );
             }

             if ( tport_putmsg(  OutputRegion
                              , &ResultMsgLogo
                              ,  _neededlength
                              ,  TransmitBuffer
                              )
                  != PUT_OK
                )
             {
                throw worm_exception("tport_putmsg() failed");
             }

             break;

        default:
             // other modes invalid
             {
                worm_exception _expt("invalid call for ");
                               _expt +=                MSB_MODE_NAME[Mode];
                               _expt +=                                  " mode.";
                throw _expt;
             }
      }
   }
   catch( worm_exception & _we )
   {
      r_status = WORM_STAT_FAILURE;

      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "MutableServerBase::TransmitResult(): %s\n"
                      , _we.what()
                      );
      }
   }

   if ( _myresult && _workresult != NULL )
   {
      delete ( _workresult );
   }

   return r_status;
}
//
//-------------------------------------------------------------------
//
THREAD_RETURN MutableServerBase::Stacker()
{
   static InstallWildcard = TGlobalUtils::LookupInstallationId("INST_WILDCARD");
   static ModuleWildcard  = TGlobalUtils::LookupModuleId("MOD_WILDCARD");
   static MessageWildcard = TGlobalUtils::LookupMessageTypeId("TYPE_WILDCARD");

   int      _getmsg_status;
	MSG_LOGO _arrivelogo;      // logo of arriving message
   long     _arr_msg_len;     // length of arriving message
   bool     _msg_ready;
   char     _errormsg[300];   //

   bool _attached = false;

TLogger::Logit( LoggingOptions
              , "MutableServerBase::Stacker(): A\n"
              );

   try
   {
      LastStackerPulse = time(NULL); // "I'm still alive"


      if ( InputRingKey == CommandRingKey )
      {
         InputRegion = &CommandRegion;
      }
      else
      {
         InputRegion = &InputRegionStruct;
         tport_attach( InputRegion, InputRingKey );
         _attached = true;
      }


      // Skip any messages already in ring
      //
      do
      {
         _getmsg_status = tport_getmsg(  InputRegion
                                      ,  ServeLogo
                                      ,  ServeLogoCount
                                      , &_arrivelogo
                                      , &_arr_msg_len
                                      ,  MessageBuffer
                                      ,  MaxMessageLength
                                      );
      }
      while ( _getmsg_status != GET_NONE );
      

      // Add wildcard to those accepted

      while ( Running )
      {
         LastStackerPulse = time(NULL); // "I'm still alive"

         _msg_ready = true;

         switch( (_getmsg_status = tport_getmsg(  InputRegion
                                               ,  ServeLogo
                                               ,  ServeLogoCount
                                               , &_arrivelogo
                                               , &_arr_msg_len
                                               ,  MessageBuffer
                                               ,  MaxMessageLength
                                               ))
               )
         {
           case GET_OK:
                break;

           case GET_MISS:
           case GET_MISS_LAPPED:
                // report error, handle message
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), MSB_ERR_MISSMSG, "missed messages" );
                break;

           case GET_MISS_SEQGAP:
                // report error, handle message
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), MSB_ERR_MISSMSG, "saw sequence gap" );
                break;

           case GET_NOTRACK:
                // report error, handle message
                sprintf( _errormsg
                       , "no tracking for logo i%d m%d t%d in %s"
                       , (int)_arrivelogo.instid
                       , (int)_arrivelogo.mod
                       , (int)_arrivelogo.type
                       , InputRingName
                       );
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), MSB_ERR_NOTRACK, _errormsg );
                break;

           case GET_TOOBIG:
                // report error, return to main loop to sleep
                sprintf( _errormsg
                       , "tport msg[%ld] i%d m%d t%d too big. Max is %ld"
                       , _arr_msg_len
                       , (int)_arrivelogo.instid
                       , (int)_arrivelogo.mod
                       , (int)_arrivelogo.type
                       , MaxMessageLength
                       );
                SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), MSB_ERR_TOOBIG, _errormsg );

                _msg_ready = false;

               break;

           case GET_NONE:

                TTimeFuncs::MSecSleep(500);
                _msg_ready = false;

               break;

         }  // switch( tport_getmsg() )


         if ( _msg_ready )
         {
            // NULL-terminate string buffer
            MessageBuffer[_arr_msg_len] = 0;

            // truncate end-of-line if present
            if ( MessageBuffer[_arr_msg_len-1] == '\n' )
            {
               _arr_msg_len--;
               MessageBuffer[_arr_msg_len] = 0;
            }

            for ( int _i = 0, _sz = ServeLogoCount ; _i < _sz ; _i++ )
            {
               if (   (   ServeLogo[_i].instid == InstallWildcard
                       || _arrivelogo.instid   == InstallWildcard
                       || _arrivelogo.instid   == ServeLogo[_i].instid
                      )
                   && (   ServeLogo[_i].mod == ModuleWildcard
                       || _arrivelogo.mod   == ModuleWildcard
                       || _arrivelogo.mod   == ServeLogo[_i].mod
                      )
                   && (   ServeLogo[_i].type == MessageWildcard
                       || _arrivelogo.type   == MessageWildcard
                       || _arrivelogo.type   == ServeLogo[_i].type
                      )
                  )
               {
                  std::string _newmsg = MessageBuffer;

                  // put the new message on the queue for the handler thread
                  MessageQueue.push( _newmsg );

                  break;
               }
            } // check each logo for match
         } // msg_ready

      } // Running

   }
   catch( worm_exception & _we )
   {
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::Stacker() Error: %s"
                    , _we.what()
                    );
      TransmitResult( NULL );
   }
   catch( ... )
   {
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::Stacker() Unknown Error"
                    );
      TransmitResult( NULL );
   }

   if ( _attached )
   {
      tport_detach( InputRegion );
   }
}
//
//-------------------------------------------------------------------
//
THREAD_RETURN MutableServerBase::Handler()
{

   bool _attached = false;

   std::string _message;

   MutableServerRequest * _requestcontainer = NULL;

   MutableServerResult * _resultcontainer = NULL;

   try
   {
      LastHandlerPulse = time(NULL); // "I'm still alive"


      if ( (_requestcontainer = GetRequestContainer()) == NULL )
      {
         throw worm_exception("failed to create request container");
      }

      if ( (_resultcontainer = GetResultContainer()) == NULL )
      {
         throw worm_exception("failed to create result container");
      }


      if      ( OutputRingKey == CommandRingKey )
      {
         OutputRegion = &CommandRegion;
      }
      else if ( OutputRingKey == InputRingKey )
      {
         OutputRegion = &InputRegionStruct;
      }
      else
      {
         OutputRegion = &OutputRegionStruct;
         tport_attach( OutputRegion, OutputRingKey );
         _attached = true;
      }


      WORM_STATUS_CODE _processing_state;


      while ( Running )
      {
         LastHandlerPulse = time(NULL); // "I'm still alive"

         if ( MessageQueue.empty() )
         {
            TTimeFuncs::MSecSleep(500);
         }
         else
         {
            // get the next message
            _message = MessageQueue.front();

            // remove the message from the queue
            MessageQueue.pop();

            try
            {
               _requestcontainer->ParseFromBuffer( _message.c_str() );
            }
            catch( worm_exception & _we )
            {
               TLogger::Logit( LoggingOptions
                             , "MutableServerBase::Handler() Error parsing message:\n%s\n%s\n"
                             , _message.c_str()
                             , _we.what()
                             );

               //  loop for next messsage
               continue;
            }
         

            _processing_state = ProcessRequest( _requestcontainer, _resultcontainer );

            if ( _processing_state == WORM_STAT_FAILURE )
            {
               throw worm_exception("ProcessRequest() returned error");
            }

            if ( TransmitResult( _resultcontainer ) != WORM_STAT_SUCCESS )
            {
               // failure to transmit here means that we're unable
               // to send the response to the ring -- should die
               throw worm_exception("ParseRequestFromMessage() returned error");
            }

         } // handling pending message

      } // Running

   }
   catch( worm_exception & _we )
   {
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::Handler() Error: %s\n"
                    , _we.what()
                    );
      TransmitResult( NULL );
   }
   catch( ... )
   {
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::Handler() Unknown Error\n"
                    );
      TransmitResult( NULL );
   }


   if ( _attached )
   {
      tport_detach( OutputRegion );
   }


   if ( _requestcontainer != NULL )
   {
      delete( _requestcontainer );
   }

   if ( _resultcontainer != NULL )
   {
      delete( _resultcontainer );
   }
}
//
//-------------------------------------------------------------------
//
THREAD_RETURN MutableServerBase::ClientServicer( void * p_socketdescriptor )
{
   if ( p_socketdescriptor == NULL )
   {
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::ClientServicer() Error: NULL descriptor parameter\n"
                    );
      return;
   }

   char * _buffer = NULL;

   MutableServerRequest * _requestcontainer = NULL;

   MutableServerResult * _resultcontainer = NULL;


   SOCKET _mapkey = *((SOCKET *)p_socketdescriptor);

   bool _send_error_message = false;

TLogger::Logit( LoggingOptions
              , "MutableServerBase::ClientServicer(): descriptor %d\n"
              , (int)_mapkey
              );

   try
   {
      // tell the main thread this thread is alive
      ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"
      ThreadsInfo[_mapkey].status = THREAD_INITIALIZING;

      if ( ThreadsInfo.size() == MaxClients )
      {
         // no more service threads allowed, tell client that
         // we're busy

         if ( (_resultcontainer = GetResultContainer()) == NULL )
         {
            throw worm_exception("Failed creating result container");
         }

         ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"
         ThreadsInfo[_mapkey].status = THREAD_WAITING;

         _resultcontainer->SetStatus( MSB_RESULT_BUSY );

         if ( TransmitResult( _resultcontainer, &_mapkey ) != WORM_STAT_SUCCESS )
         {
            throw worm_exception("TransmitResult() returned error");
         }
      }
      else
      {
         if ( (_buffer = new char[GetMaxSocketBufferSize()+1]) == NULL )
         {
            throw worm_exception("Failed getting request buffer");
         }

         if ( (_requestcontainer = GetRequestContainer()) == NULL )
         {
            throw worm_exception("Failed creating request container");
         }

         if ( (_resultcontainer = GetResultContainer()) == NULL )
         {
            throw worm_exception("Failed creating result container");
         }

         ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"
         ThreadsInfo[_mapkey].status = THREAD_WAITING;


         bool _messagefinished = false
            , _readfinished    = false
            ;

         int _length;


         // set number of seconds to wait for client's message
         //
         time_t _quit_time = time(NULL)
                           + ( RecvTimeoutMS / 1000 < 5 ? 5 : RecvTimeoutMS / 1000 )
                           ;

         if ( WORM_LOG_STATUS <= LoggingLevel )
         {
            TLogger::Logit( LoggingOptions
                          , "MutableServerBase::ClientServicer(): calling ListenForMsg(,,,%d)\n"
                          , RecvTimeoutMS
                          );
         }


         do
         {
            // Get message line from socket

         
            _length = GetMaxSocketBufferSize();

            switch ( ListenForMsg(  _mapkey
                                 ,  _buffer
                                 , &_length
                                 ,  500   // 500 ms for this pass
                                 )
                   )
            {
              case 0:    // success
                   _buffer[_length] = '\0';

                   if ( WORM_LOG_ERRORS <= LoggingLevel )
                   {
                      TLogger::Logit( LoggingOptions
                                    , "MutableServerBase::ClientServicer(): received msg line: %s\n"
                                    , _buffer
                                    );
                   }

                   _readfinished = _messagefinished = _requestcontainer->ParseMessageLine( _buffer );
/*
                   if ( ! (_messagefinished = _requestcontainer->ParseMessageLine( _buffer )) )
                   {
                      _readfinished = false;
                   }
                   else
                   {
                      _readfinished = true;
                   }
*/
                   break;

              case -1:  // timed out
                   if ( _quit_time < time(NULL) )
                   {
                      _send_error_message = true;
                      throw worm_exception("Client connection timed out");
                   }
                   break;

              case -2:  // client closed socket
                   throw worm_exception("Client closed connection");

              case -3:  // socket error
                   throw worm_exception("Socket error");

              case -4:  // msg too large for buffer
                   _send_error_message = true;
                   throw worm_exception("Message too large for transmit buffer");
            }


//TLogger::Logit( LoggingOptions
//              , "MutableServerBase::ClientServicer(): DEBUG M\n"
//              );
        

         } while( ! _readfinished );

TLogger::Logit( LoggingOptions
              , "MutableServerBase::ClientServicer(): DEBUG N\n"
              );

         ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"
         ThreadsInfo[_mapkey].status = THREAD_PROCESSING;

         if ( _messagefinished )
         {

TLogger::Logit( LoggingOptions
              , "MutableServerBase::ClientServicer(): DEBUG O -- message finished\n"
              );

            WORM_STATUS_CODE _state = ProcessRequest( _requestcontainer, _resultcontainer );

            ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"

            if ( _state == WORM_STAT_FAILURE )
            {
               throw worm_exception("ProcessRequest() returned error");
            }

            ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"

            if ( TransmitResult( _resultcontainer, &_mapkey ) != WORM_STAT_SUCCESS )
            {
               throw worm_exception("TransmitResult() returned error");
            }
         }
         else
         {
            throw worm_exception("Incomplete message received");
         }

//         ThreadsInfo[_mapkey].lastpulse = time(NULL); // "I'm still alive"
      } // service thread(s) not maxed out

      ThreadsInfo[_mapkey].status = THREAD_COMPLETED;
   
   }
   catch( worm_exception & _we )
   {
      ThreadsInfo[_mapkey].status = THREAD_ERROR;
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::ClientServicer(thr: %d) Error: %s\n"
                    , ThreadsInfo[_mapkey].threadid
                    , _we.what()
                    );
      if ( _send_error_message )
      {
         TransmitResult( NULL, &_mapkey );
      }
   }
   catch( ... )
   {
      ThreadsInfo[_mapkey].status = THREAD_ERROR;
      TLogger::Logit( LoggingOptions
                    , "MutableServerBase::ClientServicer(thr: %d) Unknown error\n"
                    , ThreadsInfo[_mapkey].threadid
                    );
      if ( _send_error_message )
      {
         TransmitResult( NULL, &_mapkey );
      }
   }


   // Close socket

   if ( _mapkey != NULL )
   {
      closesocket_ew( _mapkey, SOCKET_CLOSE_IMMEDIATELY_EW );
   }

   if ( _buffer != NULL )
   {
      delete[] _buffer;
   }

   if ( _requestcontainer != NULL )
   {
      delete( _requestcontainer );
   }

   if ( _resultcontainer != NULL )
   {
      delete( _resultcontainer );
   }
}

