// str.cpp: General string class - modeled after CString
#include <windows.h>
#include <stdlib.h>
#include "str.h"

// 20020508 cej Added Empty() method
// 20020509 cej Added string addition methods (operater+)
// 20020509 cej Added Find() method
// 20020509 cej Added Left(n) and Right(n) methods
// 20020511 cej Fixed many small bugs and memory leaks
// 20020512 cej Added CStr(const CStr *) constructor

//---------------------------------------------------------------------------------------CStr
//		CStr *pstr = new CStr();
CStr::CStr() {
	sBuf = 0;
	sFld = 0;
	iBuf = 0;
}

//---------------------------------------------------------------------------------------CStr
// Constructor
//		CStr *pstr = new CStr("Hello World");
CStr::CStr(const char *str) {
	int n = strlen(str);
	sBuf = new char[n+1];
	strcpy(sBuf, str);
	sFld = 0;
	iBuf = 0;
}

//---------------------------------------------------------------------------------------CStr
// Copy constructor
//		CStr cstr1;
//		CStr cstr2 = CStr(CStr1);
CStr::CStr(const CStr &cstr) {
	int n;
	sBuf = 0;
	if(cstr.sBuf) {
		n = strlen(cstr.sBuf);
		sBuf = new char[n+1];
		strcpy(sBuf, cstr.sBuf);
	}
	sFld = 0;
	iBuf = 0;
}

//---------------------------------------------------------------------------------------CStr
// Clone constructor
//		CStr cstr1;
//		CStr cstr2 = CStr(CStr *pstr);
CStr::CStr(const CStr *pstr) {
	int n;
	sBuf = 0;
	if(pstr->sBuf) {
		n = strlen(pstr->sBuf);
		sBuf = new char[n+1];
		strcpy(sBuf, pstr->sBuf);
	}
	sFld = 0;
	iBuf = 0;
}

//---------------------------------------------------------------------------------------~CStr
// Destructor
//		CStr *pstr = new CStr();
//		delete pstr;
CStr::~CStr() {
	if(sBuf)
		delete [] sBuf;
	if(sFld)
		delete [] sFld;
}

//---------------------------------------------------------------------------------------operator=
// Assign string to string object
//		CStr cstr = "Hello World";
CStr CStr::operator=(const char *str) {
	int n = strlen(str);
	if(sBuf)
		delete [] sBuf;
	sBuf = new char[n+1];
	strcpy(sBuf, str);
	return *this;
}

//---------------------------------------------------------------------------------------operator+
// Append string to string object
//		CStr cstr1 = "Hello";
//		Cstr cstr2 = cstr1 + " World";
CStr CStr::operator+(const char *str) {
	int m = 0;
	int n = strlen(str);
	if(sBuf)
		m = strlen(sBuf);
	char *s = new char[n+m+1];
	strcpy(s, sBuf);
	strcat(s, str);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return *this;
}

//---------------------------------------------------------------------------------------operator=
// Ordinary assigment operator
//		CStr cstr1'
//		CStr cstr2 = cstr1;
CStr CStr::operator=(const CStr &cstr) {
	int n;
	if(sBuf)
		delete [] sBuf;
	sBuf = 0;
	if(cstr.sBuf) {
		n = strlen(cstr.sBuf);
		sBuf = new char[n+1];
		strcpy(sBuf, cstr.sBuf);
	}
	iBuf = 0;
	return *this;
}

//---------------------------------------------------------------------------------------operator+
// Append string object to string object
//		CStr cstr1 = "Hello";
//		CStr cstr2 = " World";
//		CStr cstr3 = cstr1 + cstr2;
CStr CStr::operator+(const CStr &cstr) {
	int m = 0;
	if(sBuf)
		m = strlen(sBuf);
	int n = 0;
	if(cstr.sBuf)
		n = strlen(cstr.sBuf);
	char *s = new char[n+m+1];
	strcpy(s, sBuf);
	if(m)
		strcat(s, cstr.sBuf);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return *this;
}

//---------------------------------------------------------------------------------------operator&
// Convert string object to string
//		CStr cstr;
//		const char *str = &cstr;
const char* CStr::operator&() {
	return sBuf;
}

//---------------------------------------------------------------------------------------operator*
// Operator (const char *)
//		CStr cstr;
//		const char *str = (const char *)cstr;
const char* CStr::operator*() {
	return sBuf;
}

//---------------------------------------------------------------------------------------operator*
// Operator (const char *)
//		CStr cstr;
//		const char *str = (const char *)cstr;
 CStr::operator char *() const {
	return sBuf;
}

//---------------------------------------------------------------------------------------operator[]
//		CStr cstr;
//		char c = cstr[5];
char CStr::operator[](int i) {
	if(!sBuf)
		return 0;
	int n = strlen(sBuf);
	if(i >= n)
		return 0;
	return sBuf[i];
}

//---------------------------------------------------------------------------------------operator==
bool CStr::operator==(const CStr &cstr) {
	if(sBuf == 0 && cstr.sBuf == 0)
		return true;
	if(sBuf == 0 && cstr.sBuf != 0)
		return false;
	if(sBuf != 0 && cstr.sBuf == 0)
		return false;
	return !strcmp(sBuf, cstr.sBuf);
}

//---------------------------------------------------------------------------------------Cat
// Append string to string buffer
char *CStr::Cat(char *str) {
	char *s;
	int m = 0;
	int n = strlen(str);
	if(sBuf) {
		m = strlen(sBuf);
		s = new char[n+m+1];
		strcpy(s, sBuf);
	} else {
		s = new char[n+1];
		s[0] = 0;
	}
	strcat(s, str);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return sBuf;
}

//---------------------------------------------------------------------------------------Cat
// Format integer and append to string buffer
char *CStr::Cat(int i) {
	char str[64];
	int n;
	int m;

	_itoa(i, str, 10);

	char *s;
	m = 0;
	n = strlen(str);
	if(sBuf) {
		m = strlen(sBuf);
		s = new char[n+m+1];
		strcpy(s, sBuf);
	} else {
		s = new char[n+1];
		s[0] = 0;
	}
	strcat(s, str);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return sBuf;
}

//---------------------------------------------------------------------------------------Cat
// Format hexadecimal integer and append to string buffer
char *CStr::Hex(int i) {
	char str[64];
	int n;
	int m;

	_itoa(i, str, 16);

	char *s;
	m = 0;
	n = strlen(str);
	if(sBuf) {
		m = strlen(sBuf);
		s = new char[n+m+1];
		strcpy(s, sBuf);
	} else {
		s = new char[n+1];
		s[0] = 0;
	}
	strcat(s, str);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return sBuf;
}

//---------------------------------------------------------------------------------------Cat
// Format integer in a field of width w and append to string buffer
char *CStr::Cat(int i, int w) {
	char str[64];
	char buf[64];
	int n;
	int m;
	int j;
	int pad;

	_itoa(i, buf, 10);
	pad = w - strlen(buf);
	if(pad < 0)
		pad = 0;
	if(pad > 0) {
		for(j=0; j<pad; j++)
			str[j] = ' ';
	}
	str[pad] = 0;
	strcat(str, buf);

	char *s;
	m = 0;
	n = strlen(str);
	if(sBuf) {
		m = strlen(sBuf);
		s = new char[n+m+1];
		strcpy(s, sBuf);
	} else {
		s = new char[n+1];
		s[0] = 0;
	}
	strcat(s, str);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return sBuf;
}

//---------------------------------------------------------------------------------------Cat
// Format double in a field of width w and precision p and append to string buffer
char *CStr::Cat(double d, int w, int p) {
	char str[64];
	char buf[64];
	char *tmp;
	int n;
	int m;
	int j;
	int i;
	int pad;
	int dec;
	int sgn;

	tmp = _fcvt(d, p, &dec, &sgn);
	n = strlen(tmp);
	j = 0;
	if(sgn)
		buf[j++] = '-';
	for(i=0; i<n; i++) {
		if(i == dec)
			buf[j++] = '.';
		buf[j++] = tmp[i];
	}
	buf[j] = 0;
	pad = w - strlen(buf);
	if(pad < 0)
		pad = 0;
	if(pad > 0) {
		for(j=0; j<pad; j++)
			str[j] = ' ';
	}
	str[pad] = 0;
	strcat(str, buf);

	char *s;
	m = 0;
	n = strlen(str);
	if(sBuf) {
		m = strlen(sBuf);
		s = new char[n+m+1];
		strcpy(s, sBuf);
	} else {
		s = new char[n+1];
		s[0] = 0;
	}
	strcat(s, str);
	if(sBuf)
		delete [] sBuf;
	sBuf = s;
	return sBuf;
}

//---------------------------------------------------------------------------------------Empty
// Make string 0 length
//		CStr cstr;
//		cstr.Empty();
void CStr::Empty() {
	delete [] sBuf;
	sBuf = new char[1];
	sBuf[0] = 0;
}

//---------------------------------------------------------------------------------------IsEmpty
// Tests to see if string is uninitialized or of zero length
//		Cstr cstr
//		if(cstr.IsEmpty())
//			do something iportant;
bool CStr::IsEmpty() {
	if(sBuf == 0)
		return true;
	if(sBuf[0] == 0)
		return true;
	return false;
}

//---------------------------------------------------------------------------------------GetLength
// Get length of string
//		CStr cstr;
//		int n = cstr.GetLength();
int CStr::GetLength() {
	if(!sBuf)
		return 0;
	return strlen(sBuf);
}

//---------------------------------------------------------------------------------------Find
// Find occurence of character in string object. If not found returns -1
int CStr::Find(char c) {
	if(!sBuf)
		return -1;
	int n = strlen(sBuf);
	for(int i=0; i<n; i++)
		if(sBuf[i] == c)
			return i;
	return -1;
}

//---------------------------------------------------------------------------------------Left
// Return string containing left n bytes
CStr CStr::Left(int n) {
	CStr cstr;
	int m = GetLength();
	if(m < 1)
		return cstr;
	if(n > m)
		n = m;
	char *s = new char[n+1];
	for(int i=0; i<n; i++)
		s[i] = sBuf[i];
	s[n] = 0;
	cstr.sBuf = s;
	return cstr;
}

//---------------------------------------------------------------------------------------Right
// Return string containing right n bytes
CStr CStr::Right(int n) {
	CStr cstr;
	int m = GetLength();
	if(m < 1)
		return cstr;
	if(n > m)
		n = m;
	char *s = new char[n+1];
	for(int i=0; i<n; i++)
		s[i] = sBuf[i+m-n];
	s[n] = 0;
	cstr.sBuf = s;
	return cstr;
}

//---------------------------------------------------------------------------------------Mid
// Retrieve the substring of length n beginning with the ith character
CStr CStr::Mid(int i, int n) {
	if(!sBuf)
		return CStr(" ");
	int nstr = strlen(sBuf);
	if(i < 0 || i >= nstr)
		return CStr(" ");
	if(i + n >= nstr)
		n = nstr - i;
	char *stmp = new char[n+1];
	for(int j=0; j<nstr; j++)
		stmp[j] = sBuf[i++];
	stmp[n] = 0;
	return CStr(stmp);
}

//---------------------------------------------------------------------------------------TrimLeft
// Removes whitespace (blank, comma, and tabs) from beginning of string
void CStr::TrimLeft() {
	if(!sBuf)
		return;
	int n = strlen(sBuf);
	char *stmp = new char[n+1];
	int j = 0;
	for(int i=0; i<n; i++) {
		if(!j) {
			if(sBuf[i] == ' ')
				continue;
			if(sBuf[i] == ',')
				continue;
			if(sBuf[i] == '\t')
				continue;
		}
		stmp[j++] = sBuf[i];
	}
	stmp[j] = 0;
	delete [] sBuf;
	sBuf = stmp;
	if(sFld) {
		delete [] sFld;
		sFld = 0;
	}
	iBuf = 0;
}

//---------------------------------------------------------------------------------------TrimRight
// Removes whitespace (blank, comma, and tabs) from emd of string
void CStr::TrimRight() {
	if(!sBuf)
		return;
	int n = strlen(sBuf);
	if(sFld)
		delete [] sFld;
	iBuf = 0;
	for(int i=n-1; i>=0; i--) {
		if(sBuf[i] == ' ' || sBuf[i] == ',' || sBuf[i] == '\t') {
			sBuf[i] = 0;
		} else {
			return;
		}
	}
}

//---------------------------------------------------------------------------------------Remove
// Removes all occurrences of given character from string, returns number removed
int CStr::Remove(char c) {
	if(!sBuf)
		return 0;
	int n = strlen(sBuf);
	char *stmp = new char[n+1];
	int j = 0;
	for(int i=0; i<n; i++) {
		if(sBuf[i] != c)
			stmp[j++] = sBuf[i];
	}
	stmp[j] = 0;
	delete [] sBuf;
	sBuf = stmp;
	if(sFld) {
		delete [] sFld;
		sFld = 0;
	}
	iBuf = 0;
	return j;
}

//---------------------------------------------------------------------------------------SpanExcluding
// Returns a string object containing the first characters in original string up to
// but excluding any character in the argument string.
CStr CStr::SpanExcluding(const char *str) {
	int i, j;
	CStr cstr;
	char *snew;
	int nstr = strlen(str);
	int n = strlen(sBuf);
	for(i=0; i<n; i++) {
		for(j=0; j<nstr; j++)
			if(sBuf[i] == str[j]) {
				snew = new char[i+1];
				if(i)
					memmove(snew, sBuf, i);
				snew[i] = 0;
				cstr = CStr(snew);
				delete [] snew;
				return cstr;
			}
	}
	cstr = sBuf;
	return cstr;
}

//---------------------------------------------------------------------------------------GetBuffer
// Returns a string pointer to the string object string buffer
char *CStr::GetBuffer() {
	return sBuf;
}

//---------------------------------------------------------------------------------------String
// Returns a string pointer to the next white space delimited token. Not that this
// pointer is ephemeral and is only valid to the next call to one of the parsing
// functions. If a permanent pointer is desired, either create a new string object
// using the return pointer as an argument in the constructor, or us the Token()
// method which returns the same information in string object form.
char *CStr::String()
{
	int i;
	int i1 = -1;
	int i2 = -1;

	if(!sBuf)
		return 0;
	if(sFld) {
		delete [] sFld;
		sFld = 0;
	}
	int nbuf = strlen(sBuf);
	// Find first non white space
	for(i=iBuf; i<nbuf; i++) {
		switch(sBuf[i]) {
		case ' ':
		case '/t':
			continue;
		default:
			i1 = i;
			break;
		}
		break;
	}
	if(i1 < 0)
		return 0;
	// Find end of token
	for(i=i1+1; i<nbuf; i++) {
		switch(sBuf[i]) {
		case ' ':
		case '/t':
			i2 = i;
			break;
		default:
			continue;
		}
		break;
	}
	if(i2 < 0)
		i2 = nbuf;
	// Extract token
	String(i2-i1, i1);
	return sFld;
}

//---------------------------------------------------------------------------------------String
// Extract an n character string beginning with the ith character. Note that same
// restrictions apply as discussed for the String() method above.
char *CStr::String(int n, int i1) {
	char c;
	if(sFld) {
		delete [] sFld;
		sFld = 0;
	}
	sFld = new char[n+1];
	int j = 0;
	for(int i=0; i<n; i++) {
		c = sBuf[i1+i];
		if(c == ' ')
			continue;
		if(c == '\t')
			continue;
		sFld[j++] = c;
	}
	sFld[j] = 0;
	iBuf = i1 + n;
	return sFld;
}

//---------------------------------------------------------------------------------------Token
// Return the next white space delimited field as a string object
CStr CStr::Token() {
	char *s = String();
	CStr cstr(s);
	return cstr;
}

//---------------------------------------------------------------------------------------Long
// Return the next white space delimited field as an integer
long CStr::Long()
{
	if(!String())
		return -1;
	return atoi(sFld);
}

//---------------------------------------------------------------------------------------Long
// Interpret the next n unparsed bytes as an integer
long CStr::Long(int n) {
	return Long(n, iBuf);
}

//---------------------------------------------------------------------------------------Long
// Interprest the string from s[off] ... s[off+n-1] inclusive as an interger
long CStr::Long(int n, int off) {
	if(!String(n, off))
		return -1;
	long l = atoi(sFld);
	return l;
}

//---------------------------------------------------------------------------------------Double
// Interpret the next white space delimited field as a double float
double CStr::Double()
{
	if(!String())
		return 0.0;
	double d = atof(sFld);
	return d;
}

//---------------------------------------------------------------------------------------Double
// Interpret the next n unparsed bytes as a double float
double CStr::Double(int n) {
	return Double(n, iBuf);
}

//---------------------------------------------------------------------------------------Double
// Interprest the string from s[off] ... s[off+n-1] inclusive as a double float
double CStr::Double(int n, int off) {
	if(!String(n, off))
		return 0.0;
	double d = atof(sFld);
	return d;
}

//---------------------------------------------------------------------------------------Is
// Returns true if the last parsed filed is the same as the string provided, otherwise
// returns false.
bool CStr::Is(const char *str)
{
	if(!sFld)
		return false;
	if(strcmp(str, sFld))
		return false;
	return true;
}




