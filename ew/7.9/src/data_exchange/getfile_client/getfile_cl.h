/*   getfilecl.h    */

#ifndef GETFILECL_H
#define GETFILECL_H
 
#define VERSION "getfilecl, V1.0"

#define GETFILE_FAILURE   -1
#define GETFILE_SUCCESS    0
#define GETFILE_DONE       1
#define BUFLEN          4096      /* Max value is 999999 */
#define MAXCLIENT        100      /* Max number of trusted clients */
#define MAXPATHSTR       128      /* Max string length for paths   */

/* Functions declarations
**********************/
void GetConfig( char * );
void LogConfig( char * );
void log_init( char *, char *, int, int );
void log( char *, char *, ... );
int  chdir_ew( char * );
int  rename_ew( char *, char * );

void SocketSysInit( void );
int  ConnectToSendfile( void );
int  GetBlockFromSocket( char [], int * );
int  SendAckToSocket( void );
void CloseReadSock( void );
void CloseSocketConnection( void );
void TzSet( char * );
void sleep_ew( unsigned );

#endif
