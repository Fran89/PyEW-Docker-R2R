#include <windows.h>
#include "comfile.h"
#include <stdlib.h>
#include "comfile.h"
#include "strtab.h"
extern "C" {
#include "utility.h"
}

// 19951123 cej Added com file nesting
// 19980101 cej DLL linkage
// 19980101 cej ComFile -> CComFile
// 19980913 cej Added Null method and nul attribute.
// 19990922 cej Added Destructor
// 19991211 cej Added FORTRANesqe fixed field parcing
// 20000622 cej Fixed problem with null field at end of line
// 20020214 cej DeMFCed
// 20020516 cej Added root command
// 20020606 cej Added Arg(name, value) method and string substitution between {}
// 20020607 cej Implemented nested file macro capabilities with up to 10 arguments
//

//---------------------------------------------------------------------------------------CComFile
CComFile::CComFile()
{
	mode = 0;
	err = 0;
	nfile = 0;
	nul = true;
	root = "";
	pStrTab = new CStrTab();
}

//---------------------------------------------------------------------------------------~CComFile
CComFile::~CComFile() {
	delete pStrTab;
}

//---------------------------------------------------------------------------------------Arg
void CComFile::Arg(char *name, char *val) {
	pStrTab->Add(name, val);
}

//---------------------------------------------------------------------------------------Root
void CComFile::Root(const char *base) {
	// root = root;  // DK 061603 This looks like a bug
  root = base;
}

//---------------------------------------------------------------------------------------Open
bool CComFile::Open(const char *str)
{
	eof = false;
	char name[256];
	if(str[0] != '/' && str[0] != '\\') {
		strcpy(name, &root);
		strcat(name, str);
	} else {
		strcpy(name, str);
	}
	if(strlen(name) == 0)   // DK 030520 fopen() bombs if you pass it a blank name
		return(false);
	if(Nest[nfile].fCom = fopen(name, "rt"))
	{
		mode = MODE_FILE;
		nfile++;
		return true;	
	}
	return false;
}

//---------------------------------------------------------------------------------------Close
void CComFile::Close()
{
	while(nfile) {
		fclose(Nest[nfile-1].fCom);
    Nest[nfile-1].fCom = NULL;
		nfile--;
	}

}

//---------------------------------------------------------------------------------------Read
int CComFile::Read()
{
	CStr name;
	CStr carg;
	char fil[256];
	char buf[2000];
	char sin[2000];
	char txt[80];
	char *crd;
	char *str;
	int iarg;
	int m;
	int j;
	int mode;
	int nsin;
	char ch;
	char arg[64];
	int narg;
	
	if(eof)
		return -1;
  if(!nfile)    // DK 20030616 make sure there is a valid open file 
    return -1;  //    before attempting a read.
file:
	crd = fgets(sin, sizeof(sin)-1, Nest[nfile-1].fCom);
	if(crd) {
		m = 0;
		mode = 0;
		nsin = strlen(sin);
		for(j=0; j<nsin; j++) {
			ch = sin[j];
			if(ch == 10 || ch == 13)
				continue;
			switch(mode) {
			case 0:	// Normal state
				if(ch == '{') {
					narg = 0;
					mode = 1;
					continue;
				}
				buf[m++] = ch;
				break;
			case 1: // building argument
				if(ch == '}') {
					buf[m] = 0;
					arg[narg] = 0;
					mode = 0;
					if(narg == 1 && arg[0] >= '0' && arg[0] <= '9') {
						iarg = arg[0] - '0';
						if(iarg < Nest[nfile-1].nArg) {
							strcat(buf, Nest[nfile-1].cArg[iarg].GetBuffer());
						} else {
							strcat(buf, arg);
						}
						m = strlen(buf);
						continue;
					}
					str = pStrTab->Get(arg);
					if(str) {
						strcat(buf, str);
					} else {
						strcat(buf, "{");
						strcat(buf, arg);
						strcat(buf, "}");
					}
					m = strlen(buf);
					continue;
				}
				if(narg < sizeof(arg) - 2)
					arg[narg++] = ch;
				break;
			}
		}
		buf[m] = 0;
//		DebugOn();
//		Debug("[%s]\n", buf);
		i = -1;
		if(buf[0] == '@') {
			buf[0] = ' ';
			card = buf;
			name = Token();
			str = name.GetBuffer();
			if(str[0] != '/' && str[0] != '\\') {
				strcpy(fil, &root);
				strcat(fil, str);
			} else {
				strcpy(fil, str);
			}
			Nest[nfile].nArg = 0;
			for(iarg=0; iarg<MAXARG; iarg++) {
				carg = Token();
				if(nul)
					break;
				Nest[nfile].cArg[iarg] = carg;
				Nest[nfile].nArg++;
			}
			
			if(Nest[nfile].fCom = fopen(fil, "rt")) 
      {
				nfile++;
				goto file;
			} else {
				sprintf(txt, "File <%s> not found.\n", &buf[1]);
			}
		}   
		card = buf;
		return card.GetLength();
	}
	fclose(Nest[nfile-1].fCom);
  Nest[nfile-1].fCom = NULL;
	nfile--;
	if(nfile > 0)
		goto file;
	field = "eof";
	eof = true;
	return -1;
}

//---------------------------------------------------------------------------------------Load
// Load : Load string into command buffer, initialize for parse.
int CComFile::Load(const char *cmd)
{
	card = cmd;
	i = -1;
	err = 0;
	mode = MODE_COMMAND;
	return card.GetLength();
}

//---------------------------------------------------------------------------------------Error
int CComFile::Error()
{
	int k;
	
	k = err;
	err = 0;
	return k;
}

//---------------------------------------------------------------------------------------Card
CStr CComFile::Card()
{
	return card.SpanExcluding("\n");
}

//---------------------------------------------------------------------------------------Null
bool CComFile::Null()
{
	return nul;
}

//---------------------------------------------------------------------------------------Token
CStr CComFile::Token()
{
	int n;
	char ch;
	int nfld;
	int narg = -1;
	char fld[256];
	
	n = card.GetLength();
	nfld = 0;
	nul = false;
	if(i >= n-1)
		goto null;
	
// Scan for beginning of token.
	for(i++; i<n; i++)
	{
		ch = card[i];
		if(ch == '"')	goto quote;
		if(ch == ' ')	continue;
		if(ch == '\t')	continue;
		if(ch == ',')	goto null;
		if(ch == '\n')	goto null;
		break;
	}
	
// Scan for end of token.
	nfld = 0;
	fld[0] = 0;
	for(; i<n; i++)
	{   
		ch = card[i];
		if(ch == ' ' || ch == ',' || ch == '\t' || ch == '\n')	break;
		fld[nfld] = ch;
		if(nfld < sizeof(fld)-1)
			nfld++;
		fld[nfld] = 0;
	}
	field = fld;
	return field;
	
// Quoted tokens
quote:
	i++;
	for(; i<n; i++)
	{
		ch = card[i];
		if(ch == '\n')	goto null;
		if(ch == '"')	break;
		fld[nfld] = ch;
		if(nfld < sizeof(fld)-1)
			nfld++;
	}
	i++;
	fld[nfld] = 0;
	field = fld;
	return field;
	
null:
//	TRACE("CComFile::Token: Null argument\n");
	nul = true;
	field = "";
	return field;	
}

//---------------------------------------------------------------------------------------Token
CStr CComFile::Token(int n) {
	return Token(n, i+1);
}

//---------------------------------------------------------------------------------------Token
// Get fixed position token, strips leading and trailing blanks
CStr CComFile::Token(int n, int off) {
	int j;
	char ch;
	char fld[1024];
	int j1 = off;
	int j2 = off + n;
	if(j2 > card.GetLength())
		j2 = card.GetLength();
	for(j=j1; j<j2; j++) {
		ch = card[j];
		if(ch == ' ')
			continue;
		break;
	}
	int nfld = 0;
	int jj = 0;
	for(; j<j2; j++) {
		ch = card[j];
		fld[jj++] = ch;
		if(ch == ' ')
			break;
		nfld = jj;
	}
	fld[nfld] = 0;
	field = fld;
	i = j2 - 1;
	return field;
}

//---------------------------------------------------------------------------------------String
CStr CComFile::String()
{
	return CComFile::Token();
}

//---------------------------------------------------------------------------------------Long
long CComFile::Long()
{
	CComFile::Token();
	if(field.GetLength() < 1)
		return -1;
	long l = atoi(field.GetBuffer());
	return l;
}

//---------------------------------------------------------------------------------------Long
long CComFile::Long(int n) {
	return Long(n, i+1);
}

//---------------------------------------------------------------------------------------Long
long CComFile::Long(int n, int off) {
	CComFile::Token(n, off);
	long l = atoi(field.GetBuffer());
	return l;
}

//---------------------------------------------------------------------------------------Double
double CComFile::Double()
{
	CComFile::Token();
	double d = atof(field.GetBuffer());
	return d;
}

//---------------------------------------------------------------------------------------Double
double CComFile::Double(int n) {
	return Double(n, i+1);
}

//---------------------------------------------------------------------------------------Double
double CComFile::Double(int n, int off) {
	CComFile::Token(n, off);
	double d = atof(field.GetBuffer());
	return d;
}

//---------------------------------------------------------------------------------------Is
bool CComFile::Is(const char *str)
{
	CStr fld(str);
	if(field == fld)
		return true;
	return false;
}


//-------------------------------------------------------------------------------CurrentToken
CStr CComFile::CurrentToken()
{
	return(field);
}



