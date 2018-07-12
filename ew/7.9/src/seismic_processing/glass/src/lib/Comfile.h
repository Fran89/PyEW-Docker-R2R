#ifndef _COMFILE_H_
#define _COMFILE_H_

#include "str.h"
#include <stdio.h>

#define MODE_FILE		1
#define MODE_ARCHIVE	2
#define MODE_COMMAND	3
#define MAXFIL 10
#define MAXARG 10

typedef struct {
	FILE	*fCom;
	int		nArg;
	CStr	cName;
	CStr	cArg[MAXARG];
} NEST;

class CStrTab;
class CComFile
{
public:
	CComFile();
	~CComFile();
	void Arg(char *name, char *val);
	void Root(const char *base);
	bool Open(const char *file);
	void Close();
	int Read();
	int Load(const char *str);
	CStr Card();
	bool Null();
	CStr Token();
	CStr Token(int n);
	CStr Token(int n, int off);
	CStr String();
	long Long();
	long Long(int n);
	long Long(int n, int off);
	double Double();
	double Double(int n);
	double Double(int n, int off);
	int Error();
	bool Is(const char *str);
	CStr CurrentToken();
	
private:
	CStrTab		*pStrTab;
	NEST		Nest[MAXFIL];
	CStr		root;
	CStr		name;
	CStr		card;
	CStr		field;
	bool		eof;
	bool		nul;
	int			nfile;
	int			mode;
	int			i;
	int			err;
};
#endif // _COMFILE_H_