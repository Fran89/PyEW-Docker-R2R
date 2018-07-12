// nexus.h

#ifndef NEXUS_H
#define NEXUS_H

#include "INexus.h"
#include "array.h"

struct IModule;
struct IMessage;
class CMessage;
class CStatus;
class CNexus : public INexus {
public:
// Attributes
	bool bMonitor;
	HINSTANCE hInst;
	CStatus *pStat;
	CArray arrMod;
	int nPoll;
	IModule *pPoll[100];

// Methods
	CNexus();
	~CNexus();
	void *Alloc(int n);
	void Free(void *p);
	bool Add(IModule *mod);
	IModule *Load(char *name);
	bool Idle();
	bool Dispatch(IMessage *msg);
	bool Broadcast(IMessage *msg);
	void Dump(char *txt);
	void Dump(char code, char *modnam, IMessage *msg);
};

#endif
