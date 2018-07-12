// module.h

#ifndef MODULE_H
#define MODULE_H

#include "IModule.h"
#include "message.h"
#include "Debug.h"

class CModule : public IModule {
// Attributes
public:
	INexus *pNexus;
	IMessage *pMessage;
	char sName[16];

// Methods
public:
	CModule();
	virtual ~CModule();
	bool Initialize(INexus *nexus, IMessage *msg);
	bool Poll();
	void setName(char *name);
	char *getName();
	virtual bool Action(IMessage *msg);
	virtual void Release();
	IMessage *CreateMessage(char *code);
	bool Dispatch(IMessage *msg);
	bool Broadcast(IMessage *msg);

protected:
  CDebug Debug;
};

#endif
