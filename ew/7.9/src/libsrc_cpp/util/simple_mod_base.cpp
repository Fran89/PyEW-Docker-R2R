/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: simple_mod_base.cpp 1334 2004-03-17 17:35:20Z dhanych $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/03/17 17:31:28  dhanych
 *     for RCS
 *
 *     Revision 1.1  2003/09/05 19:12:07  dhanych
 *     Initial revision
 *
 *
 *
 */

#include "simple_mod_base.h"

extern "C" {

/* avoid including the socket garbage from earthworm_complex_funcs.h  */
void CreateSemaphore_ew( void );            /* sema_ew.c    system-dependent */
void PostSemaphore   ( void );              /* sema_ew.c    system-dependent */
void WaitSemPost     ( void );              /* sema_ew.c    system-dependent */
void DestroySemaphore( void );              /* sema_ew.c    system-dependent */
void CreateMutex_ew  ( void );              /* sema_ew.c    system-dependent */
void RequestMutex( void );                  /* sema_ew.c    system-dependent */
void ReleaseMutex_ew( void );               /* sema_ew.c    system-dependent */
void CloseMutex( void );                    /* sema_ew.c    system-dependent */
void CreateSpecificMutex( mutex_t * );
void CloseSpecificMutex( mutex_t * );
void RequestSpecificMutex( mutex_t * );
void ReleaseSpecificMutex( mutex_t * );
}

#include <worm_environ.h>
#include <comfile.h>

/*
 * -------------------------------------------------------------------------
 */
SimpleModuleBase::SimpleModuleBase()
{
   Running               = false;
   
   MainLoopSleepMS       = DEF_LOOP_SLEEP_MS;
   StackerSleepMS        = DEF_STACK_SLEEP_MS;
   HandlerSleepMS        = DEF_HANDLE_SLEEP_MS;
   
   MaxMessageSize        = 0;
   MaxQueueElements      = 0;
   MessageQueue.pQE      = NULL;
   
   LoggingLevel          = DEF_LOGGING_LEVEL;
   HBIntervalSec         = DEF_HEARTBEAT_INTV_SEC;
   
   MyInstallId           = WORM_INSTALLATION_INVALID;
   MyModuleId            = WORM_MODULE_INVALID;
   
   InputLogoCount        = 0;
   LogoAlloc             = 0;
   InputLogoList         = NULL;
   
   CommandRingName[0]    = '\0';
   CommandRingKey        = WORM_RING_INVALID;
   CommandRing.addr      = NULL;
   CommandRegion         = &CommandRing;

   InputRingName[0]      = '\0';
   InputRingKey          = WORM_RING_INVALID;
   InputRing.addr        = NULL;
   InputRegion           = &CommandRing;

   OutputRingName[0]     = '\0';
   OutputRingKey         = WORM_RING_INVALID;
   OutputRing.addr       = NULL;
   OutputRegion          = InputRegion;
   
   StackerInfo.threadid  = 0;
   StackerInfo.status    = THREAD_STATUS_QUIT;
   StackerInfo.lastpulse = 0;
   
   HandlerInfo.threadid  = 0;
   HandlerInfo.status    = THREAD_STATUS_QUIT;
   HandlerInfo.lastpulse = 0;
   
   if ( GetLocalInst( &MyInstallId ) < 0 )
   {
      sleep_ew( 2000 );
      throw worm_exception("Failed getting installation id");
   }

   if ( GetType( "TYPE_HEARTBEAT", &TYPE_HEARTBEAT ) != 0 )
   {
      sleep_ew( 2000 );
      throw worm_exception( "SimpleModuleBase(): GetType(\"TYPE_HEARTBEAT\") returned invalid");
   }

   if ( GetType( "TYPE_ERROR", &TYPE_ERROR ) != 0 )
   {
      sleep_ew( 2000 );
      throw worm_exception( "SimpleModuleBase(): GetType(\"TYPE_ERROR\") returned invalid");
   }
   
   CreateSpecificMutex( &QueueMutex );
//   if ( QueueMutex == 0 )
//   {
//      throw worm_exception( "SimpleModuleBase(): failed creating mutex for message queue");
//   }
}
/*
 * -------------------------------------------------------------------------
 */
SimpleModuleBase::~SimpleModuleBase()
{
   if ( OutputRing.addr != NULL )
   {
      tport_detach( &OutputRing );
      OutputRing.addr = NULL;
   }
   
   if ( InputRing.addr != NULL )
   {
      tport_detach( &InputRing );
      InputRing.addr = NULL;
   }
   
   if ( CommandRing.addr != NULL )
   {
      tport_detach( &CommandRing );
      CommandRing.addr = NULL;
   }
   
   if ( InputLogoList != NULL )
   {
      delete [] InputLogoList;
      InputLogoList = NULL;
   }
   
//   if ( QueueMutex != 0 )
//   {
      CloseSpecificMutex( &QueueMutex );
      
//   }

   if ( MessageQueue.pQE != NULL )
   {
      free( MessageQueue.pQE );
      MessageQueue.pQE = NULL;
   }
}
/*
 * -------------------------------------------------------------------------
 */
int SimpleModuleBase::EnsureMessageSize( unsigned long p_maxsize )
{
   if ( ! Running )
   {
      MaxMessageSize = ( p_maxsize < MaxMessageSize ? MaxMessageSize : p_maxsize );
      return 0;
   }
   return -1;
}
/*
 * -------------------------------------------------------------------------
 */
void SimpleModuleBase::SendMessageToRing(       SHM_INFO    * p_ring
                                        ,       MSG_LOGO    * p_logo
                                        , const long          p_messagelength
                                        ,       char        * p_messagetext
                                        )
{
   if ( p_ring != NULL &&  p_logo != NULL && p_messagetext != NULL )
   {
      if ( tport_putmsg( p_ring, p_logo, p_messagelength, p_messagetext ) != PUT_OK )
      {
         if ( WORM_LOG_ERRORS <= LoggingLevel )
         {
            logit( ""
                 , "SimpleModuleBase::SendMessageToRing() Error: tport_putmsg returned error for message:\n%s\n"
                 , ( p_messagetext == NULL ? "<<no message>>" : p_messagetext )
                 );
         }
      }
   }
}
/*
 * -------------------------------------------------------------------------
 */
void SimpleModuleBase::SendHeartbeat()
{
   if ( 0 < HBIntervalSec )
   {
      static MSG_LOGO s_logo = { TYPE_HEARTBEAT, MyModuleId, MyInstallId };
      static long     s_lastbeat = time(NULL) - HBIntervalSec - 1;
   
      time_t _now = time(NULL);

      if ( (s_lastbeat + HBIntervalSec) < _now )
      {
//         long _len;
         char _msg[80];
         sprintf( _msg, "%d\n\0", _now );
         SendMessageToRing( &CommandRing, &s_logo, strlen(_msg), _msg );
         s_lastbeat = _now;
      }
   }
}
/*
 * -------------------------------------------------------------------------
 */
void SimpleModuleBase::SendError( const short         p_errornumber
                                , const char        * p_messagetext /*  = NULL */
                                )
{
   static const int s_maxmsglen = 240;
   static MSG_LOGO s_logo = { TYPE_ERROR, MyModuleId, MyInstallId };
   
   long _len;
   char _msg[s_maxmsglen+1];

   sprintf( _msg
          , "%d %d"
          , time(NULL)
          , p_errornumber
          );
   
   if ( p_messagetext == NULL )
   {
      strcat( _msg, "\n" );
   }
   else
   {
      strcat( _msg, " " );
      
      // calculate how must space is available
      // for the message text
      // add 1 for terminal '\n'
      _len = s_maxmsglen - ( strlen(_msg) + 1 );
      
      if ( strlen(p_messagetext) <= _len )
      {
         // enough space for the entire text
         strcat( _msg , p_messagetext );
      }
      else
      {
         // only enough space for part of the text
         int _newlen = strlen(_msg) + _len;
         strncat( _msg, p_messagetext, _len );
         _msg[_newlen] = '\0';
      }
      strcat( _msg, "\n" );
   }
   
   SendMessageToRing( &CommandRing, &s_logo, strlen(_msg), _msg );
}
/*
 * -------------------------------------------------------------------------
 */
HANDLE_STATUS SimpleModuleBase::HandleConfigLine( ConfigSource * p_parser )
{
   // Do not manipulate ConfigState herein.

   HANDLE_STATUS r_handled = HANDLER_UNUSED;

   try
   {
      if ( p_parser == NULL )
      {
         throw worm_exception("parser parameter passed in NULL");
      }

      char * _token;

      r_handled = HANDLER_USED;

      do
      {

         if ( p_parser->Its("InputLogo") )
         {
            WORM_INSTALLATION_ID _instid = WORM_INSTALLATION_WILD;
            WORM_MODULE_ID       _modid  = WORM_MODULE_WILD;
            WORM_MSGTYPE_ID      _typeid = WORM_MSGTYPE_WILD;
            
            _token = p_parser->String();
            if ( strcmp(_token,"*") == 0 )
            {
               if ( GetInst("INST_WILDCARD", &_instid ) != 0 )
               {
                  throw worm_exception("Failed getting INST_WILDCARD value");               
               }
            }
            else
            {
               if ( GetInst(_token, &_instid ) != 0 )
               {
                  throw worm_exception("invalid <InputLogo> installation value");               
               }
            }
            
            _token = p_parser->String();
            if ( strcmp(_token,"*") == 0 )
            {
               if ( GetModId("MOD_WILDCARD", &_modid ) != 0 )
               {
                  throw worm_exception("Failed getting MOD_WILDCARD value");               
               }
            }
            else
            {
               if ( GetModId(_token, &_modid ) != 0 )
               {
                  throw worm_exception("invalid <InputLogo> module value");               
               }
            }
            
            _token = p_parser->String();
            if ( strcmp(_token,"*") == 0 )
            {
               if ( GetType("TYPE_WILDCARD", &_typeid ) != 0 )
               {
                  throw worm_exception("Failed getting TYPE_WILDCARD value");               
               }
            }
            else
            {
               if ( GetType(_token, &_typeid ) != 0 )
               {
                  throw worm_exception("invalid <InputLogo> message type value");               
               }
            }
            
            if ( InputLogoCount == LogoAlloc )
            {
               // Currently using max logos, extend the list length
                  
               MSG_LOGO * _newlist = NULL;
               
               LogoAlloc += 5;
               
               if ( (_newlist = new MSG_LOGO[LogoAlloc]) == NULL )
               {
                  throw worm_exception("Failed allocating logo list extension.");
               }
               
               if ( InputLogoList != NULL )
               {
                  // Copy existing logos into new list
                  for ( int _i = 0 ; _i < InputLogoCount ; _i++ )
                  {
                     _newlist[_i].type   = InputLogoList[_i].type;
                     _newlist[_i].mod    = InputLogoList[_i].mod;
                     _newlist[_i].instid = InputLogoList[_i].instid;
                  }
                  
                  delete [] InputLogoList;
               }
               
               InputLogoList = _newlist;
            }
            
            InputLogoList[InputLogoCount].type   = _typeid;
            InputLogoList[InputLogoCount].mod    = _modid;
            InputLogoList[InputLogoCount].instid = _instid;
            InputLogoCount++;
            
            continue;
         }
         
         if ( p_parser->Its("ModuleName") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <ModuleName> value");
            }

            if ( GetModId(_token, &MyModuleId ) != 0 )
            {
               throw worm_exception("invalid <ModuleName> value");
            }
            continue;
         }

         if ( p_parser->Its("LoggingLevel") )
         {
            int _wrkint = p_parser->Int();
            
            if (   _wrkint == ConfigSource::INVALID_INT
                || _wrkint <  0
                ||            WORM_LOG_DEBUG < _wrkint
               )
            {
               if ( WORM_LOG_ERRORS <= LoggingLevel )
               {
                  logit( "o"
                       , "invalid <LoggingLevel> value %d, reverting to default %d\n"
                       , _wrkint
                       , DEF_LOGGING_LEVEL
                       );
               }
               LoggingLevel = DEF_LOGGING_LEVEL;
            }
            else
            {
               LoggingLevel = (WORM_LOGGING_LEVEL)_wrkint;
            }
            continue;
         }

         if ( p_parser->Its("QueueSize") )
         {
            MaxQueueElements = p_parser->Int();
            if (   MaxQueueElements == ConfigSource::INVALID_INT
                || MaxQueueElements < 1
               )
            {
               MaxQueueElements = 0;
               throw worm_exception("missing or invalid <QueueSize> MaxElements value");
            }
            
            MaxMessageSize = p_parser->Int();
            if (   MaxMessageSize == ConfigSource::INVALID_INT
                || MaxMessageSize < 1
               )
            {
               MaxMessageSize = 0;
               throw worm_exception("missing or invalid <QueueSizes> MaxMessageSize value");
            }
            continue;
         }



         if ( p_parser->Its("CmdRingName") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing <CmdRingName> value");
            }
            
            if ( (CommandRingKey = GetKey(_token)) < 0 )
            {
               throw worm_exception("invalid <CmdRingName> value");
            }
            strcpy( CommandRingName, _token );
            continue;
         }

         if ( p_parser->Its("InRingName") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               InputRegion = NULL; // show bad config by NULLifying
               throw worm_exception("missing <InRingName> value");
            }

            if ( (InputRingKey = GetKey(_token)) < 0 )
            {
               InputRegion = NULL; // show bad config by NULLifying
               throw worm_exception("invalid <InRingName> value");
            }
            strcpy( InputRingName, _token );
            continue;
         }

         if ( p_parser->Its("OutRingName") )
         {
            _token = p_parser->String();
            if ( strlen(_token) == 0 )
            {
               OutputRegion = NULL; // show bad config by NULLifying
               throw worm_exception("missing <OutRingName> value");
            }

            if ( (OutputRingKey = GetKey(_token)) < 0 )
            {
               OutputRegion = NULL; // show bad config by NULLifying
               throw worm_exception("invalid <OutRingName> value");
            }
            strcpy( OutputRingName, _token );
            continue;
         }

         if ( p_parser->Its("MainLoopSleepMS") )
         {
            if ( (MainLoopSleepMS = p_parser->Int()) == ConfigSource::INVALID_INT )
            {
               if ( WORM_LOG_ERRORS <= LoggingLevel )
               {
                  logit( "o"
                       , "invalid <MainLoopSleepMS> value %d, reverting to default %d\n"
                       , MainLoopSleepMS
                       , DEF_LOOP_SLEEP_MS
                       );
               }
               MainLoopSleepMS = DEF_LOOP_SLEEP_MS;
            }
            continue;
         }

         if ( p_parser->Its("StackerSleepMS") )
         {
            if ( (StackerSleepMS = p_parser->Int()) == ConfigSource::INVALID_INT )
            {
               if ( WORM_LOG_ERRORS <= LoggingLevel )
               {
                  logit( "o"
                       , "invalid <StackerSleepMS> value %d, reverting to default %d\n"
                       , StackerSleepMS
                       , DEF_STACK_SLEEP_MS
                       );
               }
               StackerSleepMS = DEF_STACK_SLEEP_MS;
            }
            continue;
         }

         if ( p_parser->Its("HandlerSleepMS") )
         {
            if ( (HandlerSleepMS = p_parser->Int()) == ConfigSource::INVALID_INT )
            {
               if ( WORM_LOG_ERRORS <= LoggingLevel )
               {
                  logit( "o"
                       , "invalid <HandlerSleepMS> value %d, reverting to default %d\n"
                       , HandlerSleepMS
                       , DEF_HANDLE_SLEEP_MS
                       );
               }
               HandlerSleepMS = DEF_HANDLE_SLEEP_MS;
            }
            continue;
         }
         
         if ( p_parser->Its("HeartbeatSecs") )
         {
            if ( (HBIntervalSec = p_parser->Int()) == ConfigSource::INVALID_INT )
            {
               if ( WORM_LOG_ERRORS <= LoggingLevel )
               {
                  logit( "o"
                       , "invalid <HeartbeatSecs> value %d, reverting to default %d\n"
                       , HBIntervalSec
                       , DEF_HEARTBEAT_INTV_SEC
                       );
               }
               HBIntervalSec = DEF_HEARTBEAT_INTV_SEC;
            }
            continue;
         }
         
         
         r_handled = HANDLER_UNUSED;

      } while ( false );
   }
   catch( worm_exception _we )
   {
      r_handled = HANDLER_INVALID;
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "e"
              , "SimpleModuleBase::HandleConfigLine(): configuration error: %s\nin line: %s\n"
              , _we.what()
              , p_parser->GetCurrentLine()
              );
      }
   }

   return r_handled;
}
/*
 * -------------------------------------------------------------------------
 */
void SimpleModuleBase::CheckConfig()
{
   if ( MyModuleId == WORM_MODULE_INVALID )
   {
      logit( "e", "SimpleModuleBase::CheckConfig(): <ModuleName> tag missing from configuration file.\n" );
      ConfigState = WORM_STAT_BADSTATE;
   }
   
   if ( InputLogoList == NULL )
   {
      logit( "e", "SimpleModuleBase::CheckConfig(): No <InputLogo> tag(s) in configuration file.\n" );
      ConfigState = WORM_STAT_BADSTATE;
   }
   
   if ( CommandRingKey == WORM_RING_INVALID )
   {
      logit( "e", "SimpleModuleBase::CheckConfig(): <CommandRing> tag missing or invalid in configuration file.\n" );
      ConfigState = WORM_STAT_BADSTATE;
   }
   
   if ( InputRegion == NULL )
   {
      logit( "e", "SimpleModuleBase::CheckConfig(): <InputRing> tag invalid in configuration file.\n" );
      ConfigState = WORM_STAT_BADSTATE;
   }
   
   if ( OutputRegion == NULL )
   {
      logit( "e", "SimpleModuleBase::CheckConfig(): <OutputRing> tag invalid in configuration file.\n" );
      ConfigState = WORM_STAT_BADSTATE;
   }
   
   if (   MaxMessageSize   == 0
       || MaxQueueElements == 0
      )
   {
      logit( "e", "SimpleModuleBase::CheckConfig(): <QueueSize> tag missing or invalid in configuration file.\n" );
      ConfigState = WORM_STAT_BADSTATE;
   }
}
/*
 * -------------------------------------------------------------------------
 */
bool SimpleModuleBase::PrepareToRun()
{
   bool r_status = true;
   
   try
   {
      
      if ( WORM_LOG_DEBUG <= LoggingLevel )
      {
            logit( ""
                 , "SimpleModuleBase::PrepareToRun(), parameters:\n\n"
                 );
            logit( ""
                 , "Installation id: %d\n"
                 , MyInstallId
                 );
            logit( ""
                 , "Module id: %d\n"
                 , MyModuleId
                 );
            logit( ""
                 , "MessageTypes:  TYPE_HEARTBEAT: %d,  TYPE_ERROR: %d\n"
                 , TYPE_HEARTBEAT
                 , TYPE_ERROR
                 );
            logit( ""
                 , "CommandRing:  %d  %s\n"
                 , CommandRingKey
                 , ( strlen(CommandRingName) == 0 ? "<no name>" : CommandRingName )
                 );
         if (   InputRingKey == WORM_RING_INVALID
             || InputRingKey == CommandRingKey
            )
         {
            logit( "", "InputRing: using CommandRing\n" );
         }
         else
         {
            logit( ""
                 , "InputRing:  %d  %s\n"
                 , InputRingKey
                 , ( strlen(InputRingName) == 0 ? "<no name>" : InputRingName )
                 );
         }
         if (   OutputRingKey == WORM_RING_INVALID
             || OutputRingKey == CommandRingKey
            )
         {
            logit( "", "OutputRing: using CommandRing\n" );
         }
         else if ( OutputRingKey == InputRingKey )
         {
            logit( "", "OutputRing: using InputRing\n" );
         }
         else
         {
            logit( ""
                 , "OutputRing:  %d  %s\n"
                 , OutputRingKey
                 , ( strlen(OutputRingName) == 0 ? "<no name>" : OutputRingName )
                 );
         }
            logit( ""
                 , "Heartbeat interval:  %d secs\n"
                 , HBIntervalSec
                 );
            logit( ""
                 , "Loop sleep times;  main: %d ms,  stacker: %d ms,  handler: %d ms\n"
                 , MainLoopSleepMS
                 , StackerSleepMS
                 , HandlerSleepMS
                 );
            logit( "", "Logos: instid:mod:type\n" );
         for ( int _i = 0 ; _i < InputLogoCount ; _i++ )
         {
            logit( ""
                 , "          %3d:%3d:%3d\n"
                 , InputLogoList[_i].instid
                 , InputLogoList[_i].mod
                 , InputLogoList[_i].type
                 );
         }
            logit( ""
                 , "Internal queue:  %d message of max %d bytes\n"
                 , MaxQueueElements
                 , MaxMessageSize
                 );
      }
      
      /*
       * Attach to the desired rings
       */
      tport_attach( &CommandRing, CommandRingKey );
   
      if       ( InputRingKey == CommandRingKey )
      {
         InputRegion = &CommandRing;
      }
      else if ( InputRingKey != WORM_RING_INVALID )
      {
         InputRegion = &InputRing;  
         tport_attach( InputRegion, InputRingKey );      
      }
      
      if      ( OutputRingKey == CommandRingKey )
      {
         OutputRegion = &CommandRing;
      }
      else if ( OutputRingKey == InputRingKey )
      {
         OutputRegion = &InputRing;
      }
      else
      {
         OutputRegion = &OutputRing;  
         tport_attach( OutputRegion, OutputRingKey );      
      }
      
      /*
       * Initialize the message queue
       */
       
      switch( initqueue( &MessageQueue, MaxQueueElements, MaxMessageSize ) )
      {
        case 0:     /* okay */
             break;
        case -1:
             throw worm_exception("Insufficient memory for queue as specified in config file");
        case -2:
             throw worm_exception("Specified queue size is too large" );
             break;     
      }
      
      
      /*
       * Start Stacker and Handler threads.
       */
      int _arg;
//      TO_THREAD_ID _tid;

//logit( "o", "Starting handler thread\n" );
      HandlerInfo.status = THREAD_STATUS_STARTING;
      HandlerInfo.lastpulse = time(NULL) + 2;
      _arg = 2;
      StartThreadWithArg( THREAD_STACK, &HandlerInfo.threadid, &_arg );
      
      /*
       * Give the first thread time to start before clobbering _arg
       */
      sleep_ew( 250 );
      
//logit( "o", "Hendler thread id %d\n", HandlerInfo.threadid);
//logit( "o", "Starting stacker thread\n" );

      StackerInfo.status = THREAD_STATUS_STARTING;
      StackerInfo.lastpulse = time(NULL) + 2;
      _arg = 1;
      StartThreadWithArg( THREAD_STACK, &StackerInfo.threadid, &_arg );
//logit( "o", "Stacker thread id %d\n", StackerInfo.threadid);

      sleep_ew( 250 );
   }
   catch( worm_exception & _we )
   {
      r_status = false;
      logit( "e"
           , "SimpleModuleBase::PrepareToRun(): %s\n"
           , _we.what()
           );
   }
   
   return true;
}
/*
 * -------------------------------------------------------------------------
 */
THREAD_RETURN SimpleModuleBase::Stacker()
{
   
//logit( "o" , "DEBUG Stacker() Entering\n" );

   char * _msgbuf = NULL;
   
   StackerInfo.status = THREAD_STATUS_INIT;

   try
   {
      MSG_LOGO _logo;
      
      long  _msglength;
      
      int  _retc;
      
      if ( (_msgbuf = new char[MaxMessageSize+1]) == NULL )
      {
         throw worm_exception("failed allocating stacker message buffer");
      }
      
      StackerInfo.status = THREAD_STATUS_GOOD;
      
      while ( Running )
      {

         StackerInfo.lastpulse = time(NULL);
         
         switch( tport_getmsg(  InputRegion
                             ,  InputLogoList
                             ,  InputLogoCount
                             , &_logo
                             , &_msglength
                             ,  _msgbuf
                             ,  MaxMessageSize
                )            )
         {
           case GET_NONE:
                /*
                 * No messages waiting, sleep for a bit
                 */
   
//logit( "o" , "DEBUG Stacker() Sleeping\n" );
                sleep_ew( StackerSleepMS );   
//logit( "o" , "DEBUG Stacker() Back from Sleep\n" );
                break;
                 
           case GET_TOOBIG:
                if ( WORM_LOG_ERRORS <= LoggingLevel )
                {
                   logit( "et"
                        , "SimpleModuleBase::Stacker() Error: Message buffer size (%d) larger than MaxMessageSize (%d)\n"
                         , _msglength
                        , MaxMessageSize
                        );
                }
                SendError( SMB_ERR_MSG_TOO_LONG, "message longer than maximum" );
                break;
                
           // case GET_OK:
           // case GET_MISS:
           // case GET_NOTRACK:
           // case GET_MISS_LAPPED:
           // case GET_MISS_SEQGAP:
           default:
                 if ( WantMessage( _logo, _msglength, _msgbuf ) )
                 {
//logit( "o" , "DEBUG Stacker() Requesting mutex\n" );
                    RequestSpecificMutex( &QueueMutex );
//logit( "o" , "DEBUG Stacker() Obtained mutex\n" );
                    _retc = enqueue( &MessageQueue, _msgbuf, _msglength, _logo );
                    ReleaseSpecificMutex( &QueueMutex );
//logit( "o" , "DEBUG Stacker() Released mutex\n" );
                 
                    switch( _retc )
                    {
                      case 0:     /* Okay */
                           break;
                      case -1:    /* Message too large */
                           logit( "e", "SimpleModuleBase::Stacker() coding error: message too long for queue\n" );
                           SendError( SMB_ERR_MSG_TOO_LONG, "message longer than maximum" );
                           break;
                      case -3:    /* Wrapped tail of the buffer */
                           if ( WORM_LOG_ERRORS <= LoggingLevel )
                           {
                              static int _lostcount = 0;
                              if ( (_lostcount % 20) == 0 )
                              {
                                 logit( "et"
                                       , "SimpleModuleBase::Stacker(): %d messages lost by queue wrapping\n"
                                     , _lostcount
                                      );
                                 SendError( SMB_ERR_QUEUE_WRAPPED, "Message lost by queue wrapping" );
                              }
                           }
                           break;
                    }
                 }
                 break;
         }
      }
      
      StackerInfo.status = THREAD_STATUS_QUIT;
   }
   catch( worm_exception & _we )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::Stacker() Error: %s\n"
              , _we.what()
              );
      }
      StackerInfo.status = THREAD_STATUS_ERROR;
   }
   catch( ... )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::Stacker() Unknown Error\n"
              );
      }
      StackerInfo.status = THREAD_STATUS_ERROR;
   }
   
   if ( _msgbuf != NULL )
   {
      delete [] _msgbuf;
   }
   
   if ( WORM_LOG_DETAILS <= LoggingLevel )
   {
      logit( "ot"
           , "SimpleModuleBase::Stacker() Exiting method\n"
           );
   }
}
/*
 * -------------------------------------------------------------------------
 */
THREAD_RETURN SimpleModuleBase::Handler()
{
   
//logit( "o" , "DEBUG Handler() Entering\n" );

   char * _msgbuf = NULL;
   
   StackerInfo.status = THREAD_STATUS_INIT;
   
   try
   {
      MSG_LOGO _logo;
      
      long  _msglength;
      
      int  _retc;
      
      if ( (_msgbuf = new char[MaxMessageSize+1]) == NULL )
      {
         throw worm_exception("failed allocating stacker message buffer");
      }
      
      HandlerInfo.status = THREAD_STATUS_GOOD;
      
      while ( Running )
      {
         HandlerInfo.lastpulse = time(NULL);
         
         _msglength = MaxMessageSize;

         RequestSpecificMutex( &QueueMutex );
         _retc = dequeue( &MessageQueue, _msgbuf, &_msglength, &_logo );
         ReleaseSpecificMutex( &QueueMutex );

         if ( _retc == 0 )
         {
            if ( ! MessageHandler( _logo, _msglength, _msgbuf ) )
            {
               throw worm_exception("HandleMessage returned error");
            }
         }
         else
         {
            sleep_ew( HandlerSleepMS );
         }
      }
      
      HandlerInfo.status = THREAD_STATUS_QUIT;
   }
   catch( worm_exception & _we )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::Handler() Error: %s\n"
              , _we.what()
              );
      }
      HandlerInfo.status = THREAD_STATUS_ERROR;
   }
   catch( ... )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::Handler() Unknown Error\n"
              );
      }
      HandlerInfo.status = THREAD_STATUS_ERROR;
   }
   
   if ( _msgbuf != NULL )
   {
      delete [] _msgbuf;
   }
   
   if ( WORM_LOG_DETAILS <= LoggingLevel )
   {
      logit( "ot"
           , "SimpleModuleBase::Handler() Exiting method\n"
           );
   }
}
/*
 * -------------------------------------------------------------------------
 */
bool SimpleModuleBase::CheckForFatal()
{
   bool r_status = false;
   
   long _timenow = time(NULL);
   
   /*
    * Check Stacker thread status
    */
   
   if ( StackerInfo.status == THREAD_STATUS_ERROR )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::CheckForFatal(): Stacker thread reported error\n"
              );
      }
      r_status = true;
   }
   else if ( (StackerInfo.lastpulse + WORK_THREAD_DEAD_SEC) < _timenow )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::CheckForFatal(): Stacker thread didn't pulse heartbeat\n"
              );
      }
      r_status = true;
   }

   /*
    * Check Handler thread status
    */
   
   if ( HandlerInfo.status == THREAD_STATUS_ERROR )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::CheckForFatal(): Handler thread reported error\n"
              );
      }
      r_status = true;
   }
   else if ( (HandlerInfo.lastpulse + WORK_THREAD_DEAD_SEC) < _timenow )
   {
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "et"
              , "SimpleModuleBase::CheckForFatal(): Handler thread didn't pulse heartbeat\n"
              );
      }
      r_status = true;
   }      
   
   return r_status;
}
/*
 * -------------------------------------------------------------------------
 */
WORM_STATUS_CODE SimpleModuleBase::Run( const char * p_programName
                                      , const char * p_configFilename /* = NULL */
                                      )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   TComFileParser * _parser = NULL;

   try
   {
      
      if ( p_programName == NULL || strlen(p_programName) == 0 )
      {
         throw worm_exception("Program name passed in as NULL or empty");
      }
      
      
      {
         /*
          * This block is just to isolate a few variables that
          * are only needed to prepare for execution
          */
          
         char * _ptr;
   
         // Skip over path if included
#if defined(_WINNT) || defined(_WINDOWS) || defined(_Windows)
         if ( getenv(WORM_TIME_ZONE) != NULL ) _tzset();
   
         if ((_ptr = strrchr(p_programName, '\\')) != NULL)
#else
         if ((_ptr = strrchr(p_programName, '/')) != NULL)
#endif
         {
            strcpy(ProgramName, _ptr + 1);
         }
         else
         {
            strcpy(ProgramName, p_programName);
         }
      
         // snip off ".???" extension if present
         _ptr = strchr(ProgramName, '.');
         if (_ptr != NULL )
         {
            *_ptr = '\0';
         }
   
   
         if ( getenv(EW_HOME_KEY) == NULL )
         {
            strcpy( HomeDirectory, "" );
         }
         else
         {
            strcpy( HomeDirectory, getenv(EW_HOME_KEY) );
         }
   
   
         if ( getenv(EW_VERSION_KEY) == NULL )
         {
            strcpy( Version, "Unknown" );
         }
         else
         {
            strcpy( Version, getenv(EW_VERSION_KEY) );
         }


         GEN_FILENAME _configdir
                    , _configfilename
                    ;
   
         if ( getenv( EW_CONFIG_DIR ) == NULL )
         {
            logit( "e"
                 , "SimpleModuleBase::Run: Environment variable <%s> not defined\n                using current directory\n"
                 , EW_CONFIG_DIR
                 );
            strcpy( _configdir, "." );
         }
         else
         {
            strcpy( _configdir, getenv(EW_CONFIG_DIR) );
         }
   
         int _len = strlen( _configdir );
#if defined(_WINNT) || defined(_WINDOWS) || defined(_Windows)
         if( 0 < _len && _configdir[_len-1] != '\\' )
         {
            strcat( _configdir, "\\" );
         }
#else
         if( 0 < _len && _configdir[_len-1] != '/' )
         {
            strcat( _configdir, "/" );
         }
#endif


         strcpy( _configfilename, _configdir );
         
         if ( p_configFilename == NULL )
         {
            /*
             * no config file name supplied, build it from the program name
             */
            strcat( _configfilename, ProgramName );
            strcat( _configfilename, ".d" );
         }
         else
         {
            strcat( _configfilename, p_configFilename );
         }
   
   
   
         // check for this module's configuration file
         if ( (_parser = new TComFileParser()) == NULL )
         {
            throw worm_exception("failed creating parser for configuration file");
         }
   
         if ( ! _parser->Open(_configfilename) )
         {
            worm_exception _expt("failed opening configuration file: ");
                           _expt += _configfilename;
            throw _expt;
         }
   
   
         bool _parsing = true;

         do
         {
            switch ( _parser->ReadLine() )
            {
              case COMFILE_ERROR:
                   {
                      char * _errortxt;
                      _parser->Error( &_errortxt );
                      worm_exception _expt( "Error while parsing config file " );
                      _expt += _configfilename;
                      _expt += "\n";
                      _expt += _errortxt;
                      throw _expt;
                   }
   
              case COMFILE_EOF:
                   _parsing = false;
                   break;
   
              default:
   
                   _parser->NextToken();
   
                   // check if the derivative class uses the configuration line
                   if ( HandleConfigLine(_parser) == HANDLER_UNUSED )
                   {
                      logit( "e"
                           , "unrecognized command in line:\n%s\n%s\n%s\n"
                           , _parser->GetCurrentLine()
                           , "while parsing config file:"
                           , p_programName
                           );
                   }
                   
                   break;
   
            } // switch()
   
         } while(_parsing);
   
   
         delete( _parser );
         _parser = NULL;
      }
      

      if ( ! IsReady() )
      {
         throw worm_exception( "Module is not configured, must exit");
      }

#ifdef SOLARIS
      // Set our "nice" value back to the default value.
      // This does nothing if you started at the default.
      // Requested by D. Chavez, University of Utah.
      //
      nice( -nice( 0 ) );

      // Catch process termination signals
      //
      sigignore( SIGINT );             /* Control-c */
      sigignore( SIGQUIT );            /* Control-\ */
      sigset( SIGTERM, SigtermHandler );

      // Catch tty input signal, in case we are in background
      sigignore( SIGTTIN );
#endif

      /*
       * Start the worker threads, etc.
       */
      Running = true; // must be true to prevent worker threads from exiting.
      
      Running = PrepareToRun();
      

      int _flag;

      while( Running )
      {
         if ( CheckForFatal() )
         {
            if ( WORM_LOG_ERRORS <= LoggingLevel )
            {
               logit( "et"
                    , "SimpleModuleBase::Run(): CheckForFatal() reported fatal error, exiting\n"
                    );
            }
            Running = false;
         }
         else
         {
            _flag = tport_getflag( &CommandRing );

            if (   _flag == TERMINATE
                || _flag == MyModuleId
               )
            {
               if ( WORM_LOG_STATUS <= LoggingLevel )
               {
                  logit( "ot"
                       , "SimpleModuleBase::Run(): Shut down down flag received\n"
                       );
               }
               Running = false;
            }
            else
            {
               SendHeartbeat();

               switch ( MainThreadActions() )
               {
                 case WORM_STAT_NOMATCH:
                      // No actions to take at this time, sleep for a bit
                      // waiting for the situation to change.
                      sleep_ew(MainLoopSleepMS);

                 case WORM_STAT_SUCCESS:
                      // Commonly, successful processing indicates that an
                      // incoming message was handled or other action was performed.
                      // Since there might be other actions pending, don't sleep,
                      // return to allow other actions.
                      break;

                 case WORM_STAT_FAILURE:
                      if ( WORM_LOG_DEBUG <= LoggingLevel )
                      {
                         logit( "et"
                              , "SimpleModuleBase::Run(): MainThreadActions() reported fatal error\n"
                              );
                      }
                      r_status = WORM_STAT_FAILURE;
                      Running = false;
                      break;
               }
            }
         }
      }

   }
   catch( worm_exception & _we )
   {
      Running = false;
      if ( WORM_LOG_ERRORS <= LoggingLevel )
      {
         logit( "e"
              , "SimpleModuleBase::Run() Error: %s\n"
              , _we.what()
              );
      }
      r_status = WORM_STAT_FAILURE;
      // Some lower-level exceptions might end up here without
      // having told the worker threads to stop via the Running
      // variable.
   }
   
   /* sleep a bit so the worker threads can shut down */
   sleep_ew( ( StackerSleepMS < HandlerSleepMS ? HandlerSleepMS : StackerSleepMS ) + 100 );

   FinishedRunning();

   if ( _parser != NULL )
   {
      delete( _parser );
      _parser = NULL;
   }

   return r_status;
}
/*
 * -------------------------------------------------------------------------
 */
