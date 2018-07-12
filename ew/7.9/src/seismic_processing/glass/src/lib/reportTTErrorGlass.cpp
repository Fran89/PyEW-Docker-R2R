#include <Debug.h>

extern "C" 
{
#include <ttt.h>
}


int  reportTTError(int severityLevel, const char *messageFormat, ...)
{
	va_list ap;
  int     rc;
  int     iLogLevel;

  switch(severityLevel)
  {
  case TT_ERROR_DEBUG : 
    iLogLevel = DEBUG_MINOR_INFO;
    break;
  case TT_ERROR_WARNING : 
    iLogLevel = DEBUG_MINOR_WARNING;
    break;
  case TT_ERROR_FATAL : 
    iLogLevel = DEBUG_MAJOR_ERROR;
    break;
  default:
    iLogLevel = DEBUG_MAJOR_ERROR;
    break;
  }

  va_start(ap,messageFormat);
  rc = CDebug::LogVA(iLogLevel, messageFormat, (va_list)ap);
  va_end(ap);

  return(rc); 
}