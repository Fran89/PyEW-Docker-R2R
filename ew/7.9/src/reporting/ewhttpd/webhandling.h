
// Defines
#define MAX_USER_LEN  20
#define MAX_SESSIONS 2
#define MAX_USERS 100
#define SESSION_TTL 120

// Structures
struct session {		// Describes web session.
   char session_id[33];		// Session ID, must be unique
   char random[20];		// Random data used for extra user validation
   char user[MAX_USER_LEN];	// Authenticated user
   time_t expire;		// Expiration timestamp, UTC
};

typedef struct {
   char user[MAX_USER_LEN];	// Username
   char password[MAX_USER_LEN];	// Password
} WEBUSER;


void clear_users( void );
int user_count( void );
int add_user( char* user, char* password );
int is_authorized( const struct mg_connection *conn,
      const struct mg_request_info *request_info );
void redirect_to_login( struct mg_connection *conn,
      const struct mg_request_info *request_info );
int is_autorization_request( const struct mg_request_info *request_info );
int is_ajax_request( const struct mg_request_info *request_info );
void authorize( struct mg_connection *conn,
      const struct mg_request_info *request_info );
int check_password( WEBUSER user );
void print_ajax_header( struct mg_connection *conn );

#ifdef PRIVATE_WEBHANDLING_FUNCS
static struct session *new_session( void );
static void my_strlcpy( char *dst, const char *src, size_t len );
static struct session *get_session( const struct mg_connection *conn );
static void generate_session_id( char *buf, const char *random,
      const char *user );
#endif

