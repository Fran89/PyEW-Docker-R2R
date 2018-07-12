// message.h

#ifndef MESSAGE_H
#define MESSAGE_H

#include "IMessage.h"
#include "array.h"

class CPar {
// Attributes
public:
	int	iType;	// Parameter type (string, int, ...)
	char *sPar;	// Parameter name
	void *pVal;	// Parameter value

// Methods
	CPar();
	~CPar();
};

class CMessage : public IMessage {
// Attributes
public:
	int iCode;
	int	iSeq;		// Message sequencer
	char *sCode;
	CArray arrPar;

// Methods
public:
	CMessage();
	CMessage(const char *code);
	~CMessage();
	IMessage *CreateMessage(const char *code);
	void Release();
	bool Is(const char *code);
	void   setCode(const char *code);
	char  *getCode();
	void   setPtr(const char *name, const void *ptr);
	void  *getPtr(const char *name);
	void   setStr(const char *name, const char *str);
	char  *getStr(const char *name);
	void   setInt(const char *name, int ipar);
	int    getInt(const char *name);
	void   setDbl(const char *name, double dpar);
	double getDbl(const char *name);
	void   Dump(char *txt, int n);
private:
	CPar *FindPar(const char *name);
};

#endif
