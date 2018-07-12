// STEP 1: Include the header file for the appropriate server object
//
#include "server_template.h"

#include <logger.h>
#include <comfile.h>
#include <globalutils.h>

#include <stdio.h>

//#ifdef _Windows
//#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
//#endif



// a Borland-specific pragma to suppress warnings
#pragma argsused
int main( int argc, char* argv[] )
{
   int r_status = MSB_RESULT_GOOD;

   TComFileParser * _parser = NULL;

   try
   {
#ifdef _DEBUG
      TLogger::TruncateOnOpen();
#endif

      logit_init( argv[0], 0, 1024, 1 );

      // This read the lookup files specified in the environment.
      TGlobalUtils _global = TGlobalUtils(argv[0]);

// STEP 2: Change "ServerTemplate" to the appropriate server object type.
//
      ServerTemplate _server;


      // Check command line args
      //
      // Generally
      //
      //     Mode        command line formats
      //     ----        ----------------------
      //     standalone  p
      //                 p c.d
      //
      //     client      p
      //                 p c.d
      //
      //     server      <nothing>
      //                 c.d
      //
      //     module      <nothing>
      //                 c.d
      //
      //       Where:  p   = one or more process parameters
      //               c.d = configuration file
      //
      // If the configuration is not given for server or
      // module modes, will build the name
      // from <server>.GetDefaultConfigFileName()
      // If that is also NULL, then try argv[0].d
     
      std::string _configfilename;

      std::string _param;

      for ( int _p = 1 ; _p < argc ; _p++ )
      {
         _param = argv[_p];

         if ( 2 < _param.length() )
         {
            if ( _param.compare( _param.length()-2, 2, ".d" ) == 0 )
            {
               _configfilename = argv[_p];
               _p = argc;
            }
         }
      }


      if ( _configfilename.length() == 0 )
      {
         _configfilename = argv[0];
         _configfilename.append( ".d" );
      }

      if ( (_parser = new TComFileParser()) == NULL )
      {
         throw worm_exception("Failed creating TComFileParser to parse configuration file");
      }

      if ( ! _parser->Open(_configfilename.c_str()) )
      {
         char _msg[80];
         sprintf( _msg, "Failed opening configuration file %s", _configfilename.c_str() );
         throw worm_exception(_msg);
      }


      bool _reading = true;

      char * _token;

      do
      {
          switch( _parser->ReadLine() )
          {
            case COMFILE_EOF:  // eof
                 _reading = false;
                 break;

            case COMFILE_ERROR: // error
                 throw worm_exception("error returned by TComFileParser::ReadLine()");

            case 0:  // empty line, TComFileParser should not return this
                 break;

            default:

                 // Get the command into _token
                 _token = _parser->NextToken();

                 // check if module .d commands override globals (logging level, etc.)
                 //
                 if ( _global.HandleConfigLine(_parser) != HANDLER_UNUSED )
                 {
                    continue;
                 }

                 if ( _server.HandleConfigLine(_parser) == HANDLER_UNUSED )
                 {
                    // Not handled by this, the global config, or the server.
                    //
                    // Don't throw an error here because the server will be queried about
                    // it's readiness shortly, and it is preferable to report all error
                    // in the config file.
                    //
                    TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR
                                  , "main(): unrecognized config file parameter: %s\n"
                                  , _token
                                  );
                    continue;
                 }
          }
      } while( _reading );

      delete( _parser );
      _parser = NULL;


      if ( ! _global.IsReady() )
      {
         throw worm_exception("Global utilities not configured properly");
      }

      if ( ! _server.IsReady() )
      {
          throw worm_exception("Server component not configured properly");
      }

      switch ( _server.Run( argc, argv ) )
      {
        case WORM_STAT_SUCCESS:
             break;
        case WORM_STAT_BADSTATE:
             r_status = MSB_RESULT_FAIL;
             break;
        case WORM_STAT_FAILURE:
             throw worm_exception("ServerTemplate::Run() returned error");
      }
      
   }
   catch( worm_exception _we )
   {
      TLogger::Logit( WORM_LOG_TOFILE|WORM_LOG_TOSTDERR|WORM_LOG_TIMESTAMP
                   , "main(): Exiting due to\n%s\n"
                   , _we.what()
                   );
      r_status = MSB_RESULT_ERROR;
   }

   if ( _parser != NULL )
   {
      delete( _parser ) ;
   }

   TLogger::Close();

   return r_status;
}
//---------------------------------------------------------------------------

