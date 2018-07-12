#include <stdio.h>
#include <stdlib.h>
#include "q3302ew.h"

#include "transport.h"
#include "earthworm_simple_funcs.h"
#include "earthworm_complex_funcs.h"
#include "earthworm.h"

#include "q3302ew_platform.h"
#include "logging.h"
#include "externs.h"
#include "options.h"
#include "lib330Interface.h"
#include "heart.h"

static unsigned char InstId = 255;

MSG_LOGO DataLogo;
MSG_LOGO OtherLogo;
SHM_INFO Region;
int ShutMeDown = 0;


#define MAX_WAIT_STATE_BEFORE_EXIT 240 /* max seconds to sit in WAIT for reg state */

void setuplogo(MSG_LOGO *logo) {
   /* only get the InstId once */
   if (InstId == 255  && GetLocalInst(&InstId) != 0) {
      fprintf( stderr, "%s: Invalid Installation code; exiting!\n", Q3302EW_NAME);
   }
   logo->mod = QModuleId;
   logo->instid = InstId;
}

void q3302ew_die(char *msg) {
  fprintf(stderr, "%s exiting: %s", Q3302EW_NAME, msg);
  q3302ew_cleanupAndExit();
}

void q3302ew_cleanup() {
  lib330Interface_cleanup();
}

void q3302ew_cleanupAndExit(int i) {
  q3302ew_cleanup();
  exit(1);
}

void setupSignalHandlers() {
#ifndef WIN32
  signal(SIGHUP, q3302ew_cleanupAndExit);
  signal(SIGINT, q3302ew_cleanupAndExit);
  signal(SIGQUIT, q3302ew_cleanupAndExit);
  signal(SIGTERM, q3302ew_cleanupAndExit);
#endif
}

int main(int argc, char **argv) {
  int registration_count = 0;
  int wait_counter = 0;
  time_t lastStatusUpdate;

  if (argc<2) {
    usage();
    exit(1);
  }
  // then read config
  handle_opts(argc, argv);


  // initialize some internals
  logInitialize(argv[1], gConfig.LogFile);
  printConfigStructToLog();
  lib330Interface_initialize();
  setupSignalHandlers();

  // logo setups
  if ( GetType( "TYPE_HEARTBEAT", &TypeHB ) != 0 ) {
    fprintf( stderr,
	     "%s: Invalid message type <TYPE_HEARTBEAT>\n", Q3302EW_NAME);
    return( -1 );
  }
  if ( GetType( "TYPE_TRACEBUF2", &TypeTrace ) != 0 ) {
    fprintf( stderr,
	     "%s: Invalid message type <TYPE_TRACEBUF>; exiting!\n", Q3302EW_NAME);
    return(-1);
  }

  setuplogo(&DataLogo);
  setuplogo(&OtherLogo);
  DataLogo.type = TypeTrace;

#ifndef WIN32
  // attach to the shared memory 
  Region.mid = -1;
#endif

  tport_attach(&Region, gConfig.RingKey);
  // start our heartbeat
  time(&Heartbeat_lastBeatTime);
  StartThread((void (*)(void *))Heartbeat, 8192, &Heartbeat_threadID);

  // keep trying to register as long as 1) we haven't and 2) we're
  // not supposed to die and 3) we don't hit the max retry counts (default 5)
  lib330Interface_startRegistration();
  registration_count++;
  while(!lib330Interface_waitForState(LIBSTATE_RUN, 120) && (tport_getflag(&Region) != TERMINATE)) {
    lib330Interface_startRegistration();
    registration_count++;
    if (registration_count > gConfig.RegistrationCyclesLimit) 
    {
      logit("et", "q3302ew: regsitration limit of %d tries reached, exiting", registration_count);
      q3302ew_cleanupAndExit(0);
    }
    else
    {
      logit("et", "q3302ew: retrying registration: %d\n", registration_count);
    }
  }

  // now we're registered and getting data.  We'll keep doing so until we're told to stop.
  lastStatusUpdate = time(NULL);
  while(tport_getflag( &Region ) != TERMINATE) {
    if( (time(NULL) - lastStatusUpdate) >= gConfig.statusinterval ) {
      lib330Interface_displayStatusUpdate();
      lastStatusUpdate = time(NULL);
    }
    sleep_ew(1000);
    /* new code to detect if we fall into a WAIT for registration state for too long */
    if (lib330Interface_waitForState(LIBSTATE_WAIT, 1) == 1) {
       wait_counter++;
    } else {
       wait_counter = 0;
    }
    if (wait_counter >= MAX_WAIT_STATE_BEFORE_EXIT) {
       logit("et", "q3302ew: hung in wait state for more than: %d seconds\n", MAX_WAIT_STATE_BEFORE_EXIT);
       q3302ew_cleanupAndExit(0);
    }
  }
  ShutMeDown = 1;
  // we've been asked to terminate
  q3302ew_cleanupAndExit(0);
  return(0);	/* should never reach here, but left in place to close a warning */
}
