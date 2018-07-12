// str.h
#ifndef STR_H
#define STR_H

class CStr {
public:
// Attributes
	char *sBuf;
	char *sFld;
	int iBuf;		// Buffer pointer used in format conversion

// Methods
	CStr();
	virtual ~CStr();
	CStr(const char *str);
	CStr(const CStr &cstr);
	CStr(const CStr *pstr);
	CStr operator=(const char *str);
	CStr operator+(const char *str);
	CStr operator=(const CStr &cstr);
	CStr operator+(const CStr &cstr);
	const char* operator*();
	const char* operator&();
  operator char *() const;
	bool operator==(const CStr &cstr);
	char operator[](int i);
	char *Cat(char *str);
	char *Hex(int i);
	char *Cat(int i);
	char *Cat(int i, int w);
	char *Cat(double d, int w, int p);
	void Empty();
	bool IsEmpty();
	int GetLength();
	int Find(char c);
	CStr Left(int n);
	CStr Right(int n);
	CStr Mid(int i, int n);
	void TrimLeft();
	void TrimRight();
	int Remove(char c);
	CStr Remove();
	char *GetBuffer();
	CStr SpanExcluding(const char *str);
	char *String();
	char *String(int n, int off);
	CStr Token();
	bool Is(const char *str);
	long Long();
	long Long(int n);
	long Long(int n, int off);
	double Double();
	double Double(int n);
	double Double(int n, int off);
};

#endif
