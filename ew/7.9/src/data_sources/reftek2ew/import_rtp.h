/* @(#)import_rtp.h	1.2 07/23/98 */
/*
*  Revised:
*		06Jul12	---- (rs) provide means to set rate from header packet with
*							get_samplerate_from_eh
*/

#ifndef import_rtp_h_included
#define import_rtp_h_included

/* 1.7.1 - added tracebuf2 splitting */
/* 1.7.2 - fixed valid tracebuf2 check for 1 sample packets */
/* 1.7.3 - when C0 is set, log fills with too many splitting messages, set to debug level 1 or greater */

#define VERSION_ID "1.7.3"


#include <time.h>
#include "time_ew.h"
#include "earthworm.h"
#include "transport.h"
#include "trace_buf.h"

#include "rtp.h"
#include "reftek.h"
#include "util.h"


/* limits */

#define SNAMELEN TRACE2_STA_LEN-1  /* TRACE2_HEADER sta  */
#define CNAMELEN TRACE2_CHAN_LEN-1 /* TRACE2_HEADER chan */
#define NNAMELEN TRACE2_NET_LEN-1  /* TRACE2_HEADER net  */
#define LNAMELEN TRACE2_LOC_LEN-1  /* TRACE2_HEADER loc  */

/* defaults */

#define DEFAULT_HOST          "localhost"
#define DEFAULT_PORT          RTP_DEFAULT_PORT
#define DEFAULT_HEARTBEAT     30
#define DEFAULT_NODATA_ALARM  300

/* Module idents for meaningful exit codes */

#define IMPORT_RTP_MAIN     ((INT32) 1000)
#define IMPORT_RTP_INIT     ((INT32) 2000)
#define IMPORT_RTP_HBEAT    ((INT32) 3000)
#define IMPORT_RTP_SEND     ((INT32) 4000)
#define IMPORT_RTP_NOTIFY   ((INT32) 5000)
#define IMPORT_RTP_SAMPRATE ((INT32) 6000)


/* EW_STATUS ERROR CODES for reporting to EW statmgr */
#define SERVER_OK             1
#define SERVER_NOT_RESPONDING 2
#define NO_DATA_FROM_SERVER   3
#define EW_STATUS_ERROR_CHANNEL_LOOKUP     4
#define EW_STATUS_ERROR_INTERNAL_POINTER   5
#define EW_STATUS_ERROR_UNKNOWN            6
#define EW_STATUS_ERROR_INVALID_TRACEBUF   7

#define MAX_SAMPLE_RATES 15


/* Run time parameters */

typedef struct ring {
    CHAR     *name;
    INT32     key;
    SHM_INFO  shm;
    BOOL      defined;
} RING;

struct param {
    CHAR *MyModName;
    CHAR *prog;  /* program name */
    UINT8 InstId;
    UINT8 Mod;
    RING WavRing;
    RING RawRing;
    INT32 hbeat;
    INT32 nodata;
    INT32 SendUnknownChan;
    CHAR *SCNLFile;
    CHAR *host;
    UINT16 port;
    UINT16 retry;
    UINT16 debug;
    INT32 TimeJumpTolerance;
    INT32 SendTimeTearPackets;
    REAL64 AcceptableSampleRates[MAX_SAMPLE_RATES];
    INT32 iNumAcceptableSampleRates;
    INT32 FilterOnSampleRate;
    INT32 GuessNominalSampleRate;
    INT32 DropPacketsWithDecompressionErrors;
    INT32 DropPacketsOutOfOrder;
    struct rtp_attr attr;
};

/* externs */
extern struct param par;

/* Function prototypes */

BOOL read_params(CHAR *, CHAR *, CHAR *, INT32, struct param *);
VOID note_daspkt(void);
BOOL start_hbeat(SHM_INFO *region, struct param *par);
VOID init(INT32 argc, CHAR **argv, struct param *par);
BOOL notify_init(SHM_INFO *region, struct param *par);
VOID notify_statmgr(UINT16 err);
VOID log_params(CHAR *myname, CHAR *fname, struct param *par);
BOOL read_scnlfile(CHAR *myname, CHAR *path, CHAR *buffer, INT32 len);
VOID load_scnlp(TracePacket *trace, int iEntry);
BOOL init_senders(struct param *par);
BOOL get_samprate(struct reftek_dt *dt, double *output);
VOID set_samprate_from_eh(struct reftek_eh *eh,UINT8 *pkt);
VOID send_wav(SHM_INFO *region, UINT8 *pkt);
VOID send_rtp(SHM_INFO *region, UINT8 *pkt);
VOID terminate(INT32 status);
VOID LogSCNLFile(void);

int    get_dt_index(struct reftek_dt *dt, int * pIndex);
void   reftek2ew_send_error(short ierr, char *note);
double GetLastSampleTime(int iEntryIndex);
BOOL   SetLastSampleTime(int iEntryIndex, double dSampleTime);
BOOL   EWIsValidPacket(TracePacket * pTrace, struct reftek_dt * pDT, 
                       INT32 iDT, double dCalcdSamprate);
INT32  SampleRateIsValid(int iEntryIndex, double dSampRate);
void   SetNomSampleRate(int iEntryIndex, double dSampRate);
double GetNomSampleRate(int iEntryIndex);
BOOL SampleRateIsAcceptable(double dSampRate, int *irate);

#endif  /* import_rtp_h_included */
