#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "utility.h"

int iDebugFlag = 0;

#include <time_ew.h>

//---------------------------------------------------------------------Seconds
// Calculate seconds using high performace timer
static double tBase=0.0;
static double tNow;

/* DK CLEANUP  should use hrtime_ew() instead, and use your own time
   counters in case of multithreading, or use by one procedure inside
   another
 **********************************************************************/
double Secs() {

	if(tBase == 0.0) {
		hrtime_ew(&tBase);
	}
	return(hrtime_ew(&tNow) - tBase);
}

