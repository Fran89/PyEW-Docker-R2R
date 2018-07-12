//---------------------------------------------------------------------------
#include "worm_signal.h"

#include <globalutils.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

void SignalHandler( int p_signum )
{
   switch( p_signum )
   {
     case SIGINT:    // ^c
     case SIGTERM:   // kill -15
     case SIGABRT:   // kill -9
#ifdef SIGBREAK
     case SIGBREAK:  // keyboard break
#endif
          TGlobalUtils::SetTerminateFlag();
          break;
     default:
          signal(p_signum, (SIG_HANDLR_PTR)SignalHandler);  //  reinstall signal handler
          break;
   }
}
