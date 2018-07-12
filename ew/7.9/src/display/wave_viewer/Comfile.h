#ifndef _COMFILE_H_
#define _COMFILE_H_
#define MODE_FILE		1
#define MODE_ARCHIVE	2
#define MODE_COMMAND	3
#define MAXFIL 10
class CComfile : public CObject
{
public:
	CComfile();
	BOOL Open(const CString &str);
	BOOL Archive(CArchive *ar);
	void Close();
	int Read();
	int Load(CString &cmd);
	CString Card();
	CString Token();
	CString String();
	long Long();
	double Double();
	int Error();
	BOOL Is(const char *str);
  BOOL StartsWith(const char *str);
	
private:
	CStdioFile	file[MAXFIL];
	CArchive	*ar;
	CString		name;
	CString		card;
	CString		field;
	BOOL		eof;
	int			nfile;
	int			mode;
	int			i;
	int			err;
};
#endif // _COMFILE_H_