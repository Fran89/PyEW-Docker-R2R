#include <windows.h>
#include <stdio.h>
#include "ManQuakeScroll.h"
#include "IGlint.h"
#include "ManQuakeMod.h"
#include "date.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}
#include <opcalc.h>
#include <math.h>
#include <time_ew.h>

extern CMod *pMod;


static PICK * ppArray[MAX_PHASES];

static int PickDistComp(const void *elem1, const void *elem2) {
	PICK *pck1;
	PICK *pck2;
	pck1 = *(PICK **)elem1;
	pck2 = *(PICK **)elem2;
//	CDebug::Log(DEBUG_MINOR_INFO,"Comp() %.2f %.2f\n", pck1->dDelta, pck2->dDelta);
	if(pck1->dDelta < pck2->dDelta)
		return -1;
	if(pck1->dDelta > pck2->dDelta)
		return 1;
	return 0;
}

static int ComparePickAzm(const void *elem1, const void *elem2) {
	PICK *pck1;
	PICK *pck2;
	pck1 = *(PICK **)elem1;
	pck2 = *(PICK **)elem2;
	if(pck1->dAzm < pck2->dAzm)
		return -1;
	if(pck1->dAzm > pck2->dAzm)
		return 1;
	return 0;
}


//---------------------------------------------------------------------------------------CScroll
CScroll::CScroll() {
	hFont = 0;
	hText = 18;
	pGlint = 0;
	sCat[0] = 0;
	sAdd[0] = 0;
	sFer[0] = 0;
	nPhs = 0;
	memset(&Origin, 0, sizeof(Origin));

  // DK CLEANUP hack to set window size
  this->iX = 0;
	this->iY = 400;
	this->nX = 885;
	this->nY = 340;
}

//---------------------------------------------------------------------------------------~CScroll
CScroll::~CScroll() {
	if(hFont)
		DeleteObject(hFont);
  if(hwndButton)
    DestroyWindow(hwndButton);
}

void CScroll::CreateControls() 
{

hwndEditLat = CreateWindow( 
    "EDIT",   // predefined class 
    "XX.XX",       // button text 
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | WS_TABSTOP,  // styles 
 
    // Size and position values are given explicitly, because 
    // the CW_USEDEFAULT constant gives zero values for buttons. 
    265,         // starting x position 
    10,         // starting y position 
    60,        // edit width 
    20,        // edit height 
    hWnd,       // parent window 
    NULL,       // No menu 
    (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    NULL);      // pointer not needed 

hwndEditLon = CreateWindow( 
    "EDIT",   // predefined class 
    "XXX.XX",       // button text 
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | WS_TABSTOP,  // styles 
 
    // Size and position values are given explicitly, because 
    // the CW_USEDEFAULT constant gives zero values for buttons. 
    375,         // starting x position 
    10,         // starting y position 
    60,        // edit width 
    20,        // edit height 
    hWnd,       // parent window 
    NULL,       // No menu 
    (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    NULL);      // pointer not needed 

hwndEditDepth = CreateWindow( 
    "EDIT",   // predefined class 
    "XXX",       // button text 
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,  // styles 
 
    // Size and position values are given explicitly, because 
    // the CW_USEDEFAULT constant gives zero values for buttons. 
    505,         // starting x position 
    10,         // starting y position 
    35,        // edit width 
    20,        // edit height 
    hWnd,       // parent window 
    NULL,       // No menu 
    (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    NULL);      // pointer not needed 

hwndEditTime = CreateWindow( 
    "EDIT",   // predefined class 
    "YYYYMMDDhhmmss.dd",       // button text 
    WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,  // styles 
 
    // Size and position values are given explicitly, because 
    // the CW_USEDEFAULT constant gives zero values for buttons. 
    600,         // starting x position 
    10,         // starting y position 
    150,        // edit width 
    20,        // edit height 
    hWnd,       // parent window 
    NULL,       // No menu 
    (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    NULL);      // pointer not needed 


hwndButton = CreateWindow( 
    "BUTTON",   // predefined class 
    "Associate",       // button text 
    WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // styles 
 
    // Size and position values are given explicitly, because 
    // the CW_USEDEFAULT constant gives zero values for buttons. 
    770,         // starting x position 
    10,         // starting y position 
    70,        // button width 
    20,        // button height 
    hWnd,       // parent window 
    NULL,       // No menu 
    (HINSTANCE) GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
    NULL);      // pointer not needed 



} 
//---------------------------------------------------------------------------------------Init
void CScroll::Init(HINSTANCE hinst, char *title) {
	iStyle |= WS_VSCROLL;
	CWin::Init(hinst, title);
  CreateControls();
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
	nScr = h / hText - 7;
	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------Draw
void CScroll::Draw(HDC hdc) {
	char txt[128];
	RECT r;
	int h;
	int ipos;
	int ilst;
	int i;

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

	h = 10;
	ipos = GetScrollPos(hWnd, SB_VERT);
	ilst = ipos + nScr;
	if(ilst > nPhs)
		ilst = nPhs;

  sprintf(txt, "Enter a trial Origin: Lat %5s  Lon %6s Depth %3s Time %10s",
         "","","","");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText * 1.5;

  
  strcpy(txt, " Seq Origin Time           Latitude  Longitude  Depth Neq   RMS Gap   Aff AfGp AfNeq");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;
	TextOut(hdc, 10, h, sCat, strlen(sCat));
	h += hText;
	TextOut(hdc, 10, h, sAdd, strlen(sAdd));
	h += 2*hText;
	strcpy(txt, "nPh Sta Cmp Nt Lc Phase Delta Azm Toa   Res DeltaT  Aff Dis/Res/PPD  Org  Tag");
	TextOut(hdc, 10, h, txt, strlen(txt));
	h += hText;

	for(i=ipos; i<ilst; i++) {
    // if this pick already belongs to an origin, then color it red
    if(ppArray[i]->iOrigin >= 0)
      SetTextColor(hdc, RGB(255, 0, 0));

		sprintf(txt, "%3d %-5s%-4s%-3s%-3s%-5s %5.1f %3.0f %3.0f %5.2f %6.1f %4.1f %3.1f/%3.1f/%3.1f %4d %s",
			i, ppArray[i]->sSite, ppArray[i]->sComp, ppArray[i]->sNet, ppArray[i]->sLoc, ppArray[i]->sPhase,
			ppArray[i]->dDelta, ppArray[i]->dAzm, ppArray[i]->dToa, ppArray[i]->tRes, ppArray[i]->dT - Origin.dT, 
      ppArray[i]->dAff, ppArray[i]->dAffDis, ppArray[i]->dAffRes, ppArray[i]->dAffPPD, 
      ppArray[i]->iOrigin,ppArray[i]->sTag);  // DK 072803
		TextOut(hdc, 10, h, txt, strlen(txt));
		h += hText;

    // Set color back to black
    if(ppArray[i]->iOrigin >= 0)
			SetTextColor(hdc, RGB(0, 0, 0));
	}

}

//---------------------------------------------------------------------------------------Quake
void CScroll::Quake(char *ent) {
	IMessage *m;
	PICK *pck;
  ORIGIN *org;
	char *str;
	int code;

	if(!pGlint)
		return;

	if(!(org = pGlint->getOrigin(ent)))
  {
    // Must be a deleted quake.  Do nothing
    return;
  }

  CDate dt(org->dT);

	sprintf(sCat, "%4d %s %9.4f %10.4f %6.2f%4d %5.2f %3.0f %5.2f %4.2f  %4.2f", org->iOrigin,
		dt.Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
		org->nEq, org->dRms, org->dGap, org->dAff, org->dAffGap, org->dAffNumArr);
	m = pMod->CreateMessage("FlinnEngdahl");
	m->setDbl("Lat", org->dLat);
	m->setDbl("Lon", org->dLon);
	if(pMod->Dispatch(m)) {
		code = m->getInt("Code");
		str = m->getStr("Region");
		if(str)
			sprintf(sFer, "%d:%s", code, str);
	}
	m->Release();
	sprintf(sAdd, "     Delta(%.1f %.1f %.1f)  Flynn-Engdahl:%s",
		org->dDelMin, org->dDelMod, org->dDelMax, sFer);

	nPhs = 0;
  size_t iPickRef=0;
	while(nPhs < MAX_PHASES) {
		pck = pGlint->getPicksFromOrigin(org, &iPickRef);
		CDebug::Log(DEBUG_MINOR_INFO,"Quake():%s %d %d\n", ent, nPhs, pck);
		if(!pck)
			break;
    if(!pck->bTTTIsValid)
       continue;
    memcpy(&Phs[nPhs],pck,sizeof(PICK));
		nPhs++;
	}
  pGlint->endPickList(&iPickRef);

  if(nPhs > 0)
    qsort(Phs, nPhs, sizeof(Phs[nPhs]), PickDistComp);


	ScrollBar();
	Refresh();
}

//---------------------------------------------------------------------------------------ScrollBar
// Calculate and instantiate appropriate scroll bar parameters
void CScroll::ScrollBar() {
	CDebug::Log(DEBUG_MINOR_INFO,"ScrollBar():    nPhs:%d nScr:%d\n", nPhs, nScr);
	if(nPhs <= nScr) {
		SetScrollRange(hWnd, SB_VERT, 0, 100, FALSE);
		SetScrollPos(hWnd, SB_VERT, 0, FALSE);
		ShowScrollBar(hWnd, SB_VERT, FALSE);
		return;
	}
	SetScrollRange(hWnd, SB_VERT, 0, nPhs-nScr, FALSE);
	SetScrollPos(hWnd, SB_VERT, 0, FALSE);
	ShowScrollBar(hWnd, SB_VERT, TRUE);
}

//---------------------------------------------------------------------------------------VScroll
void CScroll::VScroll(int code, int pos) {
	CDebug::Log(DEBUG_MINOR_INFO,"VScroll():Scroll %d %d\n", code, pos);
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

//---------------------------------------------------------------------------------ComparePickAff
int ComparePickAff(const void * pv1, const void * pv2)
{
  PICK * p1 = *(PICK **)pv1;
  PICK * p2 = *(PICK **)pv2;
  
  if(!p1->bTTTIsValid && !p2->bTTTIsValid)
    return(0);
  if(!p1->bTTTIsValid)
    return(1);
  if(!p2->bTTTIsValid)
    return(-1);
  if(p1->dAff > p2->dAff)
    return(-1);
  else if(p1->dAff == p2->dAff)
    return(0);
  else 
    return(1);
}



static int ProcessLocation(bool solve, ORIGIN * pOrg, PICK ** pPick, int nPck);

//---------------------------------------------------------------------------------------LeftDown
void CScroll::LeftDown(int x, int y) 
{
}



// Function called by Iterate to calculate Origin and Origin/Link 
// params, and to suggest better locations.
static int ProcessLocation(bool solve, ORIGIN * pOrg, PICK ** pPick, int nPck) 
{

  // Initialize origin vars
	int nEq = 0;
  int i;

	double sum = 0.0;
  PICK * pck;
  int iRetCode;
  double dGap;

  // Set default RMS to absurd level.  Shouldn't this be an affinity setting.
  pOrg->dRms = 10000.0;


//  CDebug::Log(DEBUG_MAJOR_INFO,"Locate(I): Org(%6.3f,%8.3f,%5.1f - %.2f)\n",
//              pOrg->dLat, pOrg->dLon, pOrg->dZ, pOrg->dT);

	for(i=0; i<nPck; i++) 
  {
		pck = pPick[i];
    iRetCode = OPCalc_CalculateOriginPickParams(pck,pOrg);
    if(iRetCode == -1)
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"ProcessLocation(): OPCalc_CalculateOriginPickParams  failed with "
                                    "terminal error.  See logfile.  Returning!\n");
      return(-1);
    }
    else if(iRetCode == 1)
    {
      // could not find TravelTimeTable entry for pOrg,pck
      CDebug::Log(DEBUG_MINOR_INFO,"ProcessLocation(): OPCalc_CalculateOriginPickParams() could not "
                                    "find TTT entry for Org,Pick!\n");
      continue;
    }

    // calculate solve coefficients if desired
    //if(solve)
    //  AddPickCoefficientsForSolve(pck, pOrg);
    
    // accumulate statistics
    sum += pck->tRes*pck->tRes;

    // increment the number of picks used to locate the origin
    nEq++;
    
    // keep the arrays from being overrun
    if(nEq > OPCALC_MAX_PICKS)
      break;
  }  // end for each pick in the Pick Array

  // if we didn't use atleast one pick.  Give up and go home.
  if(nEq < 1)
	  return(1);


  // Record the number of equations used to calc the location
  pOrg->nEq = nEq;

  // Record the residual standard deviation
  pOrg->dRms = sqrt(sum/nEq);

  /* Calculate Pick Azimuth statistics for the Origin (Gap)
   ****************************************************/
  // Sort the pick-azimuth array
  qsort(pPick, nPck, sizeof(PICK *), ComparePickAzm);

  // Calculate the gap for the last-to-first wrap
 	dGap = pPick[0]->dAzm + 360.0 - pPick[nEq-1]->dAzm;
 	for(i=0; i<nEq-1; i++) 
  {
    // for each gap between two picks, record only if largest so far
    if(pPick[i+1]->dAzm - pPick[i]->dAzm > dGap)
      dGap = pPick[i+1]->dAzm - pPick[i]->dAzm;
 	}
 	pOrg->dGap = dGap;


  /* Calculate Pick Distance statistics for the Origin
   ****************************************************/
  // Sort the pick-distance array
  qsort(pPick, nPck, sizeof(PICK *), PickDistComp);

  // Calculate the minimum station distance for the origin
	pOrg->dDelMin = pPick[0]->dDelta;

  // Calculate the maximum station distance for the origin
	pOrg->dDelMax = pPick[nEq-1]->dDelta;

  // Calculate the median station distance for the origin
	pOrg->dDelMod = 0.5*(pPick[nEq/2]->dDelta + pPick[(nEq-1)/2]->dDelta);


  /* Calculate the Affinity for the Origin (and Picks) 
   ****************************************************/
  iRetCode = OPCalc_CalcOriginAffinity(pOrg, pPick, nPck);
  if(iRetCode)
  {
    CDebug::Log(DEBUG_MINOR_ERROR,
                "Error: ProcessLocation(): OPCalc_CalcOriginAffinity() return (%d)\n",
                iRetCode);
    return(iRetCode);
  }
     
  CDebug::Log(DEBUG_MINOR_INFO,"END Locate(): dRms:%.2f nEq:%d\n",
              pOrg->dRms, nEq);
  return(0);
}  // End Process Location()


bool CScroll::Message(HWND hwnd, UINT mess, WPARAM wparam, LPARAM lparam) 
{
  HWND hwndLP;

  hwndLP = (HWND) lparam;
  int wNotifyCode = HIWORD(wparam); // notification code 


	switch(mess) {
	case WM_COMMAND:

    if(this->hwndButton == hwndLP)
    {
      AssociateLocation();
      break;
    }
  default:
    break;
  }

 
	return true;
}

bool CScroll::GetLocationParams(double * pdLat, double * pdLon, double * pdZ, double * pdTime)
{

  char szBuffer[60];
  char szTemp[6];
  int rc;
//  int wParam = (WPARAM)sizeof(szBuffer);
//  int lParams = (LPARAM)szBuffer;

  //rc = SendMessage(hwndEditLat, WM_GETTEXT, wParam, lParam);

  rc=GetWindowText(hwndEditLat, szBuffer, sizeof(szBuffer)-1);
  szBuffer[rc]=0x00;
  *pdLat = atof(szBuffer);

  rc=GetWindowText(hwndEditLon, szBuffer, sizeof(szBuffer)-1);
  szBuffer[rc]=0x00;
  *pdLon = atof(szBuffer);

  rc=GetWindowText(hwndEditDepth, szBuffer, sizeof(szBuffer)-1);
  szBuffer[rc]=0x00;
  *pdZ = atof(szBuffer);

  rc=GetWindowText(hwndEditTime, szBuffer, sizeof(szBuffer)-1);
  szBuffer[rc]=0x00;

  if(strlen(szBuffer) < 14)
  {
    *pdTime = 0.00;
  }
  else
  {
    struct tm tmTime;
    memset(&tmTime, 0, sizeof(tmTime));

    strncpy(szTemp,&szBuffer[0],4); // YR
    szTemp[4] = 0x00;
    tmTime.tm_year = atoi(szTemp) - 1900;
    
    strncpy(szTemp,&szBuffer[4],2); // MO
    szTemp[2] = 0x00;
    tmTime.tm_mon = atoi(szTemp) - 1;
    
    strncpy(szTemp,&szBuffer[6],2); // DAY
    szTemp[2] = 0x00;
    tmTime.tm_mday = atoi(szTemp);
    
    strncpy(szTemp,&szBuffer[8],2); // HR
    szTemp[2] = 0x00;
    tmTime.tm_hour = atoi(szTemp);
    
    strncpy(szTemp,&szBuffer[10],2); // MIN
    szTemp[2] = 0x00;
    tmTime.tm_min = atoi(szTemp);
    
    strncpy(szTemp,&szBuffer[12],5); // SEC
    szTemp[5] = 0x00;
    tmTime.tm_sec = (int)atof(szTemp);

    // force UTC Time  
    _putenv("TZ=UTC+0");
    *pdTime = timegm_ew(&tmTime);  // convert to time_t
    *pdTime += atof(szTemp) - tmTime.tm_sec;  /* add the partial sec back in. */

  }

       

  if(*pdLat || *pdLon || *pdZ || *pdTime)
    return(true);
  else
    return(false);
}


void CScroll::AssociateLocation()
{
  int rc;
  int i;
  ORIGIN * org = &Origin;

  memset(&Origin, 0, sizeof(Origin));

  rc = GetLocationParams(&Origin.dLat, &Origin.dLon, &Origin.dZ, &Origin.dT);

  // If we couldn't get parameters, return
  if(!rc)
    return;

  nPhs = 0;
  
  // Read Lat/Lon/Depth/Time values
  
  // Get Phases for those times
  size_t iPickRef=0;
  PICK * pck;
  while(pck=pGlint->getPicksForTimeRange(Origin.dT - 20, Origin.dT + 1680, &iPickRef) )
  {
    if(!pck)
      break;
    if(nPhs >= MAX_PHASES)
      break;
    memcpy(&Phs[nPhs],pck,sizeof(PICK));
    ppArray[nPhs] = &Phs[nPhs];
    // Mark All picks as waif's.  Mark true waifs with iOrigin = 1
    if(Phs[nPhs].iState == GLINT_STATE_WAIF)
    {
      Phs[nPhs].iOrigin = -1;
    }
    else
    {
      Phs[nPhs].iState = GLINT_STATE_WAIF;
    }
    nPhs++;
  }
  pGlint->endPickList(&iPickRef);
  
  Origin.nEq  = Origin.nPh = nPhs;
  Origin.dDelMod = 180.0;
  
  if(!nPhs)
  {
    strcpy(sCat, "No phases available!");
    ScrollBar();
    Refresh();
    return;
  }
  rc = OPCalc_CalcOriginAffinity(&Origin, ppArray, nPhs);
  
  qsort(ppArray, nPhs, sizeof(PICK *), ComparePickAff);
  
  for(i=0; ppArray[i]->bTTTIsValid && ppArray[i]->dAff >= OPCALC_dAffMinAssocValue; i++)
  {
    if(i == nPhs)
      break;
  }
  nPhs = Origin.nEq = i;
  
  // Calculate parameters for Origin
  rc = ProcessLocation(false /*bSolve*/, &Origin, ppArray, nPhs);
  
  CDate dt(org->dT);
  
  sprintf(sCat, "%4d %s %9.4f %10.4f %6.2f%4d %5.2f %3.0f %5.2f %4.2f  %4.2f", org->iOrigin,
    dt.Date20().GetBuffer(), org->dLat, org->dLon, org->dZ,
    org->nEq, org->dRms, org->dGap, org->dAff, org->dAffGap, org->dAffNumArr);

	char *str;
	int code;
	IMessage *m;


	m = pMod->CreateMessage("FlinnEngdahl");
	m->setDbl("Lat", org->dLat);
	m->setDbl("Lon", org->dLon);
	if(pMod->Dispatch(m)) {
		code = m->getInt("Code");
		str = m->getStr("Region");
		if(str)
			sprintf(sFer, "%d:%s", code, str);
	}
	m->Release();
	sprintf(sAdd, "     Delta(%.1f %.1f %.1f)  Flynn-Engdahl:%s",
		org->dDelMin, org->dDelMod, org->dDelMax, sFer);

  
  ScrollBar();
  Refresh();
}


