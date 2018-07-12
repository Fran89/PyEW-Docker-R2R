/*
 * server_template.h -- Template and server class deriving from
 *                      the DBMutableServerBase class.
 *
 *                      The base class handles communications, so this class
 *                      primarily must only concern itself with the specific
 *                      handling for this type of server (to include relevant
 *                      configuration, passports and calculations).
 * 
 */
/*
 * Variable which are available from the base classes:
 * 
 *    bool Running;    true while running
 *                    false when told to quit/shutdown
 * 
 *    int LoggingOptions = various logging parameters (set in the base
 *                         class, depending on the mode)
 * 
 *    int LoggingLevel  =  Logging level assigned from the .d file.
 *                         Values:
 *                             WORM_LOG_ERRORS
 *                             WORM_LOG_STATUS
 *                             WORM_LOG_TRACKING
 *                             WORM_LOG_DETAILS
 *                             WORM_LOG_DEBUG
 * 
 *    MUTABLE_MODE_TYPE  Mode = Current mode in which the program is operating
 *               Values:
 *                  MMT_STANDALONE = command from args/stdin, data access and calculations internally
 *                  MMT_MODULE     = command from ring, data access and calculations internally [MT]
 *                  MMT_SERVER     = command from socket, calculations internally [MT]
 *                  MMT_CLIENT     = command from atgs/stdin, socket call to client for calculations
 * 
 *
 *  
 * MODULE AND MESSAGE TYPE IDS:
 * 
 *     WORM_INSTALLATION_ID  InstId  = TGlobalUtils::LookupInstallationId("INST_XXXX");
 *     WORM_MODULE_ID        ModulId = TGlobalUtils::LookupModuleId("INST_XXXX");
 *     WORM_MSGTYPE_ID       MsgId   = TGlobalUtils::LookupMessageTypeId("INST_XXXX");
 * 
 * 
 * AVAILABLE FUNCTIONS:
 * 
 *      // Put a status/error message onto the command ring
 *      //     type must be TYPE_ERROR
 *      void SendStatus( unsigned char type, short ierr, char *note );
 * 
 *      void InitializeDB();  // called by DBMutableServer::PrepareToRun(),
 *                            // but it might also need to be called elsewhere
 *                            // for some modes
 *         
 *      void GetDefaultsFromDB(); 
 * 
 *      // Compare SCNLs with wildcards
 *      // returns true or false
 *      bool DoSCNLsMatch( const char * p_s1
 *                       , const char * p_c1
 *                       , const char * p_n1
 *                       , const char * p_l1
 *                       , const char * p_s2
 *                       , const char * p_c2
 *                       , const char * p_n2
 *                       , const char * p_l2
 *                       )                    
 * 
 * LOGGING:
 * 
 *    Based upon the value of LoggingLevel (value list above),
 *    use the TLogger object to perform the logging functions:
 * 
 *     if( WORM_LOG_ERRORS <= LoggingLevel )
 *     {
 *        TLogger::Logit( LoggingOptions
 *                     , "ServerTemplate::GetRequestFromInput(): %d\n"
 *                     , some_int_value
 *                     );
 *     }
 * 
 * ERROR HANDLING WITH EXCEPTIONS:
 * 
 *     Through base class headers a class called worm_exception is
 *     understood.
 * 
 *     Often errors are handled in the methods using a try-catch pair:
 * 
 *     try
 *     {
 *         // some code
 * 
 *         if ( some_condition )
 *         {
 *            // an error was discovered
 *            
 *            throw worm_exception("My error message");
 *         }
 * 
 * 
 *         if ( some_int_value != 0 )
 *         {
 *            // an error was discovered
 *            
 *            worm_exception MyExcept("some_int_value not = 0: ");
 *                           MyExcept += (int)some_int_value;
 *                           MyExcept += ", Must quit";
 *            throw MyExcept;
 *         }
 * 
 *     }
 *     catch( worm_exception & _we )
 *     {
 *        // Handle any worm exception thrown in the above try block
 *        
 *        // At this point, '_we' is a reference to a worm_exception object
 *        // it has one method most useful here:
 * 
 *        char * _errptr = _we.what();  // returns the error message as a pointer to char
 * 
 *        // If desired, the exception may be re-thrown
 *        // (uncommon, but can be done).
 *        throw _we;
 *     }
 *     catch( ... )
 *     {
 *         // This second catch block is not needed, but it shown here to
 *         // demonstrate how to catch any type of exception.
 *         // Unfortunately, there is no available reference to the exception.
 *     }
 * 
 *     If a method/function includes a throw statement that is not surrounded
 *     by at try block (with an applicable catch block), then execution
 *     returns from the method immediately and then entire calling chain of
 *     functions are returned from -- Until one of the function calls is
 *     surrounded by a try block with an appropriate catch block.
 *     
 *     Memory allocations are NOT released, and file pointers may not be
 *     closed, so they should be handled in a manner such as this:
 * 
 *     char * MyBuffer = NULL;
 * 
 *     try
 *     {
 *         // some code, part A
 * 
 *         if ( (MyBuffer = new char[120]) == NULL )
 *         {
 *            throw worm_exception("failed allocation buffer");
 *         }
 * 
 *         // some other code, part B
 * 
 *     }
 *     catch( worm_exception & _we )
 *     {
 *        // error handling for worm_exceptions
 *     }
 *     catch( ... )
 *     {
 *        // error handling for other exceptions
 *     }
 * 
 * 
 *     if ( MyBuffer != NULL )
 *     {
 *        // Ensure memory is released
 *        // (we allocated an array of char, so we use "delete []"
 *        // instead of just "delete")
 *        //
 *        delete [] MyBuffer;
 *     }
 */


// STEP 1: Do a global replacement of the string "ServerTemplate"
//         with the name of the new class
//

// STEP 2: Replace this include with the .h file for the new class
//
#include "server_template.h"


// STEP 3: Add includes for the appropriate type of message classes
#include "request_template.h"
#include "result_template.h"



extern "C" {

// STEP 4: Include any headers for C-style code

}


//------------------------------------------------------------------------------
ServerTemplate::ServerTemplate()
{
// STEP 5 Perform any required variable initialization here
//        Usually, that means initializing the default parameters:
   DefaultParameters.ExampleLongParam      = 0L;
   DefaultParameters.ExampleStringParam[0] = '\0'; 
   DefaultParameters.ExampleFlagParam      = false;
   
   
//         (Any pointers that will refer to dynamically allocated memory
//         must be NULLified here).
//
//     MyPointer = NULL;
}

//------------------------------------------------------------------------------
ServerTemplate::~ServerTemplate()
{
// STEP 6 Perform any requisite clean-up for server termination here

// (generally only needed to delete any dynamically-allocated memory)

   /*
    * if ( MyPointer != NULL )
    * {
    *    delete [] MyPointer;
    * }
    */
    
}

//------------------------------------------------------------------------------
HANDLE_STATUS ServerTemplate::HandleConfigLine( ConfigSource * p_parser )
{
// STEP 7: Handle any Configuration parameter parsing here.
//
// THIS METHOD RECEIVES A SINGLE CONFIGURATION LINE AT A TIME; THUS, IT IS
// CALLED REPEATEDLY.
//
// GENERALLY, ONLY THOSE CONFIGURATION ITEMS THAT MAY ONLY COME
// FROM THE CONFIGURATION FILE (i.e. STARTUP CONFIGURATION) ARE
// PROCESSED HERE.  THOSE ITEMS WHICH MAY ALSO BE CHANGED VIA A PASSPORT
// ARE HANDED OFF TO THE METHOD HandleParameterLine() TOWARDS THE BOTTOM OF
// THIS METHOD.
//
// THIS METHOD MUST RETURN ONE OF THESE VALUES: 
//
//       HANDLE_INVALID --  line invalid
//       HANDLE_UNUSED  --  line not used
//       HANDLE_USED    --  line used okay
//

   // BASE CLASSES MUST BE GIVEN THE CHANCE TO GET CONFIG VARIABLES:
   HANDLE_STATUS r_status = DBMutableServerBase::HandleConfigLine( p_parser );
   
   
   if ( r_status == HANDLE_UNUSED )
   {
      // Base class didn't use it, then it must belong to this
      // derivative class
    
      try
      {
         // this do-while is solely to allow a single return point,
         // thus simplifying the code.
         
         do
         {
            char * _token;
            
            // WHEN ARRIVING HERE, p_parser HAS THE FIRST TOKEN ON 
            // THE [COMMAND] LINE LOADED INTO THE CURRENT TOKEN.
            // THUS WE MAY PERFORM A TEST FOR THE LINE TYPE USING Its("")
            //
            // Among others, a ConfigSource object has these  methods:
            //
            //          bool   Its("");           // compare current token to a string
            //    const char * GetCurrentToken();
            //    const char * GetCurrentLine();  
            //          char * String();          // next token as a string
            //          int    Int();             // next tokan as an int
            //          int    Long();            // next tokan as a long
            //          int    Double();          // next tokan as a double
            //
            // And these member values:
            //
            //        INVALID_INT
            //        INVALID_LONG
            //        INVALID_DOUBLE
            //
            
            
            
            // EXAMPLE LINE:
            //
            //    ExampleFlagParam true
            //    ExampleFlagParam false
            // 
            if ( p_parser->Its("ExampleFlagParam") )
            {
               _token = p_parser->String();
               
               // The ConfigSource::String() method will return an empty string
               // if no token is found, must perform a specific check for 
               // that condition.
               
               if ( p_parser->IsTokenNull() )
               {
                  // Didn't find an expected value on the line
                  throw worm_exception("Incomplete <ExampleFlagParam> line");
               }
               
               // Grab the value
               if ( strcmp( _token, "true" ) == 0 )
               {
                  DefaultParameters.ExampleFlagParam = true;
               }
               else
               {
                  DefaultParameters.ExampleFlagParam = false;
               }
               
               // Set the return status to indicate that the line was handled
               // okay.
               r_status = HANDLE_USED;
               
               // Skip past any other line checking
               continue;
            }
            
            
            // Additional Its() and handling
            
            
            
            // Pass any config-file only lines to the
            // shared config/passport handler
            //
            r_status = HandleParameterLine( p_parser, &DefaultParameters );
            
         } while( false );
      }
      catch( worm_exception & _we )
      {
         // Do all of our logging [including for HandleParameterLine()] here
         
         if ( WORM_LOG_ERRORS <= LoggingLevel )
         {
            TLogger::Logit( LoggingOptions
                          , "ServerTemplate::HandleConfigLine() Error:\n%s"
                          , _we.what()
                          );
         }
               
         r_status = HANDLE_INVALID;
      }  
   }
   
   return r_status;
}

//------------------------------------------------------------------------------
HANDLE_STATUS ServerTemplate::HandleParameterLine( ConfigSource * p_parser
                                                 , void         * p_params
                                                 )
{
// STEP 8: Handle processing parameters here.
//
// GENERALLY, THIS METHOD IS USED TO HANDLE PROCESSING-RELATED PARAMETERS THAT
// MIGHT ORIGINATE FROM EITHER THE CONFIGURATION FILE, OR THE PASSPORT.
//
// THIS METHOD RECEIVES A SINGLE CONFIGURATION LINE AT A TIME; THUS, IT IS
// CALLED REPEATEDLY.
//
// THIS METHOD MUST RETURN ONE OF THESE VALUES: 
//
//       HANDLE_INVALID --  line invalid
//       HANDLE_UNUSED  --  line not used
//       HANDLE_USED    --  line used okay
// 
// OR THROW worm_exception() FOR ERRORS.
//
// Any errors that are encountered are not logged here, but are throw as an
// worm_exception for the caller to handle and report. 

   HANDLE_STATUS r_status = HANDLE_UNUSED;
   
   // Cast the arriving pointer to a parameters struct to the correct type for
   // this server
   //
   ServerTemplate_PARAMS * CurrentParameters = (ServerTemplate_PARAMS *)p_params;
   
   
   do
   {
      char * _token;
            
      // WHEN ARRIVING HERE, p_parser HAS THE FIRST TOKEN ON 
      // THE [COMMAND] LINE LOADED INTO THE CURRENT TOKEN.
      // THUS WE MAY PERFORM A TEST FOR THE LINE TYPE.
            
            
      // EXAMPLE PARAMETER/PASSPORT LINE:
      //
      //    ExampleDualValue  some_string_without_whitespace  some_integer
      // 
      if ( p_parser->Its("ExampleDualValue") )
      {
         _token = p_parser->String();
         
         if ( p_parser->IsTokenNull() )
         {
            // Didn't find an expected value on the line
            throw worm_exception("Incomplete <ExampleDualValue> line");
         }
         
         if ( 30 <  strlen( _token ) )
         {
            throw worm_exception("Invalid <ExampleDualValue> line: string too long");
         }
         
         // Grab the string value
         strcpy( CurrentParameters->ExampleStringParam, _token );
         
         // Now get the int
                  
         if ( (CurrentParameters->ExampleLongParam = p_parser->Int()) == p_parser->INVALID_INT )
         {
            // Didn't find an expected value on the line
            CurrentParameters->ExampleLongParam = 0;
            throw worm_exception("Incomplete <ExampleDualValue> line: missing int");
         }
               
         // Set the return status to indicate that the line was handled
         // okay.
         r_status = HANDLE_USED;
             
         // Skip past any other line checking
         continue;
      }
      
      
      // Additional Its() and handling
      
            
   } while( false );

   return r_status;
}


//------------------------------------------------------------------------------
void ServerTemplate::CheckConfig()
{
   // MUST ALWAYS ALLOW BASE CLASSES A CHANCE TO CHECK THEIR OWN
   // CONFIGURATION STATE.
   //
   // If the Mode is required for testing, then this statement must
   // come first.
   //
   DBMutableServerBase::CheckConfig();
   
   
   // CHECK THIS CLASS'S CONFIGURATION VARIABLES,
   //
   // There may be need to perform testing based upon the mode in which the
   // program is to run.  For example, a client won't likely need to know
   // about any waveservers, so it won't check the values of any waveserver
   // variables despite their existence in support of the server.
   // The operating mode is kept in variable 'Mode' (see top of file).
   //
   // IF A PROBLEM IS FOUND, LOG IT, AND DO:
   //
   //       ConfigStatus = WORM_STAT_BADSTATE;
   //
   // BUT DON'T RETURN, THUS ALL CONFIGURATION ERRORS CAN BE REPORTED
   // FROM ONE ATTEMPTED RUN.
   
// STEP 9: Perform any configuration checking here

   if ( DefaultParameters.ExampleLongParam == 0 )
   {
      TLogger::Logit( LoggingOptions
                    , "ServerTemplate::CheckConfig(): Did not find a valid <ExampleDualValue> line"
                    );
      ConfigStatus = WORM_STAT_BADSTATE;
   }
   
   
}

//------------------------------------------------------------------------------
bool ServerTemplate::PrepareToRun()
{
   bool r_status = false;

   try
   {
      if ( DBMutableServerBase::PrepareToRun() )
      {   
         // Get any default parameters from the database
         //
         // (this will ultimately end up calling the HandleParameterLine() method) 
         //
         if ( GetDefaultsFromDB( &DefaultParameters ) )
         {
   
// STEP 10: If the new class needs to perform any activities after configuration,
//         but before starting the main processing, add that here.
//       
//         RETURN false IF AN ERROR PREVENTS CONTINUING.
   
   
            // indicate preparations completed successfully
            r_status = true;
         }
      }
   }
   catch( worm_exception & _we )
   {
      if( WORM_LOG_ERRORS <= LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                       , "ServerTemplate::PrepareToRun() Error: %s\n"
                       , _we.what()
                       );
      }
      
      r_status = false;
   }
   return r_status;
}

//------------------------------------------------------------------------------
MutableServerRequest * ServerTemplate::GetRequestContainer()
{
// STEP 11: Change "RequestTemplate" to the appropriate type of request container
//          (2 times)
   RequestTemplate * r_object = NULL;

   try
   {
      if ( (r_object = new RequestTemplate()) == NULL )
      {
         throw worm_exception("Failed to create Request container");
      }
   }
   catch( worm_exception & _we )
   {
      if( WORM_LOG_ERRORS <= LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                      , "ServerTemplate::GetRequestContainer(): %s\n"
                      , _we.what()
                      );
      }
   }

   return r_object;
}

//------------------------------------------------------------------------------
MutableServerResult * ServerTemplate::GetResultContainer()
{
// STEP 12: Change "ResultTemplate" to the appropriate type of request container
//          (2 times)

   ResultTemplate * r_object = NULL;

   try
   {
      if ( (r_object = new ResultTemplate()) == NULL )
      {
         throw worm_exception("Failed to create Result container");
      }
   }
   catch( worm_exception & _we )
   {
      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "ServerTemplate::GetResultContainer(): %s\n"
                      , _we.what()
                      );
      }
   }

   return r_object;
}

//------------------------------------------------------------------------------
WORM_STATUS_CODE ServerTemplate::GetRequestFromInput( int    p_argc
                                                    , char * p_argv[]
                                                    , void * r_container
                                                    )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   try
   {
      if ( r_container == NULL )
      {
         throw worm_exception("NULL request container parameter");
      }

      if ( p_argc < 2 )
      {
         throw worm_exception("Too few parameters from program command line");
      }

      if ( WORM_LOG_DEBUG < LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                       , "ServerTemplate::GetRequestFromInput() argc: %d  [1] = %s\n"
                       , p_argc
                       , p_argv[1]
                       );
      }
      

// STEP 13: Cast the arriving container to the request class appropriate to
//          this server class.
//
      RequestTemplate * ThisRequest = (RequestTemplate *)r_container;


// STEP 14: Perform whatever actions are needed, based upon the command line
//          input.  Commonly this might include getting event info from the
//          database.
//
//          Since this is for client and standalone modes only, these should
//          generally be activities needed to perform the processing request.
//
//          Throw worm_exception for errors.
//
//          For example, if the request class tracks origin id there might
//          be code that looks something like:
//
//          ThisRequest->SetEventId( atoi(argv[1]) );


   }
   catch( worm_exception & _we )
   {
      r_status = WORM_STAT_FAILURE;

      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "ServerTemplate::GetRequestFromInput(): %s\n"
                      , _we.what()
                      );
      }
   }

   return r_status;
}

//------------------------------------------------------------------------------
WORM_STATUS_CODE ServerTemplate::ProcessRequest( void * p_requestcontainer
                                               , void * r_resultcontainer
                                               )
{
   // on algorithm failure (but not error) set r_status = WORM_STAT_BADSTATE
   // before throwing a worm_exception
   // 
   // (r_status is set to WORM_STAT_SUCCESS at bottom of method)
   //
   WORM_STATUS_CODE r_status = WORM_STAT_FAILURE;

   // Passport for this processing run
   ServerTemplate_PARAMS    LocalParameters;
   
   ResultTemplate  * ThisResult  = NULL;
   
   try
   {
      if ( p_request == NULL || r_result == NULL )
      {
         throw worm_exception("NULL container parameter");
      }
      
// STEP 15:
      // Cast request and result containers to the appropriate types for
      // this server
      RequestTemplate * ThisRequest = (RequestTemplate *)p_requestcontainer;
                        ThisResult  = (ResultTemplate *)r_resultcontainer;
      
      

// STEP 16: 
      // Copy default parameters into the local parameters for this calculation
      
              LocalParameters.ExampleLongParam   = DefaultParameters.ExampleLongParam;
      strcpy( LocalParameters.ExampleStringParam , DefaultParameters.ExampleStringParam );
              LocalParameters.ExampleFlagParam   = DefaultParameters.ExampleFlagParam;
      

// STEP 17:
//
// THIS IS EXAMPLE CODE THAT CAN BE USED IF THE REQUEST CONTAINER IS DERIVED
// FROM MutableServerRequest CLASS (WHICH KNOWS ABOUT PASSPORTS.
// IT CAN BE INCLUDED EVEN IF NO PASSPORT DATA IS BEING PASSED AT THIS TIME.
//
// IF THE REQUEST CONTAINER DOES NOT DERIVE FROM MutableServerRequest CLASS,
// THIS PROBABLY MUST BE REMOVED.

      if( WORM_LOG_DEBUG <= LoggingLevel )
      {
         TLogger::Logit( WORM_LOG_TOFILE
                       , "ServerTemplate::ProcessRequest() Getting Passport:\n"
                       );
      }

      
      // Process any passport lines that arrive
      //
      if ( 0 < ThisRequest->GetPassportLineCount() )
      {
         // basic command line parser object
         ConfigSource _parser;
   
         for ( int _p = 0, _sz = ThisRequest->GetPassportLineCount() ; _p < _sz ; _p++ )
         {
            if( WORM_LOG_DEBUG <= LoggingLevel )
            {
               TLogger::Logit( WORM_LOG_TOFILE
                             , "   >%s\n"
                             , _request->GetPassportLine( _p )
                            );
            }

            _parser.Load( ThisRequest->GetPassportLine( _p ) );

            // HandleParameterLine() will throw worm_exception
            // which will be caught by the catch block at the end
            // of this method.  To continue processing on parameter line
            // errors, this statement must be wrapped in at try block
            //
            HandleParameterLine( &_parser, &LocalParameters );
         }
      }
      
      
      
// STEP 18: Perform the actual seimological processing here.
//
//          Any parameters needed should be in LocalParameters (which should be
//          passed to any other methods instead of DefaultParameters.
//
//          Any event data needed should be in the ThisRequest object,
//          something like:   ThisRequest->GetEventId()
//
//          On algorithmic incompletion/failure:
//               r_status = WORM_STAT_BADSTATE;
//               ThisResult->SetStatus( MSB_RESULT_FAIL );
//               throw worm_exception("error description");
//
//          On system failure:
//               r_status = WORM_STAT_FAILURE;
//               ThisResult->SetStatus( MSB_RESULT_ERROR );
//               throw worm_exception("error description");



      // Put the appropriate result status into the Result object
      // (alternatives above)
      ThisResult->SetStatus( MSB_RESULT_GOOD );
      
      // Set the return state 
      //
      r_status = WORM_STAT_SUCCESS;
   }
   catch( worm_exception & _we )
   {
      // r_status should have been set above

      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "ServerTemplate::ProcessRequest(): %s\n"
                      , _we.what()
                      );
      }
   }
}

//------------------------------------------------------------------------------
WORM_STATUS_CODE ServerTemplate::HandleResult( void * p_resultcontainer )
{
   WORM_STATUS_CODE r_status = WORM_STAT_SUCCESS;

   try
   {
// STEP 19: Cast the result container to the appropriate type
//
      ResultTemplate  * ThisResult  = (ResultTemplate *)p_resultcontainer;


      if( WORM_LOG_DEBUG <= LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                       , "ServerTemplate::HandleResult(): DEBUG result of processing: %d\n"
                       , _result->GetStatus()
                       );
      }
      

      if ( ThisResult->GetStatus() == MSB_RESULT_GOOD )
      {
         
// STEP 20: Perform activities for a good result (i.e.: database storage)
//
//       throw worm_exception for errors

         
      } // received good magnitude result
      else
      {
         switch( _result->GetStatus() )
         {
           case MSB_RESULT_FAIL:
                r_status = WORM_STAT_BADSTATE;
                break;
           case MSB_RESULT_ERROR:
                r_status = WORM_STAT_FAILURE;
                break;
         }
      } // calculations not good
   }
   catch( worm_exception & _we )
   {
      r_status = WORM_STAT_FAILURE;

      if( WORM_LOG_ERRORS <= TGlobalUtils::GetLoggingLevel() )
      {
         TLogger::Logit( LoggingOptions
                      , "ServerTemplate::HandleResult(): %s\n"
                      , _we.what()
                      );
      }
   }

   return r_status;
}
//------------------------------------------------------------------------------
