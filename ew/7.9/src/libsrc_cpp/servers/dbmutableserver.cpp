/*
 * dbmutableserverbase.cpp -- definition of a class which extends
 *                            MutableServerBase with basic database
 *                            functionality.
 */

#include "dbmutableserver.h"

#include <oracleconfigsource.h>


//------------------------------------------------------------------------------
HANDLE_STATUS DBMutableServer::HandleConfigLine( ConfigSource * p_parser )
{
   // BASE CLASSES MUST BE GIVEN THE CHANCE TO GET CONFIG VARIABLES:
   HANDLE_STATUS r_status = MutableServerBase::HandleConfigLine(p_parser);
   
   
   if ( r_status == HANDLER_UNUSED )
   {
      // Base class didn't use it, then it might belong to this
      // derivative class
    
      try
      {
         // this do-while is solely to allow a single return point,
         // thus simplifying the code.
         
         do
         {
            char * _token;
            

            if ( p_parser->Its("DBConnection") )
            {
               _token = p_parser->String();
               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Missing <DBConnection> User value");
               }
               DB_User = _token;

               _token = p_parser->String();
               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Missing <DBConnection> Password value");
               }
               DB_Password = _token;

               _token = p_parser->String();
               if ( strlen(_token) == 0 )
               {
                  throw worm_exception("Missing <DBConnection> Servicevalue");
               }
               DB_Service = _token;

               r_status = HANDLER_USED;

               continue;
            }
            
         } while ( false ) ;
      }
      catch( worm_exception & _we )
      {
         r_status = HANDLER_INVALID;
         if( WORM_LOG_ERRORS <= LoggingLevel )
         {
            TLogger::Logit( LoggingOptions
                         , "DBMutableServer::HandleConfigLine(): configuration error:\n%s\n"
                         , _we.what()
                         );
         }
      }
   }
   
   return r_status;
}
//------------------------------------------------------------------------------
void DBMutableServer::CheckConfig()
{
   MutableServerBase::CheckConfig();

   if (   DB_User.length() == 0
       || DB_Password.length() == 0
       || DB_Service.length() == 0
      )
   {
      TLogger::Logit( LoggingOptions
                    , "DBMutableServer::CheckConfig(): <DBConnection> not complete for %s mode\n"
                    , MSB_MODE_NAME[Mode]
                    );
      ConfigState = WORM_STAT_BADSTATE;
   }
}
//------------------------------------------------------------------------------
bool DBMutableServer::PrepareToRun()
{
   bool r_status = false;

   try
   {
      if ( MutableServerBase::PrepareToRun() )
      {
         r_status = InitializeDB();
      }
   }
   catch( worm_exception & _we )
   {
      r_status = false;

      if( WORM_LOG_ERRORS <= LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                       , "DBMutableServer::PrepareToRun() Error: %s\n"
                       , _we.what()
                       );
      }
   }
   return r_status;
}

//------------------------------------------------------------------------------
bool DBMutableServer::InitializeDB()
{
   bool r_status = false;
   
   try
   {
      if (   DB_User.length()     == 0 || 39 < DB_User.length()
          || DB_Password.length() == 0 || 19 < DB_Password.length()
          || DB_Service.length()  == 0 || 19 < DB_Service.length()
         )
      {
         throw worm_exception("Database connection parameter missing or too long");
      }


      char _user[40]
         , _passwd[20]
         , _service[20]
         ;

      strcpy( _user    , DB_User.c_str() ); 
      strcpy( _passwd  , DB_Password.c_str() ); 
      strcpy( _service , DB_Service.c_str() ); 
      
      //
      // Initialize the Ora_API
      //
      if ( ewdb_base_Init( _user, _passwd, _service ) == EWDB_RETURN_FAILURE )
      {
         throw worm_exception("Database connection initialization failed");
      }
      
      r_status = true;
      
   }
   catch( worm_exception & _we )
   {
      if( WORM_LOG_ERRORS <= LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                       , "DBMutableServer::InitializeDB() Error: %s\n"
                       , _we.what()
                       );
      }
   }
   
   return r_status;
}

//------------------------------------------------------------------------------
bool DBMutableServer::GetDefaultsFromDB( void * p_parmstruct )
{
   bool r_status = true;

   OracleConfigSource * _defaults = NULL;

   try
   {
      //
      // Get Default Parameters from the database
      //
/*
      if ( (_defaults = new OracleConfigSource()) == NULL )
      {
         throw worm_exception("Failed creating Oracle configuration datasource");
      }

// EXACTLY HOW PASSPORTS ARE TO BE QUERIED FROM THE DATABASE HAS NOT YET BEEN
// DETERMINED

static const int SERVER_TYPE_ID = 47;

      _defaults->LoadFromDB( SERVER_TYPE_ID );

      HandleParameterLine( _defaults, p_parmstruct );
      
*/

  }
   catch( worm_exception & _we )
   {
      r_status = false;
      
      if( WORM_LOG_ERRORS <= LoggingLevel )
      {
         TLogger::Logit( LoggingOptions
                       , "DBMutableServer::GetDefaultsFromDB() Error: %s\n"
                       , _we.what()
                       );
      }
   }

   if ( _defaults != NULL )
   {
      delete _defaults;
   }
   
   return r_status;
}

//------------------------------------------------------------------------------
