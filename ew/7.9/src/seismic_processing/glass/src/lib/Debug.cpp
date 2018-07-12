#include "Debug.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


// CLEANUP THIS IS WHERE WE HARD CODE THE DEBUG LOGGING SETTINGS
char CDebug::szModuleName[]="xxxDEFAULTxxx";
DebugOutputStruct CDebug::dosArray[]=
{  // file, debugger, error, timestamp, status
  {1,1,1,1,1},  // MAJOR ERROR
  {1,1,1,1,1},  // MAJOR WARNING
  {1,1,0,0,0},  // MAJOR INFO
  {1,1,1,1,1},  // MINOR ERROR
  {1,1,1,0,0},  // MINOR WARNING
  {0,0,0,0,0},  // MINOR INFO
  {1,1,0,0,0},
  {1,1,0,0,0},
  {1,1,0,0,0},
  {0,0,0,0,0}   // FUNCTION TRACE
};

static char szDebugBuffer[8192];

int CDebug::SetProgramName(char * IN_szProgramName)
{
		/* initialize the underneath logit system */
    GlobalDebug::Init(IN_szProgramName,0,8192,1);
    GlobalDebug::DebugVA("","This is a test.\n");
		return(0);
}


// CDebug constructor, passing module/thread name
CDebug::CDebug(char * IN_szModuleName)
{

	strncpy(szModuleName, IN_szModuleName, sizeof(szModuleName));
  szModuleName[sizeof(szModuleName)-1] = 0x00;

}

CDebug::CDebug()
{
	strncpy(szModuleName, "CDebug::DEFAULT", sizeof(szModuleName));
  szModuleName[sizeof(szModuleName)-1] = 0x00;
}


CDebug::~CDebug()
{
}

// CDebug set module/thread name
int CDebug::SetModuleName(char * IN_szModuleName)
{

	strncpy(szModuleName, IN_szModuleName, sizeof(szModuleName));
  szModuleName[sizeof(szModuleName)-1] = 0x00;
  return(0);
}


int  CDebug::SetLevelParams(unsigned int iLevel, DebugOutputStruct * dos)
{
	if(iLevel > (sizeof(dosArray)/sizeof(DebugOutputStruct)))
		return(-1);  // iLevel is out of range

  memcpy(&dosArray[iLevel],dos,sizeof(DebugOutputStruct));
  return(0);
}



int CDebug::CheckLog(int iType)
{
  if(iType > (sizeof(dosArray)/sizeof(DebugOutputStruct)) || iType < 0)
    return(0);  // iLevel is out of range
  
  return(dosArray[iType].bOTF ||
         dosArray[iType].bOTE ||
         dosArray[iType].bOTD
        );
    
}

int CDebug::LogVA(int iType, const char *szFormat, va_list ap)
{
	char szLogCode[20];
	int iLogCodeLen=0;
	int rc=0;

		if(CheckLog(iType))
		{
      // ignore bOTF for now
			//if(!dosArray[iType].bOTF)
			//{
			//	szLogCode[iLogCodeLen] = 'n';
			//	iLogCodeLen++;
			//}
			if(dosArray[iType].bOTE)
			{
				szLogCode[iLogCodeLen] = 'e';
				iLogCodeLen++;
			}
			if(dosArray[iType].bOTD)
			{
				szLogCode[iLogCodeLen] = 'D';
				iLogCodeLen++;
			}
			if(dosArray[iType].bOTS)
			{
				szLogCode[iLogCodeLen] = 't';
				iLogCodeLen++;
			}

			szLogCode[iLogCodeLen] = 0x00;


      sprintf(szDebugBuffer,"%s: ",szModuleName);
      strncat(szDebugBuffer,szFormat,sizeof(szDebugBuffer) - strlen(szDebugBuffer));
      szDebugBuffer[sizeof(szDebugBuffer)-1] = 0x00;
			rc=GlobalDebug::Debug(szLogCode, szDebugBuffer, (va_list)ap);
      if(dosArray[iType].bOSM)
      {
        GlobalDebug::Status(iType, szDebugBuffer, (va_list)ap);
      }
			return(rc);
		}
		else
		{
			return(0);
		}
} /* end CDebug::Log(va_list) */


int CDebug::Log(int iType, const char *szFormat, ...)
{
	va_list ap;
  int     rc;

	va_start(ap,szFormat);
  rc = CDebug::LogVA(iType, szFormat, ap);
  va_end(ap);
  return(rc);
}  /* end CDebug::Log(...) */
