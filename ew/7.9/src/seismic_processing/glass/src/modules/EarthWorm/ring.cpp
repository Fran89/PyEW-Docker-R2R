// ring.cpp: Transport protocol wrapper
//
#include "ring.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <Debug.h>

// Default constructuror
CRing::CRing() {
	bAttach = false;
	nLogo = 0;
	iPid = getpid();
}

// Default destructor
CRing::~CRing() {
	if(bAttach)
		tport_detach(&shmRing);
}

long CRing::Time() {
	time_t t;
	time(&t);
	return (long)t;
}

// Attach to an existing EW transport ring
bool CRing::Attach(int key) {
	tport_attach(&shmRing, key);
	bAttach = true;
	return true;
}

// Add logo to input filter
void CRing::Logo(int type, int mod, int inst) {
	if(nLogo >= MAXLOGO)
		return;
	pLogo[nLogo].type = (unsigned char)type;
	pLogo[nLogo].mod = (unsigned char)mod;
	pLogo[nLogo].instid = (unsigned char)inst;
	nLogo++;
}

// Retrieve message
int CRing::Get() {
	int res;
  if(!bAttach)
    return(CRING_NOT_ATTACHED);
	if(tport_getflag(&shmRing) == TERMINATE
	|| tport_getflag(&shmRing) == iPid)
		return TERMINATE;
	res = tport_copyfrom(&shmRing, pLogo, nLogo, &inLogo, &nMsg, cMsg, MAXMSG-1, &cSeq);
  if(res == GET_NOTRACK)
  {
 		  CDebug::Log(DEBUG_MAJOR_ERROR, "ERROR:  tport_copyfrom returned GET_NOTRACK(too many logos), when called for %d logos.\n",
                  nLogo);
		  Sleep(5000);
  }
	return res;
}

// Post message to output ring
int CRing::Put(MSG_LOGO *logo, char *msg) {
	int res;
  if(!bAttach)
    return(CRING_NOT_ATTACHED);
	res = tport_putmsg(&shmRing, logo, (long)strlen(msg)+1, msg);
	return res;
}

// Post status message to output ring
int CRing::PutStatus(MSG_LOGO *logo, int iType, int iStatus, const char *IN_msg)
{
  char msg[256];
	int res;
  time_t t;

	time( &t );
	
	_snprintf(msg, sizeof(msg), "%ld %hd %s\n\0", (long)t, iStatus, IN_msg);
  msg[sizeof(msg)-1]=0x00;
		
	int size = (int)strlen( msg );   /* don't include the null byte in the message */
	
							/* Write the message to shared memory
	************************************/
	if( res=Put(logo,msg) != PUT_OK )
	{
    GlobalDebug::DebugVA("et", "ERROR! EarthwormMod: PutStatus()  Could not write status to wring. error:(%d)\n", 
				                 res );
	}
	
	return(res);
}

