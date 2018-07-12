/* @(#)samprate.c	1.5 07/23/98 */
/*======================================================================
 *
 * Figure out sample rate by comparing sequential packets
 *
 * Revised  :
 *	12Jul06	 (rs) make routine to use eh packet yo get dt_list item
 *            calculate rate then round to integer unless is < 1.0
 *====================================================================*/
#include <math.h>
#include "import_rtp.h"
#include "stdtypes.h"

#define MY_MOD_ID IMPORT_RTP_SAMPRATE

#define INCREMENT_OK(p, c) (((c) == (p) + 1) || ((p) == 9999 && (c) == 0))

#define THRESHOLD 5.0 /* hardcode for now (forever?) */

typedef struct {
    REAL64 tstamp;
    UINT16   nsamp;
    BOOL   set;
} DTCHAN;

typedef struct {
    UINT16   unit;
    UINT16   seqno;
    UINT16   stream;
    REAL64 samprate;
    DTCHAN chan[RTP_MAXCHAN];
} DTPARM;

struct dt_list {
    DTPARM dt;
    struct dt_list *next;
};
    
static struct dt_list head = {
    {0, 0, 0, 0.0,
        {
            {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE},
            {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE},
            {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE},
            {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE}, {0.0, 0, FALSE}
        },
    },
    (struct dt_list *) NULL
};

/* Search the list for a previous entry for this stream */

static DTPARM *prev_dtparm(struct reftek_dt *dt)
{
struct dt_list *crnt;

    crnt = head.next;
    while (crnt != (struct dt_list *) NULL) {
        if (
            crnt->dt.unit   == dt->unit   &&
            crnt->dt.stream == dt->stream
        ) return &crnt->dt;
        crnt = crnt->next;
    }

    return (DTPARM *) NULL;
}


static DTPARM *prev_ehparm(struct reftek_eh *eh)
{
struct dt_list *crnt;

    crnt = head.next;
    while (crnt != (struct dt_list *) NULL) {
        if (
            crnt->dt.unit   == eh->unit   &&
            crnt->dt.stream == eh->stream
        ) return &crnt->dt;
        crnt = crnt->next;
    }

    return (DTPARM *) NULL;
}

/* Add a new entry to the list */

static VOID add_stream(struct reftek_dt *dt)
{
UINT16 i;
struct dt_list *new;

    new = (struct dt_list *) malloc(sizeof(struct dt_list));
    if (new == (struct dt_list *) NULL) {
        logit("t", "FATAL ERROR: malloc: %s\n", strerror(errno));
        terminate(MY_MOD_ID + 1);
    }

    new->dt.unit   = dt->unit;
    new->dt.seqno  = dt->seqno;
    new->dt.stream = dt->stream;
    new->dt.samprate = -1.0;
    for (i = 0; i < RTP_MAXCHAN; i++) new->dt.chan[i].set = FALSE;
    new->dt.chan[dt->chan].tstamp = dt->tstamp;
    new->dt.chan[dt->chan].nsamp  = dt->nsamp;
    new->dt.chan[dt->chan].set    = TRUE;

    new->next = head.next;
    head.next = new;
}

/* Update entry with new values */

static VOID store(DTPARM *prev, struct reftek_dt *crnt)
{
    prev->seqno  = crnt->seqno;
    prev->chan[crnt->chan].tstamp = crnt->tstamp;
    prev->chan[crnt->chan].nsamp  = crnt->nsamp;
    prev->chan[crnt->chan].set    = TRUE;
}

/* Make sure rate calculated is one which is used by Reftek equipment */

double ReftekRates[] = {0.1,1.0,4.0,5.0,8.0,10.0,20.0,25.0,40.0,50.0,100.0,125.0,200.0,250.0,500.0,1000.0};
BOOL IsValidRate(REAL32 rate)
	{
   UINT16 i;

   for (i = 0; i < (sizeof(ReftekRates)/sizeof(double)); i++) 
		{
      if (rate == ReftekRates[i])
            return (TRUE);
    	}

    return (FALSE);
	}





/* Compare this packet with the previous one and get sample rate */

BOOL get_samprate(struct reftek_dt *dt, double *output)
{
UINT16 i;
REAL64 newrate, percent_change;
DTPARM *prev;

/* Make sure we can index off the channel number */

    if (dt->chan >= RTP_MAXCHAN) {
        logit("t", "FATAL ERROR: illegal chan id: %d\n", dt->chan);
        terminate(MY_MOD_ID + 2);
    }

/* Get the previous parameters for this stream */

    if ((prev = prev_dtparm(dt)) == (DTPARM *) NULL) {
        add_stream(dt);
        return FALSE;
    }

/* Wipe out prior data if sample numbers fail to increment OK */

    if (!INCREMENT_OK(prev->seqno,dt->seqno)) {
        for (i = 0; i < RTP_MAXCHAN; i++) prev->chan[i].set = FALSE;
    }

/* If we don't have prior data for this channel then use the previous
 * sample rate, if we've got one */

    if (!prev->chan[dt->chan].set) {
        store(prev, dt);
        if (prev->samprate > 0.0) {
            *output = prev->samprate;
            return TRUE;
        } else {
            return FALSE;
        }
    }

/* Should be able to determine a sample rate at this point */

    newrate = (double) prev->chan[dt->chan].nsamp /
              (dt->tstamp - prev->chan[dt->chan].tstamp);
    if (newrate >= 1.0) newrate = (REAL64)((INT)(newrate + .5));/* all rates are integers except .1 */
   store(prev, dt);

/* Modify rate to use if sample rate changes significantly or is the first time to set*/

   if (prev->samprate < 0.0) 
		{
		prev->samprate = newrate;
  		logit("t", "Calculate %04X:%hu rate as %.3lf\n",
            prev->unit, prev->stream, prev->samprate);
		}
	else
		{
    	percent_change = (fabs(newrate - prev->samprate)/newrate) * 100.0;
    	if (percent_change > THRESHOLD && IsValidRate(newrate)) 
			{
     		logit("t", "WARNING - %04X:%hu rate change from %.3lf to %.3lf\n",
            prev->unit, prev->stream, prev->samprate, newrate);
      	prev->samprate = newrate;
    		}
		}

/* Give it to the caller */

    *output = prev->samprate;
    return TRUE;
}

/****************************************************************************
	Purpose:	Set the sample rate to use from the header packet if available
   Returns:	Nothing
   Revised:
		06Jul12	---- (rs) provide means to set rate from header packet
===========================================================================*/
VOID set_samprate_from_eh(struct reftek_eh *eh,UINT8* pkt)
	{
	UINT16 i;
	DTPARM *prev;
	struct dt_list *new;
	REAL64 last_rate = 0.0;

	/* Get the previous parameters for this stream */
	
	if ((prev = prev_ehparm(eh)) == (DTPARM *) NULL) 
		{
	 	new = (struct dt_list *) malloc(sizeof(struct dt_list));
	 	if (new == (struct dt_list *) NULL) 
			{
			logit("t", "FATAL ERROR: malloc: %s\n", strerror(errno));
			terminate(MY_MOD_ID + 1);
	 		}
	
	 	new->dt.unit   = eh->unit;
	 	new->dt.seqno  = eh->seqno;
	 	new->dt.stream = eh->stream;
	 	new->dt.samprate = reftek_eh_rate(pkt);
	 	for (i = 0; i < RTP_MAXCHAN; i++) 
			{
			new->dt.chan[i].set = FALSE;
			}
	
		new->next = head.next;
		head.next = new;
	 	}
	else
		{
		last_rate = prev->samprate;
		prev->samprate = reftek_eh_rate(pkt);
		if (last_rate != prev->samprate)
			{
			logit("t", "EH sample rate for %hx:%d is %f\n",eh->unit,eh->stream,prev->samprate);
			}
		}
	}

 
