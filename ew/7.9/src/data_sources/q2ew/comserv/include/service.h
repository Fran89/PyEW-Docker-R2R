/*   Server Access Include File
     Copyright 1994-1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 14 Mar 94 WHO Derived from test header file.
    1 27 Mar 94 WHO Separate data/blockette structure definitions replaced
                    by common structure. Blockette structures are only
                    as long as needed.
    2 31 Mar 94 WHO Client now receives data blocks and blockettes mixed
                    together in the order received by comserv.
    3 13 Apr 94 WHO CSCM_CLIENTS and CSCM_UNBLOCK commands added.
    4 30 May 94 WHO CSQ_TIME removed, now uses CSQ_FIRST.
    5  6 Jun 94 WHO The server's user ID and sleep timing counts for
                    both privileged and non-privileged users added to
                    tserver_struc. CSCR_PRIVILEGE added. PERM changed
                    to 666 to allow other users access.
    6 19 Jun 95 WHO CSCM_LINKSET added.
    7 21 Jun 96 WHO CSF_MM256 removed.
    8  3 Dec 96 WHO Add CSIM_BLK, BLKQ, and change CHAN.
    9 27 Jul 97 WHO Add CSCM_FLOOD_CTRL.
   10  3 Nov 97 WHO More stations for Unix version, add c++ cond.
*/
/* NOTE : SEED data structure definitions (seedstrc.h) are not required
   to be used for gaining access to the server. This allows a client
   process to use it's own version of SEED data structure definitions.
*/
/* General purpose constants */
#define OK 0
#define ERROR -1
#define NOCLIENT -1

/* Following are used as the command for the cs_svc call */
/* Local server commands (does not communicate with DA), returns immediately */
#define CSCM_ATTACH 0           /* Attach me to server if not already attached */
#define CSCM_DATA_BLK 1         /* Request combinations of data and blockettes */
#define CSCM_LINK 2             /* link format info */
#define CSCM_CAL 3              /* Calibrator info */
#define CSCM_DIGI 4             /* Digitizer info */
#define CSCM_CHAN 5             /* Channel recording status */
#define CSCM_ULTRA 6            /* Misc flags and Comm Event names */
#define CSCM_LINKSTAT 7         /* Accumulated link performance info */
#define CSCM_CLIENTS 8          /* Get information about clients */
#define CSCM_UNBLOCK 9          /* Unblock packets associated with client N */
#define CSCM_RECONFIGURE 10     /* Link reconfigure request */
#define CSCM_SUSPEND 11         /* Suspend link operation */
#define CSCM_RESUME 12          /* Resume link operation */
#define CSCM_CMD_ACK 13         /* Client acknowledges command finished */
#define CSCM_TERMINATE 14       /* Terminate server */
#define CSCM_LINKSET 15         /* Set server link parameters */

/* Ultra-Shear DP to DA commands */
#define CSCM_SHELL 20           /* Send shell command */
#define CSCM_VCO 21             /* Set VCO frequency */
#define CSCM_LINKADJ 22         /* Change link parameters */
#define CSCM_MASS_RECENTER 23   /* Mass recentering */
#define CSCM_CAL_START 24       /* Start calibration */
#define CSCM_CAL_ABORT 25       /* Abort calibration */
#define CSCM_DET_ENABLE 26      /* Detector on/off */
#define CSCM_DET_CHANGE 27      /* Change detector parameters */
#define CSCM_REC_ENABLE 28      /* Change recording status */
#define CSCM_COMM_EVENT 29      /* Set remote command mask */
#define CSCM_DET_REQUEST 30     /* Request detector parameters */
#define CSCM_DOWNLOAD 31        /* Start file download */
#define CSCM_DOWNLOAD_ABORT 32  /* Abort download */
#define CSCM_UPLOAD 33          /* Start file upload */
#define CSCM_UPLOAD_ABORT 34    /* Abort upload */
#define CSCM_FLOOD_CTRL 35      /* Turn flooding on and off */

/* Data/Blockettes datamask bits used with CSCM_DATA_BLK command */
#define CSIM_DATA 1             /* Return data */
#define CSIM_EVENT 2            /* Return event blockettes */
#define CSIM_CAL 4              /* Return cal blockettes */
#define CSIM_TIMING 8           /* Return timing blockettes */
#define CSIM_MSG 0x10           /* Return message records */
#define CSIM_BLK 0x20           /* Return blockette records */

/* Command return values */
#define CSCR_GOOD 0             /* Inquiry processed */
#define CSCR_ENQUEUE 1          /* Could not find an empty service queue */
#define CSCR_TIMEOUT 2          /* Timeout waiting for service */
#define CSCR_INIT 3             /* Server not intialized */
#define CSCR_REFUSE 4           /* Server refuses your attachment */
#define CSCR_NODATA 5           /* No data or blockettes available */
#define CSCR_BUSY 6             /* Cannot process command at this time */
#define CSCR_INVALID 7          /* Command not valid */
#define CSCR_DIED 8             /* Server has apparently died */
#define CSCR_CHANGE 9           /* Server has changed */
#define CSCR_PRIVATE 10         /* Could not create client's module */
#define CSCR_SIZE 11            /* Command output buffer not large enough */
#define CSCR_PRIVILEGE 12       /* Privileged command */

/* Link formats */
#define CSF_QSL 0               /* QSL Format - variable length up to 512 bytes */
#define CSF_Q512 1              /* Q512 Format - fixed length 512 bytes */

/* Command completion status */
#define CSCS_IDLE 0             /* No command being processed */
#define CSCS_INPROGRESS 20      /* Command in progress */
#define CSCS_FINISHED 21        /* Command done, waiting for client to clear status */
#define CSCS_REJECTED 22        /* Command not known by DA */
#define CSCS_ABORTED 23         /* File transfer aborted */
#define CSCS_NOTFOUND 24        /* File not found */
#define CSCS_TOOBIG 25          /* File to big to transfer */
#define CSCS_CANT 26            /* Can't create file on DA */

/* Sequence control */
#define CSQ_NEXT 0              /* Get next available data/blockettes */
#define CSQ_FIRST 1             /* Get first available data/blockettes */
#define CSQ_LAST 2              /* Get last available data/blockette */
#define CSQ_TIME 3              /* Get first data/blockettes at or after time */

/* System limits */
#ifdef _OSK
#define MAXSTATIONS 32          /* maximum number of stations */
#define MAXCLIENTS 16           /* maximum number of clients */
#else
#define MAXSTATIONS 64          /* maximum number of stations */
#define MAXCLIENTS 16           /* maximum number of clients */
#endif

/* Shared memory and semaphore permissions */
#define PERM 0666 /* Everybody can access server structures */

/* Queue ring indexes and bit positions */
#define DATAQ 0
#define DETQ 1
#define CALQ 2
#define TIMQ 3
#define MSGQ 4
#define BLKQ 5
#define NUMQ 6
#define CHAN 6

/* Definitions of client accessable portion of server's shared memory segment */
/* Service queue structure */
typedef struct
  begin
    int clientseg ;
    complong clientname ;
  end tsvc ;
 
typedef struct
  begin
    char init ;                /* Is "I" if structure initialized */
    int server_pid ;           /* PID of server */
    int server_semid ;         /* Id of access semaphore */
    int server_uid ;           /* UID of server */
    long client_wait ;         /* Number of microseconds client should wait for service */
    long privusec ;            /* Microseconds per wait for privileged users */
    long nonusec ;             /* Microseconds per wait for non-privileged users */
    long next_data ;           /* Next data packet number */
    double servcode ;          /* Unique server invocation code */
    tsvc svcreqs[MAXCLIENTS] ; /* Service queue */
  end tserver_struc ;

typedef tserver_struc *pserver_struc ;

typedef struct
  begin
    short first ;   /* first selector */
    short last ;    /* last selector */
  end selrange ;

/* 
   Definition of server accessable portion of client's shared memory segment. This
   section is repeated for each station 
*/
typedef struct
  begin
    complong name ;           /* station name */
    int seg_key ;             /* station segment key */
    short command ;           /* Command to perform for this call */
    boolean blocking ;        /* Client is blocking */
    byte status ;             /* station status */
    long next_data ;          /* Next data packet I want */
    double last_attempt ;     /* time at last attempt to talk to server */ 
    double last_good ;        /* time of last good access to server */
    double servcode ;         /* Server reference code for sequence validity */
    pserver_struc base ;      /* address of server's shared memory segment in my address space */
    long cominoffset ;        /* Offset to command input buffer */
    long comoutoffset ;       /* Offset to command output buffer */
    long comoutsize ;         /* Size of command output buffer */
    long dbufoffset ;         /* Offset to start of data buffers */
    long dbufsize ;           /* Size of user data to be moved */
    short maxdbuf ;           /* Maximum number of data buffers that can be filled */
    short reqdbuf ;           /* Number of data packets requested in this call */
    short valdbuf ;           /* Number of data packets valid after call */
    short seqdbuf ;           /* Sequence control for data request */
    double startdbuf ;        /* Earliest time for packets */
    long seloffset ;          /* Offset to start of selector array */
    short maxsel ;            /* Number of selectors */
    selrange sels[CHAN+1] ;   /* Selector ranges for each type of data and channel request */
    short datamask ;          /* Data/blockette request command bitmask */
/* 
   Command input buffer, command output buffer, data buffers, blockette buffers, and
   selector array follows.
*/
  end tclient_station ;

typedef tclient_station *pclient_station ;

/* 
   This is the header for the client's shared memory segment, tclient_station structures
   follow, one for each station. Offsets contains zeroes for unsupported stations (valid
   entries are 0 to maxstation-1).
*/
typedef struct
  begin
    complong myname ;           /* Client's Name */
    int client_pid ;            /* Client's PID */
    int client_shm ;            /* Client's shared memory */
    int client_uid ;            /* Client's UID */
    boolean done ;              /* Service has been performed */
    byte spare ;
    short error ;               /* Error code for service */
    short maxstation ;          /* Number of stations this client works with */
    short curstation ;          /* Current station */
    long offsets[MAXSTATIONS] ; /* Offsets from start of this structure to start
                                   of the tclient_station structures for each station */
  end tclient_struc ;

typedef tclient_struc *pclient_struc ;

/* 
   Structures used to generate a shared memory segment for multiple stations
   using the cs_gen procedure.
*/
typedef struct
  begin
    complong stationname ;    /* Station's name */
    long comoutsize ;         /* Comoutbuf size */
    short selectors ;         /* Number of selectors wanted */
    short mask ;              /* Data request mask */
    boolean blocking ;        /* Blocking connection */
    int segkey ;              /* Segment key for this station */
    char directory[120] ;     /* Directory for station */
  end tstation_entry ;
  
typedef struct
  begin
    complong myname ;                           /* Client's name */
    boolean shared ;                            /* TRUE if comoutbuffer is shared among all stations */
    short station_count ;                       /* Number of stations in list */
    short data_buffers ;                        /* Number of data buffers to allocate */
    tstation_entry station_list[MAXSTATIONS] ;  /* Array of station specific information */
  end tstations_struc ;
  
typedef tstations_struc *pstations_struc ;
 
/* Data record structure returned to user */
typedef struct
  begin
    double reception_time ;
    double header_time ;
    byte data_bytes[512] ; /* Up to 512 byte data record, may be shorter */
  end tdata_user ;

typedef tdata_user *pdata_user ;

typedef char seltype[6] ; /* selector definitions, LLSSS\0 format */

typedef seltype selarray[] ; /* for however many there are */

typedef selarray *pselarray ;

#ifdef __cplusplus
extern "C" {
#endif

/*
  This procedure will setup the tstations_struc for all comlink stations
  found in the "/etc/stations.ini" file or the specified station. You can then
  customize stations to suit your needs if you desire. For each station it puts 
  in the default number of data buffers and selectors specified. If shared is TRUE,
  then there is one shared command input/output buffer for all stations (of size
  comsize), else each station has it's own buffer. If blocking is TRUE, then
  a blocking connection will be requested.
*/
  void cs_setup (pstations_struc stations, pchar name, pchar sname, boolean shared, 
      boolean blocking, short databufs, short sels, short mask, long comsize) ;

/* 
   Remove a selected station from the tstations_struc structure, must be
   called before cs_gen to have any effect.
*/
  void cs_remove (pstations_struc stations, short num) ;

/*
  This function takes your tstations_struc and builds your shared memory segment
  and returns it's address. This should be done when client starts. You can
  either setup the tstations_struc yourself, or use the cs_all procedure.
  It attaches to all servers that it can.
*/
  pclient_struc cs_gen (pstations_struc stations) ;

/* 
  This function detaches from all stations and then removes your shared
  memory segment. This should be called before the client exits.
*/
  void cs_off (pclient_struc client) ;

/*
  This is the basic access to the server to handle any command. The command
  must have already been setup in the "tclient_station" structure and the
  station number into "curstation" in the "tclient_struc" structure before
  calling. Returns one of the "CSCR_xxxx" values as status.
*/
  short cs_svc (pclient_struc client, short station_number) ;

/* This is a polling routine used by cs_scan. The current time is the third
   parameter. It will check the station to see :
     1) If it does not have good status, then every 10 seconds it tries :
         a) cs_link, and if good, does :
         b) cs_attach.
     2) If a station has good status then it checks the server reference code
        to make sure the server hasn't changed invocations while the client
        was away. If it did it :
         a) Resets the next record counters and changes the sequencing to CSQ_FIRST.
         b) Sets the status to CSCR_CHANGE and returns the same.
     3) If the station has good status and the server has not changed then it
        checks to see if the server has newer data or blockettes than the ones
        the client has. If so it returns with CSCR_GOOD, if not returns CSCR_NODATA ;
        To keep the link alive it will return with CSCR_GOOD if no activity
        within 10 seconds.
*/
  byte cs_check (pclient_struc client, short station_number, double now) ;

/* 
  For each station it calls cs_check, and it returns with CSCR_GOOD does a
  CSCM_DATA_BLK command to try to get data. If it gets data then it returns
  the station number, or NOCLIENT if no data. Also, if there is change in
  status of the server (goes away, or comes back for instance) then the station
  number is also returned, and the alert flag is set. You should then check the
  status byte for that station to find out what happened.
*/     
  short cs_scan (pclient_struc client, boolean *alert) ;

/* try to link to server's shared memory segment. first copies server reference
   code into client's structure */
  void cs_link (pclient_struc client, short station_number, boolean first) ;

/* try to send an attach request to the server */
  void cs_attach (pclient_struc client, short station_number) ;

#ifdef __cplusplus
}
#endif
