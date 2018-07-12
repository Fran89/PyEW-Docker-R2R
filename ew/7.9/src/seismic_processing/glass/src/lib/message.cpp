// message.cpp
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "message.h"

#define PAR_STR	1
#define PAR_INT	2
#define PAR_DBL	3
#define PAR_PTR 4

static int iSerial = 0;

// Creats, manages, and delets simple content addressable messages. Two things should
// be noted for future development. Both changes can be application transparent.
//		1. In the clarity of 20-20 hindsight it would be reasonable to concatenate
//			both the name and the content into a single memory segment. This would
//			decrease the number of allocations of small memory objects by a facter
//			of 2.
//		2. This capability can be made cross platform compatible by adding an iEndian
//			paramter to CPar and setting to 1 in the CPar constructor. In addition
//			the CArray would be deleted and all message would be concatenated end
//			to end. Then, iEndian can be checked to see if it is 1 on the remote
//			platform, and if not perform the reversible Endian conversion as
//			appropriate.
// Nota Bene: Because of a bug in the core of Microsoft Windows, always access this
//		class through the IMessage interface to insure that all "free" and "delete"
//		helper functions are executed in the appropriate .dll.

//---------------------------------------------------------------------------------------CPar
CPar::CPar() {
	iType = 0;
	sPar = 0;
	pVal = 0;
}

//---------------------------------------------------------------------------------------~CPar()
CPar::~CPar() {
	char txt[64];
	strcpy(txt, sPar);
	char *str;
	if(sPar)
		delete [] sPar;
	switch(iType) {
	case PAR_STR:
		str = (char *)pVal;
		delete [] str;
		break;
	case PAR_INT:
		str = (char *)pVal;
		delete [] str;
		break;
	case PAR_DBL:
		str = (char *)pVal;
		delete [] str;
		break;
	case PAR_PTR:
		str = (char *)pVal;
		delete [] str;
		break;
	}
}

//---------------------------------------------------------------------------------------CMessage
CMessage::CMessage() {
	iCode = 0;
	sCode = 0;
	iSeq = iSerial++;
}

//---------------------------------------------------------------------------------------CMessage
CMessage::CMessage(const char *code) {
	iCode = 0;
	iSeq = iSerial++;
	int n = (int)strlen(code);
	sCode = new char[n+1];
	for(int i=0; i<n; i++)
		sCode[i] = code[i];
	sCode[n] = 0;
}

//---------------------------------------------------------------------------------------~CMessage
CMessage::~CMessage() {
	if(sCode)
		delete [] sCode;
	CPar *par;
	for(int i=0; i<arrPar.GetSize(); i++) {
		par = (CPar *)arrPar.GetAt(i);
		delete par;
	}
}

//---------------------------------------------------------------------------------------CreateMessage
// Message factory, used to create messages in modules to prevent windows bug
// where memory objects created in one .dll cannon be deleted in another.
IMessage *CMessage::CreateMessage(const char *card) {
	CMessage *msg = 0;
	bool b;	// blank encounted in value -- sensitize to colons
	bool q;	// true while processing quote string
	char code[64];
	char cpar[64];
	char cval[256];
	char ctyp;		// S, I, or D
	char ctmp;
	char c;
	int state = 0;
	int n = (int)strlen(card);
	int i;
	int nc = 0;
	int np = 0;
	int nv = 0;
	for(i=0; i<=n; i++) {
		c = card[i];
	//	if(i == n)
	//		c = ':';
		switch(state) {
		case 0:		// Looking for first character
			if(c == ' ')
				continue;
			code[nc++] = c;
			state = 1;
			break;
		case 1:		// Looking for end of code
			if(c == ' ' || c == ':' || c == 0) {
				code[nc] = 0;
				msg = new CMessage(code);
				state = 2;
			} else {
				code[nc++] = c;
			}
			break;
		case 2:	// Field Type
			if(c == ' ')
				continue;
			ctyp = c;
			np = 0;
			nv = 0;
			state = 3;
			break;
		case 3:	// colon, suppose I should check
			state = 4;
			break;
		case 4:	// Building parameter name
			if(c == '=') {
				cpar[np] = 0;
				state = 5;
				b = false;
				q = false;
			} else {
				cpar[np++] = c;
			}
			break;
		case 5:	// Building value
			if(q) {
				if(c == '"') {
					q = false;
					continue;
				}
				cval[nv++] = c;
				continue;
			}
			if(c == '"') {
				q = true;
				continue;
			}
			if(c == ' ')
				b = true;
			if(c != 0) {
				if(!b || c != ':') {
					ctmp = c;
					cval[nv++] = c;
					continue;
				}
			}
			if(nv && i != n)
				nv--;
			while(nv > 0 && cval[nv-1] == ' ')	// Strip terminal blanks
				nv--;
			cval[nv] = 0;
			switch(ctyp) {
			case 'S':	msg->setStr(cpar, cval);	break;
			case 'I':	msg->setInt(cpar, atoi(cval)); break;
			case 'D':	msg->setDbl(cpar, atof(cval)); break;
			}
			ctyp = ctmp;
			np = 0;
			nv = 0;
			state = 4;
		}
	}
	return (IMessage *)msg;
}

//---------------------------------------------------------------------------------------Release
// Correct way to destroy a message. Do not uses delete (same as with COM object)
// Admittedly deleting yourself seems a little bit like having bees living in you
// head -- but there they are =)
void CMessage::Release() {
	delete this;
}

//---------------------------------------------------------------------------------------Is
bool CMessage::Is(const char *code) {
	int n1 = (int)strlen(code);
	int n2 = (int)strlen(sCode);
	if(n1 != n2)
		return false;
	for(int i=0; i<n1; i++)
		if(code[i] != sCode[i])
			return false;
	return true;
}

//---------------------------------------------------------------------------------------FindPar
// Search message for instance of parameter
CPar *CMessage::FindPar(const char *name) {
	CPar *par;
	for(int i=0; i<arrPar.GetSize(); i++) {
		par = (CPar *)arrPar.GetAt(i);
		if(!strcmp(name, par->sPar))
			return par;
	}
	return 0;
}

//---------------------------------------------------------------------------------------setCode
void CMessage::setCode(const char *code) {
	int n = (int)strlen(code);
	sCode = new char[n+1];
	for(int i=0; i<n; i++)
		sCode[i] = code[i];
	sCode[n] = 0;
}

//---------------------------------------------------------------------------------------getCode
char *CMessage::getCode() {
	return sCode;
}

//---------------------------------------------------------------------------------------setPointer
void CMessage::setPtr(const char *name, const void *ptr) {
	int i;
	CPar *par = new CPar();
	par->iType = PAR_PTR;
	int n = (int)strlen(name);
	par->sPar = new char[n+1];
	for(i=0; i<n; i++)
		par->sPar[i] = name[i];
	par->sPar[n] = 0;
	n = sizeof(ptr);
	par->pVal = new char[n];
	memmove(par->pVal, &ptr, n);
	arrPar.Add(par);
}

//---------------------------------------------------------------------------------------getPointer
void *CMessage::getPtr(const char *name) {
	CPar *par = FindPar(name);
	if(!par || par->iType != PAR_PTR)
		return 0;
	void *ptr;
	int n = sizeof(ptr);
	memmove(&ptr, par->pVal, n);
	return ptr;
}

//---------------------------------------------------------------------------------------setString
void CMessage::setStr(const char *name, const char *str) {
	int i;
	CPar *par = new CPar();
	par->iType = PAR_STR;
	int n = (int)strlen(name);
	par->sPar = new char[n+1];
	for(i=0; i<n; i++)
		par->sPar[i] = name[i];
	par->sPar[n] = 0;
	n = (int)strlen(str);
	par->pVal = new char[n+1];
	char *s = (char *)par->pVal;
	for(i=0; i<n; i++)
		s[i] = str[i];
	s[n] = 0;
	arrPar.Add(par);
}

//---------------------------------------------------------------------------------------getString
char *CMessage::getStr(const char *name) {
	CPar *par = FindPar(name);
	if(!par || par->iType != PAR_STR)
		return 0;
	return (char *)par->pVal;
}

//---------------------------------------------------------------------------------------setInt
void CMessage::setInt(const char *name, int ipar) {
	int i;
	CPar *par = new CPar();
	par->iType = PAR_INT;
	int n = (int)strlen(name);
	par->sPar = new char[n+1];
	for(i=0; i<n; i++)
		par->sPar[i] = name[i];
	par->sPar[n] = 0;
	n = sizeof(ipar);
	par->pVal = new char[n];
	memmove(par->pVal, &ipar, n);
	arrPar.Add(par);
}

//---------------------------------------------------------------------------------------getInt
int CMessage::getInt(const char *name) {
	CPar *par = FindPar(name);
	if(!par || par->iType != PAR_INT)
		return -1000;
	int ival;
	int n = sizeof(ival);
	memmove(&ival, par->pVal, n);
	return ival;
}

//---------------------------------------------------------------------------------------setDbl
void CMessage::setDbl(const char *name, double dpar) {
	int i;
	CPar *par = new CPar();
	par->iType = PAR_DBL;
	int n = (int)strlen(name);
	par->sPar = new char[n+1];
	for(i=0; i<n; i++)
		par->sPar[i] = name[i];
	par->sPar[n] = 0;
	n = sizeof(dpar);
	par->pVal = new char[n];
	memmove(par->pVal, &dpar, n);
	arrPar.Add(par);
}

//---------------------------------------------------------------------------------------getDbl
double CMessage::getDbl(const char *name) {
	CPar *par = FindPar(name);
	if(!par || par->iType != PAR_DBL)
		return 0;
	double dval;
	int n = sizeof(dval);
	memmove(&dval, par->pVal, n);
	return dval;
}

//---------------------------------------------------------------------------------------getDbl
// Format message into ascii string in txt up to a maximum of n characters
void CMessage::Dump(char *txt, int n) {
	char prt[256];
	CPar *par;
	double dval;
	int ival;
	void *pval;

	sprintf(txt, "%6d %d:%s", iSeq, iCode, sCode);
	for(int i=0; i<arrPar.GetSize(); i++) {
		par = (CPar *)arrPar.GetAt(i);
		switch(par->iType) {
		case PAR_STR:
			sprintf(prt, " %s:%s", par->sPar, (char *)par->pVal);
			break;
		case PAR_INT:
			ival = getInt(par->sPar);
			sprintf(prt, " %s:%d", par->sPar, ival);
			break;
		case PAR_DBL:
			dval = getDbl(par->sPar);
			sprintf(prt, " %s:%.6f", par->sPar, dval);
			break;
		case PAR_PTR:
			pval = getPtr(par->sPar);
			sprintf(prt, " %s:%x", par->sPar, (unsigned)pval);
			break;
		}
		if((int)(strlen(txt) + strlen(prt)) < n-1)
			strcat(txt, prt);
	}
}

