#include "INexus.h"
#include "message.h"

struct IModule {
	virtual bool Initialize(INexus *nexus, IMessage *) = 0;
	virtual bool Poll() = 0;
	virtual void setName(char *name) = 0;
	virtual char *getName() = 0;
	virtual bool Action(IMessage *msg) = 0;
	virtual void Release() = 0;
};