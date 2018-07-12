#include <windows.h>
#include <stdio.h>
#include "PublisherScroll.h"
#include "IGlint.h"
#include "PublisherMod.h"
#include "date.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

extern CMod *pMod;
#define CSCROLL_TEXT_SIZE 14
static char szOriginLogfileName[256];

//---------------------------------------------------------------------------------------CScroll
CScroll::CScroll() {
	hFont = 0;
	hText = CSCROLL_TEXT_SIZE;
	nPub = 0;
	iSelectedPub = -1;
  
  memset(PubStatusList, 0, sizeof(PubStatusList));

  tListBuilt = 0;

//	DebugOn();
  iX = 0;
  iY = 755;
	nX = 450;
	nY = 235;

}

//---------------------------------------------------------------------------------------~CScroll
CScroll::~CScroll() {
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
	char *pub;
	char txt[128];
	RECT r;
	int h;
	int iPub;

	if(!hFont) {
		hFont = CreateFont(CSCROLL_TEXT_SIZE, 0, 0, 0, FW_NORMAL,
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

	i1 = nPub - MAXPUB;
	if(i1 < 0)
		i1 = 0;
	i1 += ipos;
	i2 = i1 + nScr - 1;
	if(i2 >= nPub) // DK 022504  wrapped catalog should be in the range [nPub-MAXPUB, nPub-1]
		i2 = nPub-1;

  h = 10;
//	sprintf(txt, "%4d %s %9.4f %10.4f %6.2f%4d%6.2f", org->iOrigin,
//		dt->Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
//		org->nEq, org->dRms);
	strncpy(txt, " Seq   DeclareTime    LastPub NextPub Nph Npub  Xs  Lat    Lon  Dep    Time", sizeof(txt)-1);
        txt[sizeof(txt)-1] = 0x00;
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;
	for(iPub=i1; iPub<=i2; iPub++) {
		pub = PubStatusList[iPub%MAXPUB];
		sprintf(txt, "%s", &pub[1]);
    if(pub[0] == '-')
    {
			SetTextColor(hdc, RGB(128, 128, 128));
	    TextOut(hdc, 10, h, txt, strlen(txt));
			SetTextColor(hdc, RGB(0, 0, 0));
    }
    else if(iPub == iSelectedPub)
    {
			SetTextColor(hdc, RGB(255, 0, 0));
	    TextOut(hdc, 10, h, txt, strlen(txt));
			SetTextColor(hdc, RGB(0, 0, 0));
    }
    else
    {
	    TextOut(hdc, 10, h, txt, strlen(txt));
    }
		h += hText;
	}
}


//--------------------------------------------------------------------------------------Date13
char * Date13(time_t tTime, char * szBuffer)
{
   struct tm stm;      /* time as struct tm               */


   gmtime_ew( &tTime, &stm );

/* Build character string
 ************************/
   sprintf( szBuffer, 
           "%02d/%02d %02d:%02d:%02d",
            stm.tm_mon+1,
            stm.tm_mday,
            stm.tm_hour,
            stm.tm_min,
            stm.tm_sec);
 
   return( szBuffer );
}


//---------------------------------------------------------------------------------------Date5
char * Date5(time_t tTime, char * szBuffer)
{
   struct tm stm;      /* time as struct tm               */


   gmtime_ew( &tTime, &stm );

/* Build character string
 ************************/
   sprintf( szBuffer, 
           "%02d:%02d",
            stm.tm_hour,
            stm.tm_min);
 
   return( szBuffer );
}


//---------------------------------------------------------------------------------UpdateList
void CScroll::UpdateList() 
{
  ORIGIN *pOrigin;

  char sztDeclare[16], sztLastPub[8], sztNextPub[8], sztOrigin[16];


  time(&tListBuilt);
  nPub = 0;

  for(int i=iOldestQuake; i <= iYoungestQuake; i++)
  {
    pOrigin = pGlint->getOrigin(QList[i].idOrigin);
    if(QList[i].tLastPub <= 0)
      sprintf(sztLastPub, " NONE");
    else
       Date5(QList[i].tLastPub, sztLastPub);
    if(QList[i].tNextPub <= 0)
      sprintf(sztNextPub, " NONE");
    else
       Date5(QList[i].tNextPub, sztNextPub);

    if(pOrigin)
    {
      sprintf(PubStatusList[nPub],
              "%c%4d  %13s   %5s   %5s %4d %3d  %3d %5.1f %6.1f %3.0f %13s",
              (QList[i].bDead) ? '-' : ' ',
              pOrigin->iOrigin, Date13(QList[i].tDeclare,sztDeclare),
              sztLastPub, sztNextPub, 
              pOrigin->nPh, QList[i].iNumPub, QList[i].iNumChanges,
              QList[i].dLat, QList[i].dLon, QList[i].dZ, Date13((time_t)QList[i].tOrigin,sztOrigin));
    }
    else
    {
      /* this is a dead quake */
      sprintf(PubStatusList[nPub],
              "%c%4d  %13s   %5s   %5s %4d %3d  %3d %5.1f %6.1f %3.0f %13s",
              (QList[i].bDead) ? '-' : ' ',
              QList[i].iOrigin, Date13(QList[i].tDeclare,sztDeclare),
              sztLastPub, sztNextPub, 
              0, QList[i].iNumPub, QList[i].iNumChanges,
              QList[i].dLat, QList[i].dLon, QList[i].dZ, Date13((time_t)QList[i].tOrigin,sztOrigin));
    }
    nPub++;
  }
	ScrollBar();
	Refresh();
}  // end UpdateList()


//---------------------------------------------------------------------------------------ScrollBar
// Calculate and instantiate appropriate scroll bar parameters
void CScroll::ScrollBar() 
{
  int nscr = nPub;

  int ipos = GetScrollPos(hWnd, SB_VERT);
  
  if(nscr > MAXPUB)
    nscr = MAXPUB;
  int iscr = nscr - nScr;
  if(iscr <= 0) {
    SetScrollRange(hWnd, SB_VERT, 0, 100, FALSE);
    SetScrollPos(hWnd, SB_VERT, 0, TRUE);
    ShowScrollBar(hWnd, SB_VERT, FALSE);
  } 
  else 
  {

    ipos = iscr-(nPub-(nScr/2)-iSelectedPub);
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
}  /* end VScroll() */

//---------------------------------------------------------------------------------------LeftDown
void CScroll::LeftDown(int x, int y) {
	int i1 = nPub - MAXPUB;
	if(i1 < 0)
		i1 = 0;
	int ipos = GetScrollPos(hWnd, SB_VERT);
	int iPub = ipos + i1 + (y - 10)/hText - 1;
  CDebug::Log(DEBUG_MINOR_INFO,"iPub = %d\n", iPub);
	if(iPub < i1)
		return;
	if(iPub >= nPub)
		return;

  iSelectedPub = iPub;
  /******************************************************************
	IMessage *m = pMod->CreateMessage("Grok");
	m->setStr("Quake", PubStatusList[iPub%MAXPUB].idQuake);  // DK added 061903
	pMod->Broadcast(m);
	m->Release();
  *******************************************************************/
	ScrollBar();
	Refresh();

}  /* end LeftDown() */

