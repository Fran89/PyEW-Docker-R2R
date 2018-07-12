#include "stdafx.h"
#include "comfile.h"
// 951123 Added com file nesting
CComfile::CComfile()
{
	mode = 0;
	err = 0;
	nfile = 0;
}

BOOL CComfile::Open(const CString &str)
{
	eof = FALSE;
	if(file[nfile].Open(str, CFile::modeRead))
	{
		mode = MODE_FILE;
		nfile = 1;
		return TRUE;	
	}
	return FALSE;
}

BOOL CComfile::Archive(CArchive *arc)
{
	mode = MODE_ARCHIVE;
	eof = FALSE;
	ar = arc;
	return TRUE;
}

void CComfile::Close()
{
	while(nfile) {
		file[nfile-1].Close();
		nfile--;
	}

}

int CComfile::Read()
{
	CString name;	
	char buf[200];
	char chr[4];
	char txt[80];
	int nbuf;
	if(eof)
		return -1;
	if(mode == MODE_ARCHIVE)
		goto archive;
	if(mode == MODE_FILE)
		goto file;
	return -1;
	
file:
	if(file[nfile-1].ReadString(buf, sizeof(buf)-1))
	{
		i = -1;
		if(buf[0] == '@') {
			buf[0] = ' ';
			card = buf;
			name = Token();
			if(file[nfile].Open(name, CFile::modeRead)) {
				nfile++;
				goto file;
			} else {
				sprintf(txt, "File <%s> not found.\n", &buf[1]);
				AfxMessageBox(txt);
			}
		}   
		card = buf;
		return card.GetLength();
	}
	file[nfile-1].Close();
	nfile--;
	if(nfile > 0)
		goto file;
	field = "eof";
	eof = TRUE;
	return -1;
	
archive:
	if(ar->Read(chr, 1) < 1)
	{
		field = "eof";
		eof = TRUE;
		return -1;
	}
	if(chr[0] == '\r')
		goto archive;
	if(chr[0] == '\n')
	{
		buf[0] = 0;
		card = buf;
		i = -1;
		return 0;
	}

	nbuf = 1;
	buf[0] = chr[0];	
next:
	if(ar->Read(chr, 1) < 1)
	{
		field = "eof";
		eof = TRUE;
		return -1;
	}
	if(chr[0] == '\r')
		goto next;
	if(chr[0] == '\n')
	{
		buf[nbuf] = 0;
		card = buf;
		i = -1;
		return nbuf;
	}
	buf[nbuf++] = chr[0];
	goto next;
	
}

// Load : Load string into command buffer, initialize for parse.
int CComfile::Load(CString &cmd)
{
	card = cmd;
	i = -1;
	err = 0;
	mode = MODE_COMMAND;
	return card.GetLength();
}

int CComfile::Error()
{
	int k;
	
	k = err;
	err = 0;
	return k;
}

CString CComfile::Card()
{
	return card.SpanExcluding("\n");
}

CString CComfile::Token()
{
	int n;
	int nfld;
	char ch;
	char fld[100];
	
	n = card.GetLength();
	nfld = 0;
	
// Scan for beginning of token.
	for(i++; i<n; i++)
	{   
		ch = card[i];
		if(ch == '"')	goto quote;
		if(ch == ' ')	continue;
		if(ch == '\t')	continue;
		if(ch == ',')	goto null;
		if(ch == '\n')	goto null;
		nfld = 0;
		break;
	}
	
// Scan for end of token.
	for(; i<n; i++)
	{   
		ch = card[i];
		if(ch == ' ' || ch == ',' || ch == '\t' || ch == '\n')	break;
		fld[nfld] = ch;
		if(nfld < sizeof(fld)-1)
			nfld++;
	}
	fld[nfld] = 0;
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
	field = "";
	return field;	
}

CString CComfile::String()
{
	return CComfile::Token();
}

long CComfile::Long()
{
	long l;
	
	CComfile::Token();
	if(field == "")
		return -1;
	l = atol((const char *)field);
	return l;
}

double CComfile::Double()
{
	double d;
	
	CComfile::Token();
	d = atof((const char *)field);
	return d;
}

BOOL CComfile::Is(const char *str)
{
	CString fld(str);
	if(field == fld)
		return TRUE;
	return FALSE;
}

// Added DK 08/16/01
BOOL CComfile::StartsWith(const char *str)
{
  return(!strncmp(str,(LPCTSTR)field,strlen(str)));
}

