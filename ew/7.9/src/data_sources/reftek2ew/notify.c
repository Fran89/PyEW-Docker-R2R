/* @(#)notify.c	1.2 07/01/98 */
/*======================================================================
 *
 * Messages to statmgr
 *
 *====================================================================*/
#include "import_rtp.h"

static MSG_LOGO Logo;
static SHM_INFO *Region = (SHM_INFO *) NULL;
static UINT16 server_status = SERVER_OK;

BOOL notify_init(SHM_INFO *region, struct param *par)
{
CHAR *str;

    Region      = region;
    Logo.instid = par->InstId;
    Logo.mod    = par->Mod;

    if (GetType(str = "TYPE_ERROR", &Logo.type) != 0) {
        logit("t", "FATAL ERROR: invalid message type <%s>", str);
        return FALSE;
    }

    return TRUE;
}

VOID notify_statmgr(UINT16 err)
{
INT32 now;
BOOL transition;
REAL64 dtime;
CHAR *text;
int length, status;
static CHAR msg[256];
static CHAR *server_ok             = "RTP server OK";
static CHAR *server_not_responding = "RTP server not responding";
static CHAR *no_data_from_server   = "no data from RTP server";

/* Make sure we've changed state */

    RequestMutex();
        transition = (err != server_status);
    ReleaseMutex_ew();

    if (!transition) return;

    RequestMutex();
        server_status = err;
    ReleaseMutex_ew();

/* Send the appropirate message */

    now = (INT32) hrtime_ew(&dtime);

    switch (err) {
      case SERVER_OK:
        text = server_ok;
        break;

      case SERVER_NOT_RESPONDING:
        text = server_not_responding;
        break;

      case NO_DATA_FROM_SERVER:
        text = no_data_from_server;
        break;

      default:
        logit("t", "unrecognized statmgr message code (%d) ignored", err);
        return;
    }

    logit("t", "%s\n", text);
    sprintf(msg, "%ld %hd %s\n\0", now, err, text);
    length = strlen(msg);

    RequestMutex();
        status = tport_putmsg(Region, &Logo, length, msg);
    ReleaseMutex_ew();

    if (status != PUT_OK) {
        logit( "t", "Error sending message to statmgr\n");
    } else {
        logit("t", "statmgr notified\n");
    }
}
