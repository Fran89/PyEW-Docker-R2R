#include "configurable.h"


TConfigurable::TConfigurable()
{
   ConfigState = WORM_STAT_NOTINIT;
}

HANDLE_STATUS TConfigurable::HandleConfigLine( ConfigSource * p_parser )
{
   return HANDLER_UNUSED;
}

bool TConfigurable::IsReady()
{
   if ( ConfigState == WORM_STAT_NOTINIT )
   {
      ConfigState = WORM_STAT_SUCCESS;
      // this, and all base classes will set ConfigState = WORM_STAT_BADSTATE
      // if a configuration error is encountered.
      CheckConfig();
   }
   return (ConfigState == WORM_STAT_SUCCESS);
}


