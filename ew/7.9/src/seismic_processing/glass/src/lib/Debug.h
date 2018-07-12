// Debug.h

#ifndef _DEBUG_H
#define _DEBUG_H

typedef struct _DebugOutputStruct
{
		char bOTF; /* OutputToFile */
		char bOTD; /* bOutputToDebugger */
		char bOTE; /* bOutputToError */
		char bOTS; /* bOutputTimeStamp */
		char bOSM; /* bOutputStatusMessage */
} DebugOutputStruct;

#define DEBUG_MAJOR_ERROR 0
#define DEBUG_MAJOR_WARNING 1
#define DEBUG_MAJOR_INFO 2
#define DEBUG_MINOR_ERROR 3
#define DEBUG_MINOR_WARNING 4
#define DEBUG_MINOR_INFO 5
#define DEBUG_FUNCTION_TRACE 9

#define DEBUG_MAX_LEVEL 0
#define DEBUG_MIN_LEVEL 9

#include <GlobalDebug.h>

class  CDebug {
public:

  // Static
  static int SetProgramName(char * IN_szProgramName);

  // Constructors
	CDebug();
  CDebug(char * IN_szModuleName);

	// Destructors
	~CDebug();

	// Member Functions
	static int  Log(int iType, const char *szFormat, ...);
  static int  LogVA(int iType, const char *szFormat, va_list ap);
	static int  CheckLog(int iType);
	static int  SetModuleName(char *szModName);
  static int  SetLevelParams(unsigned int iLevel, DebugOutputStruct * dos);
protected:

	// static Attributes
	static char szModuleName[256];
	static DebugOutputStruct dosArray[DEBUG_MIN_LEVEL+1];
//	static char szDebugBuffer[1024];


};

#endif // _DEBUG_H