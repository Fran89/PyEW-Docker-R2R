/* @(#)hbeat.c	1.3 07/01/98 */
/*======================================================================
 *
 * System heartbeats and termination flag check.
 *
 *====================================================================*/
#include "import_rtp.h"

static int      MyPid;
static MSG_LOGO Logo;
static SHM_INFO *Region = (SHM_INFO *) NULL;
static struct {
    INT32 hbeat;
    INT32 daspkt;
} Last, Wait;

/* Update the packet received time stamp */

VOID note_daspkt()
{
double dtime;
INT32   now;

    now = (INT32) hrtime_ew(&dtime);

    RequestMutex();
        Last.daspkt = now;
    ReleaseMutex_ew();

    notify_statmgr(SERVER_OK);
}

/* Check for server up/down transitions */

static VOID check_dataflow(INT32 now)
{
INT32 then, elapsed;

    RequestMutex();
        then = Last.daspkt;
    ReleaseMutex_ew();

  /*elapsed = (now - Wait.daspkt);*/ /*bug*/
    elapsed = (now - then);
    if (elapsed > Wait.daspkt) notify_statmgr(NO_DATA_FROM_SERVER);
}

static VOID send_heartbeat(INT32 now)
{
int status, length;
static char msg[32];

    sprintf(msg, "%ld %d\n\0", now, MyPid );
    length = strlen(msg);

    RequestMutex();
        status = tport_putmsg(Region, &Logo, length, msg);
    ReleaseMutex_ew();

    if (status != PUT_OK) {
        logit( "t", "Error sending heartbeat (ignored)\n");
    }

    Last.hbeat = now;
}

static thr_ret heartbeat_thread(void *dummy)
{
INT32   now;
double dtime;

    logit("t", "heartbeat thread started\n");

    while(1) {

        now = (INT32) hrtime_ew(&dtime);

    /* Check for termination flag */

        if (tport_getflag(Region) == TERMINATE  ||
            tport_getflag(Region) == MyPid         ) {
            logit("t", "termination flag set... exiting\n");
            terminate(0);
        }

    /* Send heartbeat if necessary */

        if (now - Last.hbeat >= Wait.hbeat) send_heartbeat(now);

    /* Check for no data from live server condition */

        check_dataflow(now);

    /* Wait a bit and do it all again */

        sleep_ew((unsigned) 1000);
    }
}

BOOL start_hbeat(SHM_INFO *region, struct param *par)
{
CHAR *str;
INT32 now;
REAL64 dtime;
unsigned tid;

    now = (INT32) hrtime_ew(&dtime);

    Logo.instid = par->InstId;
    Logo.mod    = par->Mod;
    Region      = region;
    Wait.hbeat  = par->hbeat;
    Wait.daspkt = par->nodata;
    Last.hbeat  = now - Wait.hbeat;
    Last.daspkt = now;

    MyPid = getpid();
    if( MyPid == -1 )
    {
        logit("t","FATAL ERROR: Cannot get pid\n");
        return FALSE;
    }

    if (GetType(str = "TYPE_HEARTBEAT", &Logo.type) != 0) {
        logit("t", "FATAL ERROR: invalid message type <%s>\n", str);
        return FALSE;
    }

    if (StartThread(heartbeat_thread, 0, &tid) == -1) {
        logit("t", "FATAL ERROR: can't start heartbeat thread\n");
        return FALSE;
    }

    return TRUE;
    
}
