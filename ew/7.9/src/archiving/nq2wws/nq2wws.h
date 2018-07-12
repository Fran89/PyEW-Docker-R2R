#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>

#include "earthworm.h"
#include <kom.h>
#include <transport.h>
#include "trace_buf.h"
		
#include "qlib2.h"
 
#include "msdatatypes.h"

#include "seedstrc.h"

#include "externs.h"

/* the defines below map into the q2ew.desc error file */
#define Q2EW_DEATH_SIG_TRAP   2
#define Q2EW_DEATH_EW_PUTMSG  3
#define Q2EW_DEATH_EW_TERM    4
#define Q2EW_DEATH_EW_CONFIG  5

#define VER_NO "1.0.2 -  1999.172"
/* keep track of version notes here */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*****************************************************************************
 *  defines                                                                  *
 *****************************************************************************/

#define MAXCHANS       32  /* Max number of channels in a mSEED file         */
#define MAXBUFF      8192  /* Max buffer (>2 x a mSEED packet)               */

/*****************************************************************************
 *  Define the structure for keeping track of a buffer of trace data.        *
 *****************************************************************************/

typedef struct _DATABUF {
  char sncl[20];     /* SNCL for this data channel                           */
  int buf[MAXBUFF];  /* The raw trace data; native byte order                */
  int bufptr;        /* The nominal time between sample points               */
  double starttime;  /* time of first sample in raw data buffer              */
  double endtime;    /* time of last sample in raw data buffer               */
} DATABUF;

