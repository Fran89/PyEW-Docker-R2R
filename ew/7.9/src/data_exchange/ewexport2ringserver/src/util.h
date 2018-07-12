
#ifndef UTIL_H
#define UTIL_H 1

#include "ewdefs.h"

extern void safe_usleep (unsigned long int useconds);

extern int WaveMsgMakeLocal (TRACE_HEADER* wvmsg);
extern int WaveMsg2MakeLocal (TRACE2_HEADER* wvmsg);
extern int WaveMsg2XMakeLocal (TRACE2X_HEADER* wvmsg);
extern int WaveMsgVersionMakeLocal (TRACE2X_HEADER* wvmsg, char version);

#endif
