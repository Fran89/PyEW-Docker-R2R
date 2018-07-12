# define _DEBUG_SO_EXPORT  __declspec( dllexport)

#include <stdlib.h>
#include <stdio.h>
#include "GlobalDebug.h"

int GlobalDebug::bInitialized = false;
int (__cdecl *GlobalDebug::pStatusFunction )(int iType, const char * szOutput) = 0x00;

extern "C"
{
  int logit_core( char *flag, char *format, va_list ap);
}

void GlobalDebug::Init( char *prog, short mid, int bufSize, int logflag)
{
  bInitialized = true;
  logit_init(prog,mid,bufSize,logflag);
}

int  GlobalDebug::DebugVA( char *flag, char *format, ... )
{
  int     rc;
  va_list ap;

  if(!bInitialized)
    Init("GlobalDebug_DEFAULT",0,8192,1);

  va_start(ap,format);
  rc=logit_core(flag, format, (va_list)ap);
  va_end(ap);
  return(rc);
}  // GlobalDebug::DebugVA()


int  GlobalDebug::Debug( char *flag, char *format, va_list ap)
{
  if(!bInitialized)
    Init("GlobalDebug_DEFAULT",0,8192,1);

  return(logit_core(flag,format,ap));
}  //GlobalDebug::Debug

int  GlobalDebug::Status(int iType, char *format, va_list ap)
{
  if(!pStatusFunction)
    return(-1);

  char szOutput[1024];

  _vsnprintf(szOutput,sizeof(szOutput),format,ap);
  szOutput[sizeof(szOutput)-1] = 0x00;
  return(pStatusFunction(iType,szOutput));
}




int  GlobalDebug::SetStatusFunction(int (__cdecl *IN_pStatusFunction )(int iType, const char * szOutput) )
{
  pStatusFunction = IN_pStatusFunction;
  return(0);
}
