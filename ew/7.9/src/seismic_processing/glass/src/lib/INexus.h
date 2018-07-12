// INexus.h

#ifndef _INEXUS_H_
#define _INEXUS_H_

struct IMessage;
struct INexus {
public:
	virtual bool Dispatch(IMessage *msg) = 0;
	virtual bool Broadcast(IMessage *msg) = 0;
	virtual void Dump(char *txt) = 0;
	virtual void Dump(char code, char *modnam, IMessage *msg) = 0;
};

#endif
