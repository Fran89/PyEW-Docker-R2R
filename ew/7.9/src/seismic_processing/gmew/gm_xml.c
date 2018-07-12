/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_xml.c 6306 2015-04-22 13:22:58Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2006/03/15 14:21:54  paulf
 *     SCNL version of gmew v0.2.0
 *
 *     Revision 1.2  2001/04/02 23:07:18  lombard
 *     corrected format in station line of XML writer.
 *
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_xml.c: write shakemap XML file from strong motion data
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include <time.h>
#include <rw_strongmotionII.h>
#include <trace_buf.h>
#include "gm.h"
#include "gm_xml.h"
#include "shake_dtd.h"

/* Items for various mappings needed to write XML files for shakemapII */
static int nAgency    = 0;  /* Number of Agency table entries       */
static int nLSN       = 0;  /* Number of Station Name table entries */
static int nInstTypes = 0;  /* Number of Instr. Type table entries  */
static AGENCY   *pAgency;    /* The Agency mapping table             */
static LONGSTANAME *pLSN;    /* The long station name mapping table  */
static INSTTYPE   *pInst;    /* The instrument type mapping table    */

static char tempfile[PATH_MAX*2], outfile[PATH_MAX*2];
static char laststa[TRACE_STA_LEN+1], lastnet[TRACE_NET_LEN+1];
static char lastInst[INST_TYPE_LEN+1];
static FILE *fXML;
static int xmlStat;
static int sta_open;
static INSTTYPE *pI;

static const double Period1 = 0.3;  /* 1st interesting spectral period (s) */
static const double Period2 = 1.0;  /* 2nd interesting spectral period (s) */
static const double Period3 = 3.0;  /* 3rd interesting spectral period (s) */

/* Internal Function Prototypes */
int    CompareAgency( const void *, const void *);
int    CompareLSN( const void *, const void *);
int    CompareInstType( const void *, const void *);
void   new_sta( STA *, SM_INFO *, int *, char *, FILE *, INSTTYPE *);
void   close_sta( FILE * );

/*
 * Start_XML_file: set the XML file names, open the temporary file,
 *                 initialize things for writing.
 *    Returns: 0 on success
 *            -1 on errors
 */
int Start_XML_file(  GMPARAMS *pgmParam, EVENT *pEvt )
{
  char  fname[PATH_MAX-1];

  if (xmlStat > 0)
  {
    logit("et", "Start_XML_file called while XML file already open\n");
    return( -1 );
    
  }
  xmlStat = -1;  /* In case we have errors */

  sprintf( fname,  "%s_dig_dat.xml", pEvt->eventId );
  sprintf(tempfile, "%s/%s", pgmParam->TempDir, fname );
  sprintf(outfile, "%s/%s", pgmParam->XMLDir, fname );

  if ( (fXML = fopen( tempfile, "w")) == (FILE *)NULL)
  {
    logit("et", "Start_XML_file: Unable to open ShakeMap file: %s\n", tempfile );
    return( -1 );
  }   
     
  fprintf(fXML,
          "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n");
  fprintf(fXML, "<!DOCTYPE stationlist [\n%s]>\n", STALIST_DTD);
  fprintf(fXML, "<stationlist created=\"%ld\">\n", (long)time((time_t *)0));
     
  memset(laststa, 0, TRACE_STA_LEN+1);
  memset(lastnet, 0, TRACE_NET_LEN+1);
  memset(lastInst, 0, INST_TYPE_LEN+1);
  sta_open = 0;
  pI = (INSTTYPE *)NULL;

  xmlStat = 1;
  return( 0 );
}

/*
 * next_XML_SCNL: write the next SCNL's strongmotion data to XML file.
 */
void next_XML_SCNL(GMPARAMS *glmParam, STA *pSta, SM_INFO *pSM)
{
  INSTTYPE kInst;
  int irsa;
  double sp1, sp2, sp3, period;
  const double G = 9.81;  /* Convert from cm/sec/sec to percent G */
  
  /* If the XML file isn't ready, give up now! */
  if (xmlStat <= 0)
    return;
  
  /* Look up the instrument type; in shakemap, this is attached  *
     * to a station name; in earthworm it is attached to something *
     * like the first 2 letters of C in SCN(L)                     */
  if (nInstTypes > 0)
  {
    strcpy(kInst.sta, pSM->sta);
    strcpy(kInst.comp, pSM->comp);
    strcpy(kInst.net, pSM->net);
    pI = (INSTTYPE *)bsearch((void *)&kInst, (void *)pInst, nInstTypes,
                             sizeof(INSTTYPE), CompareInstType);
  }
  /* If station, net or instType has changed, start new station entry */
  if ( (strcmp(laststa, pSM->sta) != 0) || 
       (strcmp(lastnet, pSM->net) != 0) || 
       ( pI != (INSTTYPE *)NULL && strcmp(lastInst, pI->instType) != 0) )
  {
    new_sta(pSta, pSM, &sta_open, lastnet, fXML, pI);
    strcpy(laststa, pSM->sta);
    if (pI != (INSTTYPE *)NULL) strcpy(lastInst, pI->instType);
  }
       
  /* Extract the desired spectral response values */
  sp1 = sp2 = sp3 = 0.0;
  for (irsa = 0; irsa < pSM->nrsa; irsa++) 
  {
    period = pSM->pdrsa[irsa];
    if     ( fabs(period-Period1) < 0.1 ) sp1 = pSM->rsa[irsa];
    else if( fabs(period-Period2) < 0.1 ) sp2 = pSM->rsa[irsa];
    else if( fabs(period-Period3) < 0.1 ) sp3 = pSM->rsa[irsa];
  }

  /* Write component info */
  fprintf(fXML, "<comp name=\"%s\">\n", pSM->comp);
  fprintf(fXML, "<acc value=\"%.4f\" />\n", pSM->pga / G);
  fprintf(fXML, "<vel value=\"%.4f\" />\n", pSM->pgv);
  fprintf(fXML, "<psa03 value=\"%.4f\" />\n", sp1 / G);
  fprintf(fXML, "<psa10 value=\"%.4f\" />\n", sp2 / G);
  fprintf(fXML, "<psa30 value=\"%.4f\" />\n", sp3 / G);
  fprintf(fXML, "</comp>\n");

  fflush(fXML);

  return;
}

/*
 * Close_XML_file: close out the XML file, move it from temporary to
 *                 final location.
 *     returns: 0 on success
 *             -1 on error
 */
int Close_XML_file( void )
{
  if (xmlStat > 0) 
  {
    if (sta_open)
      close_sta(fXML);
  
    fprintf(fXML, "</stationlist>\n");
    fclose(fXML);
    
    /* Move completed file to output directory */
    if( rename( tempfile, outfile ) != 0 )
    {
      logit("et","Close_XML_file: Error renaming ShakeMap file: %s to: %s\n", 
            tempfile, outfile );
      return( -1 );
    }
    xmlStat = 0;
  }

  return( 0 );
}

/*
 * Send_XML_activate:   send an ACTIVATE_MESSAGE with arguments eventID and XML file 
 *     returns: 0 on success
 *             -1 on error
 */
int Send_XML_activate(EVENT *pEvt, GMPARAMS *pgmParam) 
{
char msg[PATH_MAX*3];
long size;


	sprintf(msg, "%d %s %s", pgmParam->pEW->amLogo.mod, pEvt->eventId, outfile);
 	size = strlen(msg);
    	if( tport_putmsg( &(pgmParam->pEW->OutRegion), &(pgmParam->pEW->amLogo), size, msg ) != PUT_OK )
	{
      		logit("et","Send_XML_activate: Error sending message to ring: msg=%s\n", 
            		msg );
      		return( -1 );
 	}
	return (0);

}

/* Write a new <station> line in the XML file; *
 * optionally closes the previous <station> line */
void new_sta( STA *pSta, SM_INFO *pSM, int *sta_open, char *lastnet, FILE *f, 
              INSTTYPE *pI)
{
  AGENCY kAgency;
  static AGENCY *pA;
  LONGSTANAME kLSN, *pS;
  
  /* Look up the long "agency" name from the network code */
  if (nAgency > 0)
  {
    if (strcmp(lastnet, pSM->net) != 0)
    {  /* net changed; look up new agency */
      strcpy(kAgency.net, pSM->net);
      pA = (AGENCY *) bsearch((void *)&kAgency, (void *)pAgency, nAgency, 
                              sizeof(AGENCY), CompareAgency);
      strcpy(lastnet, pSM->net);
    }
  }
  
  /* Look up the long station name */
  if (nLSN > 0)
  {
    strcpy(kLSN.sta, pSM->sta);
    strcpy(kLSN.net, pSM->net);
    pS = (LONGSTANAME *)bsearch((void *)&kLSN, (void *)pLSN, nLSN, 
                                sizeof(LONGSTANAME), CompareLSN);
  }
  else
    pS = (LONGSTANAME *)NULL;
  
  if (*sta_open)
    close_sta(f);
  *sta_open = 1;
  
  fprintf(f, "<station code=\"%s\" name=\"%s\" insttype=\"%s\" lat=\"%f\" "
          "lon=\"%f\" source=\"%s\" netid=\"%s\" commtype=\"DIG\" dist=\"%f\" loc=\"%s\" >\n", pSM->sta,
          ( (pS == (LONGSTANAME *)NULL) ? "" : pS->longName), 
          ( (pI == (INSTTYPE *)NULL) ? "" : pI->instType), 
          pSta->lat, pSta->lon,
          ( (pA == (AGENCY *)NULL) ? pSM->net : pA->agency), pSta->net, pSta->dist, pSM->loc);
  return;
}

/* Write the closing line for <station> to the XML file f */
void close_sta( FILE *f )
{
  fprintf(f, "</station>\n");
  return;
}


/************************************************************************
 * CompareAgency() a function passed to qsort; used to sort an array    *
 *   of AGENCY by idChan (ascending order) and            *
 *   DBMS load time (descending order; most recent first)               *
 ************************************************************************/
int CompareAgency( const void *s1, const void *s2 )
{
  AGENCY *t1 = (AGENCY *) s1;
  AGENCY *t2 = (AGENCY *) s2;

  return( strcmp( t1->net, t2->net) );
}

/******************************************************************
 * CompareLSN() a function passed to qsort; used to sort an array *
 *   of LONGSTANAME by SN (alphabetical order)                    *
 ******************************************************************/
int CompareLSN( const void *s1, const void *s2 )
{
  int rc;
  LONGSTANAME *t1 = (LONGSTANAME *) s1;
  LONGSTANAME *t2 = (LONGSTANAME *) s2;

  rc = strcmp( t1->sta, t2->sta );
  if ( rc != 0 ) return(rc);
  rc = strcmp( t1->net,  t2->net );
  return(rc);
}

/***********************************************************************
 * CompareInstType() a function passed to qsort; used to sort an array *
 *   of INSTTYPE structures by SCN (alphabetical order)                *
 ***********************************************************************/
int CompareInstType( const void *s1, const void *s2 )
{
  int rc;
  INSTTYPE *t1 = (INSTTYPE *) s1;
  INSTTYPE *t2 = (INSTTYPE *) s2;

  rc = strcmp( t1->sta, t2->sta );
  if ( rc != 0 ) return(rc);
  rc = strcmp( t1->comp, t2->comp);
  if ( rc != 0 ) return(rc);
  rc = strcmp( t1->net,  t2->net );
  return(rc);
}


/****************************************************************
 * initMappings() initialize the three mapping tables used in   *
 *   writing XML files for shakemapII. We read the mapping file *
 *   allocate space for the table, fill in and sort the table.  *
 *   This should be called at module startup; otherwise errors  *
 *   could embarrass us after an event.                         *
 *   Returns: 0 on success                                      *
 *           -1 on error (out of memory, parse errors)          *
 ****************************************************************/
int initMappings( char *mapFile )
{
  FILE *fh;
  char line[PATH_MAX];
  int len;
  int ns, na, ni;

  if ( (fh = fopen(mapFile, "r")) == (FILE *)NULL)
  {
    logit("e", "initMappings: error opening <%s> for reading\n", mapFile);
    return -1;
  }
  
  /* Scan the file once, counting entries of each type */
  while ( fgets( line, PATH_MAX, fh) != NULL)
  {
    if (line[0] == '#') continue;  /* Skip comments */
    if (memcmp( line, "Agency", 6) == 0)
      nAgency++;
    else if (memcmp( line, "Station", 7) == 0)
      nLSN++;
    else if (memcmp( line, "Inst", 4) == 0)
      nInstTypes++;
  }
  rewind( fh );
  
  if (nAgency > 0)
  {
    if ( (pAgency = (AGENCY *)calloc((size_t)nAgency, sizeof(AGENCY))) 
         == (AGENCY *)NULL)
    {
      logit("e", "initMappings: out of memory for %d Agency structs\n", 
            nAgency);
      return -1;
    }
  }
  if (nLSN > 0)
  {
    if ( (pLSN = (LONGSTANAME *)calloc((size_t)nLSN, sizeof(LONGSTANAME))) 
         == (LONGSTANAME *)NULL)
    {
      logit("e", "initMappings: out of memory for %d StaName structs\n", 
            nLSN);
      return -1;
    }
  }
  if (nInstTypes > 0)
  {
    if ( (pInst = (INSTTYPE *) calloc((size_t)nInstTypes, sizeof(INSTTYPE)))
         == (INSTTYPE *)NULL)
    {
      logit("e", "initMappings: out of memory for %d InstType structs\n", 
            nInstTypes);
      return -1;
    }
  }
  
  /* Read the file again, parsing the entries */
  ns = na = ni = 0;
  while ( fgets( line, PATH_MAX, fh) != NULL)
  {
    if (line[0] == '#') continue;  /* Skip comments */
    len = strlen(line);
    if (line[len-1] == '\n') line[len-1] = '\0'; /* chomp the newline */
    if (memcmp( line, "Agency", 6) == 0)
    {
      if (na == nAgency)
      {
        logit("e", "initMappings: error counting Agency lines\n");
        return -1;
      }
      if ( 2 != sscanf(line, "Agency %8s %40c", pAgency[na].net, 
                       pAgency[na].agency))
      {
        logit("e", "initMappings: error parsing <%s>\n", line);
        return -1;
      }
      na++;
    }
    else if (memcmp( line, "Station", 7) == 0)
    {
      if (ns == nLSN)
      {
        logit("e", "initMappings: error counting Station lines\n");
        return -1;
      }
      if ( 3 != sscanf(line, "Station %6s %8s %20c", pLSN[ns].sta, 
                       pLSN[ns].net, pLSN[ns].longName))
      {
        logit("e", "initMappings: error parsing <%s>\n", line);
        return -1;
      }
      ns++;
    }
    else if (memcmp( line, "Inst", 4) == 0)
    {
      if (ni == nInstTypes)
      {
        logit("e", "initMappings: error counting Inst lines\n");
        return -1;
      }
      if ( 4 != sscanf(line, "Inst %6s %8s %8s %50c", pInst[ni].sta,
                       pInst[ni].comp, pInst[ni].net, 
                       pInst[ni].instType))
      {
        logit("e", "initMappings: error parsing <%s>\n", line);
        return -1;
      }
      ni++;
    }
  }
  fclose( fh );

  /* Sort the tables for lookup using bsearch */
  if (nAgency > 1)
    qsort(pAgency, nAgency, sizeof(AGENCY), CompareAgency);
  if (nLSN > 1)
    qsort(pLSN, nLSN, sizeof(LONGSTANAME), CompareLSN);
  if (nInstTypes > 1)
    qsort(pInst, nInstTypes, sizeof(INSTTYPE), CompareInstType);

  logit("e", "initMappings: read %d agencies, %d stations, %d instruments\n",
        nAgency, nLSN, nInstTypes);
  
  return 0;
}

