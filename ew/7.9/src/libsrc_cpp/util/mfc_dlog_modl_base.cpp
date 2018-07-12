// mfc_dlog_modl_base.cpp: implementation of the CMFCDialogModuleBase class.
//
//////////////////////////////////////////////////////////////////////

#include "mfc_dlog_modl_base.h"
#include <worm_environ.h>

#include <timefuncs.h>

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[]=__FILE__;
//#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMFCDialogModuleBase::CMFCDialogModuleBase()
{
   MyGlobalUtils = NULL;

   CommandRingKey = WORM_RING_INVALID;
   CommandRegion.addr = NULL;
   InputRingKey = WORM_RING_INVALID;
   InputRegion.addr = NULL;
   MaxMessageLength = 0;
   MessageBuffer = NULL;
   AcceptLogoCount = 0;
}

CMFCDialogModuleBase::~CMFCDialogModuleBase()
{
   if ( InputRingKey != CommandRingKey )
   {
      if ( InputRegion.addr != NULL )
      {
         tport_detach( &InputRegion );
         InputRegion.addr = NULL;
      }
   }
   if ( CommandRingKey )
   {
      if ( CommandRegion.addr != NULL )
      {
         tport_detach( &CommandRegion );
         CommandRegion.addr = NULL;
      }
   }
   if ( MessageBuffer != NULL )
   {
      delete( MessageBuffer );
      MessageBuffer = NULL;
   }
   if ( MyGlobalUtils != NULL )
   {
      delete( MyGlobalUtils );
      MyGlobalUtils = NULL;
   }
}

//////////////////////////////////////////////////////////////////////

bool CMFCDialogModuleBase::PrepApp(const char * p_configfilename)
{
   try
   {
      // ----- Get the global utilities before parsing the command file -----
      //
      // Need an instance of the TGlobalUtils class for holding global values.
      // (Once initialized, all further uses are through static class methods).
      //
      // The constructor reads the lookup files specified in the environment.
      // Then ParseCommandFile() allows any method-specific overrides.
      // 
      if ( (MyGlobalUtils = new TGlobalUtils(m_lpCmdLine)) == NULL )
      {
         throw worm_exception("failed creating global utilities object");
      }

      LoggingLevel = TGlobalUtils::GetLoggingLevel();

      // Change working directory to environment variable EW_PARAMS value
      char * runPath = NULL;

      bool _isEW = false;

      if ( (runPath = TGlobalUtils::GetEnvironmentValue( WORM_CONFIG_DIR )) == NULL )
      {
         _isEW = true;
         runPath = TGlobalUtils::GetEnvironmentValue( EW_CONFIG_DIR );
      }

      if ( runPath == NULL )
      {
         worm_exception _expt( "Neither environment variable <" );
         _expt += WORM_CONFIG_DIR;
         _expt += "> nor <";
         _expt += EW_CONFIG_DIR;
         _expt += "> is defined";
         throw _expt;
      }

      if ( *runPath == '\0' )
      {
         worm_exception _expt( "Environment variable " );
         _expt += (_isEW ? EW_CONFIG_DIR : WORM_CONFIG_DIR);
         _expt += " defined, but has no value";
         throw _expt;
      }

      if ( ! SetCurrentDirectory( runPath ) )
      {
         worm_exception _expt( "Params directory not found: " );
         _expt += runPath;
         _expt += "\nPlease set environment variable ";
         _expt += WORM_CONFIG_DIR;
         throw _expt;
      }
   }
   catch( worm_exception _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                    , "CMFCDialogModuleBase::PrepApp(): %s\n"
                    , _we.what()
                    );
      return false;
   }

   return true;
}

//////////////////////////////////////////////////////////////////////

HANDLE_STATUS CMFCDialogModuleBase::HandleConfigLine(ConfigSource *p_parser)
{
   // Do not manipulate ConfigState herein.
   //

   HANDLE_STATUS r_handled = MyGlobalUtils->HandleConfigLine( p_parser );


   if ( r_handled == HANDLER_UNUSED )
   {
      try
      {
         static char * _token;

         do
         {


            if( p_parser->Its("AcceptLogo") )
            {
               r_handled = HANDLER_USED;

               if ( AcceptLogoCount == SERVE_MAX_LOGOS )
               {
                  throw worm_exception("Attempt to load too many <AcceptLogo> lines");
               }

               // AcceptLogo	INST_WILDCARD	MOD_GLASS	TYPE_WILDCARD

               // INSTALLATION

               _token = p_parser->String();

               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Incomplete <AcceptLogo> line (no installation)");
               }

               AcceptLogo[AcceptLogoCount].instid = TGlobalUtils::LookupInstallationId(_token);

               if (   AcceptLogo[AcceptLogoCount].instid == TGlobalUtils::LookupInstallationId("INST_WILDCARD")
                   && strcmp(_token, "INST_WILDCARD") != 0
                  )
               {
                  worm_exception _expt("Unrecognized installation id: ");
                  _expt += _token;
                  throw _expt;
               }

               // MODULE

               _token = p_parser->String();

               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Incomplete <AcceptLogo> line (no module)");
               }

               AcceptLogo[AcceptLogoCount].mod = TGlobalUtils::LookupModuleId(_token);

               if (   AcceptLogo[AcceptLogoCount].mod == TGlobalUtils::LookupModuleId("MOD_WILDCARD")
                   && strcmp(_token, "MOD_WILDCARD") != 0
                  )
               {
                  worm_exception _expt("Unrecognized module id: ");
                  _expt += _token;
                  throw _expt;
               }

               // MESSAGE TYPE

               _token = p_parser->String();

               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Incomplete <AcceptLogo> line (no message type)");
               }

               AcceptLogo[AcceptLogoCount].type = TGlobalUtils::LookupMessageTypeId(_token);

               if (   AcceptLogo[AcceptLogoCount].type == TGlobalUtils::LookupMessageTypeId("TYPE_WILDCARD")
                   && strcmp(_token, "TYPE_WILDCARD") != 0
                  )
               {
                  worm_exception _expt("Unrecognized message type id: ");
                  _expt += _token;
                  throw _expt;
               }

               AcceptLogoCount++;

               continue;
            }

            if ( p_parser->Its("CmdRingName") )
            {
               r_handled = HANDLER_USED;
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

            if( p_parser->Its("InputRingName") )
            {
               r_handled = HANDLER_USED;
               _token = p_parser->String();
               if ( strlen(_token) == 0 )
               {
                 throw worm_exception("missing <InputRing> for input messages");
               }
               strncpy( InputRingName, _token, MAX_RINGNAME_LEN );

               if ( (InputRingKey = TGlobalUtils::LookupRingKey(InputRingName)) == WORM_RING_INVALID )
               {
                 throw worm_exception("invalid <InputRing>");
               }
               continue;
            }

            if( p_parser->Its("MaxMsgSize") )
            {
               r_handled = HANDLER_USED;
               int _maxmsglen = p_parser->Int();
               if ( _maxmsglen <= 0 )
               {
                 throw worm_exception("invalid <MaxMsgSize> for input messages");
               }
               MaxMessageLength = _maxmsglen;
               continue;
            }

         } while( false );
      }
      catch( worm_exception _we )
      {
         r_handled = HANDLER_INVALID;
         TLogger::Logit( WORM_LOG_TOFILE
                      , "CMFCDialogModuleBase::HandleConfigLine(): configuration error: %s\nin line: %s\n"
                      , _we.what()
                      , p_parser->GetCurrentLine()
                      );
      }
   }

   return r_handled;
}

/////////////////////////////////////////////////////////////////////////////

void CMFCDialogModuleBase::CheckConfig()
{
//   CMFCDialogAppBase::CheckConfig();

   if ( MyGlobalUtils == NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): Global Configurations object not created\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }
   else
   {
      LoggingLevel = TGlobalUtils::GetLoggingLevel();
      
      if ( ! MyGlobalUtils->IsReady() )
      {
         TLogger::Logit( WORM_LOG_TOFILE
                       , "CMFCDialogModuleBase::CheckConfig(): Global Configurations object not configured\n"
                       );
         ConfigState = WORM_STAT_BADSTATE;
      }
   }

   if ( TGlobalUtils::GetHeartbeatInt() < 1 )
   {
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): No <HeartBeatInt> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( AcceptLogoCount == 0 )
   {
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): No <AcceptLogo> lines found in the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( CommandRingKey == WORM_RING_INVALID )
   {
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): No <CmdRingName> value from the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }


 	if( (TYPE_HEARTBEAT = TGlobalUtils::LookupMessageTypeId("TYPE_HEARTBEAT")) == WORM_MSGTYPE_INVALID )
	{
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): Message type TYPE_HEARTBEAT not defined\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
	}

 	if( (TYPE_ERROR = TGlobalUtils::LookupMessageTypeId("TYPE_ERROR")) == WORM_MSGTYPE_INVALID )
	{
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): Message type TYPE_ERROR not defined\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
	}

   if ( MaxMessageLength <= 1 )
   {
      TLogger::Logit( WORM_LOG_TOFILE
                    , "CMFCDialogModuleBase::CheckConfig(): No valid <MaxMsgSize> found in the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }
}

//////////////////////////////////////////////////////////////////////

bool CMFCDialogModuleBase::InitApp()
{
   bool r_status = true;


   try
   {
      // Start status thread


      if ( (StatusThread = AfxBeginThread( StartMFCWorkerThread
                                         , this
                                         , 0
                                         , 0
                                         , NULL
                                         )) == NULL )
      {
         throw worm_exception("Failed starting the status thread");
      }
   
      // Five previous thread time to lookup thread type id variable
      TTimeFuncs::MSecSleep( 300 );

   }
   catch( worm_exception _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                   , "CMFCDialogModuleBase::InitApp(): Error: %s\n"
                   , _we.what()
                   );
      r_status = false;
   }

   return r_status;
}

//////////////////////////////////////////////////////////////////////

bool CMFCDialogModuleBase::BeforeMessage() { return true; }

//////////////////////////////////////////////////////////////////////

UINT CMFCDialogModuleBase::StartWorkerThread()
{
   return StatusAndReadLoop();
}

//////////////////////////////////////////////////////////////////////

UINT CMFCDialogModuleBase::StatusAndReadLoop()
{
   unsigned char InstallWildcard = TGlobalUtils::LookupInstallationId("INST_WILDCARD")
               , ModuleWildcard  = TGlobalUtils::LookupModuleId("MOD_WILDCARD")
               , MessageWildcard = TGlobalUtils::LookupMessageTypeId("TYPE_WILDCARD")
               ;

   UINT r_status = 0;

   try
   {
      // pause for second to allow the dialog window to initialize
      TTimeFuncs::MSecSleep( 1000 );

//      if ( ConfigState != WORM_STAT_SUCCESS )
//      {
//         throw worm_exception("WormServerBase::Run() Server not configured");
//      }

      // create the command/status ring writer

      tport_attach( &CommandRegion, CommandRingKey );


      if ( InputRingKey != WORM_RING_INVALID )
      {
         // having a valid ring key means that 
         // we want to be able to read messages from
         // an input ring.
         //

         if ( (MessageBuffer = new char[MaxMessageLength+1]) == NULL )
         {
            throw worm_exception("RingReaderServer::PrepareToRun(): failed allocating message buffer");
         }

         // The input region may or may not be the same as the command region
         //
         if ( InputRingKey != CommandRingKey )
         {
            tport_attach( &InputRegion, InputRingKey );
         }
         else
         {
            memcpy( (void *)&InputRegion , (void *)&CommandRegion, sizeof(InputRegion) );
         }
      }



      int flag;
   
      long CurrentTime
         , LastBeatTime
         ;
      bool     MsgIsReady = false;
      MSG_LOGO _arrivelogo;        // logo of arriving message
      long     _arr_msg_len;       // length of arriving message
      char     _errormsg[300];


      // initialize the Times
      //
      time(&CurrentTime);
      LastBeatTime = CurrentTime;


      // skip over any messages in the ring
      //
      while( tport_getmsg( &InputRegion
                         ,  AcceptLogo
                         ,  AcceptLogoCount
                         , &_arrivelogo
                         , &_arr_msg_len
                         ,  MessageBuffer
                         ,  MaxMessageLength
                         ) != GET_NONE );


      if ( ! BeforeMessage() )
      {
         throw worm_exception("BeforeMessage() returned error");
      }


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

         // Send heartbeat if it is time
         if( TGlobalUtils::GetHeartbeatInt() <= (time(&CurrentTime) - LastBeatTime) )
         {
            LastBeatTime = CurrentTime;
            SendStatus( TYPE_HEARTBEAT, 0, "" );
         }

         // perform actions that derivative classes need to evaluate
         // existence of fatal error.
         if ( CheckForFatal() )
         {
            Running = false;
            continue;
         }


         if ( InputRingKey != WORM_RING_INVALID )
         {
            // Have an input ring, try to get message
            // from transport ring
            //

            MsgIsReady = true;

            switch(
                    tport_getmsg( &InputRegion
                                ,  AcceptLogo
                                ,  AcceptLogoCount
                                , &_arrivelogo
                                , &_arr_msg_len
                                ,  MessageBuffer
                                ,  MaxMessageLength
                                )
                  )
            {
              case GET_OK:
                   break;

              case GET_MISS:
              case GET_MISS_LAPPED:
                   // report error, handle message
                   SendStatus( TYPE_ERROR, ERR_MISSMSG, "missed messages" );
                   break;

              case GET_MISS_SEQGAP:
                   // report error, handle message
                   SendStatus( TYPE_ERROR, ERR_MISSMSG, "saw sequence gap" );
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
                   SendStatus( TYPE_ERROR, ERR_NOTRACK, _errormsg );
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
                   SendStatus( TYPE_ERROR, ERR_TOOBIG, _errormsg );

                   // fall - through

              case GET_NONE:
                   // no message ready, prepare to sleep before looping
                   MsgIsReady = false;
                   break;
            }

         } // have input ring


         if ( MsgIsReady )
         {
            // NULL-terminate string buffer
            MessageBuffer[_arr_msg_len] = 0;

            // truncate end-of-line if present
            if ( MessageBuffer[_arr_msg_len-1] == '\n' )
            {
               _arr_msg_len--;
               MessageBuffer[_arr_msg_len] = 0;
            }

            for ( int _i = 0, _sz = AcceptLogoCount ; _i < _sz ; _i++ )
            {
               if (   (   AcceptLogo[_i].instid == InstallWildcard
                       || _arrivelogo.instid   == InstallWildcard
                       || _arrivelogo.instid   == AcceptLogo[_i].instid
                      )
                   && (   AcceptLogo[_i].mod == ModuleWildcard
                       || _arrivelogo.mod   == ModuleWildcard
                       || _arrivelogo.mod   == AcceptLogo[_i].mod
                      )
                   && (   AcceptLogo[_i].type == MessageWildcard
                       || _arrivelogo.type   == MessageWildcard
                       || _arrivelogo.type   == AcceptLogo[_i].type
                      )
                  )
               {
                  if ( ! HandleMessage(_arrivelogo, MessageBuffer) )
                  {
                     // HandleMessage() reported error state
                     Running = false;
                  }
                  break;
               }
            }

         } // message is ready
         else
         {
            // only sleep if no message to check
            TTimeFuncs::MSecSleep(500);
         }

      } while( Running );

   }
   catch( worm_exception _we )
   {
      r_status = WORM_STAT_FAILURE;
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                    , "CMFCDialogModuleBase::StatusAndReadLoop(): error: %s\n"
                    , _we.what()
                    );
   }

   if ( GetMainWindow() != NULL )
   {
      ((CDialog *)GetMainWindow())->EndDialog(IDCANCEL);
   }

   return r_status;
}

//////////////////////////////////////////////////////////////////////

void CMFCDialogModuleBase::HeartBeat()
{
   static long _lastBeat  = 0
             , _interval = TGlobalUtils::GetHeartbeatInt()
             ;

   if ( 0 < _interval )
   {
      if ( (_lastBeat + _interval) < time(NULL) )
      {
         SendStatus(TYPE_HEARTBEAT);
         _lastBeat = time(NULL);
      }
   }
}

//////////////////////////////////////////////////////////////////////

void CMFCDialogModuleBase::SendStatus( WORM_MSGTYPE_ID   p_type
                                     , short             p_ierr
                                     , const char      * p_text
                                     )
{
   if ( CommandRegion.addr == NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                    , "CMFCDialogModuleBase::SendStatus(): Command Ring share is NULL\n"
                    );
      return;
   }

   try
   {
      MSG_LOGO    logo;
	   char        msg[256];
	   long        size;
	   long        t;
	
      /* Build the message
      *******************/
      logo.instid = TGlobalUtils::GetThisInstallationId();
      logo.mod    = TGlobalUtils::GetThisModuleId();
      logo.type   = p_type;
	
      time( &t );

      strcpy( msg, "" );
   
      if( p_type == TYPE_HEARTBEAT )
      {
         sprintf( msg, "%ld %ld\n\0", t, TGlobalUtils::GetPID());
      }
      else if( p_type == TYPE_ERROR )
      {
         sprintf( msg, "%ld %hd %s\n\0", t, p_ierr, p_text);
      }
      else
      {
         if ( LoggingLevel == WORM_LOG_DEBUG )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                          , "CMFCDialogModuleBase::SendStatus(): invalid message type %d\n"
                          , p_type
                          );
         }
         return;
      }
	
      size = strlen( msg );   /* don't include the null byte in the message */

      /* Write the message to shared memory
      ************************************/

      if( tport_putmsg( &CommandRegion, &logo, size, msg ) != PUT_OK )
      {
         if( p_type == TYPE_HEARTBEAT )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                          , "WormServerBase::SendStatus(): Error sending heartbeat.\n"
                          );
         }
         else // if( type == TYPE_ERROR )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                          , "WormServerBase::SendStatus(): Error sending error: %d.\n"
                          , p_ierr
                          );
         }
      }
   }
   catch( ... )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TIMESTAMP
                    , "TModuleBase::SendStatus() Error\n"
                    );
      // so serious we must exit
      Running = false;
   }
}

//////////////////////////////////////////////////////////////////////
