#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <transport.h>
#include <earthworm.h>
#include <rdpickcoda.h>
#include <netdb.h>
#include <signal.h>


#include <kom.h>
#include <lockfile.h>

// Earthworm BUF
#define MAX_CHAR 256
#define MAX_BUFSIZ   51740
// Calendar codes
#define GREGORIAN       1             // Modern calendar
#define JULIAN          0             // Old-style calendar
#define BASEJDN         2440588       // 1 January 1970
// Time intervals
#define SECOND          1L
#define MINUTE          (60L*SECOND)
#define HOUR            (60L*MINUTE)
#define DAY             (24L*HOUR)
// Socket
#define uchar unsigned char
#define SOCKETS 2048
#define SOCKET_TIMEOUT 10
#define TCP_BUF_LENGTH	1440
// Macros 
#define qfloor( x, y ) ( x>0 ? (x)/y : -((y-1-(x))/y) )

typedef void Sigfunc(int);

//Date struct
typedef struct _ddate {
	int yr;
	int mon;
	int dy;
	int hr;
	int min;
	float sec;
} DDate;
//-- For every second share memory
struct PEEW {
    int    flag;  // 0: empty, 1: used,  -1: writing.
    uchar  stn_name[8];
    double report_time;   // in seconds. epoch seconds.
    double Pga;
    double Pgv;
    double Pgd;
};
//-- For P arrival share memory
struct PEEWp {
    int    flag;  // 0: empty, 1: used,  -1: writing.
    int    Ptype;
    uchar  stn_name[8];
    double latitude;
    double longitude;
    double altitude;
    double Parrival;
    double Pa;
    double Pv;
    double Pd;
    double Tc;
    double report_time;   // in seconds. epoch seconds.
};
//-- socket
typedef struct socket_state {
  int                active;  	/* socket is active */
  int                init;    	/* socket just initiated */
  struct sockaddr_in sin;     	/* client address */
  int	socket_tmr;				/* socket timer from last data exchange */
} socket_state;
//-- socket
union{
unsigned short i[TCP_BUF_LENGTH / 2]; uchar b[TCP_BUF_LENGTH];
}buf_tcp;


//===================== Palert Queue Structure
typedef struct
{
	unsigned char data[TCP_BUF_LENGTH*5];
	int rear;
	int flag;
	char ip[20];
}Squeue;
void iniQueue(Squeue *S);
void EnQueue (Squeue *S, unsigned char *buf_in, int num_in);
void DeQueue (Squeue *S, unsigned char *buf_out);


//===================== Earthworm SHM function
int put_msg( char *ring, TRACE2_HEADER *trh, int *data  );


//===================== Date and time functions
double make_mstime( int, int, int, int, int, double );
int    isleap( int, int );
void month_day(int, int, int *, int *);
long   jdn( int, int, int, int );
time_t ttime(DDate dd);	//for putmsg


//===================== Sanlien Added functions
int mappingtable(char *name_t);
void get_table();
int do_echo( int skt );

//============================================================Earthworm modul

#define   MAXLOGO   2
#define BUF_SIZE 60000          /* define maximum size for an event msg   */
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */


void  template_config  ( char * );
void  template_lookup  ( void );
void  template_status  ( unsigned char, short, char * );
//===================== NTUST TCHIN
#define LISTENQ 1024int serv(int port);
