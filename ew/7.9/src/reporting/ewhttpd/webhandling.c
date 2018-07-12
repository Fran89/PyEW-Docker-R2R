// Standard Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>


// Mongoose Webserver Includes
#include "mongoose.h"
#define PRIVATE_WEBHANDLING_FUNCS 1	/* used to turn on static declarations in webhandling.h */
#include "webhandling.h"


// Constants
static const char *login_url = "/ewhttpd_login.html";
static const char *authorize_url = "/authorize";
static const char *ajax_url = "/ewajax";
static const char *ajax_reply_start = "HTTP/1.1 200 OK\r\n Cache: no-cache\r\n"
      "Content-Type: text/html\r\n\r\n";
      //"Content-Type: application/json\r\n\r\n";
      //"Content-Type: application/x-javascript\r\n\r\n";

// Globals
static WEBUSER	*Users[MAX_USERS];	// Array of users
static int	nUsers = 0;		// Number of users
// Protects messages, sessions, last_message_id
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static struct session sessions[MAX_SESSIONS];  // Current sessions


// Creates a new user and adds it to the users array
int add_user( char* user, char* password )
{
   // Possible errors
   // - Too many users
   // - Very long user name
   // - Very long password
   if( nUsers >= MAX_USERS ||
         strlen( user ) > MAX_USER_LEN ||
         strlen( password ) > MAX_USER_LEN ) return -1; 
   
   // Reserve memory for new user
   Users[nUsers] = ( WEBUSER* ) malloc( sizeof( WEBUSER ) );
   if( Users[nUsers] == NULL ) return -1;
   
   // Record user name and password
   strncpy( Users[nUsers]->user, user, MAX_USER_LEN );
   strncpy( Users[nUsers]->password, password, MAX_USER_LEN );
   
   // Update number of users
   nUsers++;
   
   return 1;
}



// Clears all users
void clear_users( void )
{
   int i;
   for( i = 0; i<nUsers; i++ ) free( Users[i] );
   nUsers = 0;
}


int user_count( void )
{
   return nUsers;
}

// Checks authorization
// Return 1 if request is authorized, 0 otherwise.
int is_authorized( const struct mg_connection *conn,
      const struct mg_request_info *request_info )
{
   struct session *session;
   char valid_id[33];
   int authorized = 0;

   // Always authorize accesses to login page and to authorize URI
   if( !strcmp( request_info->uri, login_url ) ||
         !strcmp( request_info->uri, authorize_url ) )
      return 1;

   pthread_rwlock_rdlock( &rwlock );
   if( ( session = get_session( conn ) ) != NULL )
   {
      generate_session_id( valid_id, session->random, session->user );
      if( strcmp( valid_id, session->session_id ) == 0 )
      {
         session->expire = time(0) + SESSION_TTL;
         authorized = 1;
      }
   }
   pthread_rwlock_unlock(&rwlock);

   return authorized;
}


// Get session object for the connection. Caller must hold the lock.
static struct session *get_session( const struct mg_connection *conn )
{
   int i;
   char session_id[33];
   time_t now = time( NULL );
   mg_get_cookie( conn, "session", session_id, sizeof( session_id ) );
   for( i = 0; i < MAX_SESSIONS; i++ )
      if( sessions[i].expire != 0 &&
            sessions[i].expire > now &&
            strcmp( sessions[i].session_id, session_id ) == 0 )
         break;

  return i == MAX_SESSIONS ? NULL : &sessions[i];
}

// Generate session ID. buf must be 33 bytes in size.
// Note that it is easy to steal session cookies by sniffing traffic.
// This is why all communication must be SSL-ed.
static void generate_session_id( char *buf, const char *random,
      const char *user ) 
{
  mg_md5( buf, random, user, NULL );
}

// Redirect user to the login form. In the cookie, store the original URL
// we came from, so that after the authorization we could redirect back.
void redirect_to_login( struct mg_connection *conn,
      const struct mg_request_info *request_info )
{
   mg_printf( conn, "HTTP/1.1 302 Found\r\n"
      "Set-Cookie: original_url=%s\r\n"
      "Location: %s\r\n\r\n",
      request_info->uri, login_url );
}

int is_autorization_request( const struct mg_request_info *request_info )
{
   return ( strcmp( request_info->uri, authorize_url ) == 0 );
}

int is_ajax_request( const struct mg_request_info *request_info )
{
   return ( strcmp( request_info->uri, ajax_url ) == 0 );
}


// A handler for the /authorize endpoint.
// Login page form sends user name and password to this endpoint.
void authorize( struct mg_connection *conn,
      const struct mg_request_info *request_info ) 
{
   WEBUSER user;
   struct session *session;
   char buf[100];
   size_t nbuf;
   
   nbuf = ( size_t ) mg_read( conn, buf, sizeof( buf ) );
   mg_get_var( buf, nbuf, "user", user.user, sizeof( user.user ) );
   mg_get_var( buf, nbuf, "password", user.password, sizeof( user.password ) );
      
   if ( check_password( user ) && ( session = new_session() ) != NULL ) {
    // Authentication success:
    //   1. create new session
    //   2. set session ID token in the cookie
    //   3. remove original_url from the cookie - not needed anymore
    //   4. redirect client back to the original URL
    
      my_strlcpy(session->user, user.user, sizeof(session->user));
      snprintf(session->random, sizeof(session->random), "%d", rand());
      generate_session_id(session->session_id, session->random, session->user);
      mg_printf(conn, "HTTP/1.1 302 Found\r\n"
            "Set-Cookie: session=%s; max-age=3600; http-only\r\n"  // Session ID
            "Set-Cookie: user=%s\r\n"  // Set user, needed by Javascript code
            "Set-Cookie: original_url=/; max-age=0\r\n"  // Delete original_url
            "Location: /\r\n\r\n",
            session->session_id, session->user);
        
  } else {
    // Authentication failure, redirect to login.
    redirect_to_login(conn, request_info);
  }
  
}



// Loops through users to find one
int check_password( WEBUSER user )
{
   int i;
   for( i = 0; i < nUsers; i++ )
      if( strcmp( Users[i]->user, user.user ) == 0 &&
            strcmp( Users[i]->password, user.password ) == 0 )
         return 1;
   return 0;
}

// Allocate new session object
static struct session *new_session( void ) 
{
   int i;
   time_t now = time(NULL);
   pthread_rwlock_wrlock(&rwlock);
   for (i = 0; i < MAX_SESSIONS; i++)
   {
      if (sessions[i].expire == 0 || sessions[i].expire < now)
      {
         sessions[i].expire = time(0) + SESSION_TTL;
         break;
      }
   }
   pthread_rwlock_unlock( &rwlock );
   return i == MAX_SESSIONS ? NULL : &sessions[i];
}

static void my_strlcpy( char *dst, const char *src, size_t len ) 
{
   strncpy( dst, src, len );
   dst[len - 1] = '\0';
}

void print_ajax_header( struct mg_connection *conn )
{
   mg_printf( conn, "%s", ajax_reply_start );
}


