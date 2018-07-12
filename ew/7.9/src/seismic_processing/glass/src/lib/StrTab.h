#ifndef _STRTAB_H_
#define _STRTAB_H_
#include "array.h"
#include "str.h"

class CStrEnt {
public:
// Attributes
	char *sName;
	char *sValue;

// Methods
	CStrEnt();
	virtual ~CStrEnt();
};

class CStrTab {
// Attributes
public:
	CArray StrTab;

// Methods
	CStrTab();
	virtual ~CStrTab();
	virtual void Add(char *name, char *sval);
	virtual char *Get(char *name);
};

#endif
