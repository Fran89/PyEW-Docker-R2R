// ring.h: Transport protocol wrapper
#ifndef _RING_H_
#define _RING_H_

#define MAXLOGO	100
#define MAXMSG	4096

extern "C" {
#include "transport.h"
}

#define CRING_NOT_ATTACHED -1
#define CRING_OK PUT_OK
class CRing {
// Attributes
public:
	bool		bAttach;
	SHM_INFO	shmRing;
	int			iPid;
	int			nLogo;
	MSG_LOGO	pLogo[MAXLOGO];
	MSG_LOGO	inLogo;
	long		nMsg;
	char		cMsg[MAXMSG];
	unsigned char	cSeq;

// Operations
public:
	CRing();
	virtual ~CRing();
	bool Attach(int key);
	static long Time();
	void Logo(int type, int mod, int inst);
	int Get();
	int Put(MSG_LOGO *logo, char *msg);
	int PutStatus(MSG_LOGO *logo, int iType, int iStatus, const char *msg);
};

#endif