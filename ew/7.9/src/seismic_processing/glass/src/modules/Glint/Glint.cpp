//glint.cpp: Glint interface service module
#include <windows.h>
#include <stdio.h>
#include "glint.h"
#include "str.h"
#include <Debug.h>

#define	MAXORG 2000
#define MAXPCK 30000
int		nPck = 0;
int		nOrg = 0;
ORIGIN	Org[MAXORG];
PICK	Pck[MAXPCK];

//---------------------------------------------------------------------------------------CGlint
CGlint::CGlint() {
	strcpy(sEntity, "Golden");
	iPickEntity = iOriginEntity = 0; //DK CHANGE 0.0b6 070203 changed to 0
	nMode = GLINT_MODE_NULL;  // DK CHANGE 060303 initialize nMode to NULL
  memset(pPLS,0,sizeof(pPLS));
}

//---------------------------------------------------------------------------------------~CGlint
CGlint::~CGlint() {
}

//---------------------------------------------------------------------------------------Entity
// Get sequential series of integers using entity.d for persistence
int CGlint::PickEntity() {
	FILE *f;
	char sent[40];
	int n;

#define szGLINT_PICK_ENTITY_FILENAME "unique_pick_sequence.glass"

	if(!iPickEntity) 
  {
		if(f = fopen(szGLINT_PICK_ENTITY_FILENAME, "rt")) 
    {
			n = fread(sent, 1, sizeof(sent), f);
			sent[n] = 0;
			iPickEntity = atoi(sent);
			fclose(f);
      f=NULL;
		} 
    else 
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"CGlint::PickEntity():  ERROR! Could not get Pick sequence number from file <%s>.  Using default!\n",
                  szGLINT_PICK_ENTITY_FILENAME);
			iPickEntity = 1000;
		}
	}
	iPickEntity++;

	if(f = fopen(szGLINT_PICK_ENTITY_FILENAME, "wt"))
  {
    itoa(iPickEntity, sent, 10);
    fwrite(sent, 1, strlen(sent), f);
    fclose(f);
    f=NULL;
  }
  else
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,"CGlint::PickEntity():  ERROR! Could not save Pick sequence number to file <%s>.  Using default!\n",
                szGLINT_PICK_ENTITY_FILENAME);
    exit(-1);
  }
	return iPickEntity;
}


//---------------------------------------------------------------------------------------Entity
// Get sequential series of integers using unique_origin_sequence.glass for persistence
int CGlint::OriginEntity() {
	FILE *f;
	char sent[40];
	int n;

#define szGLINT_ORIGIN_ENITITY_FILENAME "unique_origin_sequence.glass"

	if(!iOriginEntity) 
  {
		if(f = fopen(szGLINT_ORIGIN_ENITITY_FILENAME, "rt")) 
    {
			n = fread(sent, 1, sizeof(sent), f);
			sent[n] = 0;
			iOriginEntity = atoi(sent);
			fclose(f);
      f=NULL;
		} 
    else 
    {
      CDebug::Log(DEBUG_MINOR_ERROR,"OriginEntity():  ERROR! Could not get entity/Event sequence number from file <%s>.  Using default!\n",
                  szGLINT_ORIGIN_ENITITY_FILENAME);
			iOriginEntity = 1000;
		}
	}
	iOriginEntity++;

	if(f = fopen(szGLINT_ORIGIN_ENITITY_FILENAME, "wt"))
  {
    itoa(iOriginEntity, sent, 10);
    fwrite(sent, 1, strlen(sent), f);
    fclose(f);
    f=NULL;
  }
  else
  {
    CDebug::Log(DEBUG_MAJOR_ERROR,"OriginEntity():  ERROR! Could not save entity/Event sequence number to file <%s>.  Using default!\n",
                szGLINT_ORIGIN_ENITITY_FILENAME);
    exit(-1);
  }
	return iOriginEntity;
}

//---------------------------------------------------------------------------------------putOrigin
bool CGlint::putOrigin(ORIGIN *org) {
	if(org->iVersion != GLINT_VERSION)
		return false;

	CStr id;
	char *s = org->idOrigin;
	int i;
	int n = strlen(sEntity);
	id.Cat(OriginEntity(), 16);
	strcpy(s, id.GetBuffer());
	for(i=0; i<16; i++) {
		if(s[i] == ' ')
			s[i] = '0';
		if(i < n)
			s[i] = sEntity[i];
	}

	int iorg = nOrg%MAXORG;
	org->iOrigin = nOrg;
	org->nPh = 0;
  org->bIsValid = true;  // DK 012304

	memmove(&Org[iorg], org, sizeof(ORIGIN));
	nOrg++;
	return true;
}

//---------------------------------------------------------------------------------------putPick
bool CGlint::putPick(PICK *pck) {
	if(pck->iVersion != GLINT_VERSION)
		return false;

	CStr id;
	char *s = pck->idPick;
	int i;
	int n = strlen(sEntity);
	id.Cat(PickEntity(), 16);
	strcpy(s, id.GetBuffer());
	for(i=0; i<16; i++) {
		if(s[i] == ' ')
			s[i] = '0';
		if(i < n)
			s[i] = sEntity[i];
	}

	int ipck = nPck%MAXPCK;
	pck->iOrigin = -1;
	pck->iPick = nPck;
	pck->iState = GLINT_STATE_WAIF;
	pck->dTrav = 0.0;
	pck->dDelta = 0.0;
	pck->dAzm = 0.0;
	pck->dToa = 0.0;
	memmove(&Pck[ipck], pck, sizeof(PICK));
	nPck++;
	return true;
}


bool CGlint::deleteOrigin(char *ent) 
{
  ORIGIN *pOrigin = getOrigin(ent);
  PICK * pPick;

  if(!pOrigin)
    return(false);

    // Remove Phases
    size_t iPickRef=0;
    while(pPick = getPicksFromOrigin(pOrigin,&iPickRef))
    {
      // DK CLEANUP, move this functionality into UnassocPick() and
      // call it from both here, and from UnOriginPick()
      pOrigin->nPh--;
      pPick->iOrigin = -1;
      pPick->iState = GLINT_STATE_WAIF;
      pPick->dTrav = 0.0;
      pPick->dDelta = 0.0;
      pPick->dAzm = 0.0;
      pPick->dToa = 0.0;
    }
    pOrigin->bIsValid = false;

    return(true);
}

//---------------------------------------------------------------------------------------getOrigin
// This needs to be implemented with a hash table
ORIGIN *CGlint::getOrigin(char *ent) {
	int iorg;
	int iorg2 = nOrg;
	int iorg1 = iorg2 - MAXORG;
	if(iorg1 < 0)
		iorg1 = 0;
	for(iorg=iorg1; iorg<iorg2; iorg++) {
		if(!strcmp(ent, Org[iorg%MAXORG].idOrigin))
    {
      if(Org[iorg%MAXORG].bIsValid == true)
			  return(&Org[iorg%MAXORG]);
      else
        return(0);
    }
	}
	return 0;
}

//---------------------------------------------------------------------------------------getOrigin
// Scan origin list and return all origin falling withing given time interval
// Returns 0 when no more origins found
ORIGIN *CGlint::getOrigin(double t1, double t2) {
	ORIGIN *org;
	// Restart new scan if necessary
	if(nMode != GLINT_MODE_ORIGIN_T) {
		nMode = GLINT_MODE_ORIGIN_T;
		iScan = nOrg - MAXORG;
		if(iScan < 0)
			iScan = 0;
	}
	while(iScan < nOrg) {
		org = &Org[iScan%MAXORG];
		iScan++;
		if(org->dT > t1 && org->dT < t2  && org->bIsValid == true)
			return org;
	}
	nMode = GLINT_MODE_NULL;
	return 0;
}

//---------------------------------------------------------------------------------------getPick
// This needs to be implemented with a hash table
PICK *CGlint::getPickFromidPick(char *ent) {
	int ipck;
	int ipck2 = nPck;
	int ipck1 = ipck2 - MAXPCK;
	if(ipck1 < 0)
		ipck1 = 0;
	for(ipck=ipck1; ipck<ipck2; ipck++) {
		if(!strcmp(ent, Pck[ipck%MAXPCK].idPick))
			return &Pck[ipck%MAXPCK];
	}
	return 0;
}

//---------------------------------------------------------------------------------------getPick
// Scan pick list and return all picks falling withing given time interval
// Returns 0 when no more origins found
PICK *CGlint::getPicksForTimeRange(double t1, double t2, size_t *hRef)
{
	PICK *pck;
  PickListStruct * pPLI;

//  CDebug::Log(DEBUG_MINOR_ERROR,"getPicksForTimeRange(): called for pPLI(%u).\n",
//              *hRef);
  if(pPLI = (PickListStruct *)*hRef)
  {
    if(!ValidatePickList(pPLI))
    {
      *hRef = NULL;
      return(NULL);
    }
  }
  else
  {
    if(!(pPLI = getPickList()))
      return(NULL);
    *hRef = (size_t)pPLI;
  }

	while(pPLI->iCurrentPick < nPck) {
		pck = &Pck[pPLI->iCurrentPick%MAXPCK];
		pPLI->iCurrentPick++;
		if(pck->dT < t1)
			continue;
		if(pck->dT > t2)
			continue;
		return pck;
	}
  endPickList(hRef);
	return NULL;
}

//---------------------------------------------------------------------------------------getPick
// Scan pick list and return all unassociated picks falling withing given time interval
// Returns 0 when no more origins found
PICK *CGlint::getWaifsForTimeRange(double t1, double t2, size_t *hRef)
{
	PICK *pck;
  PickListStruct * pPLI;

//  CDebug::Log(DEBUG_MINOR_ERROR,"getWaifsForTimeRange(): called for pPLI(%u).\n", *hRef);

  if(pPLI = (PickListStruct *)*hRef)
  {
    if(!ValidatePickList(pPLI))
    {
      *hRef = NULL;
      return(NULL);
    }
  }
  else
  {
    if(!(pPLI = getPickList()))
      return(NULL);
    *hRef = (size_t)pPLI;
  }

	while(pPLI->iCurrentPick < nPck) {
		pck = &Pck[pPLI->iCurrentPick%MAXPCK];
		pPLI->iCurrentPick++;

		if(pck->iState != GLINT_STATE_WAIF)
			continue;
		if(pck->dT < t1)
			continue;
		if(pck->dT > t2)
			continue;
		return pck;
	}
  endPickList(hRef);
//  CDebug::Log(DEBUG_MINOR_ERROR,"End getWaifsForTimeRange(%u).\n",   *hRef);
	return NULL;
} // end getWaifsForTimeRange()



//---------------------------------------------------------------------------------------getPick
// Scan pick list and return all with the specified origin
// Returns 0 when no more origins found
PICK *CGlint::getPicksFromOrigin(ORIGIN *org, size_t *hRef)
{
	PICK *pck;
  PickListStruct * pPLI;

//  CDebug::Log(DEBUG_MINOR_ERROR,"getPicksFromOrigin(): called for pPLI(%u).\n",   *hRef);

  if(!(org->bIsValid == true))
  {
    endPickList(hRef);
    return(NULL);
  }

  if(pPLI = (PickListStruct *)*hRef)
  {
    if(!ValidatePickList(pPLI))
    {
      *hRef = NULL;
      return(NULL);
    }
  }
  else
  {
    if(!(pPLI = getPickList()))
      return(NULL);
    *hRef = (size_t)pPLI;
  }

	while(pPLI->iCurrentPick < nPck) {
		pck = &Pck[pPLI->iCurrentPick%MAXPCK];
		pPLI->iCurrentPick++;

		if(!((pck->iState == GLINT_STATE_ASSOC) && (pck->iOrigin == org->iOrigin)))
			continue;
		return pck;
	}
  endPickList(hRef);
	return NULL;
} // end getPicksFromOrigin()

//---------------------------------------------------------------------------------------OriginPick
bool CGlint::OriginPick(char *idorg, char *idpck) {
	ORIGIN *org = getOrigin(idorg);
	if(!org)
		return false;
	PICK *pck = getPickFromidPick(idpck);
	if(!pck)
		return false;
  if(pck->iState == GLINT_STATE_ASSOC)
    CDebug::Log(DEBUG_MINOR_ERROR, 
                "OriginPick(): Pick(%s) already associated "
                "with Origin(%d).  Trying to assoc with Origin(%s/%d).\n",
                idpck, pck->iOrigin, idorg, org->iOrigin);


  org->nPh++;
	pck->iOrigin = org->iOrigin;
	pck->iState = GLINT_STATE_ASSOC;
  CDebug::Log(DEBUG_MINOR_INFO, "PGL:OP: Pick(%.2f %.2f %.0f - %.1f) Origin(%.2f/%.2f/%.0f - %.2f)\n",
              pck->dTrav, pck->dDelta, org->dZ, pck->dT - org->dT - pck->dTrav, 
              org->dLat, org->dLon, org->dZ, org->dT);

//	return false;
  // DK Changed to return true;
  return true;
}

//---------------------------------------------------------------------------------------Pau
bool CGlint::UnOriginPick(char *org, char *idpck)
{

  PICK *pck = getPickFromidPick(idpck);
  ORIGIN * pOrigin;
	if(!pck)
		return false;

  pOrigin=getOriginForPick(pck);
  if(!pOrigin)
    return(false);

  CDebug::Log(DEBUG_MAJOR_INFO, "PGL:UOP: Pick(%.2f %.2f %.0f - %.1f) Origin(%.2f/%.2f/%.0f - %.2f)\n",
              pck->dTrav, pck->dDelta, pOrigin->dZ, pck->dT - pOrigin->dT - pck->dTrav, 
              pOrigin->dLat, pOrigin->dLon, pOrigin->dZ, pOrigin->dT);

  pOrigin->nPh--;
	pck->iOrigin = -1;
	pck->iState = GLINT_STATE_WAIF;
	pck->dTrav = 0.0;
	pck->dDelta = 0.0;
	pck->dAzm = 0.0;
	pck->dToa = 0.0;
	return true;
}

/*
ORIGIN * CGlint::getOriginFromNum(int iOriginNum)
{
  	int iorg = iOriginNum%MAXORG;
    return(&Org[iorg]);

}
*/

//---------------------------------------------------------------------------------------Pau
ORIGIN * CGlint::getOriginForPick(PICK * pPick)
{
  int iorg;

  if(pPick->iState != GLINT_STATE_ASSOC)
    return(NULL);

  iorg = pPick->iOrigin%MAXORG;
  if(Org[iorg].bIsValid == true)
    return(&Org[iorg]);
  else 
    return(NULL);
}

//---------------------------------------------------------------------------------------Pau
int CGlint::ComparePickChannels(PICK * pck, PICK * pPick)
{
  int iRes;

  if(iRes=strcmp(pck->sSite, pPick->sSite))
    return(iRes);
  if(iRes=strcmp(pck->sNet, pPick->sNet))
    return(iRes);
  if(iRes=strcmp(pck->sComp, pPick->sComp))
    return(iRes);
  return(iRes=strcmp(pck->sLoc, pPick->sLoc));
}

//---------------------------------------------------------------------------------------Pau
// Terminate scan before end of list encountered. This is necessary in case another
// scan of the same type is started immediately.
void CGlint::Pau() {
	nMode = GLINT_MODE_NULL;
}

//--------------------------------------------------------------------------------endPickList
// Terminate scan before end of list encountered. This is necessary in case search is stopped
void CGlint::endPickList(size_t * piPickRef)
{
//  CDebug::Log(DEBUG_MINOR_ERROR,"endPickList(): called for pPLI(%u).\n",
//              *piPickRef);
  if(*piPickRef == NULL)
    return;
  PickListStruct * pPLI = (PickListStruct *)*piPickRef;
    if(ValidatePickList(pPLI))
      pPLI->bValid = false;

  *piPickRef = NULL;
  return;
}


//--------------------------------------------------------------------------------getPickList
// Get a PickList reference
PickListStruct * CGlint::getPickList()
{
  int i;

//  CDebug::Log(DEBUG_MAJOR_INFO,"getPickList():  Executing:  ");
  for(i=0; i < GLINT_PICKLIST_SIZE; i++)
  {
    if(i>1)
    {
      i=i;  // DK DEBUG;
    }

    if(!(pPLS[i].bValid))
    {
      pPLS[i].bValid = true;
      pPLS[i].iCurrentPick = nPck - MAXPCK;
  		if(pPLS[i].iCurrentPick < 0)
	  		pPLS[i].iCurrentPick = 0;
//      CDebug::Log(DEBUG_MAJOR_INFO,"returning (%d)\n",i);
      return(&(pPLS[i]));
    }
  }
  CDebug::Log(DEBUG_MINOR_ERROR,"getPickList():  All pick lists already "
                                "in use. BAD!\n");
  return(NULL);
}

//--------------------------------------------------------------------------------ValidatePickList
// Get a PickList reference
BOOL CGlint::ValidatePickList(PickListStruct * pPLI)
{
  if(!(pPLI >= pPLS  && pPLI < (pPLS + GLINT_PICKLIST_SIZE)))
    CDebug::Log(DEBUG_MINOR_ERROR,"ValidatePickList():  pPLI(%u) was not valid(range (%u-%u).\n",
                pPLI,pPLS,&(pPLS[GLINT_PICKLIST_SIZE]));

  return(pPLI >= pPLS  && pPLI < (pPLS + GLINT_PICKLIST_SIZE));
}

