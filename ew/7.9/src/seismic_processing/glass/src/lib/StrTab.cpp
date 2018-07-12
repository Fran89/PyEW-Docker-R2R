// StrTab.cpp
#include <string.h>
#include "StrTab.h"

// 20020606 cej Created.
// 20020607 cej Replace value if string pair already defined

//---------------------------------------------------------------------------------------CStrEnt
CStrEnt::CStrEnt() {
	sName = 0;
	sValue = 0;
}

//---------------------------------------------------------------------------------------~CStrEnt
CStrEnt::~CStrEnt() {
	if(sName)
		delete [] sName;
	if(sValue)
		delete [] sValue;
}

//---------------------------------------------------------------------------------------CStrTab
CStrTab::CStrTab() {
}

//---------------------------------------------------------------------------------------~CStrTab
CStrTab::~CStrTab() {
	for(int i=0; i<StrTab.GetSize(); i++)
		delete (CStrEnt *)StrTab.GetAt(i);
	StrTab.RemoveAll();
}

//---------------------------------------------------------------------------------------Add
void CStrTab::Add(char *name, char *val) {
	CStrEnt *ent;
	// First check to see if entry already exists
	for(int i=0; i<StrTab.GetSize(); i++) {
		ent = (CStrEnt *)StrTab.GetAt(i);
		if(strcmp(ent->sName, name))
			continue;
		delete [] ent->sValue;
		ent->sValue = new char[strlen(val)+1];
		strcpy(ent->sValue, val);
		return;
	}
	// Does not exist, create
	ent = new CStrEnt();
	ent->sName = new char[strlen(name)+1];
	strcpy(ent->sName, name);
	ent->sValue = new char[strlen(val)+1];
	strcpy(ent->sValue, val);
	StrTab.Add(ent);
}

//---------------------------------------------------------------------------------------Get
char *CStrTab::Get(char *name) {
	CStrEnt *ent;
	for(int i=0; i<StrTab.GetSize(); i++) {
		ent = (CStrEnt *)StrTab.GetAt(i);
		if(!strcmp(ent->sName, name))
			return ent->sValue;
	}
	return 0;
}
