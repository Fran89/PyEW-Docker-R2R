/* @(#)scn.c    1.2 07/22/98 */
/*======================================================================
 *
 * Pin number lookups.
 *
 * Revised:
 *		13Jul06  (rs) provide a means to reset nominal in case the units 
 *                  sample rate is changed
 *====================================================================*/
#include "import_rtp.h"

#define DELIMITERS " \t"
/* MAX_TOKEN is the maximum number of tokens we would see on a line */
/* 9 is the most we should ever see, but it'd be a shame to blow up over a comment */
#define MAX_TOKEN    24  
/* NUM_REQ_TOKENS is the number of tokens required on each line */
#define NUM_REQ_TOKENS    8

typedef struct _lookup_table {
    CHAR sname[SNAMELEN+1];
    CHAR cname[CNAMELEN+1];
    CHAR nname[NNAMELEN+1];
    CHAR lname[LNAMELEN+1];
    INT32 pinno;
    UINT16 unit;
    UINT16 stream;
    UINT16 chan;
    REAL64 samprate;
    REAL64 tLastSample;
    BOOL   bFromSCNLFile;
    BOOL   bSampRateSet;
} lookup_table;


static int  compare_lookup_table_entries(const void * p1, const void * p2);

static lookup_table *entry;


/* number of channels used in the channel entry list */
static INT32 nentry = 0;

/* number of channels read from SCN config file */
static INT32 nSCNListEntry = 0;


/* number of channels allocated in the channel entry list */
static INT32 nListSize = 0;

BOOL read_scnlfile(CHAR *prog, CHAR *path, CHAR *buffer, INT32 len)
{
FILE *fp;
INT32 i, status, lineno, ntok;
CHAR *token[MAX_TOKEN];
BOOL rcSampRate = TRUE;

/* init buffers */
     memset(token, 0 , sizeof(token));

/* Open data file */

    if ((fp = fopen(path, "r")) == (FILE *) NULL) {
        logit("et", "%s: FATAL ERROR: fopen: ", prog);
        perror(path);
        return FALSE;
    }

/* Read once to count how many entries */

    lineno = 0;
    nSCNListEntry = 0;
    while ((status = util_getline(fp, buffer, len, '#', &lineno)) == 0) {
        if (strncmp(buffer, "//", 2) == 0) continue;
        ntok = util_parse(buffer, token, DELIMITERS, MAX_TOKEN, 0);
        if (ntok < NUM_REQ_TOKENS) {
            logit("et", "%s: FATAL ERROR: ", prog);
            logit("et", "illegal entry in SCNLFile %s, line %d",
                   path, lineno );
            return FALSE;
        }
        ++nSCNListEntry;
    }

    if (nSCNListEntry == 0 )
    {
      if(!par.SendUnknownChan) 
      {
        logit("et", "%s: FATAL ERROR: %s: ", prog, path);
        logit("et", "no valid entries found\n");
        return FALSE;
      }
      else
      {
        logit("et", "%s: WARNING %s: ", prog, path);
        logit("et", "no valid channel entries found in SCN config list.\n");
      }
    }  /* end if nSCNListEntry == 0 */

/* Allocate space for everything and read it in */
    if(par.SendUnknownChan)
    {
      /* we will populate the entry lookup_table with all of
         the channels that we see coming in the data stream.
         So we should allocate more than just the number
         of channels in the scnl list. */
      nListSize = 2 * nSCNListEntry;
    }
    else
    {
      nListSize = nSCNListEntry;
    }
    entry = (lookup_table *)
            malloc(nListSize*sizeof(lookup_table));
    if (entry == (lookup_table *) NULL) {
        logit("et", "%s: FATAL ERROR: mallocing %d bytes during read_scnlfile.\n", 
                prog, nListSize*sizeof(lookup_table));
        perror("malloc");
        return FALSE;
    }

    i = 0;
    rewind(fp);
    lineno = 0;
    while ((status = util_getline(fp, buffer, len, '#', &lineno)) == 0) {
        if (strncmp(buffer, "//", 2) == 0) continue;
        ntok = util_parse(buffer, token, DELIMITERS, MAX_TOKEN, 0);
        if (ntok < NUM_REQ_TOKENS) continue;

        if (i == nSCNListEntry) {
            logit("et", "%s: FATAL ERROR: ", prog);
            logit("et", "logic error 1\n");
            return FALSE;
        }

        memset(&entry[i], 0, sizeof(lookup_table));
        entry[i].unit   = (UINT16) strtoul(token[0],(char **)NULL,16);
        entry[i].stream = (UINT16) atoi(token[1]) - 1;
        entry[i].chan   = (UINT16) atoi(token[2]) - 1;
        memcpy((void *) entry[i].sname, (void *) token[3], SNAMELEN);
        memcpy((void *) entry[i].cname, (void *) token[4], CNAMELEN);
        memcpy((void *) entry[i].nname, (void *) token[5], NNAMELEN);
        memcpy((void *) entry[i].lname, (void *) token[6], LNAMELEN);
        entry[i].pinno  = (INT32) atoi(token[7]);
        if(ntok > 8)
        {
          int ir;
          entry[i].samprate = atof(token[8]);
          if( !SampleRateIsAcceptable( (double)entry[i].samprate, &ir ) )
          {
            logit("e","Channel %d (%s %s %s %s) Invalid nominal "
                      "sample rate (%.2f) in scnl file.\n",
                  i+1, entry[i].sname, entry[i].cname, entry[i].nname,
                   entry[i].lname, entry[i].samprate);
            rcSampRate = FALSE;
          }
          else
          {
            logit("et","Setting nominal sample rate for (%s %s %s %s) to %6.2f "
                  "from scnl file.\n",
                  entry[i].sname, entry[i].cname, 
                  entry[i].nname, entry[i].lname,
                  entry[i].samprate);
            entry[i].bSampRateSet = TRUE;
          }
        }
        else if(par.FilterOnSampleRate && (!par.GuessNominalSampleRate))
        {
            logit("e","Channel %d (%s %s %s %s) missing sample rate in scnl file.\n",
                  i+1, entry[i].sname, entry[i].cname, entry[i].nname, entry[i].lname,
                  entry[i].samprate);
            rcSampRate = FALSE;
        }

        entry[i].sname[SNAMELEN] = 0;
        entry[i].cname[CNAMELEN] = 0;
        entry[i].nname[NNAMELEN] = 0;
        entry[i].lname[LNAMELEN] = 0;
        entry[i].bFromSCNLFile = TRUE;

        ++i;
    }

    nentry = i;

    if (nSCNListEntry != nentry) {
        logit("et", "%s: FATAL ERROR: ", prog);
        logit("et", "logic error 2\n");
        return FALSE;
    }

    /* sort the list for better performance */
    qsort(entry, nentry, sizeof(lookup_table),
          compare_lookup_table_entries);

    if(par.FilterOnSampleRate)
      return(rcSampRate);

    return TRUE;
}

static int compare_lookup_table_entries(const void * p1, const void * p2)
{
  lookup_table * plt1 = (lookup_table *) p1;
  lookup_table * plt2 = (lookup_table *) p2;

  if(plt1->unit < plt2->unit)
    return(-1);
  else if(plt1->unit > plt2->unit)
    return(1);
  else if(plt1->stream < plt2->stream)
    return(-1);
  else if(plt1->stream > plt2->stream)
    return(1);
  else if(plt1->chan < plt2->chan)
    return(-1);
  else if(plt1->chan > plt2->chan)
    return(1);
  else  /* equal */
    return(0);
}  /* end compare_lookup_table_entries() */

BOOL add_dt_entry_to_list(struct reftek_dt *dt)
{
  /* check to see if the list is large enough to accomodate another entry */
  if(nentry == nListSize)
  {
  /* allocate more list space */
    lookup_table * pTemp;

    /* double the list size.  Creating a new list is an expensive op */
    nListSize *= 2;

    /* allocate the new list */
    pTemp = (lookup_table *)
            malloc(nListSize*sizeof(lookup_table));

    /* check for allocation error */
    if (pTemp == (lookup_table *) NULL) 
    {
        logit("et", "%s: FATAL ERROR: mallocing %d bytes during add_dt_entry_to_list.\n", 
                par.prog, nListSize*sizeof(lookup_table));
        perror("malloc");
        return FALSE;
    }

    /* copy the contents of the old list to the new list */
    memcpy(pTemp, entry, nentry * sizeof(lookup_table));

    /* free the old list */
    free(entry);

    /* make the list pointer point at the new list */
    entry = pTemp;
  }

  /* create new entry for dt, and add it to the list */
  memset(&entry[nentry], 0, sizeof(lookup_table));
  entry[nentry].unit   = dt->unit;
  entry[nentry].stream = dt->stream;
  entry[nentry].chan   = dt->chan;
  entry[nentry].tLastSample = 0.0;
  entry[nentry].bFromSCNLFile = FALSE;

  /* makeup the pin and SCNL using reftek convention */
  entry[nentry].pinno = -1;
  sprintf(entry[nentry].sname, "%04X", dt->unit   );
  sprintf(entry[nentry].cname, "C%02d", dt->chan  );
  sprintf(entry[nentry].nname, "S%1d", dt->stream );
  sprintf(entry[nentry].lname,  LOC_NULL_STRING   );

  nentry++;

  /* resort the list */
  qsort(entry, nentry, sizeof(lookup_table),
        compare_lookup_table_entries);

  /* done */
  return(TRUE);
}


/* 
   get_dt_index()
     looks up a dt entry in the lookup table list.
     Returns the index number of the entry via pIndex.
     Return code indicates status of the operation.
   return codes:
   0  :  SUCCESS
   1  :  Unlisted DT
  -1  :  Unexpected ERROR
  -2  :  Invalid Pointer!
 *****************************/
int get_dt_index(struct reftek_dt *dt, int * pIndex)
{
  lookup_table key;
  lookup_table * plte;

  /* set the index to -1, just so there's no confusion */
  *pIndex = -1;

  /* validate the pointer */
  if(!(dt && pIndex))
  {
    /* invalid pointer */
    return(-2);
  }

  /* copy the dt info over to our lookup_table key */
  key.unit   = dt->unit;
  key.stream = dt->stream;
  key.chan   = dt->chan;

  /* Find the dt entry in the table.  Return the index. */
  plte = bsearch(&key, entry, nentry, sizeof(lookup_table), 
                 compare_lookup_table_entries);


  if(plte)
  {
    /* SUCCESS: we found the dt in the list, calculate the index */
    *pIndex = (plte - entry);
    return(0);
  }
  else
  {
    /* we didn't find the dt in the list. Add it? */
    if(par.SendUnknownChan)
    {
      /* add the entry */
      if(add_dt_entry_to_list(dt))
      {
        /* now that the entry's added, we still have to
           find it in the list.  (since the list is sorted)
         ********************************************/
        get_dt_index(dt,pIndex);  /* danger - recursion */
        if(*pIndex >= 0)
        {
          /* success */
          return(0);
        }
        else
        {
          logit("et", "dt (%04X S%d C%d), successfully added to the list, but "
                  "then not found in list by get_dt_index().\n",
                  dt->unit, dt->stream, dt->chan);
          return(-1);
        }
      }  /* end if(add_dt_entry_to_list() ) */
      else  /* add_dt_entry_to_list() failed */
      {
        /* error, probably in malloc, return -1 */
        return(-1);
      }  /* end else(add_dt_entry_to_list() ) */
    }  /* end if par.SendUnknownChan */
    else
    {
      /* entry not found */
      return(1);
    }  /* end else par.SendUnknownChan */
  }  /* end else we didn't find the entry in list */
}  /* end get_dt_index() */
    


VOID load_scnlp(TracePacket *trace, int iEntry)
{

  strcpy(trace->trh2.sta,  entry[iEntry].sname);
  strcpy(trace->trh2.chan, entry[iEntry].cname);
  strcpy(trace->trh2.net,  entry[iEntry].nname);
  strcpy(trace->trh2.loc,  entry[iEntry].lname);
  trace->trh2.pinno = entry[iEntry].pinno;
  return;

}  /* load_scnlp() */


VOID LogSCNLFile()
{
int i;

    logit("t", "  Unit Id    Stream    Chan     Sta   Chan   Net   Loc   Pinno  NomSampRate\n");

    for (i = 0; i < nentry; i++) {
        logit("t", "    %04X %8d %8d %9s %4s  %4s  %4s %8ld",
            entry[i].unit,
            entry[i].stream+1,
            entry[i].chan+1,
            entry[i].sname,
            entry[i].cname,
            entry[i].nname,
            entry[i].lname,
            entry[i].pinno
        );
        if( entry[i].bSampRateSet ) logit(""," %10.2f\n", entry[i].samprate);
        else                        logit("","\n");
    }
}


double GetLastSampleTime(int iEntryIndex)
{
  if(iEntryIndex >= 0 && iEntryIndex < nentry)
    return(entry[iEntryIndex].tLastSample);
  else
    return(-1.0);
}

BOOL   SetLastSampleTime(int iEntryIndex, double dSampleTime)
{
  if(iEntryIndex >= 0 && iEntryIndex < nentry)
  {
    if(dSampleTime > 0.0)
    {
      entry[iEntryIndex].tLastSample = dSampleTime;
      return(TRUE);
    }
  }
  return(FALSE);
}


/* Store the nominal sample rate for the channel */
void SetNomSampleRate(int iEntryIndex, double dSampRate)
{
  if(iEntryIndex >= 0 && iEntryIndex < nentry)
  {
  /* The rate was set already */
     if(entry[iEntryIndex].bSampRateSet)  return;

  /* Set rate if it's valid */
     if(par.GuessNominalSampleRate)  /* we're supposed to guess the rate */
     {
     /* it's not acceptable; skip it */
        int ir;
        if( !SampleRateIsAcceptable(dSampRate, &ir) )   
        {
           logit("et","Channel %s %s %s %s calculated sample rate (%.2f) "
                 "not acceptable\n", 
                  entry[iEntryIndex].sname, entry[iEntryIndex].cname, 
                  entry[iEntryIndex].nname, entry[iEntryIndex].lname,
                  dSampRate );
           entry[iEntryIndex].samprate = 0.0;
           return;
        }

     /* it IS acceptable; store it */
        entry[iEntryIndex].samprate = par.AcceptableSampleRates[ir]; 
        entry[iEntryIndex].bSampRateSet = TRUE;
        logit("et","Setting nominal sample rate for (%s %s %s %s) to %6.2f\n",
               entry[iEntryIndex].sname, entry[iEntryIndex].cname, 
               entry[iEntryIndex].nname, entry[iEntryIndex].lname,
               entry[iEntryIndex].samprate);
        return;
     }
  }
  else
  {
  logit("et","Bad scnl line calculated %d",iEntryIndex);
}
  return; /* invalid iEntryIndex */
}

/* Get the nominal sample rate for the channel */
double GetNomSampleRate(int iEntryIndex)
{
  if(iEntryIndex >= 0 && iEntryIndex < nentry)
  {
     if(entry[iEntryIndex].bSampRateSet)  return(entry[iEntryIndex].samprate);
  }
  return(-1.0); /* invalid iEntryIndex or rate not set */
}

/* express ALLOWABLE SAMPLE RATE SLOP */
#define ALLOWABLE_SAMPLE_RATE_SLOP  0.2  /* samples per second (not percent!) */


BOOL SampleRateIsAcceptable(double dSampRate, int *irate)
{
  int i;
  for(i=0; i < par.iNumAcceptableSampleRates; i++)
  {
    if( dSampRate >= (par.AcceptableSampleRates[i] - ALLOWABLE_SAMPLE_RATE_SLOP) &&
        dSampRate <= (par.AcceptableSampleRates[i] + ALLOWABLE_SAMPLE_RATE_SLOP)    ) 
    {
      *irate = i;
       return(TRUE);
    }
  }
  *irate = -1;
  return(FALSE);
}


/* SampleRateIsValid() is an attempt by Davidk and LDD to
   determine the nominal sample rate for a channel, based
   on the data currently available to send_wav(), and to
   compare the calculated samplerate (from RefTek get_samprate()
   function, to the nominal.
   It is estimated that the Davidk/LDD method for calculating
   sample rate will provide better and more robust results
   than the get_samprate() function, especially around 
   time discontinuities in the data stream.
   DK 06/09/2004
   **************************************************/

/* SampleRateIsValid() return codes:
    0 : Valid (compared to rate stored for this SCNL)
    1 : Rate for channel not yet set
   -1 : Invalid (compared to rate stored for this SCNL)
 ******************************************************/
INT32 SampleRateIsValid(int iEntryIndex, double dSampRate)
{
  if(iEntryIndex >= 0 && iEntryIndex < nentry)
  {

 /* Sample rate has been stored already 
  *************************************/
    if(entry[iEntryIndex].bSampRateSet==TRUE) 
    {
      if( dSampRate >= (entry[iEntryIndex].samprate - ALLOWABLE_SAMPLE_RATE_SLOP) &&
          dSampRate <= (entry[iEntryIndex].samprate + ALLOWABLE_SAMPLE_RATE_SLOP)    )
      {
        return(0);   /* matches stored rate */
      }
      else
      {
        return(-1);  /* does not match stored rate */
      }
    }

 /* Sample rate has not been stored yet 
  *************************************/
    else  
    {
       return(1);    /* valid rate not stored yet */
    }
  }

  return(1);         /* invalid index into lookup table */

}  /* end SampleRateIsValid() */

