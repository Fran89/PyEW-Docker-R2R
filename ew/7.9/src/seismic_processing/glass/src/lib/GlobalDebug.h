// GlobalDebug.h

#ifndef _GLOBAL_DEBUG_H
#define _GLOBAL_DEBUG_H

#ifndef _DEBUG_SO_EXPORT 
# define _DEBUG_SO_EXPORT  __declspec( dllimport)
#endif // _DEBUG_SO_EXPORT

extern "C" 
{
#include <stdarg.h>
#  include <earthworm_simple_funcs.h>
}

class  _DEBUG_SO_EXPORT GlobalDebug {
public:

  // static
  static void CheckInit();
  static void Init( char *prog, short mid, int bufSize, int logflag);
  static int  DebugVA( char *flag, char *format, ... );
  static int  Debug( char *flag, char *format, va_list ap);
  static int  Status(int iType, char *format, va_list ap);
  static int SetStatusFunction(int (__cdecl *IN_pStatusFunction )(int iType, const char * szOutput) );

  // Constructors
GlobalDebug(){}

	// Destructors
~GlobalDebug(){}

  // static attributes
  static char szProgramName[256];
	static int bInitialized;
  static int (__cdecl *pStatusFunction )(int iType, const char * szOutput);

};

#endif // _GLOBAL_DEBUG_H