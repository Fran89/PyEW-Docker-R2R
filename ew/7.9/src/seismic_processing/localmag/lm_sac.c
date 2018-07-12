/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_sac.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.16  2010/03/15 16:22:50  paulf
 *     merged from Matteos branch for adding version to directory naming
 *
 *     Revision 1.15.2.1  2010/03/15 15:15:56  quintiliani
 *     Changed output file and directory name
 *     Detailed description in ticket 22 from
 *     http://bigboy.isti.com/trac/earthworm/ticket/22
 *
 *     Revision 1.15  2008/12/01 16:23:31  paulf
 *     fixed sac input files reading as per found by Claudio Satriano
 *
 *     Revision 1.14  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.13  2002/03/17 18:27:11  lombard
 *     Modified some function calls to support new argument in
 *        traceEndTime and setTaperTime, which now need the EVENT structure.
 *
 *     Revision 1.12  2002/02/16 18:37:49  lombard
 *     Added check to make sure SAC data would fit in buffer
 *     Corrected some debug logging under Solaris.
 *
 *     Revision 1.11  2002/01/24 19:34:09  lombard
 *     Added 5 percent cosine taper in time domain to both ends
 *     of trace data. This is to eliminate `wrap-around' spikes from
 *     the pre-event noise-check window.
 *
 *     Revision 1.10  2002/01/15 21:23:03  lucky
 *     *** empty log message ***
 *
 *     Revision 1.9  2001/06/21 21:22:22  lucky
 *     Modified the code to support the new amp pick format: there can be one
 *     or two picks, each consisting of time, amplitude, and period. This is
 *     reflected in the new TYPE_MAGNITUDE message, as well as the SAC header
 *     fields that get filled in.
 *     Also, the labels for SAC fields were shortened to comply with the K_LEN
 *     limitation from sachead.h
 *
 *     Revision 1.8  2001/06/10 21:24:30  lombard
 *     Added test to ensure that desired time window is in the SAC file.
 *
 *     Revision 1.7  2001/05/31 17:27:11  lucky
 *     Fixed checkSACEvent -- it improperly reported shifts in Evt longitude
 *     and depth even when there were none.
 *
 *     Revision 1.6  2001/04/11 21:07:08  lombard
 *     "site.?" renamed to "lm_site.?" for clarity.
 *
 *     Revision 1.5  2001/03/01 05:25:44  lombard
 *     changed FFT package to fft99; fixed bugs in handling of SCNPars;
 *     changed output to Magnitude message using rw_mag.c
 *
 *     Revision 1.4  2001/01/15 03:55:55  lombard
 *     bug fixes, change of main loop, addition of stacker thread;
 *     moved fft_prep, transfer and sing to libsrc/util.
 *
 *     Revision 1.3  2000/12/31 17:27:25  lombard
 *     More bug fixes and cleanup.
 *
 *     Revision 1.2  2000/12/25 22:14:39  lombard
 *     bug fixes and development
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_sac.c: a collection of SAC-related functions for localmag.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WINNT
#include <dirent.h>
#include <unistd.h>
#endif
#include <math.h>
#include <errno.h>
#include <earthworm.h>
#include <chron3.h>
#include <time.h>
#include <swap.h>
#include <time_ew.h>
#include "lm.h"
#include "lm_config.h"
#include "lm_misc.h"
#include "lm_sac.h"
#include "sachead.h"
#include "lm_site.h"   /* for strib() */

/* Some file-global variables */
#ifdef _WINNT
static HANDLE gSACFind;
#else
static DIR *gpSACDir = (DIR *)NULL;
#endif
static char gSACfile[LM_MAXTXT+1];
static struct SAChead gSAChd;   /* The global SAC header */
static float *gSACData;  /* the SAC data buffer */
static long gBufLen;
static char gSACOutDir[PATH_MAX];  /* Where SAC output files go */
static int gfSACSaveInit = 0;


/* Internal Function Prototypes */
static int openSACDir(char *);
static int nextSACFile( FILE **, LMPARAMS *);
static void closeSACDir( void );
static int readSACHdr(FILE *);
static double sacRefTime( struct SAChead * );
static int getSACEvent(EVENT *);
static int checkSACEvent(EVENT *);
static int getSACSCNL( EVENT *, LMPARAMS *, STA **, COMP3 ** );
static int getSACTrace( STA *, COMP3 *, LMPARAMS *, EVENT *, DATABUF *, 
                        FILE *);
static void sacInitHdr( struct SAChead *);
static void cpystrn(char *, const char *, int );
static int getAmpFromSACHdr(COMP3 *, EVENT *);


/*
 * getAmpFromSAC: obtain the peak Wood-Anderson amplitudes from a 
 *                directory of SAC files. This will either read the
 *                amplitudes from the header, or read trace data.
 *                if the trace data is raw, a Wood-Anderson trace
 *                will be synthesized; otherwise the W-A trace is
 *                read from the SAC file. Peak amplitudes will be
 *                measured from W-A trace data. Newly synthesized
 *                Wood-Anderson data will optionally be written
 *                to a new file.
 *   Returns: 0 on success
 *           -1 on fatal errors
 */
int getAmpFromSAC(EVENT *pEvt, LMPARAMS *plmParams, DATABUF *pTrace)
{
  FILE *pSACfile;
  int newEvent = 1;
  int rc;
  STA *pSta  ;
  COMP3 *pComp;

  /* Try to open the SAC input directory */
  if (openSACDir(plmParams->sacInDir) < 0)
    return -1; /* don't complain; openSACDir already did */
  pSACfile = (FILE *)NULL;

  /* Loop through all the likely SAC files in that directory */
  while (nextSACFile( &pSACfile, plmParams ) > 0)
  {
    /* Make sure nextSACFile actually opened the file */
    if (pSACfile == (FILE *)NULL) 
      goto EndLoop;

    if ( readSACHdr(pSACfile) < 0)
      goto EndLoop;
    
    /* Examine the event data from the SAC header if needed */
    if (plmParams->eventSource == LM_ES_SAC)
    {
      if (newEvent)
      {
        if ( getSACEvent(pEvt) < 0)
          goto Abort;

        newEvent = 0;
        if (plmParams->debug)
        {
          char date[22];
          date20(pEvt->origin_time + GSEC1970, date);
          logit("", "SAC event id: <%s> origin: %s\n", pEvt->eventId, date);
          logit("", "\tlat: %10.6lf lon: %10.6lf depth: %6.2lf\n", pEvt->lat,
                pEvt->lon, pEvt->depth);
        }
      }
      else
      {   /* Make sure event data is consistent across all SAC files */
        if ( (rc = checkSACEvent(pEvt)) != 0)
            goto EndLoop;
      }
    }  /* if eventSource == LM_ES_SAC */

    /* Get the SCNL SAC data and see if we want this one */
    if ( (rc = getSACSCNL(pEvt, plmParams, &pSta, &pComp)) < 0)
      goto Abort;
    else if (rc > 0)
      goto EndLoop;

    if (plmParams->debug)
      logit("", "Processing <%s.%s.%s.%s>, numSta is %d\n", 
				pSta->sta, pComp->name, pSta->net, pComp->loc, pEvt->numSta);
    
    if (plmParams->fGetAmpFromSource == 1)
    {
      if ( getAmpFromSACHdr(pComp, pEvt) != 0)
        goto EndLoop;
    }
    else      
    {
      /* This SCN is wanted, so get its trace data */
      EstPhaseArrivals(pSta, pEvt, plmParams->debug & LM_DBG_TIME);
      if ( getSACTrace( pSta, pComp, plmParams, pEvt, pTrace, pSACfile) != 0 )
        goto EndLoop;

      prepTrace(pTrace, pSta, pComp, plmParams, pEvt, plmParams->fWAsource);

      if (plmParams->fWAsource == 0)
      {   /* Now we have a generic trace; turn it into a Wood-Anderson trace */
        rc = makeWA( pSta, pComp, plmParams, pEvt );
        if ( rc > 0)
          goto EndLoop;
        else if (rc < 0)
          return -1;
      }
    
      /* Get the peak-to-peak and zero-to-peak amplitudes */
      getPeakAmp( pTrace, pComp, pSta, plmParams, pEvt);
      
      if (plmParams->saveTrace != LM_ST_NO)
        saveWATrace( pTrace, pSta, pComp, pEvt, plmParams);

    }
    
  EndLoop:
    if (pSACfile != (FILE *)NULL)
    {
      fclose(pSACfile);
      pSACfile = (FILE *)NULL;
    }
  }  /* End of loop over SAC files in sacDir */
  
  return 0;
  
  /* Fatal error: clean up and go home */
 Abort:
  if (pSACfile != (FILE *)NULL)
  {
    fclose(pSACfile);
    memset(gSACfile, 0, LM_MAXTXT);
  }
  closeSACDir();
  return -1;

}

#ifdef _WINNT
static int openSACDir( char *sacDir )
{
  DWORD attr;
  size_t len;
  
  /* We can't end the directory string with the path separator here */
  len = strlen(sacDir);
  if (sacDir[len-1] == '\\' || sacDir[len-1] == '/')
  {
    len--;
    sacDir[len] = '\0';
  }
  
  if ( (attr = GetFileAttributes(sacDir)) == -1)
  {
    logit("", "openSACDir: error reading sacDir <%s>\n", sacDir);
    return -1;
  }
  if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
  {
    logit("", "openSACDir: <%s> is not a directory\n", sacDir);
    return -1;
  }
  
  /* Reset the FIND handle */
  gSACFind = INVALID_HANDLE_VALUE;
  return 0;
}

static int nextSACFile( FILE **ppFile, LMPARAMS *plmParams)
{
  static WIN32_FIND_DATA FFD;
  static char *sf;
  static char pathName[4096];
  DWORD error, attr;

  while (*ppFile == (FILE *)NULL)
  {
    if (gSACFind == INVALID_HANDLE_VALUE)
    {   /* this is the first time searching this directory */
      strcpy(pathName, plmParams->sacInDir);
      strcat(pathName, "\\*");
      sf = pathName + strlen(pathName) - 1;
      
      if ( (gSACFind = FindFirstFile( pathName, &FFD)) ==
           INVALID_HANDLE_VALUE)
      {
        logit("", "nextSACFile: FindFirstFile (%s) error\n", 
              plmParams->sacInDir);
        return -1;
      }
    }
    else
    {   /* we've been in this directory before */
      if ( FindNextFile( gSACFind, &FFD) == 0)
      {
        if ( (error = GetLastError()) == ERROR_NO_MORE_FILES)
          return 0;
        else
        {
          logit("", "nextSACFile: FindNextFile returns error %ld\n", error);
          return -1;
        }
      }
    }
  
    /* Copy the new filename onto the end of pathName */
    strcpy( sf, FFD.cFileName);
    if (plmParams->debug & LM_DBG_SAC)
      logit("", "nextSACFile trying file: <%s>\n", pathName);
    if ( (attr = GetFileAttributes(pathName)) == -1)
      {
        logit("", "nextSACFile: error reading attributes of <%s>\n", pathName);
        continue;
      }
    if ((attr & (FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_READONLY )) == 0)
      continue;
    
    if (isMatch(FFD.cFileName, plmParams->sourceNameFormat))
    {
      if (plmParams->debug & LM_DBG_SAC)
        logit("", "nextSACFile: found match\n");
      
      strncpy(gSACfile, FFD.cFileName, LM_MAXTXT);
      if ( (*ppFile = fopen(pathName, "rb")) == (FILE *)NULL)
      {
        logit("", "nextSACFile: error opening <%s>\n", pathName);
      }
    }
    else if (plmParams->debug & LM_DBG_SAC)
      logit("", "nextSACFile: no match\n");
  }
  return 1;
}

static void closeSACDir(void)
{
  (void)FindClose(gSACFind);
  return;
}

  

#else  /* NOT WINDOZ NT */
/*
 * openSACDir: open the SAC directory for reading. If it is already open,
 *             then rewind it.
 *   Returns: 0 on success
 *           -1 on error
 */
static int openSACDir(char *sacDir)
{
  if (gpSACDir == NULL)
  {
    gpSACDir = opendir(sacDir);
    if (gpSACDir == (DIR *)NULL)
    {
      logit("", "openSACDir: error opening <%s>: %s\n", sacDir,
            strerror(errno));
      return -1;
    }
  }
  else
    rewinddir( gpSACDir );
  
  return 0;
}

/*
 * nextSACFile: get the next file in the SAC input directory with name
 *              matching the format specifier in plmParams. 
 *   Returns: 1 when a matching file is found
 *            0 when no more files are found
 *           -1 on error
 */
static int nextSACFile( FILE **ppFile, LMPARAMS *plmParams )
{
  char pathname[PATH_MAX];
  char *ptr;
  struct dirent *pD;
  struct stat statbuf;

  strcpy(pathname, plmParams->sacInDir);
  ptr = pathname + strlen(pathname) - 1;  
  if (*ptr != '/' || *ptr != '\\')
  {
    *++ptr = '/';
    *++ptr = 0;
  }
  else
    ptr++;
  
  while (*ppFile == (FILE *)NULL)
  {
    if ( (pD = readdir(gpSACDir)) == (struct dirent *)NULL)
    {
      if (errno == EBADF)
      {
        logit("", "nextSACFile: bad directory descriptor\n");
        return -1;
      }
      else
        return 0;
    }
    *ptr = 0;

    if (plmParams->debug & LM_DBG_SAC)
      logit("", "nextSACFile: trying <%s%s>\n", pathname, pD->d_name);
    
    if (strcmp(pD->d_name, ".") == 0 || strcmp(pD->d_name, "..") == 0)
      continue;

    /* Add filename onto pathname */
    strcpy(ptr, pD->d_name);
    if (lstat(pathname, &statbuf) < 0)
    {
      logit("", "nextSACFile: stat error on <%s>: %s\n", pathname, 
            strerror(errno));
      continue;
    }
    if (!S_ISREG(statbuf.st_mode))
      continue;
    
    if (isMatch(pD->d_name, plmParams->sourceNameFormat))
    {
      if (plmParams->debug & LM_DBG_SAC)
        logit("", "nextSACFile: found match\n");
      
      strncpy(gSACfile, pD->d_name, LM_MAXTXT);
      if ( (*ppFile = fopen(pathname, "r")) == (FILE *)NULL)
      {
        logit("", "nextSACFile: error opening <%s>: %s\n", pathname,
              strerror(errno));
      }
    }
    else if (plmParams->debug & LM_DBG_SAC)
      logit("", "nextSACFile: no match\n");
  }
  return 1;
}

static void closeSACDir(void)
{
  closedir(gpSACDir);
  return;
}
#endif

/*
 * readSACHdr: read the header portion of a SAC file into memory.
 *  arguments: file pointer: pointer to an open file from which to read
 *             filename: pathname of open file, for logging.
 * returns: 0 on success
 *         -1 on error reading file
 *     The file is left open in all cases.
 */
static int readSACHdr(FILE *fp)
{
  int i;
  struct SAChead2 *psh;
  
  psh = (struct SAChead2 *)&gSAChd;
  
  if (fread( &gSAChd, sizeof(gSAChd), 1, fp) != 1)
  {
    logit("", "readSACHdr: error reading from <%s>: %s\n", gSACfile,
          strerror(errno));
    return -1;
  }

  /* SAC files are always in "_SPARC" byte order; swap if necessary */
#ifdef _INTEL
  for (i = 0; i < NUM_FLOAT; i++)
    SwapInt32( (int32_t *) &(psh->SACfloat[i]));
  for (i = 0; i < MAXINT; i++)
    SwapInt32( (int32_t *) &(psh->SACint[i]));
#endif
  
  return 0;
}

/*
 * getSACEvent: get the EVENT data from the SAC header, checking for 
 *              missing values. Assumes that SAC header has already been
 *              read into gSAChd and byte-swapped to local convention.
 *   Returns: 0 on success
 *           -1 on missing essential data
 */
static int getSACEvent(EVENT *pEvt)
{
  int rc = 0;
  
  if (gSAChd.nzyear == SACUNDEF || gSAChd.nzjday == SACUNDEF ||
      gSAChd.nzhour == SACUNDEF || gSAChd.nzmin == SACUNDEF ||
      gSAChd.nzsec == SACUNDEF || gSAChd.nzmsec == SACUNDEF )
  {
    logit("", "getSACEvent: reference time not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if ( gSAChd.o == (SACWORD)SACUNDEF)
  {
    logit("", "getSACEvent: origin time not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if (gSAChd.evla == (SACWORD)SACUNDEF || gSAChd.evlo == (SACWORD)SACUNDEF)
  {
    logit("", "getSACEvent: event location not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if (gSAChd.evdp == (SACWORD)SACUNDEF)
  {
    logit("", "getSACEvent: event depth not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if (rc != 0)    
    return rc;
  
  pEvt->origin_time = (double)gSAChd.o + sacRefTime(&gSAChd);
  pEvt->lat = (double)gSAChd.evla;
  pEvt->lon = (double)gSAChd.evlo;
  pEvt->depth = (double)gSAChd.evdp;
  
  return 0;
}

/*
 * checkSACEvent: check the EVENT data from one SAC header against the
 *                previously read header. If there are missing or
 *                mis-matched values, the check fails.
 *                Origin time must match to within one second;
 *                latitude and longitude must match within 0.1 degrees,
 *                and depth must match within 1 km.
 *                Assumes that SAC header has already been read into
 *                gSAChd and byte-swapped to local convention.
 *   Returns: 0 on success
 *           -1 on missing
 *           +1 on mismatched data
 */
static int checkSACEvent(EVENT *pEvt)
{
  int rc = 0;
  
  if (gSAChd.nzyear == SACUNDEF || gSAChd.nzjday == SACUNDEF ||
      gSAChd.nzhour == SACUNDEF || gSAChd.nzmin == SACUNDEF ||
      gSAChd.nzsec == SACUNDEF || gSAChd.nzmsec == SACUNDEF )
  {
    logit("", "checkSACEvent: reference time not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if ( gSAChd.o == (SACWORD)SACUNDEF)
  {
    logit("", "checkSACEvent: origin time not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if (gSAChd.evla == (SACWORD)SACUNDEF || gSAChd.evlo == (SACWORD)SACUNDEF)
  {
    logit("", "checkSACEvent: event location not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if (gSAChd.evdp == (SACWORD)SACUNDEF)
  {
    logit("", "checkSACEvent: event depth not set in SAC file <%s>\n",
          gSACfile);
    rc = -1;
  }
  if (rc == 0)    
  {
    if ( fabs(pEvt->origin_time - ((double)gSAChd.o + sacRefTime(&gSAChd))) > 1.0)
    {
      logit("", "checkSACEvent: origin_time shift in <%s> this: %f last %f\n",
            gSACfile, (double)gSAChd.o + sacRefTime(&gSAChd), 
            pEvt->origin_time);
      rc = +1;
    }
    
    if ( fabs(pEvt->lat - (double)gSAChd.evla) > 0.1) 
    {
      logit("", "checkSACEvent: evt Lat shift in <%s> this: %f last %f\n",
            gSACfile, (double)gSAChd.evla, pEvt->lat);
      rc = +1;
    }
    
    if ( fabs(pEvt->lon - (double)gSAChd.evlo) > 0.1) 
    {
      logit("", "checkSACEvent: evt Lon shift in <%s> this: %f last %f\n",
            gSACfile, (double)gSAChd.evlo, pEvt->lon);
      rc = +1;
    }
    
    if ( fabs(pEvt->depth - (double)gSAChd.evdp) > 1.01) 
    {
      logit("", "checkSACEvent: evt depth shift in <%s> this: %f last %f\n",
            gSACfile, (double)gSAChd.evdp, pEvt->depth);
      rc = +1;
    }
  }
  if (rc != 0)
    logit("", "checkSACEvent: skipping SAC file <%s>\n", gSACfile);
  
  return rc;
}
  


/*
 * sacRefTime: return SAC reference time as a double.
 *             Uses a trick of mktime() (called by timegm_ew): if tm_mday
 *             exceeds the normal range for the month, tm_mday and tm_mon
 *             get adjusted to the correct values. So while mktime() ignores
 *             tm_yday, we can still handle the julian day of the SAC header.
 *             This routine does NOT check for undefined values in the
 *             SAC header.
 *  Returns: SAC reference time as a double.
 */
static double sacRefTime( struct SAChead *pSH )
{
  struct tm tms;
  time_t sec;
  
  tms.tm_year = pSH->nzyear - 1900;
  tms.tm_mon = 0;    /* Force the month to January */
  tms.tm_mday = pSH->nzjday;  /* tm_mday is 1 - 31; nzjday is 1 - 366 */
  tms.tm_hour = pSH->nzhour;
  tms.tm_min = pSH->nzmin;
  tms.tm_sec = pSH->nzsec;
  tms.tm_isdst = 0;
  sec = timegm_ew(&tms);
  return (double)sec + pSH->nzmsec / 1000.0;
}

/*
 * getSACSCN: get SCN info from the global SAC header; checks for null values.
 *            New SCN info is added the the EVENT structure pEvt. 
 *    Returns: 0 on success
 *            -1 on error: out of memory
 *            +1 on non-fatal error or rejection
 */
static int getSACSCNL( EVENT *pEvt, LMPARAMS *plmParams, STA **ppSta, 
                      COMP3 **ppComp )
{
  COMP3 *pComp;
  int rc;
  char sta[STA_LEN+1], comp[COMP_LEN+1], net[NET_LEN+1], loc[LOC_LEN+1];
  
  *ppSta = (STA *)NULL;
  *ppComp = (COMP3 *)NULL;

  /* Can't use strcmp, since SAC header `strings' aren't null-terminated */
  if (memcmp(gSAChd.kstnm, SACSTRUNDEF, K_LEN) == 0 ||
      memcmp(gSAChd.kcmpnm, SACSTRUNDEF, K_LEN) == 0 ||
      memcmp(gSAChd.khole, SACSTRUNDEF, K_LEN) == 0 ||
      memcmp(gSAChd.knetwk, SACSTRUNDEF, K_LEN) == 0)
  {
    logit("", "getSACSCNL: missing station, comp, net, or loc name from <%s>\n",
          gSACfile);
    return +1;
  }
  
  strncpy(sta, gSAChd.kstnm, STA_LEN);
  sta[STA_LEN] = '\0';
  strib(sta);
  strncpy(comp, gSAChd.kcmpnm, COMP_LEN);
  comp[COMP_LEN] = '\0';
  strib(comp);
  strncpy(net, gSAChd.knetwk, NET_LEN);
  net[NET_LEN] = '\0';
  strib(net);
  strncpy(loc, gSAChd.khole, LOC_LEN);
  loc[LOC_LEN] = '\0';
  strib(loc);
  
  if ( (rc = addCompToEvent(sta, comp, net, loc, pEvt, plmParams, ppSta, 
                            &pComp) ) < 0)
  {    /* don't complain, addCompToEvent already did */
    return -1;
  }
  else if (rc > 0)
    return +1;      /* this SCN not selected for some reason */
  
  *ppComp = pComp;
    
  return 0;
}

/*
 * getSACTrace: read trace data from a SAC file, swap bytes to local order,
 *              and copy desired portion into Trace buffer.
 *              Assumes that the SAC file has already been opened and its
 *              header has been read in.
 *   Returns: 0 on success
 *           +1 on failure; the FILE is left open
 */  
static int getSACTrace( STA *pSta, COMP3 *pComp, LMPARAMS *plmParams, 
                        EVENT *pEvt, DATABUF *pTrace, FILE *pSACfile)
{
  double sacStart, sacEnd;
  long isamp, nsamp, i;
  
  /* Initialize the trace buffer */
  cleanTrace();

  /* See what the SAC header has to offer */
  if ( gSAChd.delta == (SACWORD)SACUNDEF || gSAChd.delta == 0.0 ||
       gSAChd.b == (SACWORD)SACUNDEF || gSAChd.e == (SACWORD)SACUNDEF ||
       gSAChd.npts == SACUNDEF )
  {
    logit("", "getSACTrace: SAC file <%s> missing delta, b, e, or npts\n", 
          gSACfile);
    return +1;
  }
  
  if (gSAChd.npts > gBufLen) 
  {
    logit("", "getSACTrace: SAC file <%s> length %d is longer than MaxTrace %d\n",
          gSACfile, gSAChd.npts, gBufLen);
    return +1;
  }

  /* Try for the configured trace start and end times */
  if (plmParams->fWAsource == 0)
    /* set up for taper if trace needs processing */
    setTaperTime(pSta, plmParams, pEvt);

  pTrace->starttime = traceStartTime(pSta, plmParams) - pSta->timeTaper;
  pTrace->endtime = traceEndTime(pSta, plmParams, pEvt) + pSta->timeTaper;

  if (gSAChd.odelta != (SACWORD)SACUNDEF)
    pTrace->delta = (double)gSAChd.odelta;
  else
    pTrace->delta = (double)gSAChd.delta;

  sacStart = (double) gSAChd.b + sacRefTime( &gSAChd );
  sacEnd = (double) gSAChd.e + sacRefTime( &gSAChd );

  if (sacStart > pTrace->starttime || sacEnd < pTrace->endtime)
  {
    logit("", "SAC file contains no data for requested period\n");
    logit("", "\trequested %lf - %lf; actual %lf - %lf\n",
          pTrace->starttime, pTrace->endtime, sacStart, sacEnd);
    return( +1 );
  }
  
  if (sacStart > pTrace->starttime)
  {
    pTrace->starttime = sacStart;
    isamp = 0;
  }
  else
  {  /* where to start reading the SAC data into the trace buffer */
    isamp = (long)( 0.5 + (pTrace->starttime - sacStart) / pTrace->delta);
  }
  
  if (sacEnd < pTrace->endtime)
  {
    pTrace->endtime = sacEnd;
    nsamp = gSAChd.npts;
  }
  else
  {
    nsamp = gSAChd.npts - (long)( (sacEnd - pTrace->endtime) / pTrace->delta);
  }
  /* If it won't fit, chop some of the end */
  if ( nsamp - isamp > pTrace->lenRaw)
  {
    pTrace->endtime -= (double)( (nsamp - isamp - pTrace->lenRaw ) * 
                                 pTrace->delta);
    nsamp = pTrace->lenRaw - isamp;
  }
  pTrace->nRaw = nsamp - isamp;
  
  /* Read the sac data into a buffer */
  if ( (i = (long)fread(gSACData, sizeof(float), gSAChd.npts, pSACfile)) !=
      (long)gSAChd.npts)
  {
    logit("", "getSACTrace: error reading (%d:%d) SAC data from <%s.%s.%s.%s>: %s\n",
          i, gSAChd.npts, pSta->sta, pComp->name, pSta->net, pComp->loc, strerror(errno));
    pTrace->nRaw = 0;
    return -1;
  }

  /* SAC files are always in "_SPARC" byte order; swap if necessary */
#ifdef _INTEL
  for (i = 0; i < gSAChd.npts; i++)
  {
    SwapInt32( (int32_t *) &(gSACData)[i] );
  }
#endif

  i = 0;
  for ( ; isamp < nsamp; isamp++)
    pTrace->rawData[i++] = (double) gSACData[isamp];
  
  return 0;
}

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
 *   Returns: nothing; since saving SAC files is not localmag's primary
 *              mission, we don't care if this routine fails. A logit()
 *              entry is sufficient notification of the problem.
 */
void initSACSave(EVENT *pEvt, LMPARAMS *plmParams)
{
  struct tm tms;
  time_t ot;
  char format[LM_MAXTXT];
  char *f, *s;
  char origin_version_str[10];
  
  if (gfSACSaveInit == 1)
  {
    logit("", "initSACSave called when initialized; call termSACSave first\n");
    return;   /* Don't initialize twice */
  }

  gfSACSaveInit = -1;
  
  /* First do the base directory */
  strcpy(gSACOutDir, plmParams->sacOutDir);
  if ( RecursiveCreateDir(gSACOutDir) == EW_FAILURE)
    return;
  strcat(gSACOutDir, "/");
  
  /* Now add the formatted part of the directory path */
  if ( plmParams->saveDirFormat != (char *)NULL)
  {
    memset(format, 0, LM_MAXTXT);
    s = plmParams->saveDirFormat;
    f = format;
    while ( *s != 0 )
    {
      if (*s == '%')
      {
        s++;
        if (*s == 'i') 
        {     /* Event ID */
          strcat(f, pEvt->eventId);
          f += strlen(pEvt->eventId);
        }
        else
        if (*s == 'v') 
        {     /* Version ID */
	  snprintf(origin_version_str, 10, "%d", pEvt->origin_version);
          strcat(f, origin_version_str);
          f += strlen(origin_version_str);
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
      logit("", "initSACSave: strftime failed parsing format <%s>\n",
            format);
      return;
    }
    if ( RecursiveCreateDir(gSACOutDir) == EW_FAILURE)
      return;
    strcat(gSACOutDir, "/");
  }

  /* Set our flag to say we're initialized */
  gfSACSaveInit = 1;
  return;
}

/*
 * saveSACWATrace: write the synthetic Wood-Anderson trace to a SAC file
 *                 The SAC directory must be named and created beforehand
 *                 by calling initSACSave. The SAC header is filled in with
 *                 as much information as we know about the event and SCN.
 *                 The peak-to-peak and zero-to-peak amplitudes and times
 *                 are set in the same manner as that used by Jim Pechmann's
 *                 ML SAC macro.
 *                 This function returns no status, on the grounds that saving
 *                 trace data is not the primary function of localmag.
 *                 If saving fails, localmag will continue on unimpeded.
 */
void saveSACWATrace( DATABUF *pTrace, STA *pSta, COMP3 *pComp, EVENT *pEvt,
                    LMPARAMS *plmParams)
{
  time_t ltime;
  struct tm *pTm;
  float stel, stdp, az, baz, gcarc, delta, odelta;
  double dmin, dmax, dmean;
  FILE *fp;
  char filename[PATH_MAX];
  int len;
  long i;
  struct SAChead2 *psh;

  /* Make sure we are prepared for action */
  if (gfSACSaveInit == 0)
  {
    logit("", "saveSACWATrace called while SACSave not initialized\n");
    return;
  }
  else if (gfSACSaveInit != 1)
    return;  /* Init failed; don't bother doing anything more */
  
  /* Name and open the new file; if it doesn't work, *
   * we can quit and go home early                   */
  memset(filename, 0, PATH_MAX);
  strcpy(filename, gSACOutDir);
  len = (int)strlen(filename);
  if (fmtFilename(pSta->sta, pComp->name, pSta->net, pComp->loc, filename, PATH_MAX - len, 
                  plmParams->saveNameFormat) < 0)
  {
    logit("", "saveSACWATrace: error formating SACSaveFile <%s>\n", 
          filename);
    return;
  }
  if ( (fp = fopen(filename, "wb")) == (FILE *)NULL)
  {
    logit("", "saveSACWATrace: error opening <%s>: %s\n", filename,
          strerror(errno));
    return;
  }

  /*
   * We set up a new SAC header: the reference time may have changed,
   * phase picks are irrelevant, so there isn't much point in keeping 
   * the old header, except for a few values.
   */
  if (plmParams->traceSource == LM_TS_SAC)
  {
    stel = gSAChd.stel;
    stdp = gSAChd.stdp;
    az   = gSAChd.az;
    baz  = gSAChd.baz;
    gcarc= gSAChd.gcarc;
    delta = gSAChd.delta;
    odelta = gSAChd.odelta;
  }
  else
  {
    stel = 0.0;
    stdp = 0.0;
    az   = 0.0;
    baz  = 0.0;
    gcarc= 0.0;
    delta = 0.0;
    odelta = 0.0;
  }

  sacInitHdr( &gSAChd );
  
  /* Some standard values */
  gSAChd.idep  = SAC_IUNKN;     /* unknown independent data type */
  gSAChd.iztype = SAC_IO;       /* Reference time is Origin time */
  gSAChd.iftype = SAC_ITIME;    /* File type is time series */
  gSAChd.leven  = 1;            /* evenly spaced data */
  cpystrn(gSAChd.kinst, "W_A_(mm)", K_LEN);
  
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

  /* SCNL strings; copy in characters; don't use *
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

  /* Copy back the saved values */
  if (plmParams->traceSource == LM_TS_SAC)
  {
    gSAChd.stel = stel;
    gSAChd.stdp = stdp;
    gSAChd.az   = az;
    gSAChd.baz  = baz;
    gSAChd.gcarc = gcarc;
    gSAChd.odelta = odelta;
  }
  
  gSAChd.b = (float)(pTrace->starttime - pEvt->origin_time);
  cpystrn(gSAChd.ko, "origin", K_LEN);

  gSAChd.e = (float)(pTrace->endtime - pEvt->origin_time);
  gSAChd.npts = pTrace->nProc;
  if ((double)delta == pTrace->delta)
  {
    gSAChd.delta = delta;
  }
  else
  {
    gSAChd.delta = (float)pTrace->delta;
  }
  
  /* Copy the data into the SAC data buffer */
  dmax = dmin = pTrace->procData[0];
  dmean = 0.0;
  for (i = 0; i < gSAChd.npts; i++)
  {
    dmean += pTrace->procData[i];
    dmin = (dmin < pTrace->procData[i]) ? dmin : pTrace->procData[i];
    dmax = (dmax > pTrace->procData[i]) ? dmax : pTrace->procData[i];
    gSACData[i] = (float)pTrace->procData[i];
  }
  dmean /= (double)gSAChd.npts;
  gSAChd.depmin = (float)dmin;
  gSAChd.depmax = (float)dmax;
  gSAChd.depmen = (float)dmean;
  
  if (plmParams->slideLength > 0.0)
  {
    /* Peak to Peak in sliding window */

	/* Times */
    gSAChd.t0 = (SACWORD) SACUNDEF;
    gSAChd.t1 = (float)(pComp->p2pMinTime - pEvt->origin_time);
    gSAChd.t2 = (float)(pComp->p2pMaxTime - pEvt->origin_time);

	/* Amplitudes */
    gSAChd.user0 = (SACWORD) SACUNDEF;
    gSAChd.user1 = (float)pComp->p2pMin;
    gSAChd.user2 = (float)pComp->p2pMax;

	/* Labels */
    cpystrn(gSAChd.kt0, SACSTRUNDEF, K_LEN);
    cpystrn(gSAChd.kt1, "PPMin", K_LEN);
    cpystrn(gSAChd.kt2, "PPMax", K_LEN);
    cpystrn(gSAChd.kuser0, SACSTRUNDEF, K_LEN);
    cpystrn(gSAChd.kuser1, "PPMinAmp", K_LEN);
    cpystrn(gSAChd.kuser2, "PPMaxAmp", K_LEN);
  }
  else
  {
    /* Zero to Peak */

	/* Times */
    gSAChd.t0 = (float)(pComp->z2pTime - pEvt->origin_time);
    gSAChd.t1 = (SACWORD) SACUNDEF;
    gSAChd.t2 = (SACWORD) SACUNDEF;


	/* Amplitudes */
    gSAChd.user0 = (float)pComp->z2pAmp;
    gSAChd.user1 = (SACWORD) SACUNDEF;
    gSAChd.user2 = (SACWORD) SACUNDEF;

	/* Labels */
    cpystrn(gSAChd.kt0, "ZPMax", K_LEN);
    cpystrn(gSAChd.kt1, SACSTRUNDEF, K_LEN);
    cpystrn(gSAChd.kt2, SACSTRUNDEF, K_LEN);
    cpystrn(gSAChd.kuser0, "ZPMaxAmp", K_LEN);
    cpystrn(gSAChd.kuser1, SACSTRUNDEF, K_LEN);
    cpystrn(gSAChd.kuser2, SACSTRUNDEF, K_LEN);
  }
  
  
  
  /* SAC files are always in "_SPARC" byte order; swap if necessary */
#ifdef _INTEL
  psh = (struct SAChead2 *)&gSAChd;
  for (i = 0; i < NUM_FLOAT; i++)
    SwapInt32( (int32_t *) &(psh->SACfloat[i]));
  for (i = 0; i < MAXINT; i++)
    SwapInt32( (int32_t *) &(psh->SACint[i]));
  for (i = 0; i < (int)pTrace->nProc; i++)
  {
    SwapInt32( (int32_t *) &(gSACData)[i] );
  }
#endif

  if (fwrite( &gSAChd, sizeof(gSAChd), 1, fp) != 1)
  {
    logit("", "saveSACWATrace: error writing SAC header: %s\n", strerror(errno));
    fclose(fp);
    return;
  }
  if (fwrite( gSACData, sizeof(float), pTrace->nProc, fp) != 
      (size_t)pTrace->nProc)
  {
    logit("", "saveSACWATrace: error writing SAC data: %s\n", 
          strerror(errno));
  }

  fclose(fp);
  return;
}

/*
 * termSACSave: Finish up what needs to be done for saving an event in SAC.
 *              Currently there isn't anything significant, but we could
 *              add things like writing some SAC macros.
 */
void termSACSave(EVENT *pEvt, LMPARAMS *plmParams)
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

/*
 * getSACStaDist: get station location and distance from the SAC header.
 *                Assumes that the SAC header has already been read into
 *                memory and is in local byte-order.
 *    Returns: 0 if location and distance have been read from header
 *            +1 if location but not distance were read
 *            -1 if neither location nor distance were read from SAC header.
 */
int getSACStaDist(double *pDist, double *pLat, double *pLon)
{
  int rc = 0;
  
  if (gSAChd.dist == (SACWORD)SACUNDEF)
    rc = +1;
  else
    *pDist = (double)gSAChd.dist;

  if ( gSAChd.stla == (SACWORD)SACUNDEF || gSAChd.stlo == (SACWORD)SACUNDEF)
    rc = -rc; /* if we already have dist, we don't need lat & lon */
  else
  {
    *pLat = (double)gSAChd.stla;
    *pLon = (double)gSAChd.stlo;
  }

  if (rc < 0)
    logit("", "getSACStaDist: sta location and distance not in SAC file <%s>\n",
          gSACfile);

  return rc;
}

/*
 * getAmpFromSACHdr: read the amplitude value from the SAC header.
 *                   Assumes user1 is the minus peak-to-peak amplitude, 
 *                   user2 is the plus peak-to-peak amplitude,
 *                   and that user0 is the zero-to-peak amplitude.
 *    Returns: 0 on success
 *            +1 if required fields are empty (SACUNDEF).
 */
static int getAmpFromSACHdr(COMP3 *pComp, EVENT *pEvt)
{
  if (gSAChd.user0 != (SACWORD)SACUNDEF)
  {
    /* doing zero2peak mode */
    pComp->z2pAmp = (double)gSAChd.user0;

    if (gSAChd.t0 != (SACWORD)SACUNDEF)
      pComp->z2pTime = gSAChd.t0 + pEvt->origin_time;
    else 
      pComp->z2pTime = pEvt->origin_time;
  }
  else
  {
    /* doing peak2peak mode */
    if ((gSAChd.user1 == (SACWORD)SACUNDEF) || 
          (gSAChd.user2 == (SACWORD)SACUNDEF))
    {
       logit("", "getAmpFromSACHdr: SAC user1 and user2 values not set in <%s>\n",
                      gSACfile);
       return +1;
    }

    pComp->p2pMin = (double)gSAChd.user1;
    pComp->p2pMax = (double)gSAChd.user2;
    pComp->p2pAmp = pComp->p2pMax - pComp->p2pMin;

    if (gSAChd.t1 != (SACWORD)SACUNDEF)
      pComp->p2pMinTime = gSAChd.t1 + pEvt->origin_time;
    else
      pComp->p2pMinTime = pEvt->origin_time;

    if (gSAChd.t2 != (SACWORD)SACUNDEF)
      pComp->p2pMaxTime = gSAChd.t2 + pEvt->origin_time;
    else
      pComp->p2pMaxTime = pEvt->origin_time;
  }


  return 0;

}
