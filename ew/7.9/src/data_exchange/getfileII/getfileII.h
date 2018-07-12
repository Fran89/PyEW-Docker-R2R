/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getfileII.h 4543 2011-08-10 14:54:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/07/28 17:59:41  dietz
 *     added more string-length checking to avoid array overflows
 *
 *     Revision 1.3  2003/01/15 22:53:48  dietz
 *     Added prototype for CloseSock()
 *
 *     Revision 1.2  2003/01/02 21:16:43  lombard
 *     Changed version to 1.0
 *
 *     Revision 1.1  2002/12/20 02:39:11  lombard
 *     Initial revision
 *
 *
 *
 */

/*   getfileII.h    */

#ifndef GETFILEII_H
#define GETFILEII_H
 
#define VERSION "getfileII, V1.1"

#define GETFILE_FAILURE   -1
#define GETFILE_SUCCESS    0
#define GETFILE_DONE       1
#define BUFLEN          4096      /* Max value is 999999 */
#define MAXCLIENT        100      /* Max number of trusted clients */
#define MAXPATHSTR       128      /* Max string length for paths   */

typedef struct
{
    char ip[20];            /* IP address of trusted client */
    char indir[MAXPATHSTR]; /* Where to store files from the client */
} CLIENT;

/* Functions declarations
**********************/
void GetConfig( char * );
void LogConfig( char * );
void log_init( char *, char *, int, int );
void my_log( char *, char *, ... );
int  chdir_ew( char * );
int  rename_ew( char *, char * );
void SocketSysInit( void );
void InitServerSocket( void );
int  AcceptConnection( int * );
int  GetBlockFromSocket( char [], int * );
void CloseReadSock( void );
int  SendAckToSocket( void );
void CloseSock( void );
void TzSet( char * );

#endif
