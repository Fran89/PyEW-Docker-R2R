
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: statmgr.h 6129 2014-07-25 14:08:57Z philip $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2009/12/14 20:07:10  scott
 *     Fixed revision #s for startstop & statmgr
 *
 *     Revision 1.8  2009/12/13 15:00:00  stefan
 *     Use negative limits for pager and/or email to mean no limit.
 *
 *     Revision 1.7  2007/02/27 05:16:16  stefan
 *     new status types
 *
 *     Revision 1.6  2006/04/26 00:25:34  dietz
 *     + Modified to allow up to 10 pagegroup commands in statmgr config file.
 *     + Modified descriptor files to allow optional module-specific settings for
 *     pagegroup (up to 10) and mail (up to 10) recipients. Any pagegroup or
 *     mail setting in a descriptor file override the statmgr config settings.
 *     + Modified logfile name to use the name of the statmgr config file (had
 *     been hard-coded to 'statmgr*'.
 *     + Modified logging of configuration and descriptor files.
 *
 *     Revision 1.5  2002/07/09 23:10:09  dietz
 *     added optional command pagegroup to descriptor file.
 *     If it exists, it overrides the pagegroup command in statmgr config
 *
 *     Revision 1.4  2002/05/09 19:21:10  alex
 *     changed MAXDESC from 50 to 100. Alex
 *
 *     Revision 1.3  2000/07/24 20:21:00  lucky
 *     Implemented global limits to module, installation, ring, and
 *     message type strings.
 *
 *     Revision 1.2  2000/06/30 17:54:57  lombard
 *     Added check for return status of SendMail(); add transport flush
 *     on startup; added module ID name to module health messages.
 *
 *     Revision 1.1  2000/02/14 19:39:55  lucky
 *     Initial revision
 *
 *
 */

   /***************************************************************
    *                          statmgr.h                          *
    *                                                             *
    *           Include file for status manager program.          *
    ***************************************************************/

/* startstop_lib.h contains MAX_RING def */
#include <startstop_lib.h>

/* max desc should be the Max number of procs to monitor, see MAX_CHILD of startstop_lib.h */
#define MAXDESC 200

/* Maximum number of error types per module */
#define MAXERR 75

/* Maximum number of mail servers */
#define MAXMS 10

/* Maximum number of page/mail recipients */
#define MAXRECIP     10
#define MAXRECIPLEN  60
#define RESTARTME    1
#define UNKNOWNPID   2
#define STOPPED      3

/* Things to get from configuration file
 ***************************************/
unsigned char MyModId;        /* module id for statmgr          */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
unsigned char InstId;         /* local installation id          */
unsigned char TypeHeartBeat;
unsigned char TypeStop;
unsigned char TypeError;
unsigned char TypePage;
unsigned char TypeRestart;
unsigned char TypeStatus;

/***** Structure definitions *****/

typedef struct
{
   char ringName[MAX_RING_STR];           /* Name of input tranport ring      */
   int  ringKey;                          /* Which rings to attach to         */
   int  heartbeatPageit;                  /* Pageit heartbeat interval        */
   int  npagegroup;                       /* Number of pager groups           */
   char pagegroup[MAXRECIP][MAXRECIPLEN]; /* Array of pager group names       */
   int  logswitch;                        /* 1=write logfile to disk; 0=don't */
   int  nmail;                            /* Number of mail recipients        */
   char mail[MAXRECIP][MAXRECIPLEN];      /* Array of mail recipients         */
   int  CheckAllRings;                    /* 1= check all rings;              */
                                          /* from startstop before giving up  */
                                          /* and restarting                   */
   int  DontReportUnknownModule;	  /* if 1, don't report the unknown mod msg */
} CNF;

typedef struct
{
    char            ringName[MAX_RING][MAX_RING_STR];
                                          /* Array of ring names.
                                             MAX_RING_STR is defined in
                                             earthworm_defs.h and at this time
                                             happens to be set to 32          */
    long            ringSize[MAX_RING];  /* Ring size in kbytes               */
    int             ringKey[MAX_RING];   /* Key to shared mem region          */
    SHM_INFO        region[MAX_RING];    /* Region pointer                    */
    int             ringcount;

} RINGER;

typedef struct
{
   int      tsec;          /* Maximum interval between hbeats in secs */
   int      page;          /* Maximum number of pages to send */
   int      mail;          /* Maximum number of mail messages to send */
   int      alive;         /* 1 if heart is beating; 0 if not */
   time_t   timer;         /* the last time a heartbeat was received */
   int      pagecnt;       /* Page count */
   int      mailcnt;       /* Mail messages count */
} HBEAT;

typedef struct
{
   short    err;           /* Error number */
   int      nerr;          /* Number of permitted errors per tsec seconds */
   int      tsec;
   int      page;          /* Maximum number of pages to send */
   int      mail;          /* Maximum number of mail messages to send */
   char     text[80];      /* Text of error */
   time_t   tref;          /* Reference time for timing errors */
   int      errcnt;        /* Cumulative number of errors */
   int      pagecnt;       /* Page count */
   int      mailcnt;       /* Mail messages count */
} ERR;                     /* ERROR is already taken in Windows NT */

typedef struct
{
   char          modName[40];   /* Module name (no white space) */
   char          sysName[30];   /* name of system on which module is running */
   char          modIdName[MAX_MOD_STR]; /* The MOD_ID string from earthworm.d */
   unsigned char modId;         /* Module id number */
   unsigned char instId;        /* Installation id */
   HBEAT         hbeat;         /* Heartbeat info */
   int           nerr;          /* Number of error messages for this module */
   ERR           err[MAXERR];   /* All possible errors for this module */
   int           restart;       /* if =1, restart on cessation of heartbeat               */
                                /*    =2, module wants restart but pid unknown            */
                                /*    =3, module wanting a restart defined as "stopped."  */
                                /*        Statmgr won't try and restart.  If an external  */
                                /*        restart request is sent, then this 3 becomes 1. */
                                /*    =0, If module doesn't ever want a restart, as       */
                                /*        defined in the module's .desc file              */
   char          modPid[30];    /* process id as ascii string, from heartbeats */
   int           npagegroup;    /* Number of pager groups  */
   char          pagegroup[MAXRECIP][MAXRECIPLEN]; /* pager group array */
   int           nmail;         /* Number of mail recipients */
   char          mail[MAXRECIP][MAXRECIPLEN];      /* mail recipient array */
} DESCRIPTOR;


/**** Function prototypes ****/

void statmgr_config( char * );
void statmgr_getdf( char *, DESCRIPTOR *, int, int );
int statmgr_checkheartbeat(char *, DESCRIPTOR *, int );
int  LookUpModId( unsigned char, unsigned char );
int  LookUpErrorNumber( short, int );
void PrintDf( DESCRIPTOR *, int );
void PrintCnf( CNF );
void ReportHealth( int, time_t *, char * );
void ReportErr( int, int, time_t *, char *);

