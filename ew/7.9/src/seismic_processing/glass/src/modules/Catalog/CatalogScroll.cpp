#include <windows.h>
#include <stdio.h>
#include "CatalogScroll.h"
#include "IGlint.h"
#include "CatalogMod.h"
#include "date.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

#define szORIGIN_LOGFILE_BASE_NAME "glass_origin_log.txt"

extern CMod *pMod;


static char szOriginLogfileName[256];

//---------------------------------------------------------------------------------------CScroll
CScroll::CScroll() {
	hFont = 0;
	hText = 18;
	pGlint = 0;
	nCat = 0;
	iCat = -1;
  memset(Cat,0,sizeof(Cat));

  bLogOrigins = false;
//	DebugOn();
  iX = 0;
  iY = 0;
	nX = 800;
	nY = 400;
}

//---------------------------------------------------------------------------------------~CScroll
CScroll::~CScroll() {
	CatOut();
	if(hFont)
		DeleteObject(hFont);
}

//---------------------------------------------------------------------------------------Init
void CScroll::Init(HINSTANCE hinst, char *title) 
{
	iStyle |= WS_VSCROLL;
	CWin::Init(hinst, title);
	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------Refresh
// Redraw map
void CScroll::Refresh() {
	InvalidateRect(hWnd, NULL, true);
	UpdateWindow(hWnd);
}

//---------------------------------------------------------------------------------------Size
void CScroll::Size(int w, int h) {
	nScr = h / hText - 2;
	ScrollBar();
	Refresh();
}


//---------------------------------------------------------------------------------------Draw
void CScroll::Draw(HDC hdc) {
	CAT *cat;
	char txt[128];
	RECT r;
	int h;

	if(!hFont) {
		hFont = CreateFont(18, 0, 0, 0, FW_NORMAL,
			false, false, false, OEM_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			FIXED_PITCH, NULL);
		if(!hFont)
			return;
		SelectObject(hdc, hFont);
	}

	SelectObject(hdc, hFont);
	GetClientRect(hWnd, &r);

	int ipos = GetScrollPos(hWnd, SB_VERT);
  int i1, i2;

	i1 = nCat - MAXCAT;
	if(i1 < 0)
		i1 = 0;
	i1 += ipos;
	i2 = i1 + nScr - 1;
	if(i2 >= nCat) // DK 022504  wrapped catalog should be in the range [nCat-MAXCAT, nCat-1]
		i2 = nCat-1;
	int icat;
	h = 10;
//	sprintf(txt, "%4d %s %9.4f %10.4f %6.2f%4d%6.2f", org->iOrigin,
//		dt->Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
//		org->nEq, org->dRms);
	strncpy(txt, " Seq Origin Time           Latitude  Longitude  Depth Neq  Nph  RMS  Aff  Gap idOrg", sizeof(txt)-1);
        txt[sizeof(txt)-1] = 0x00;
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;
	for(icat=i1; icat<=i2; icat++) {
		cat = &Cat[icat%MAXCAT];
		sprintf(txt, "%s", cat->sCat);
		if(icat == iCat) {
			SetTextColor(hdc, RGB(255, 0, 0));
			TextOut(hdc, 10, h, txt, strlen(txt));
			SetTextColor(hdc, RGB(0, 0, 0));
		} else {
			TextOut(hdc, 10, h, txt, strlen(txt));
		}
		h += hText;
	}
}

//---------------------------------------------------------------------------------------Quake
void CScroll::Quake(char *ent) {
	int icat;
	int jcat;
	char txt[256];
	int i1,i2;
	int i;

	if(!pGlint)
		return;
	ORIGIN *org = pGlint->getOrigin(ent);

  // DK CLEANUP add NULL origin check!!!
  if(!org)
  {
    // Must be a delete message

    // Delete the quake from the list!  Catalog should currently range from nCat-MAXCAT to nCat-1
    i1 = nCat - MAXCAT;
    i2 = nCat - 1;  

    if(i1 < 0)
      i1 = 0;
    iCat = -1;

    CDebug::Log(DEBUG_MINOR_INFO,"Deleting Quake nCat/MC/i1/i2 = %d/%d/%d/%d\n",
                nCat,MAXCAT,i1,i2);

    for(i=i1; i<nCat; i++) 
    {
      jcat = i%MAXCAT;
      //	TRACE("Cat[%d].iOrigin:%d org->iOrigin:%d\n", jcat, Cat[jcat].iOrigin, org->iOrigin);
      if(strcmp(Cat[jcat].idQuake, ent))
        continue;
      
      // We found the deleted quake in the list
      // Delete it from the list

      /* There are two cases here:
         1) The index of the start of the catalog is less than (or equal to) 
             the index of the end of the catalog.  This is the simple case, 
             and we just copy every element after the one to-be-deleted, 
             forward one index in the  array.
         2) The index of the start of the catalog is after the index of the
             end of the catalog.  This is the complex case.  We have to copy
             all of the elements (except the deleted one), maintain the order,
             handle wrapping around the end of the array, etc.
       **************************************************************************/
      //if(nCat < MAXCAT)               // DK 022304
      if((i1%MAXCAT) <= (i2%MAXCAT))   // DK 022304
      {
        CDebug::Log(DEBUG_MINOR_INFO,"Using Delete Method 1.  jcat/nCat = %d/%d\n",
                    jcat,nCat);
        memmove(&Cat[jcat], &Cat[jcat+1], (nCat-jcat-1) * sizeof(Cat[jcat]));
        nCat--;
        iCat = nCat-1;
      }
      else
      {
        int iMaxCat = i2%MAXCAT;
        int iMinCat = i1%MAXCAT;
        int iTempCat = 0;
        int nNumRecords;
        CAT	* TempCat;
        CDebug::Log(DEBUG_MINOR_INFO,"Using Delete Method 2.  jcat/nCat = %d/%d\n",
                    jcat,nCat);

        TempCat = (CAT *) malloc(MAXCAT * sizeof(CAT));
        if(!TempCat)
        {
          CDebug::Log(DEBUG_MAJOR_ERROR, "Quake():  Could not allocate %d bytes for Temporary catalog.  Returning  error.\n",
            MAXCAT*sizeof(CAT));
          exit(-1);
          return;
        }
        
        if(jcat >= iMinCat)
        {
          CDebug::Log(DEBUG_MINOR_INFO,"Using Delete Method 2.  jcat/min/max/tc = %d/%d/%d/%u\n",
                      jcat,iMinCat,iMaxCat,TempCat);
          // copy from iMinCat through (jcat - 1)
          nNumRecords = (jcat - 1) - iMinCat + 1;
          memcpy(&TempCat[iTempCat], &Cat[iMinCat], nNumRecords * sizeof(CAT));
          iTempCat += nNumRecords;

          // copy from (jcat + 1) through (MAXCAT - 1)
          nNumRecords = (MAXCAT - 1) - (jcat + 1) + 1;
          memcpy(&TempCat[iTempCat], &Cat[jcat + 1], nNumRecords * sizeof(CAT));
          iTempCat += nNumRecords;

          // copy from 0 through iMaxCat
          nNumRecords = iMaxCat - 0 + 1;
          memcpy(&TempCat[iTempCat], &Cat[0], nNumRecords * sizeof(CAT));
          iTempCat += nNumRecords;
        }
        else
        {
          CDebug::Log(DEBUG_MINOR_INFO,"Using Delete Method 3.  jcat/min/max/tc = %d/%d/%d/%u\n",
                      jcat,iMinCat,iMaxCat,TempCat);
          // copy from iMinCat through (MAXCAT - 1)
          nNumRecords = (MAXCAT - 1) - iMinCat + 1;
          memcpy(&TempCat[iTempCat], &Cat[iMinCat], nNumRecords * sizeof(CAT));
          iTempCat += nNumRecords;

          // copy from 0 through (jcat - 1)
          nNumRecords = (jcat - 1) - 0 + 1;
          memcpy(&TempCat[iTempCat], &Cat[0], nNumRecords * sizeof(CAT));
          iTempCat += nNumRecords;

         // copy from (jcat + 1) through iMaxCat
          nNumRecords = iMaxCat - (jcat + 1) + 1;
          memcpy(&TempCat[iTempCat], &Cat[jcat + 1], nNumRecords * sizeof(CAT));
          iTempCat += nNumRecords;
        }

        // set nCat = (MAXCAT - 1)
        nCat = MAXCAT - 1;
        iCat = nCat - 1;
        memcpy(Cat,TempCat,iTempCat * sizeof(CAT));
        free(TempCat);  // DK 022603  Free temporary catalog memory allocated above
        CDebug::Log(DEBUG_MINOR_INFO,"Copied %d records during deletion(%s)\n", iTempCat,ent);
      }  // end else    (nCat >= MAXCAT)

      sprintf(txt, "Origin %s deleted!\n", ent);
      LogOrigin(-1,txt);
      ScrollBar();
      Refresh();

      return;
    }  // end for each quake in list

    // Quake not found, issue an error message
    CDebug::Log(DEBUG_MINOR_ERROR, "Got quake message for id(%s), but could "
                                   "not find quake in Glint or in Cat list.\n", 
                ent);
    return;
  }  // end if quake not found in Glint

	CDate dt(org->dT);
  char * sz0 = strchr(org->idOrigin, '0');
  int iIDOrigin;
  if(sz0)
    iIDOrigin = atoi(sz0);
  else
    iIDOrigin = -1;

	_snprintf(txt, sizeof(txt)-1, "%4d %s %9.4f %10.4f %6.2f%4d %4d %4.2f %5.2f %3d %5d", org->iOrigin,
		dt.Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
		org->nEq, org->nPh, org->dRms, org->dAff, (int)org->dGap, iIDOrigin);
  txt[sizeof(txt)-1] = 0x00;
	i1 = nCat - MAXCAT;
	if(i1 < 0)
		i1 = 0;
	iCat = -1;
	for(i=i1; i<nCat; i++) {
		jcat = i%MAXCAT;
	//	TRACE("Cat[%d].iOrigin:%d org->iOrigin:%d\n", jcat, Cat[jcat].iOrigin, org->iOrigin);
		if(Cat[jcat].iOrigin != org->iOrigin)
			continue;
		strncpy(Cat[jcat].sCat, txt, sizeof(Cat[jcat].sCat)-1);
    Cat[jcat].sCat[sizeof(Cat[jcat].sCat)-1] = 0x00;
		iCat = i;
		break;
	}
	if(iCat < 0) {
		iCat = nCat;
		icat = nCat%MAXCAT;
		Cat[icat].iOrigin = org->iOrigin;
		strncpy(Cat[icat].idQuake, ent, sizeof(Cat[icat].idQuake)-1);
    Cat[icat].idQuake[sizeof(Cat[icat].idQuake)-1] = 0x00;
		strncpy(Cat[icat].sCat, txt, sizeof(Cat[icat].sCat)-1);
    Cat[icat].sCat[sizeof(Cat[icat].sCat)-1] = 0x00;
		nCat++;
	}
  LogOrigin(iCat,txt);
	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------ScrollBar
// Calculate and instantiate appropriate scroll bar parameters
void CScroll::ScrollBar() 
{
  int nscr = nCat;

  int ipos = GetScrollPos(hWnd, SB_VERT);
  
  if(nscr > MAXCAT)
    nscr = MAXCAT;
  int iscr = nscr - nScr;
  if(iscr <= 0) {
    SetScrollRange(hWnd, SB_VERT, 0, 100, FALSE);
    SetScrollPos(hWnd, SB_VERT, 0, TRUE);
    ShowScrollBar(hWnd, SB_VERT, FALSE);
  } 
  else 
  {
    int i1;
    if(nCat < MAXCAT)
      i1 = 0;
    else
      i1 = nCat - MAXCAT;
    
    // ipos  - the current position of the scroll bar
    // nscr  - the number of quakes in the display list
    // nScr  - the number of quakes that will fit on the screen
    // iscr  - the number of scroll positions (list size - screen size)
    int iMaxQuake = nCat - (iscr - ipos);
    int iMinQuake = iMaxQuake - (nScr -1);
    if(iCat < iMinQuake || iCat > iMaxQuake)
    {
      // change the scroll position so that the selected quake
      // will be on the screen.
      ipos = iscr-(nCat-(nScr/2)-iCat);

    }
    // else we're already set

    if(ipos < 0) ipos = 0;
    else if(ipos > iscr) ipos = iscr;
    SetScrollRange(hWnd, SB_VERT, 0, iscr, FALSE);
    SetScrollPos(hWnd, SB_VERT, ipos, TRUE);
    ShowScrollBar(hWnd, SB_VERT, TRUE);
  }
}  // end CScroll::ScrollBar() 

//---------------------------------------------------------------------------------------VScroll
void CScroll::VScroll(int code, int pos) {
//	TRACE("Scroll %d %d\n", code, pos);
	int ipos = GetScrollPos(hWnd, SB_VERT);
	switch(code) {
	case SB_LINEUP:
		ipos--;
		break;
	case SB_LINEDOWN:
		ipos++;
		break;
	case SB_PAGEUP:
		ipos-=5;
		break;
	case SB_PAGEDOWN:
		ipos+=5;
		break;
	case SB_THUMBPOSITION:
		ipos = pos;
		break;
	}
	if(ipos < 0)
		ipos = 0;
	SetScrollPos(hWnd, SB_VERT, ipos, TRUE);
	Refresh();
}

//---------------------------------------------------------------------------------------LeftDown
void CScroll::LeftDown(int x, int y) {
	int i1 = nCat - MAXCAT;
	if(i1 < 0)
		i1 = 0;
	int ipos = GetScrollPos(hWnd, SB_VERT);
	int icat = ipos + i1 + (y - 10)/hText - 1;
  CDebug::Log(DEBUG_MINOR_INFO,"iCat = %d\n", icat);
	if(icat < i1)
		return;
	if(icat >= nCat)
		return;

	IMessage *m = pMod->CreateMessage("Grok");
	m->setStr("Quake", Cat[icat%MAXCAT].idQuake);  // DK added 061903
	pMod->Broadcast(m);
	m->Release();
}

//---------------------------------------------------------------------------------------Pau
// Called before SIAM process termination -- dump catalog to dist (catalog.txt)
void CScroll::CatOut() 
{
	FILE *f;

  char szCatalogFileName[256];
  char * szEW_LOG;

  // set the full name of the catalog file
  szEW_LOG = getenv("EW_LOG");
  if(!szEW_LOG)
    szEW_LOG = "";

  _snprintf(szCatalogFileName, sizeof(szCatalogFileName), 
            "%s%s", szEW_LOG, "catalog.txt");
  szCatalogFileName[sizeof(szCatalogFileName)-1] = 0x00;


	if(!(f=fopen(szCatalogFileName, "a")))
  {

    CDebug::Log(DEBUG_MINOR_ERROR,"CScroll::CatOut()  ERROR:  Could not open catalog file (%s) to dump catalog.\n",
                szCatalogFileName);
		return;
  }

	CAT *cat;
	int i2 = nCat-1;  // DK 022504  wrapped catalog should be in the range [nCat-MAXCAT, nCat-1]
	int i1 = nCat - MAXCAT;
	if(i1 < 0)
		i1 = 0;
	int icat;
  time_t tNow;
  time(&tNow);
	fprintf(f, "***********************************************\n"
             "*  CATALOG %35s"
             "***********************************************\n",
          ctime(&tNow));
	for(icat=i1; icat<=i2; icat++) {
		cat = &Cat[icat%MAXCAT];
		fprintf(f, "%s\n", cat->sCat);
	}
	fclose(f);
  f=NULL;
}


/**************************************************
    CScroll::SetOriginLogging(int bLog) 


     params:
      bLog - flag indicating whether or not origins should
             be logged to disk.

     return value:
         1 - disk logging enabled
         0 - disk logging disabled
 ***************************************************/
int CScroll::SetOriginLogging(int bLog) 
{
  FILE * f;

  this->bLogOrigins = false;

  if(bLog)
  {
    char * szEW_LOG = getenv("EW_LOG");
    if(!szEW_LOG)
      szEW_LOG = "";
    _snprintf(szOriginLogfileName, sizeof(szOriginLogfileName), 
              "%s%s", szEW_LOG, szORIGIN_LOGFILE_BASE_NAME);
    szOriginLogfileName[sizeof(szOriginLogfileName)-1] = 0x00;

    if(f = fopen(szOriginLogfileName, "w"))
    {
      time_t tNow;
      time(&tNow);
      fprintf(f,"\n\n"
                "########################################\n"
                "### STARTING AT %s"
                "########################################\n\n",
              ctime(&tNow));
      fclose(f);
      f = NULL;
      this->bLogOrigins = true;
    }
    else
    {
      CDebug::Log(DEBUG_MINOR_ERROR, 
                  "CScroll:SetOriginLogging(): ERROR: Unable to open origins log file (%s).\n",
                  szOriginLogfileName);
    }
  }
  return(this->bLogOrigins);
}


int  CScroll::LogOrigin(int iCat, char * txt)
{
  int rc = false;
  FILE * f;
  if(bLogOrigins)
  {
    if(f = fopen(szOriginLogfileName, "a"))
    {
      fprintf(f, "%d %s\n", iCat, txt);
      fclose(f);
      f = NULL;
      rc = true;
    }
    else
    {
      CDebug::Log(DEBUG_MINOR_ERROR, "CScroll:LogOrigin(): ERROR: Unable to open origins log file (%s).\n",
                  szOriginLogfileName);
    }
  }

  return(rc);
}  // end CScroll::LogOrigin()
