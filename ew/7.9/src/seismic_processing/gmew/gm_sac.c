/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_sac.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_sac.c: a collection of SAC-related functions for gmew.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <earthworm.h>
#include <swap.h>
#include <time_ew.h>
#include "gm.h"
#include "gm_sac.h"
#include "../localmag/lm_misc.h"
#include "../localmag/sachead.h"

/* Some file-global variables */
static struct SAChead gSAChd;   /* The global SAC header */
static float *gSACData;  /* the SAC data buffer */
static long gBufLen;
static char gSACOutDir[PATH_MAX];  /* Where SAC output files go */
static int gfSACSaveInit = 0;


/* Internal Function Prototypes */
static void sacInitHdr( struct SAChead *);
static void cpystrn(char *, const char *, int );


/*
 * initSACBuf: allocate the gSACData array; to be called at program startup
 *   Returns: 0 on success
 *           -1 on memory error
 */
int initSACBuf( long reqLen )
{
  if ( (gSACData = (float *)malloc( reqLen * sizeof(float))) == (float *)NULL)
    return -1;

  gBufLen = reqLen;
  return 0;
}

/*
 * initSACSave: initialize the SACSave mechanism. This involves setting the
 *              SAC saving directory path and creating the necessary 
 *              directories in that path. The directory path consists of a
 *              fixed base part, plus a formatted part derived from the
 *              event origin time and event ID. The format string is that
 *              used by strftime, with the addition of `%i' for the event ID
 *              string.
 *   Returns: nothing; since saving SAC files is not gmew's primary
 *              mission, we don't care if this routine fails. A logit()
 *              entry is sufficient notification of the problem.
 */
void initSACSave(EVENT *pEvt, GMPARAMS *pgmParams)
{
  struct tm tms;
  time_t ot;
  char format[GM_MAXTXT];
  char *f, *s;
  
  if (gfSACSaveInit == 1)
  {
    logit("et", 
          "initSACSave called when initialized; call termSACSave first\n");
    return;   /* Don't initialize twice */
  }

  gfSACSaveInit = -1;
  
  /* First do the base directory */
  strcpy(gSACOutDir, pgmParams->sacOutDir);
  if ( CreateDir(gSACOutDir) == EW_FAILURE)
    return;
  strcat(gSACOutDir, "/");
  
  /* Now add the formatted part of the directory path */
  if ( pgmParams->saveDirFormat != (char *)NULL)
  {
    memset(format, 0, GM_MAXTXT);
    s = pgmParams->saveDirFormat;
    f = format;
    while ( *s != 0 )
    {
      if (*s == '%')
      {
        if (*++s == 'i') 
        {     /* Event ID */
          strcat(f, pEvt->eventId);
          f += strlen(pEvt->eventId);
        }
        else
        {
          *f++ = '%';
          *f++ = *s;
        }
        s++;
      }
      else
      {
        *f++ = *s++;
      }
    }
    s = gSACOutDir + strlen(gSACOutDir);
    ot = (time_t )pEvt->origin_time;
    gmtime_ew(&ot, &tms);
    if ( strftime(s, PATH_MAX - strlen(gSACOutDir), format, &tms) == 0)
    {
      logit("et", "initSACSave: strftime failed parsing format <%s>\n",
            format);
      return;
    }
    if ( CreateDir(gSACOutDir) == EW_FAILURE)
      return;
    strcat(gSACOutDir, "/");
  }

  /* Set our flag to say we're initialized */
  gfSACSaveInit = 1;
  return;
}

/*
 * saveSACGMTraces: write the synthesized ground motion traces to SAC files.
 *                 The SAC directory must be named and created beforehand
 *                 by calling initSACSave. The SAC header is filled in with
 *                 as much information as we know about the event and SCN.
 *                 The peak amplitudes and times are set in the header.
 *                 This function returns no status, on the grounds that saving
 *                 trace data is not the primary function of gmew.
 *                 If saving fails, gmew will continue on unimpeded.
 */
void saveSACGMTraces( DATABUF *pTrace, STA *pSta, COMP3 *pComp, EVENT *pEvt,
                    GMPARAMS *pgmParams, SM_INFO *pSms)
{
  time_t ltime;
  struct tm *pTm;
  double dmin, dmax, dmean, *pData;
  FILE *fp;
  char filename[PATH_MAX];
  int len, j, npts;
  long i;
  struct SAChead2 *psh;

  /* Make sure we are prepared for action */
  if (gfSACSaveInit == 0)
  {
    logit("et", "saveSACGMTraces called while SACSave not initialized\n");
    return;
  }
  else if (gfSACSaveInit != 1)
    return;  /* Init failed; don't bother doing anything more */
  
  /* Set up a new header for each SCN; add event info */
  sacInitHdr( &gSAChd );
  
  /* Some standard values */
  gSAChd.idep  = SAC_IUNKN;     /* unknown independent data type */
  gSAChd.iztype = SAC_IO;       /* Reference time is Origin time */
  gSAChd.iftype = SAC_ITIME;    /* File type is time series */
  gSAChd.leven  = 1;            /* evenly spaced data */
  
    /* Event values */
  gSAChd.evla=(float)(pEvt->lat);
  gSAChd.evlo=(float)(pEvt->lon);
  gSAChd.evdp=(float)(pEvt->depth);
    
  /* Reference time: this is set to Event origin time */
  ltime = (time_t)pEvt->origin_time;
  /* gmttime makes juldays starting with 0 */
  pTm = gmtime( &ltime );
  gSAChd.nzyear = (int32_t)pTm->tm_year + (int32_t)1900;
  gSAChd.nzjday = (int32_t)pTm->tm_yday + (int32_t)1; /* julian day, 0 - 365 */
  gSAChd.nzhour = (int32_t)pTm->tm_hour;
  gSAChd.nzmin  = (int32_t)pTm->tm_min;
  gSAChd.nzsec  = (int32_t)pTm->tm_sec;
  gSAChd.nzmsec = (int32_t)( (pEvt->origin_time - (int32_t)pEvt->origin_time) *
                          1000.0);
  
  /* set the origin time */
  gSAChd.o      = 0.0;
  cpystrn(gSAChd.ko, "origin", K_LEN);

  /* SCN strings; copy in characters; don't use *
     * strncpy since SAC does not NULL-terminate. */
  cpystrn(gSAChd.kstnm, pSta->sta, K_LEN);
  cpystrn(gSAChd.kcmpnm, pComp->name, K_LEN);
  cpystrn(gSAChd.knetwk, pSta->net, K_LEN);
  cpystrn(gSAChd.khole, pComp->loc, K_LEN);
    
  /* orientation of seismometer -
       determine the orientation based on the third character
       of the component name */
  switch ((int) pComp->name[2]) 
  {
    /* vertical component */
  case 'Z' :
  case 'z' :
    gSAChd.cmpaz = 0;
    gSAChd.cmpinc = 0;
    break;
    /* north-south component */
  case 'N' :
  case 'n' :
    gSAChd.cmpaz = 0;
    gSAChd.cmpinc = 90;
    break;
    /* east-west component */
  case 'E' :
  case 'e' :
    gSAChd.cmpaz = 90;
    gSAChd.cmpinc = 90;
    break;
    /* anything else */
  default :
    gSAChd.cmpaz = SACUNDEF;
    gSAChd.cmpinc = SACUNDEF;
    break;
  } /* switch */
  
  gSAChd.stla = (float)pSta->lat;
  gSAChd.stlo = (float)pSta->lon;
  gSAChd.dist = (float)pSta->dist;

  gSAChd.b = (float)(pTrace->starttime - pEvt->origin_time);
  gSAChd.delta = (float)pTrace->delta;

  memset(filename, 0, PATH_MAX);
  strcpy(filename, gSACOutDir);
  len = (int)strlen(filename);
  if (fmtFilename(pSta->sta, pComp->name, pSta->net, pComp->loc, filename, 
                  PATH_MAX - len, pgmParams->saveNameFormat) < 0)
  {
    logit("et", "saveSACGMTraces: error formating SACSaveFile <%s>\n", 
          filename);
    return;
  }
  len = (int)strlen(filename);

  /* Make one file for each find of trace */
  for (j = 0; j < 6; j++)
  {
    switch (j)
    {   /* These cases are based on values set in makeGM and gma */
    case 0:  /* Acceleration */
      strcpy(&filename[len], "-acc");
      gSAChd.e = (float)(pTrace->endtime - pEvt->origin_time);
      gSAChd.npts = pTrace->nProc;
      cpystrn(gSAChd.kinst, "cm/sec^2", K_LEN);
      if (pSms->tpga > 0.0)
      {
        gSAChd.t0 = (float)(pSms->tpga - pEvt->origin_time);
        gSAChd.user0 = (float)pSms->pga;
        cpystrn(gSAChd.kt0, "Acc_max", K_LEN);
        cpystrn(gSAChd.kuser0, "Acc_amp", K_LEN);
      }
      break;
    case 1:  /* Velocity */
      strcpy(&filename[len], "-vel");
      gSAChd.e = (float)(pTrace->endtime - pEvt->origin_time);
      gSAChd.npts = pTrace->nProc;
      cpystrn(gSAChd.kinst, "cm/sec", K_LEN);
      if (pSms->tpgv > 0.0)
      {
        gSAChd.t0 = (float)(pSms->tpgv - pEvt->origin_time);
        gSAChd.user0 = (float)pSms->pgv;
        cpystrn(gSAChd.kt0, "Vel_max", K_LEN);
        cpystrn(gSAChd.kuser0, "Vel_amp", K_LEN);
      }
      break;
    case 2:   /* Displacement */
      strcpy(&filename[len], "-disp");
      gSAChd.e = (float)(pTrace->endtime - pEvt->origin_time);
      gSAChd.npts = pTrace->nProc;
      cpystrn(gSAChd.kinst, "cm", K_LEN);
      if (pSms->tpgd > 0.0)
      {
        gSAChd.t0 = (float)(pSms->tpgd - pEvt->origin_time);
        gSAChd.user0 = (float)pSms->pgd;
        cpystrn(gSAChd.kt0, "Disp_max", K_LEN);
        cpystrn(gSAChd.kuser0, "Disp_amp", K_LEN);
      }
      break;
    case 3:  /* Spectral Response at 0.3 sec */
      strcpy(&filename[len], "-psa03");
      gSAChd.e = (float)(pTrace->endtime + pTrace->delta * pTrace->padLen
                         - pEvt->origin_time);
      gSAChd.npts = pTrace->nProc + pTrace->padLen;
      cpystrn(gSAChd.kinst, "cm/sec^2", K_LEN);
      if (pSms->rsa[j-3] > 0.0)
      {
        gSAChd.t0 = (float)(pComp->RSAPeakTime[j-3] - pEvt->origin_time);
        gSAChd.user0 = (float)pSms->rsa[j-3];
        cpystrn(gSAChd.kuser0, "sr03_amp", K_LEN);
        cpystrn(gSAChd.kt0, "sr03_max", K_LEN);
      }
      break;
    case 4:
      strcpy(&filename[len], "-psa10");
      gSAChd.e = (float)(pTrace->endtime + pTrace->delta * pTrace->padLen
                         - pEvt->origin_time);
      gSAChd.npts = pTrace->nProc + pTrace->padLen;
      cpystrn(gSAChd.kinst, "cm/sec^2", K_LEN);
      if (pSms->rsa[j-3] > 0.0)
      {
        gSAChd.t0 = (float)(pComp->RSAPeakTime[j-3] - pEvt->origin_time);
        gSAChd.user0 = (float)pSms->rsa[j-3];
        cpystrn(gSAChd.kuser0, "sr10_amp", K_LEN);
        cpystrn(gSAChd.kt0, "sr10_max", K_LEN);
      }
      break;
    case 5:
      strcpy(&filename[len], "-psa30");
      gSAChd.e = (float)(pTrace->endtime + pTrace->delta * pTrace->padLen
                         - pEvt->origin_time);
      gSAChd.npts = pTrace->nProc + pTrace->padLen;
      cpystrn(gSAChd.kinst, "cm/sec^2", K_LEN);
      if (pSms->rsa[j-3] > 0.0)
      {
        gSAChd.t0 = (float)(pComp->RSAPeakTime[j-3] - pEvt->origin_time);
        gSAChd.user0 = (float)pSms->rsa[j-3];
        cpystrn(gSAChd.kuser0, "sr30_amp", K_LEN);
        cpystrn(gSAChd.kt0, "sr30_max", K_LEN);
      }
      break;
    }
    
    if ( (fp = fopen(filename, "wb")) == (FILE *)NULL)
    {
      logit("et", "saveSACGMTraces: error opening <%s>: %s\n", filename,
            strerror(errno));
      return;
    }
  
    /* Copy the data into the SAC data buffer */
    pData = &(pTrace->procData[j * pTrace->lenProc]);
    dmax = dmin = *pData;
    dmean = 0.0;
    npts = gSAChd.npts;
    for (i = 0; i < npts; i++)
    {
      dmean += *pData;
      dmin = (dmin < *pData) ? dmin : *pData;
      dmax = (dmax > *pData) ? dmax : *pData;
      gSACData[i] = (float) *pData;
      pData++;
    }
    dmean /= (double)npts;
    gSAChd.depmin = (float)dmin;
    gSAChd.depmax = (float)dmax;
    gSAChd.depmen = (float)dmean;
    
  
    /* SAC files are always in "_SPARC" byte order; swap if necessary */
#ifdef _INTEL
    psh = (struct SAChead2 *)&gSAChd;
    for (i = 0; i < NUM_FLOAT; i++)
      SwapInt32( (int32_t *) &(psh->SACfloat[i]));
    for (i = 0; i < MAXINT; i++)
      SwapInt32( (int32_t *) &(psh->SACint[i]));
    for (i = 0; i < npts; i++)
    {
      SwapInt32( (int32_t *) &(gSACData)[i] );
    }
#endif

    if (fwrite( &gSAChd, sizeof(gSAChd), 1, fp) != 1)
    {
      logit("et", "saveSACGMTraces: error writing SAC header: %s\n", strerror(errno));
      fclose(fp);
      return;
    }
    if (fwrite( gSACData, sizeof(float), npts, fp) != (size_t) npts)
    {
      logit("et", "saveSACGMTraces: error writing SAC data: %s\n", 
            strerror(errno));
    }
    fclose(fp);

#ifdef _INTEL
    /* Swap the header back to local, so we don't have to fill in so much */
    psh = (struct SAChead2 *)&gSAChd;
    for (i = 0; i < NUM_FLOAT; i++)
      SwapInt32( (int32_t *) &(psh->SACfloat[i]));
    for (i = 0; i < MAXINT; i++)
      SwapInt32( (int32_t *) &(psh->SACint[i]));
#endif
  }

  return;
}

/*
 * termSACSave: Finish up what needs to be done for saving an event in SAC.
 *              Currently there isn't anything significant, but we could
 *              add things like writing some SAC macros.
 */
void termSACSave(EVENT *pEvt, GMPARAMS *pgmParams)
{
  /* Not much to do here; just enough to keep us honest */
  gfSACSaveInit = 0;
  return;
}

/*
 * sacInitHdr: initialize the SAC header. Fill it with the SAC null values
 *             and then set the SAC ID and version numbers.
 */
static void sacInitHdr( struct SAChead *head)
{
  int i;
  struct SAChead2 *head2;   /* use a simple structure here - we don't care what
			     * the variables are - set them to 'undefined' */

  /* change to a simpler format */
  head2 = (struct SAChead2 *) head;

  /*	set all of the floats to 'undefined'	*/
  for (i = 0; i < NUM_FLOAT; ++i) head2->SACfloat[i] = SACUNDEF;
  /*	set all of the ints to 'undefined'	*/
  for (i = 0; i < MAXINT-5; ++i) head2->SACint[i] = SACUNDEF;
  /*	except for the logical integers - set them to 1 */
  for ( ; i < MAXINT; ++i) head2->SACint[i] = 1;
  /*	set all of the strings to 'undefined'	*/
  for (i = 0; i < MAXSTRING; ++i) (void) strncpy(head2->SACstring[i],
						 SACSTRUNDEF,K_LEN);

  /*	SAC I.D. number */
  head2->SACfloat[9] = SAC_I_D;
  /*	header version number */
  head2->SACint[6] = SACVERSION;

  return;	/* done */
}

/*
 * cpystrn: copy string src to dest; do not NULL-terminate, but
 *          pad with space chars to length len.
 */
static void cpystrn(char *dest, const char *src, int len)
{
  int ilen;
  const char *s;
  char *d;

  s = src;
  d = dest;
  ilen = 0;
  while( *s != 0 && ilen < len )
  {
    *d++ = *s++;
    ilen++;
  }
  while (ilen < K_LEN)
  {
    *d++ = ' ';
    ilen++;
  }
  return;
}
