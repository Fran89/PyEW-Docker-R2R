#include <windows.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "comfile.h"
#include "station.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

int __cdecl CompareStationSCNLs(const void *elem1, const void *elem2);

int __cdecl CompareStationSCNLs(const void *elem1, const void *elem2)
{
  SCNL * p1;
  SCNL * p2;
  int rc;

  p1 = &((STATION *) elem1)->Name;
  p2 = &((STATION *) elem2)->Name;

  if(!(rc = strncmp(p1->szSta, p2->szSta, sizeof(p1->szSta))))
    if(!(rc = strncmp(p1->szNet, p2->szNet, sizeof(p1->szNet))))
      if(!(rc = strncmp(p1->szLoc, p2->szLoc, sizeof(p1->szLoc))))
        rc = strncmp(p1->szComp, p2->szComp, sizeof(p1->szComp));
  return(rc);
}


// Class used for extracting information from DBMS.

CStation::CStation() {
	nSta = 0;
}

CStation::~CStation() {
}

//--------------------------------------------------------------------------Load
// Load station list in earthworm format
bool CStation::Load() {
	return LoadHypoInverse("glass_station.lis");
}

//--------------------------------------------------------------------------Load
// Load station list in hypoellipse format
bool CStation::LoadHypoEllipse(const char *name) 
{
	STATION Station;
	char crd[120];
	char sta[20];
	double dlat;
	double dlon;
	double delev;
	double deg;
	double dmn;
	char c;
	int i;
	int n;

	FILE *fs;
  
	if(!(fs=fopen(name, "r")))
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,"CStation::Hypo():  ERROR! Could not load the station list (hypoellipse-format) from file(%s)!\n",
                name);
    exit(-1);
		return false;
  }
	while(true) {
		if(!fgets(crd, sizeof(crd)-1, fs))
			break;
		n = 0;
		for(i=0; i<20; i++) {
			if(crd[i] == ' ')
				break;
			sta[n++] = crd[i];
		}
		sta[n] = 0;
		crd[17] = 0;
		deg = atof(&crd[15]);
		c = crd[25];
		crd[25] = 0;
		dmn = atof(&crd[18]);
		dlat = deg + dmn/60.0;
		if(c == 'S')
			dlat = -dlat;
		crd[29] = 0;
		deg = atof(&crd[26]);
		c = crd[37];
		crd[37] = 0;
		dmn = atof(&crd[30]);
		dlon = deg + dmn/60.0;
		if(c == 'W')
			dlon = -dlon;
		crd[42] = 0;
		delev = atof(&crd[38]);
		memset(&Station, 0, sizeof(Station));
		strncpy(Station.Name.szSta, sta, sizeof(Station.Name.szSta) - 1);
		Station.dLat = dlat;
		Station.dLon = dlon;
		Station.dElev = delev;

		Put(&Station);
  }  //end while(true)
	fclose(fs);
  fs=NULL;

  SortStationList();

	return true;
}

//--------------------------------------------------------------------------Load
// Load station list in earthworm format
bool CStation::LoadHypoInverse(const char *name) 
{


  /* Column positions of various fields from hypoinv_sta.format file */
#define STA_POS 1
#define NET_POS 7
#define CHAN_POS 11
#define LATD_POS 16
#define LATM_POS 19
#define NS_POS   26
#define LOND_POS 27
#define LONM_POS 31
#define EW_POS   38
#define ELEV_POS 39
#define PERD_POS 43
#define LOC_POS 81
#define QUAL_POS 84
#define STA_LEN 5
#define NET_LEN 2
#define CHAN_LEN 3
#define LOC_LEN 2
#define LATD_LEN 2
#define LATM_LEN 7
#define LOND_LEN 3
#define LONM_LEN 7
#define ELEV_LEN 4
#define PERD_LEN 3
#define QUAL_LEN 5
  
#define LINE_BUFFER_SIZE 200
#define NULL_LOC_CODE "--"
  
  
  FILE * fs;
  STATION Station;
  char pBuffer[LINE_BUFFER_SIZE];
  int i,done=0,retval;
  char szTemp[20];
  char flag;
  char *pTempChr;

//  DebugBreak();
  
  if(!(fs=fopen(name, "r")))
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,"CStation::Load():  ERROR! Could not load the station list (hypoinverse-format) from file(%s)!\n",
      name);
    exit(-1);
    return false;
  }
  
  for(i=0; ; i++)
  {
    memset(&Station,0,sizeof(Station));
    retval=(int)fgets(pBuffer,LINE_BUFFER_SIZE,fs);
    
    if (retval == 0)
    {
      if(feof(fs))
      {
        break;
      }
      else
      {
        logit("et","Error %d occured while reading from line %d of file %s\n",
              ferror(fs),i+1, name);
        return false;
      }
    }
    
    if(strlen(pBuffer) < PERD_POS)
      continue;
    
    strncpy(Station.Name.szSta,&pBuffer[STA_POS-1],STA_LEN);
    Station.Name.szSta[STA_LEN]=0;

    /* ignore lines that start with # */
    if(Station.Name.szSta[0] == '#')
      continue;

    strncpy(Station.Name.szNet,&pBuffer[NET_POS-1],NET_LEN);
    Station.Name.szNet[NET_LEN]=0;
    strncpy(Station.Name.szComp,&pBuffer[CHAN_POS-1],CHAN_LEN);
    Station.Name.szComp[CHAN_LEN]=0;

    if(strlen(pBuffer) < (LOC_POS + LOC_LEN - 1))
    {
      strncpy(Station.Name.szLoc, NULL_LOC_CODE, LOC_LEN);
      Station.Name.szComp[LOC_LEN]=0;
    }
    strncpy(Station.Name.szLoc,&pBuffer[LOC_POS-1],LOC_LEN);
    Station.Name.szLoc[LOC_LEN]=0;
    
    /* Fix station code */
    if ((pTempChr = strchr (Station.Name.szSta, ' ')) != NULL)
      *pTempChr = '\0';
    
    /* Fix component code */
    if ((pTempChr = strchr (Station.Name.szComp, ' ')) != NULL)
      *pTempChr = '\0';
    
    /* Fix network code */
    if ((pTempChr = strchr (Station.Name.szNet, ' ')) != NULL)
      *pTempChr = '\0';
    
    /* Fix location code */
    if ((pTempChr = strchr (Station.Name.szLoc, ' ')) != NULL)
      *pTempChr = '\0';
    if(Station.Name.szLoc[0] == 0x00)
    strncpy(Station.Name.szLoc,NULL_LOC_CODE,LOC_LEN);
    Station.Name.szLoc[LOC_LEN]=0;

    /* Latitude: degrees converted from degrees + minutes */
    /*  flag: 'N','n',' ' for North (+) latitude; 'S','s' for South (-) */
    strncpy(szTemp,&pBuffer[LATD_POS-1],LATD_LEN);
    szTemp[LATD_LEN]=0;
    Station.dLat = (float) atof(szTemp);
    strncpy(szTemp,&pBuffer[LATM_POS-1],LATM_LEN);
    szTemp[LATM_LEN]=0;
    Station.dLat += (float)(atof(szTemp)/60.0);
    flag = pBuffer[NS_POS-1]; 
    if( flag=='S' || flag=='s' ) Station.dLat *= -1.0;
        
    /* Longitude: degrees converted from degrees + minutes */
    /*  flag: 'W','w',' ' for West (-) longitude; 'E','e' for East (+) */
    strncpy(szTemp,&pBuffer[LOND_POS-1],LOND_LEN);
    szTemp[LOND_LEN]=0;
    Station.dLon = (float) atof(szTemp);
    strncpy(szTemp,&pBuffer[LONM_POS-1],LONM_LEN);
    szTemp[LONM_LEN]=0;
    Station.dLon += (float)(atof(szTemp)/60.0);
    flag = pBuffer[EW_POS-1]; 
    if( flag==' ' || flag=='W' || flag=='w' ) Station.dLon *= -1.0;
    
    /* Elevation: meters */
    strncpy(szTemp,&pBuffer[ELEV_POS-1],ELEV_LEN);
    szTemp[ELEV_LEN]=0;
    Station.dElev=(float)atof(szTemp);
    
    /* Station: quality */
    strncpy(szTemp,&pBuffer[QUAL_POS-1],QUAL_LEN);
    szTemp[QUAL_LEN]=0;
    Station.dQual=atof(szTemp);
    
    if(Station.dQual > MIN_STATION_QUAL_THRESHOLD)
    {
      /* Add it to list */
 		  if(!Put(&Station))
      {
        CDebug::Log(DEBUG_MINOR_ERROR,"CStation::Load():  ERROR adding station (%s %s %s %s) at line %d of file %s!\n",
                    Station.Name.szSta, Station.Name.szComp, Station.Name.szNet, Station.Name.szLoc, i+1, name);
      }
    }

  }  //end for(each line in file)
  
  
  fclose(fs);
  fs=NULL;

  SortStationList();
  return true;
}  // CStation::LoadHypoInverse()


bool CStation::Put(const STATION * pStation)
{

	if(nSta < MAXSTA) 
  {
    memcpy(&Sta[nSta], pStation, sizeof(STATION));
		nSta++;
		return true;
	}
	return false;
}

const STATION * CStation::Get(const SCNL *name)
{
  int i;
  STATION Station;
  STATION * pSta;

  memset(&Station, 0, sizeof(Station));
  memcpy(&Station.Name, name, sizeof(SCNL));

  pSta = (STATION *)bsearch(&Station, Sta, nSta, sizeof(STATION), CompareStationSCNLs);
  return(pSta);
  /************** replaced by bsearch call *****************************
	for(i=0; i<nSta; i++) {
		if(CompareStationSCNLs(&Station, &Sta[i]) > 0)
      continue;
    
    if(CompareStationSCNLs(&Station, &Sta[i]) == 0)
      return(&Sta[i]);
    else
      return(NULL);
  }
  return(NULL);
  **********************************************************/
}


bool CStation::GetList(STATION ** pStaList, int* pnLen)
{
  *pStaList = Sta;
  *pnLen = nSta;
  return(true);
}

void CStation::SortStationList()
{
  qsort((void *)Sta, nSta, sizeof(STATION), CompareStationSCNLs);
}

