#ifndef __EXTERNS_H__
#define __EXTERNS_H__

#include "config.h"
extern Configuration gConfig;
extern unsigned char TypeTrace;         /* Trace EW type for logo */
extern unsigned char TypeHB;            /* HB=HeartBeat EW type for logo */
extern unsigned char TypeErr;           /* Error EW type for logo */
extern unsigned char QModuleId;
extern MSG_LOGO DataLogo;
extern MSG_LOGO OtherLogo;
extern SHM_INFO Region;
extern time_t	Heartbeat_lastBeatTime;		/* time stamp since last heartbeat */
extern unsigned Heartbeat_threadID;			/* ID for the hearbeat thread */
extern int ShutMeDown;
#endif
