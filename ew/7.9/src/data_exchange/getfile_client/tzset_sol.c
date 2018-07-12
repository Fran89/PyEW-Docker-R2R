/*************************************************************************
 *                             tzset_sol.c                               *
 *                                                                       *
 *       Under Windows, it's necessary to set TZ for the gmtime()        *
 *       function to work correctly.  Solaris gets TZ from the           *
 *       global environment, so this function is a dummy.                *
 *************************************************************************/

#include "getfile_cl.h"

void TzSet( char *tz )
{
    return;
}
