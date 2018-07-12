// message.h

#ifndef IMESSAGE_H
#define IMESSAGE_H

struct IMessage {
	virtual IMessage *CreateMessage(const char *) = 0;
	virtual void Release() = 0;
	virtual bool Is(const char *code) = 0;
	virtual void   setCode(const char *code) = 0;
	virtual char  *getCode() = 0;
	virtual void   setPtr(const char *name, const void *ptr) = 0;
	virtual void  *getPtr(const char *name) = 0;
	virtual void   setStr(const char *name, const char *str) = 0;
	virtual char  *getStr(const char *name) = 0;
	virtual void   setInt(const char *name, int ipar) = 0;
	virtual int    getInt(const char *name) = 0;
	virtual void   setDbl(const char *name, double dpar) = 0;
	virtual double getDbl(const char *name) = 0;
	virtual void   Dump(char *txt, int n) = 0;
};

#endif
