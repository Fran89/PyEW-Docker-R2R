// Defines
#define SLOT_DURATION      1000       //Duration of a time slot in miliseconds
#define FNAME_LEN          256        //Maximum length of a folder or file name
#define MAX_CHANNELS       200        //Maximum number of channels
#define MAX_SAMPLE_BUFFER  8200       //Maximum number of samples on the buffer for each station
#define FORMAT_BUD         0
#define FORMAT_SCP         1

// Earthworm includes
#include <transport.h>

// Libmseed
#include <libmseed.h>

// Data Structures
typedef struct
{
   char          stat[11];
   char          chan[11];
   char          net[11];
   char          loc[11];
   MSFileParam  *msfp;
   MSRecord     *msr;
   double        recStart;
   double        recEnd;
   double        nextRead;
   int           dataFinished;
} MSEEDCHANNEL;

typedef struct 
{
   unsigned char MyModId;
   unsigned char InstId;
   long          OutKey; 
   int           HeartbeatInt;
   int           LogSwitch;
   int           Debug;
   double        StartTime;
   double        EndTime;
   double        SendLate;
   int           InterMessageDelayMillisecs;
   char          MseedArchiveFolder[FNAME_LEN];
   int           MseedArchiveFormat;
   MSEEDCHANNEL  Channels[MAX_CHANNELS];
   int           nChannels;
} PARAMS;

typedef struct 
{
   pid_t         MyPid;           // My process ID
   unsigned char InstId;          // Local installation
   unsigned char TypeHeartBeat;
   unsigned char TypeError;
   unsigned char TypeTraceBuf2;   // Waveform buffer for data
   SHM_INFO      OutRegion;       // Memory region for output
   MSG_LOGO      trb2logo;        // Tracebuf2 logo
   //MSG_LOGO      htblogo;         // Heartbeat logo
   //MSG_LOGO      errlogo;         // Error logo
} EWH;


  

// Function Prototypes
void config(char *configfile, PARAMS *Parm);
void lookup(EWH *Ewh, PARAMS *Parm);
void status(unsigned char type, short ierr, char *note, SHM_INFO OutRegion, PARAMS Parm, EWH Ewh);
char* MseedFileName(char *filename, char *foldername, double t, char *stat, char *chan, char *net, char *loc, int archiveFormat);
int ProcessData(MSEEDCHANNEL *channel, double starttime, double endtime, double deltaTime, PARAMS *Parm, EWH * Ewh);










