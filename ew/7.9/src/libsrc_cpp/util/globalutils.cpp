//---------------------------------------------------------------------------
#include <stdlib.h>  // getenv
#include <string.h>  // strcmp, strcpy, strcat
//#include <stdio.h>   // fprintf
#if defined(_WINNT) || defined(_Windows)
#include <time.h>    // _tzset -- to set the timezone related to GMT
#endif

#include <worm_environ.h>
#include <worm_exceptions.h>
#include <logger.h>
#include <comfile.h>
#include <globalutils.h>

//---------------------------------------------------------------------------
// Borland pragma, ignored by other compilers
#pragma package(smart_init)

//---------------------------------------------------------------------------

/*
** STATIC VARIABLES
*/
PROGRAM_NAME TGlobalUtils::ProgramName = { "Uninitialized" };
WORM_INSTALLATION_ID TGlobalUtils::ThisInstallation = WORM_INSTALLATION_INVALID;
WORM_MODULE_ID TGlobalUtils::ThisModuleId = WORM_MODULE_INVALID;
LOG_DIRECTORY TGlobalUtils::HomeDirectory;
char TGlobalUtils::Version[12];
long TGlobalUtils::HeartBeatInt = -1;

// don't set this to WORM_LOG_NONE here
WORM_LOGGING_LEVEL TGlobalUtils::LogLevel = WORM_LOG_ERRORS;
bool TGlobalUtils::DoLogFile = true;

std::vector<std::string> TGlobalUtils::ConfigFiles;
int  TGlobalUtils::ConfigFileCount  = 0;

INSTALLATION_MAP TGlobalUtils::InstallIds;
RING_MAP TGlobalUtils::RingIds;
MODULE_MAP TGlobalUtils::ModuleIds;
MESSAGETYPE_MAP TGlobalUtils::MessageTypeIds;

volatile bool TGlobalUtils::TerminateFlag = false;


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
/*
**  p_programname is expected as argv[0], so the path and extension will be
**  stripped off.
*/
TGlobalUtils::TGlobalUtils( char* p_programname )
{
   // only one bite at the configuration apple
   if ( ConfigState == WORM_STAT_NOTINIT )
   {
      if ( p_programname == NULL )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                       , "TGlobalUtils(): program name is NULL\n"
                       );
      }


      // Get the config directory from the environment
      //
      GEN_FILENAME _configdir
                 , _configfilename
                 ;

      if ( GetEnvironmentValue(WORM_HOME_KEY) == NULL )
      {
         if ( GetEnvironmentValue(EW_HOME_KEY) == NULL )
         {
            strcpy( HomeDirectory, "" );
         }
         else
         {
            strcpy( HomeDirectory, GetEnvironmentValue(EW_HOME_KEY) );
         }
      }
      else
      {
         strcpy( HomeDirectory, GetEnvironmentValue(WORM_HOME_KEY) );
      }


      if ( GetEnvironmentValue(WORM_VERSION_KEY) == NULL )
      {
         if ( GetEnvironmentValue(EW_VERSION_KEY) == NULL )
         {
            strcpy( Version, "" );
         }
         else
         {
            strcpy( Version, GetEnvironmentValue(EW_VERSION_KEY) );
         }
      }
      else
      {
         strcpy( Version, GetEnvironmentValue(WORM_VERSION_KEY) );
      }



      if ( GetEnvironmentValue( WORM_CONFIG_DIR ) == NULL )
      {
         if ( GetEnvironmentValue( EW_CONFIG_DIR ) == NULL )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils(): Neither environment variable <%s> nor <%s> are defined\n                using current directory"
                          , WORM_CONFIG_DIR
                          , EW_CONFIG_DIR
                          );
            strcpy( _configdir, "." );
         }
         else
         {
            strcpy( _configdir, GetEnvironmentValue( EW_CONFIG_DIR ) );
         }
      }
      else
      {
         strcpy( _configdir, GetEnvironmentValue( WORM_CONFIG_DIR ) );
      }

      int _len = strlen( _configdir );
#if defined(_WINNT) || defined(_Windows)
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

         // Get the config file names, these are later used by
         // LoadFiles() and CheckConfig()
         //
         // NOTE: if the worm configuration files are not specified in
         //       the environment, then if the EW params directory is
         //       in the environment this is assumed to be an
         //       earthworm installation and the earthworm fixed filenames
         //       are used.
         //
         if ( GetEnvironmentValue(WORM_CONFIG_GLOBAL) == NULL )
         {
            if ( GetEnvironmentValue(EW_CONFIG_DIR) != NULL )
            {
                strcpy( _configfilename, _configdir );
                strcat( _configfilename, "earthworm_global.d" );
                ConfigFiles.push_back(_configfilename);
                ConfigFileCount++;
            }
         }
         else
         {
             strcpy( _configfilename, _configdir );
             strcat( _configfilename, GetEnvironmentValue(WORM_CONFIG_GLOBAL) );
             ConfigFiles.push_back(_configfilename);
             ConfigFileCount++;
         }

         if ( GetEnvironmentValue(WORM_CONFIG_SITE) == NULL )
         {
            if ( GetEnvironmentValue(EW_CONFIG_DIR) != NULL )
            {
                strcpy( _configfilename, _configdir );
                strcat( _configfilename, "earthworm.d" );
                ConfigFiles.push_back(_configfilename);
                ConfigFileCount++;
            }
         }
         else
         {
             strcpy( _configfilename, _configdir );
             strcat( _configfilename, GetEnvironmentValue(WORM_CONFIG_SITE) );
             ConfigFiles.push_back(_configfilename);
             ConfigFileCount++;
         }

         char* _ptr;

         // Skip over path if included
#if defined(_WINNT) || defined(_Windows)
         if ( GetEnvironmentValue(WORM_TIME_ZONE) != NULL ) _tzset();

         if ((_ptr = strrchr(p_programname, '\\')) != NULL)
#else
         if ((_ptr = strrchr(p_programname, '/')) != NULL)
#endif
         {
            strcpy(ProgramName, _ptr + 1);
         }
         else
         {
            strcpy(ProgramName, p_programname);
         }

         // eliminate extension if present
         _ptr = strchr(ProgramName, '.');
         if (_ptr != NULL)
         {
            *_ptr = '\0';
         }
      }

      LoadFiles();


      // Get the institution id from environment
      char * _installname = NULL;

      short _installsource = 0;

      if ( (_installname = GetEnvironmentValue(WORM_INSTALLATION_KEY)) == NULL )
      {
         _installsource = 1;
         _installname = GetEnvironmentValue(EW_INSTALLATION_KEY);
      }

      if ( _installname == NULL )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                       , "TGlobalUtils(): Neither environment variable <%s> nor <%s> is defined\n"
                       , WORM_INSTALLATION_KEY
                       , EW_INSTALLATION_KEY
                       );
      }
      else
      {
         if ( strlen(_installname) == 0 )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils(): Environment variable <%s> is defined but empty\n"
                          , ( _installsource == 0 ? WORM_INSTALLATION_KEY : EW_INSTALLATION_KEY )
                          );
         }
         else
         {
            if ( (ThisInstallation = LookupInstallationId(_installname)) == WORM_INSTALLATION_INVALID)
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "TGlobalUtils(): Installation name <%s> not found in global configuration file\n"
                             , _installname
                             );
            }
         }
      }

}
//---------------------------------------------------------------------------
char* TGlobalUtils::GetEnvironmentValue( const char* p_key )
{
   return getenv(p_key);
}
//---------------------------------------------------------------------------
HANDLE_STATUS TGlobalUtils::HandleConfigLine( ConfigSource * p_parser )
{
   HANDLE_STATUS r_status = HANDLER_USED;

   try
   {

      do
      {
         if( p_parser->Its( "TruncLogOnStartup" ) )
         {
            TLogger::TruncateOnOpen();
         }

         //                    WORM                           EARTHWORM
         if( p_parser->Its( "ThisModule" ) || p_parser->Its( "MyModuleId" ) )
         {
            char * _token = p_parser->String();

            if ( strlen(_token) == 0 )
            {
               throw worm_exception("missing both <ThisModule> and <MyModuleId> values");
            }

            if ( (ThisModuleId = LookupModuleId(_token)) == WORM_MODULE_INVALID )
            {
               char _emsg[70];
               sprintf( _emsg
                      , "ModuleId '%s' not found in lookup tables for <ThisModule/MyModuleId> command"
                      , _token
                      );
               throw worm_exception(_emsg);
            }

            continue;
         }

         if ( p_parser->Its("HeartBeatInt") )
         {
            if ( (HeartBeatInt = p_parser->Long()) == ConfigSource::INVALID_LONG )
            {
               throw worm_exception("missing <HeartBeatInt> value");
            }
            continue;
         }

         if ( p_parser->Its("NoLogFile") )
         {
            DoLogFile = false;
            continue;
         }

         // Allow the global logging level (set through LoadFiles() --> ParseCommand())
         // to be overridden by the <module>.d file
         //
         if ( p_parser->Its("LogLevel") )
         {
            int _level;
            if ( (_level = p_parser->Int()) == ConfigSource::INVALID_INT )
            {
               throw worm_exception("missing <LogLevel> value");
            }

            LogLevel = (WORM_LOGGING_LEVEL)_level;
            continue;
         }

         r_status = HANDLER_UNUSED;

      } while( false );
   }
   catch( worm_exception & _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::HandleConfigLine(): %s from line\n%s\n"
                    , _we.what()
                    , p_parser->GetCurrentLine()
                    );
   }

   return r_status;
}
//---------------------------------------------------------------------------
void TGlobalUtils::CheckConfig()
{
   // After all files are closed, check init flags for missed commands

   bool _command_missed = false;
   if (   RingIds.size() == 0
       || ModuleIds.size() == 0
       || MessageTypeIds.size() == 0
       || InstallIds.size() == 0
//       || ThisModuleId == WORM_MODULE_INVALID
      )
   {
      ConfigState = WORM_STAT_BADSTATE;
      _command_missed = true;
   }
   if ( _command_missed )
   {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                       , "TGlobalUtils::CheckConfig(): found no"
                       );
      if ( RingIds.size() == 0 )
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR, " <Ring>" );
      if ( ModuleIds.size() == 0 )
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR, " <Module>" );
      if ( MessageTypeIds.size() == 0 )
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR, " <Message>" );
      if ( InstallIds.size() == 0 )
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR, " <Installation>" );
//      if ( ThisModuleId == WORM_MODULE_INVALID )
//         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR, " <ThisModule/MyModuleId>" );

         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR, "line(s) in file(s):\n" );
      for( int _f = 0 ; _f < ConfigFileCount ; _f++ )
      {
         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                       , "   <%s>\n"
                       , ConfigFiles[_f].c_str()
                       );
      }
   }

   if ( ThisInstallation == WORM_INSTALLATION_INVALID )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::CheckConfig(): Neither <%s> nor <%s> environment value set\n"
                    , WORM_INSTALLATION_KEY
                    , EW_INSTALLATION_KEY
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( strlen(HomeDirectory) == 0 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::CheckConfig(): Neither <%s> nor <%s> environment value set\n"
                    , WORM_HOME_KEY
                    , EW_HOME_KEY
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }

   if ( strlen(Version) == 0 )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::CheckConfig(): Neither <%s> nor <%s> environment value set\n"
                    , WORM_VERSION_KEY
                    , EW_VERSION_KEY
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }
}
//---------------------------------------------------------------------------
bool TGlobalUtils::ParseLookupLine( const char * p_filename, ConfigSource & p_parser )
{
   bool r_foundit = false;

   std::string _name;
   WORM_RING_ID _ringkey;
   int _wrkid;

   // this is a loop solely to allow use of the continue statement
   // to make the code cleaner

   do
   {
      if ( p_parser.Its("Installation") )
      {
         // get installation key
         _name = p_parser.String();

         if ( _name.size() == 0 )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Installation name missing in <%s>\n"
                          , p_filename
                          );
            continue; // next line
         }

         if ( MAX_INSTALLNAME_LEN < _name.size() )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Installation name <%s> too long in <%s> max=%d chars\n"
                          , _name.c_str()
                          , p_filename
                          , MAX_INSTALLNAME_LEN
                          );
            continue; // next line
         }

         if ( (_wrkid = p_parser.Int()) == ConfigSource::INVALID_INT )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Installation <%s>'s id missing in <%s>\n"
                          , _name.c_str()
                          , p_filename
                          );
            continue; // next line
         }

         if ( 0 < InstallIds.count(_name) )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Duplicate entry for Installation key <%s>\nin file %s\nvalues old: %d new: %d, %s\n"
                          , _name.c_str()
                          , p_filename
                          , (int)InstallIds[_name]
                          , (int)_wrkid
                          , "new setting ignored"
                          );
            continue; // next line
         }

         InstallIds[_name] = (WORM_INSTALLATION_ID)_wrkid;
         r_foundit = true;
         continue; // next line
      }

      // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

      if ( p_parser.Its("Ring") )
      {
         // get ring name
         _name = p_parser.String();

         if ( _name.size() == 0 )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Ring name missing in <%s>\n"
                          , p_filename
                          );
            continue; // next line
         }

         if ( MAX_RINGNAME_LEN < _name.size() )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Ring name <%s> too long in <%s> max=%d chars\n"
                          , _name.c_str()
                          , p_filename
                          , MAX_RINGNAME_LEN
                          );
            continue; // next line
         }

         if ( (_ringkey = p_parser.Long()) == ConfigSource::INVALID_LONG )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Ring <%s>'s id missing in <%s>\n"
                          , _name.c_str()
                          , p_filename
                          );
            continue; // next line
         }

         if ( 0 < RingIds.count(_name) )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Duplicate entry for Ring key <%s>\nin file %s\nvalues old: %d new: %d, %s\n"
                          , _name.c_str()
                          , p_filename
                          , (int)RingIds[_name]
                          , (int)_ringkey
                          , "new setting ignored"
                          );
            continue; // next line
         }

         RingIds[_name] = (WORM_RING_ID)_ringkey;
         r_foundit = true;
         continue; // next line
      }

      // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

      if ( p_parser.Its("Module") )
      {
         _name = p_parser.NextToken(); // module name

         if ( _name.size() == 0 )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Module name missing in <%s>\n"
                          , p_filename
                          );
            continue; // next line
         }

         if ( MAX_MODNAME_LEN < _name.size() )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Module name <%s> too long in <%s> max=%d chars\n"
                          , _name.c_str()
                          , p_filename
                          , MAX_MODNAME_LEN
                          );
            continue; // next line
         }

         if ( (_wrkid = p_parser.Int()) == ConfigSource::INVALID_INT )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Module <%s>'s id missing in <%s>\n"
                          , _name.c_str()
                          , p_filename
                          );
            continue; // next line
         }

         if ( 0 < ModuleIds.count(_name) )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Duplicate entry for Module key <%s>\nin file %s\nvalues old: %d new: %d, %s\n"
                          , _name.c_str()
                          , p_filename
                          , (int)ModuleIds[_name]
                          , (int)_wrkid
                          , "new setting ignored"
                          );
            continue; // next line
         }

         ModuleIds[_name] = (WORM_MODULE_ID)_wrkid;
         r_foundit = true;
         continue; // next line
      }

      // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

      if ( p_parser.Its("Message") )
      {
         _name = p_parser.NextToken(); // module name

         if ( _name.size() == 0 )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Message type name missing in <%s>\n"
                          , p_filename
                          );
            continue; // next line
         }

         if ( MAX_MSGTYPENAME_LEN < _name.size() )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Message type name <%s> too long in <%s> max=%d chars\n"
                          , _name.c_str()
                          , p_filename
                          , MAX_MSGTYPENAME_LEN
                          );
            continue; // next line
         }

         if ( (_wrkid = p_parser.Int()) == ConfigSource::INVALID_INT )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Message type <%s>'s id missing in <%s>\n"
                          , _name.c_str()
                          , p_filename
                          );
            continue; // next line
         }

         if ( 0 < MessageTypeIds.count(_name) )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): Duplicate entry for Message type <%s>\nin file %s\nvalues old: %d new: %d, %s\n"
                          , _name.c_str()
                          , p_filename
                          , (int)MessageTypeIds[_name]
                          , (int)_wrkid
                          , "new setting ignored"
                          );
            continue; // next line
         }

         MessageTypeIds[_name] = (WORM_MSGTYPE_ID)_wrkid;
         r_foundit = true;
         continue; // next line
      }

/*
      if ( p_parser.Its("ThisInstallId") )
      {
         r_foundit = true;

         _name = p_parser.String();

         if ( (ThisInstallation = LookupInstallationId(_name.c_str())) == WORM_INSTALLATION_INVALID )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::ParseLookupLine(): <ThisInstallId> value missing in <%s> \n%s\n"
                          , p_filename
                          , _name.c_str()
                          );
            continue; // next line
         }

         continue; // next line
      }
*/

      if ( p_parser.Its("LogLevel") )
      {
         // Although standard practice directs that TGlobalUtils
         // should be instantiated and initialized (thus arriving here
         // to set the logging level at the global setting)
         // before the module reads its own configuration file
         // (possibly overriding the global setting), some might
         // inappropriately call the local configuration before the global.
         // Thus the reason for WORM_LOG_MU, which is used to distinguish
         // "haven't seen it yet" from "no logging" (WORM_LOG_NONE).
         //
         // Don't set debug level from here if previously set from the
         // the <module>.d file [through HandleConfigLine()]
         r_foundit = true;
         if ( LogLevel != WORM_LOG_MU )
         {
            int _level;
            if ( (_level = p_parser.Int()) == ConfigSource::INVALID_INT )
            {
               TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                             , "TGlobalUtils::ParseLookupLine(): missing <LogLevel> value in <%s>\n"
                             , p_filename
                             );
               continue;
            }
            LogLevel = (WORM_LOGGING_LEVEL)_level;
         }
         continue;
      }

   }
   while( false );

   return r_foundit;
}
//---------------------------------------------------------------------------

void TGlobalUtils::LoadFiles()
{
   if ( ConfigState != WORM_STAT_SUCCESS )
   {

      TComFileParser _parser;

      // Loop thru the specified configuration files

      for( int _file = 0 ; _file < ConfigFileCount ; _file++ )
      {

         if ( ! _parser.Open(ConfigFiles[_file].c_str()) )
         {
            TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                          , "TGlobalUtils::LoadFiles(): Error opening config file\n%s\n"
                          , ConfigFiles[_file].c_str()
                          );
            continue; // try next file
         }

         bool _reading = true;

         do
         {

            int _read_status = _parser.ReadLine();

            switch( _read_status )
            {
              case COMFILE_ERROR:
                   {
                      // Error
                      char * _errtext;
                      _parser.Error( &_errtext );
                      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                                    , "TGlobalUtils::LoadFiles(): Error reading line\n%s\nfrom config file: %s\n"
                                    , _errtext
                                    , ConfigFiles[_file].c_str()
                                    );
//                      _file = ConfigFileCount; // leave [outer] for() loop
                   }
                   _reading = false;
                   break; // leave [inner] while() loop (may attempt subsequent file)

              case COMFILE_EOF:

                   _reading = false;
                   break;

              case 0: // empty line
                   break;

              default:
                   // line read from file okay, get command tag string
                   //
                   _parser.NextToken();

                   if ( ! _parser.IsTokenNull() )
                   {
                      if ( ! ParseLookupLine( ConfigFiles[_file].c_str(), _parser ) )
                      {
                         TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                                       , "TGlobalUtils::LoadFiles(): Invalid command line in file: %s\n%s\n"
                                       , ConfigFiles[_file].c_str()
                                       , _parser.GetCurrentLine()
                                       );
                         _reading = false; // on error, leave the loop
                      }
                   }
                   break;
            }   // read status
         } while( _reading );  // each line in opened file

         _parser.Close();

      } // each file

   } // not yet configured
}
//---------------------------------------------------------------------------
const WORM_INSTALLATION_ID TGlobalUtils::LookupInstallationId( const char* p_name )
{
   WORM_INSTALLATION_ID r_install = WORM_INSTALLATION_INVALID;
   if ( p_name == NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::LookupInstallationId(): requested name is NULL\n"
                    );
   }
   else
   {
      if ( 0 < InstallIds.count(p_name) )
      {
         r_install = InstallIds[p_name];
      }
   }
   return r_install;
}
//---------------------------------------------------------------------------
const WORM_RING_ID TGlobalUtils::LookupRingKey( const char* p_name )
{
   WORM_RING_ID r_rkey = WORM_RING_INVALID;
   if ( p_name == NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::LookupRingKey(): requested name is NULL\n"
                    );
   }
   else
   {
      if ( 0 < RingIds.count(p_name) )
      {
         r_rkey = RingIds[p_name];
      }
   }
   return r_rkey;
}
//---------------------------------------------------------------------------
const WORM_MODULE_ID TGlobalUtils::LookupModuleId( const char* p_name )
{
   WORM_MODULE_ID r_mod = WORM_MODULE_INVALID;
   if ( p_name == NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::LookupModuleId(): requested name is NULL\n"
                    );
   }
   else
   {
      if ( 0 < ModuleIds.count(p_name) )
      {
         r_mod = ModuleIds[p_name];
      }
   }
   return r_mod;
}
//---------------------------------------------------------------------------
const WORM_MSGTYPE_ID TGlobalUtils::LookupMessageTypeId( const char* p_name )
{
   WORM_MSGTYPE_ID r_mtyp = WORM_MSGTYPE_INVALID;
   if ( p_name == NULL )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                    , "TGlobalUtils::LookupMessageTypeId(): requested name is NULL\n"
                    );
   }
   else
   {
      if ( 0 < MessageTypeIds.count(p_name) )
      {
         r_mtyp = MessageTypeIds[p_name];
      }
   }
   return r_mtyp;
}

