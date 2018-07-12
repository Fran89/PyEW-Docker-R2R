#include <windows.h>
#include <stdio.h>
#include "scroll.h"
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
	nX = 700;
	nY = 400;

}

//---------------------------------------------------------------------------------------~CScroll
CScroll::~CScroll() {
	CatOut();
	if(hFont)
		DeleteObject(hFont);
}

//---------------------------------------------------------------------------------------Init
void CScroll::Init(HINSTANCE hinst, char *title) {
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
	int i2 = nCat;
	int i1 = nCat - MAXCAT;
	if(i1 < 0)
		i1 = 0;
	i1 += ipos;
	i2 = i1 + nScr;
	if(i2 > nCat)
		i2 = nCat;
	int icat;
	h = 10;
//	sprintf(txt, "%4d %s %9.4f %10.4f %6.2f%4d%6.2f", org->iOrigin,
//		dt->Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
//		org->nEq, org->dRms);
	strncpy(txt, " Seq Origin Time           Latitude  Longitude  Depth Neq  Nph   RMS", sizeof(txt)-1);
        txt[sizeof(txt)-1] = 0x00;
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;
	for(icat=i1; icat<i2; icat++) {
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
	int i1;
	int i;

	if(!pGlint)
		return;
	ORIGIN *org = pGlint->getOrigin(ent);
	CDate dt(org->dT);
	sprintf(txt, "%4d %s %9.4f %10.4f %6.2f%4d %4d %6.2f", org->iOrigin,
		dt.Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
		org->nEq, org->nPh, org->dRms);
	i1 = nCat - MAXCAT;
	if(i1 < 0)
		i1 = 0;
	iCat = -1;
	for(i=i1; i<nCat; i++) {
		jcat = i%MAXCAT;
	//	TRACE("Cat[%d].iOrigin:%d org->iOrigin:%d\n", jcat, Cat[jcat].iOrigin, org->iOrigin);
		if(Cat[jcat].iOrigin != org->iOrigin)
			continue;
		strcpy(Cat[jcat].sCat, txt);
		iCat = i;
		break;
	}
	if(iCat < 0) {
		iCat = nCat;
		icat = nCat%MAXCAT;
		Cat[icat].iOrigin = org->iOrigin;
		strcpy(Cat[icat].idQuake, ent);
		strcpy(Cat[icat].sCat, txt);
		nCat++;
	}
  LogOrigin(iCat,txt);
	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------ScrollBar
// Calculate and instantiate appropriate scroll bar parameters
void CScroll::ScrollBar() {
	int nscr = nCat;
	if(nscr > MAXCAT)
		nscr = MAXCAT;
	int iscr = nscr - nScr;
	if(iscr <= 0) {
		SetScrollRange(hWnd, SB_VERT, 0, 100, FALSE);
		SetScrollPos(hWnd, SB_VERT, 0, TRUE);
		ShowScrollBar(hWnd, SB_VERT, FALSE);
	} else {
		SetScrollRange(hWnd, SB_VERT, 0, nscr-nScr, FALSE);
		SetScrollPos(hWnd, SB_VERT, nscr-nScr, TRUE);
		ShowScrollBar(hWnd, SB_VERT, TRUE);
	}
}

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
	int i2 = nCat;
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
	for(icat=i1; icat<i2; icat++) {
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
