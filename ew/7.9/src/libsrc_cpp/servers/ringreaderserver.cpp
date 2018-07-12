//---------------------------------------------------------------------------
#include "ringreaderserver.h"

//---------------------------------------------------------------------------

#include <logger.h>
#include <worm_exceptions.h>
#include <comfile.h>

#pragma package(smart_init)


//---------------------------------------------------------------------------
RingReaderServer::RingReaderServer()
{
   InputRingKey = WORM_RING_INVALID;
   InputRegion.addr = NULL;
   MaxMessageLength = 0;
   MessageBuffer = NULL;
   ServeLogoCount = 0;
}
//---------------------------------------------------------------------------
RingReaderServer::~RingReaderServer()
{
   if ( InputRingKey != CommandRingKey )
   {
      if ( InputRegion.addr != NULL )
      {
         tport_detach( &InputRegion );
         InputRegion.addr = NULL;
      }
   }
   if ( MessageBuffer != NULL )
   {
      delete( MessageBuffer );
   }
}
//---------------------------------------------------------------------------
HANDLE_STATUS RingReaderServer::HandleConfigLine( ConfigSource * p_parser )
{
   // Do not manipulate ConfigState herein.
   //
   HANDLE_STATUS r_handled = WormServerBase::HandleConfigLine(p_parser);

   if ( r_handled == HANDLER_UNUSED )
   {
      // base class didn't use it, check if this class does

      char * _token;

      try
      {
         do
         {

            if( p_parser->Its("ServeLogo") )
            {
               r_handled = HANDLER_USED;

               if ( ServeLogoCount == SERVE_MAX_LOGOS )
               {
                  throw worm_exception("Attempt to load too many <ServeLogo> lines");
               }

               // ServeLogo	INST_WILDCARD	MOD_GLASS	TYPE_WILDCARD

               // INSTALLATION

               _token = p_parser->String();

               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Incomplete <ServeLogo> line (no installation)");
               }

               ServeLogo[ServeLogoCount].instid = TGlobalUtils::LookupInstallationId(_token);

               if (   ServeLogo[ServeLogoCount].type == TGlobalUtils::LookupInstallationId("INST_WILDCARD")
                   && strcmp(_token, "INST_WILDCARD") != 0
                  )
               {
                  worm_exception _expt("Unrecognized installation id: ");
                  _expt += _token;
                  _expt += " in <ServeLogo> line";
                  throw _expt;
               }

               // MODULE

               _token = p_parser->String();

               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Incomplete <ServeLogo> line (no module)");
               }

               ServeLogo[ServeLogoCount].mod = TGlobalUtils::LookupModuleId(_token);

               if (   ServeLogo[ServeLogoCount].mod == TGlobalUtils::LookupModuleId("MOD_WILDCARD")
                   && strcmp(_token, "MOD_WILDCARD") != 0
                  )
               {
                  worm_exception _expt("Unrecognized module id: ");
                  _expt += _token;
                  _expt += " in <ServeLogo> line";
                  throw _expt;
               }

               // MESSAGE TYPE

               _token = p_parser->String();

               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Incomplete <ServeLogo> line (no message type)");
               }

               ServeLogo[ServeLogoCount].type = TGlobalUtils::LookupMessageTypeId(_token);

               if (   ServeLogo[ServeLogoCount].type == TGlobalUtils::LookupMessageTypeId("TYPE_WILDCARD")
                   && strcmp(_token, "TYPE_WILDCARD") != 0
                  )
               {
                  worm_exception _expt("Unrecognized message type id: ");
                  _expt += _token;
                  _expt += " in <ServeLogo> line";
                  throw _expt;
               }

               ServeLogoCount++;

               continue;
            }

            if( p_parser->Its("InputRing") )
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
      catch( worm_exception _rte )
      {
         r_handled = HANDLER_INVALID;
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                       , "RingReaderServer::HandleConfigLine(): configuration error:\n%s\n"
                       , _rte.what()
                       );
         r_handled = HANDLER_INVALID;
      }
   }
   return r_handled;
}
//---------------------------------------------------------------------------
void RingReaderServer::CheckConfig()
{
   WormServerBase::CheckConfig();

   if ( ServeLogoCount == 0 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "RingReaderServer::HaveMyConfigs(): No <ServeLogo> lines found in the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( InputRingKey == WORM_RING_INVALID )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "RingReaderServer::HaveMyConfigs(): No <InputRing> found in the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( MaxMessageLength <= 1 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                    , "RingReaderServer::HaveMyConfigs(): No valid <MaxMsgSize> found in the config file\n"
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }
}
//---------------------------------------------------------------------------
bool RingReaderServer::PrepareToRun()
{
   bool r_state = true;
   try
   {
      if ( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR") == WORM_MSGTYPE_INVALID )
      {
         throw worm_exception("RingReaderServer::PrepareToRun(): message type TYPE_ERROR not found in lookups");
      }

      if ( (MessageBuffer = new char[MaxMessageLength+1]) == NULL )
      {
         throw worm_exception("RingReaderServer::PrepareToRun(): failed allocating message buffer");
      }

       
      if ( (LoggingLevel = TGlobalUtils::GetLoggingLevel()) == WORM_LOG_DEBUG )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDOUT
                       , "RingReaderServer::PrepareToRun(): A  %s  %d  %d  (%d + 1)\n"
                       , InputRingName
                       , InputRingKey
                       , InputRegion.addr
                       , MaxMessageLength
                       );
      }

      if ( InputRingKey != CommandRingKey )
      {
         tport_attach( &InputRegion, InputRingKey );
      }
      else
      {
         memcpy( (void *)&InputRegion , (void *)&CommandRegion, sizeof(InputRegion) );
      }

      if ( TGlobalUtils::GetLoggingLevel() == WORM_LOG_DEBUG )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDOUT
                       , "RingReaderServer::PrepareToRun(): B  %s  %d  %d\n"
                       , InputRingName
                       , InputRingKey
                       , InputRegion.addr
                       );
      }
   }
   catch( worm_exception _rte )
   {
      r_state = false;
   }
   return r_state;
}
//---------------------------------------------------------------------------
WORM_STATUS_CODE RingReaderServer::MainThreadActions()
{
   static int      _getmsg_status;
	static MSG_LOGO _arrivelogo;      // logo of arriving message
   static long     _arr_msg_len;     // length of arriving message
   static bool     _msg_ready;
   static char     _errormsg[300];   //

   static InstallWildcard = TGlobalUtils::LookupInstallationId("INST_WILDCARD");
   static ModuleWildcard  = TGlobalUtils::LookupModuleId("MOD_WILDCARD");
   static MessageWildcard = TGlobalUtils::LookupMessageTypeId("TYPE_WILDCARD");

   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   try
   {
      _msg_ready = true;

      // Get a message from transport ring
      _getmsg_status = tport_getmsg( &InputRegion
                                   ,  ServeLogo
                                   ,  ServeLogoCount
                                   , &_arrivelogo
                                   , &_arr_msg_len
                                   ,  MessageBuffer
                                   ,  MaxMessageLength
                                   );

      switch( _getmsg_status )
      {
        case GET_OK:
             break;

        case GET_MISS:
        case GET_MISS_LAPPED:
             // report error, handle message
             SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), ERR_MISSMSG, "missed messages" );
             break;

        case GET_MISS_SEQGAP:
             // report error, handle message
             SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), ERR_MISSMSG, "saw sequence gap" );
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
             SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), ERR_NOTRACK, _errormsg );
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
             SendStatus( TGlobalUtils::LookupMessageTypeId("TYPE_ERROR"), ERR_TOOBIG, _errormsg );

             // fall - through

        case GET_NONE:
             _msg_ready = false;

             // no message ready, return to main loop to sleep
             break;
      }

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
               if ( ! MessageFromRing(_arrivelogo, MessageBuffer) )
               {
                  throw worm_exception("MessageFromRing() reported failure");
               }
               break;
            }
         }
      }
   }
   catch( worm_exception _rte )
   {
      r_status = WORM_STAT_FAILURE;
   }

   return r_status;
}
//---------------------------------------------------------------------------
void RingReaderServer::FinishedRunning()
{
   if ( InputRegion.addr != NULL )
   {
      if ( InputRingKey != CommandRingKey )
      {
         tport_detach( &InputRegion );
      }
      InputRegion.addr = NULL;
   }
}
//---------------------------------------------------------------------------

