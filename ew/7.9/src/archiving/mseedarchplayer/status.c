/******************************************************************************
 * status() builds a heartbeat or error message & puts it into                *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
// Standard includes
#include <string.h>

// Earthworm Includes
#include <earthworm.h>
#include <transport.h>

// Local Includes
#include "mseedarchplayer.h" 

void status(unsigned char type, short ierr, char *note, SHM_INFO OutRegion, PARAMS Parm, EWH Ewh) 
{
    MSG_LOGO logo;
    char msg[256];
    long size;
    time_t t;

    /* Build the message
     *******************/
    logo.instid = Parm.InstId;
    logo.mod = Parm.MyModId;
    logo.type = type;

    time(&t);

    if (type == Ewh.TypeHeartBeat) 
    {
        sprintf(msg, "%ld %ld\n", (long) t, (long) Ewh.MyPid);
    } 
    else if (type == Ewh.TypeError) 
    {
        sprintf(msg, "%ld %hd %s\n", (long) t, ierr, note);
        logit("et", "mseedarchplayer: %s\n", note);
    }

    size = strlen(msg); /* don't include the null byte in the message */

    /* Write the message to shared memory
     ************************************/
    if (tport_putmsg(&OutRegion, &logo, size, msg) != PUT_OK) 
    {
        if (type == Ewh.TypeHeartBeat) 
        {
            logit("et", "mseedarchplayer:  Error sending heartbeat.\n");
        } 
        else if (type == Ewh.TypeError) 
        {
            logit("et", "mseedarchplayer:  Error sending error:%d.\n", ierr);
        }
    }
    return;
}
