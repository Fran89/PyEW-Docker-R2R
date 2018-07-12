/**
 * Interface functions for lib330
 **/
#include "q3302ew.h"
#include "config.h"
#include "externs.h"
#include "logging.h"
#include "libsupport.h"
#include "earthworm.h"
#include "lib330Interface.h"
#include "q3302ew_platform.h"

/**
 * EW includes
 **/
#include "trace_buf.h"

enum tlibstate currentLibState;
tpar_register registrationInfo;
tpar_create creationInfo;
tcontext stationContext;


double janFirst2000 =  946684800.000000;

// Windows' _strtoi64 is the same as strtoll
#ifdef WIN32
#define strtoll _strtoi64
WSADATA wdata;
#endif

/**
 * Fill out the creationInfo struct.
 */
void lib330Interface_initializeCreationInfo() {
  // Fill out the parts of the creationInfo that we know about
  uint64 serial;
  char continuityFile[512];

  serial = strtoll(gConfig.serialnumber, NULL, 16);

  memcpy(creationInfo.q330id_serial, &serial, sizeof(uint64));
  switch(gConfig.dataport) {
    case 1:
      creationInfo.q330id_dataport = LP_TEL1;
      break;
    case 2:
      creationInfo.q330id_dataport = LP_TEL2;
      break;
    case 3:
      creationInfo.q330id_dataport = LP_TEL3;
      break;
    case 4:
      creationInfo.q330id_dataport = LP_TEL4;
      break;
  }
  strncpy(creationInfo.q330id_station, "UNKN", 5);
  creationInfo.host_timezone = 0;
  strcpy(creationInfo.host_software, Q3302EW_NAME);
  if(strlen(gConfig.ContFileDir)) {
    sprintf(continuityFile, "%s/Q3302EW_cont_%s.bin", gConfig.ContFileDir, gConfig.ConfigFileName);
  } else {
    sprintf(continuityFile, "Q3302EW_cont_%s.bin", gConfig.ConfigFileName);
  }
  strcpy(creationInfo.opt_contfile, continuityFile);
  creationInfo.opt_verbose = gConfig.LogLevel;
  creationInfo.opt_zoneadjust = 1;
  creationInfo.opt_secfilter = OMF_ALL;
  creationInfo.opt_minifilter = 0;
  creationInfo.opt_aminifilter = 0;
  creationInfo.amini_exponent = 0;
  creationInfo.amini_512highest = -1000;
  creationInfo.mini_embed = 0;
  creationInfo.mini_separate = 1;
  creationInfo.mini_firchain = 0;
  creationInfo.call_minidata = NULL;
  creationInfo.call_aminidata = NULL;
  creationInfo.resp_err = LIBERR_NOERR;
  creationInfo.call_state = lib330Interface_stateCallback;
  creationInfo.call_messages = lib330Interface_msgCallback;
  creationInfo.call_secdata = lib330Interface_1SecCallback;
  creationInfo.call_lowlatency = NULL;
}


/**
 * Set up the registration info structure from the config
 **/
void lib330Interface_initializeRegistrationInfo() {
  uint64 auth = strtoll(gConfig.authcode, NULL, 16);
  memcpy(registrationInfo.q330id_auth, &auth, sizeof(uint64));
  strcpy(registrationInfo.q330id_address, gConfig.IPAddress);
  registrationInfo.q330id_baseport = gConfig.baseport;
  registrationInfo.host_mode = HOST_ETH;
  strcpy(registrationInfo.host_interface, "");
  registrationInfo.host_mincmdretry = 5;
  registrationInfo.host_maxcmdretry = 40;
  registrationInfo.host_ctrlport = gConfig.SourcePortControl;
  registrationInfo.host_dataport = gConfig.SourcePortData;
  registrationInfo.opt_latencytarget = 0;
  registrationInfo.opt_closedloop = 0;
  registrationInfo.opt_dynamic_ip = 0;
  registrationInfo.opt_hibertime = gConfig.MinutesToSleepBeforeRetry;
  registrationInfo.opt_conntime = gConfig.Dutycycle_MaxConnectTime;
  registrationInfo.opt_connwait = gConfig.Dutycycle_SleepTime;
  registrationInfo.opt_regattempts = gConfig.FailedRegistrationsBeforeSleep;
  registrationInfo.opt_ipexpire = 0;
  registrationInfo.opt_buflevel = gConfig.Dutycycle_BufferLevel;
}


void lib330Interface_displayStatusUpdate() {
  enum tlibstate currentState;
  enum tliberr lastError;
  topstat libStatus;
  time_t rightNow = time(NULL);
  int i;

  currentState = lib_get_state(stationContext, &lastError, &libStatus);

  // do some internal maintenence if required (this should NEVER happen)
  if(currentState != lib330Interface_getLibState()) {
    string63 newStateName;
    logMessage("XXX Current lib330 state mismatch.  Fixing...\n");
    lib_get_statestr(currentState, &newStateName);
    logMessage("+++ State change to '%s'\n", newStateName);
    lib330Interface_libStateChanged(currentState);
  }


  // version and localtime
  logMessage("+++ %s %s %s status for %s.  Local time: %s", Q3302EW_NAME, Q3302EW_VERSION, Q3302EW_BUILD,
	     libStatus.station_name, ctime(&rightNow));
  
  // BPS entries
  logMessage("--- Bps from Q330 (min/hour/day): ");
  for(i=(int)AD_MINUTE; i <= (int)AD_DAY; i = i + 1) {
    if((int)libStatus.accstats[AC_READ][i] != (int)INVALID_ENTRY) {
      logMessage("%dBps", (int) libStatus.accstats[AC_READ][i]);
    } else {
      logMessage("---");
    }
    if(i != AD_DAY) {
      logMessage("/");
    } else {
      logMessage("\n");
    }
  }

  logMessage("--- Packets from Q330 (min/hour/day): ");
  for(i=(int)AD_MINUTE; i <= (int)AD_DAY; i = i + 1) {
    if((int)libStatus.accstats[AC_PACKETS][i] != (int)INVALID_ENTRY) {
      logMessage("%dPkts", (int) libStatus.accstats[AC_PACKETS][i]);
    } else {
      logMessage("---");
    }
    if(i != AD_DAY) {
      logMessage("/");
    } else {
      logMessage("\n");
    }
  }

  // percent of the buffer left, and the clock quality
  logMessage("--- Q330 Packet Buffer Available: %d Clock Quality: %d\n", 100-((int)libStatus.pkt_full), 
	     (int)libStatus.clock_qual);
}


/**
 * Handle and log errors coming from lib330
 **/
void lib330Interface_handleError(enum tliberr errcode) {
    string63 errmsg;
    lib_get_errstr(errcode, &errmsg);
    logMessage("XXX : Encountered error: %s\n", errmsg);
}

/**
 * Initialize the lib330 interface.  Setting up the creation info
 * registration info, log all module revs etc...
 **/
void lib330Interface_initialize() {
  pmodules modules;
  tmodule *module;
  int x;

#ifdef WIN32
  WSAStartup(0x101, &wdata);
#endif 
  currentLibState = LIBSTATE_IDLE;
  lib330Interface_initializeCreationInfo();
  lib330Interface_initializeRegistrationInfo();

  modules = lib_get_modules();
  logMessage("+++ Lib330 Modules:\n");
  for(x=0; x <= MAX_MODULES - 1; x++) {
    module = &(*modules)[x];
    if(!module->name[0]) {
      continue;
    }
    if( !(x % 4)) {
      if(x > 0) {
	logMessage("\n");
      }
      if(x < MAX_MODULES-1) {
	logMessage("+++ ");
      }
    }
    logMessage("%s:%d ", module->name, module->ver);
  }
  logMessage("\n");
  logMessage("+++ Initializing station thread\n");
  lib_create_context(&(stationContext), &(creationInfo));
  if(creationInfo.resp_err == LIBERR_NOERR) {
    logMessage("+++ Station thread created\n");
  } else {
    lib330Interface_handleError(creationInfo.resp_err);
  }

}

/**
 * Set the the interface up for the new state
 */
void lib330Interface_libStateChanged(enum tlibstate newState) {
  string63 newStateName;

  lib_get_statestr(newState, &newStateName);
  logMessage("+++ State change to '%s'\n", newStateName);
  currentLibState = newState;

  /*
  ** We have no good reason for sitting in RUNWAIT, so lets just go
  */
  if(currentLibState == LIBSTATE_RUNWAIT) {
    lib330Interface_startDataFlow();
  }
}

/**
 * What state are we currently in?
 **/
enum tlibstate lib330Interface_getLibState() {
  return currentLibState;
}

/**
 * Start acquiring data
 **/
void lib330Interface_startDataFlow() {
  logMessage("+++ Requesting dataflow to start\n");
  lib330Interface_changeState(LIBSTATE_RUN, LIBERR_NOERR);
}

/**
 * Initiate the registration process
 **/
void lib330Interface_startRegistration() { 
  enum tliberr errcode;
  lib330Interface_ping();
  logMessage("+++ Starting registration with Q330\n");
  errcode = lib_register(stationContext, &(registrationInfo));
  if(errcode != LIBERR_NOERR) {
    lib330Interface_handleError(errcode);
  }  
}

/**
 * Request that the lib change its state
 **/
void lib330Interface_changeState(enum tlibstate newState, enum tliberr reason) {
  lib_change_state(stationContext, newState, reason);
}

void lib330Interface_cleanup() {
  enum tliberr errcode;
  logMessage("+++ Cleaning up lib330 Interface\n");
  lib330Interface_startDeregistration();
  while(lib330Interface_getLibState() != LIBSTATE_IDLE) {
    sleep_ew(1000);
  }
  lib330Interface_changeState(LIBSTATE_TERM, LIBERR_CLOSED);
  while(lib330Interface_getLibState() != LIBSTATE_TERM) {
    sleep_ew(1000);
  }
  errcode = lib_destroy_context(&(stationContext));
  if(errcode != LIBERR_NOERR) {
    lib330Interface_handleError(errcode);
  }
  logMessage("+++ lib330 Interface closed\n");
}

/**
 * Request a deregistration
 **/
void lib330Interface_startDeregistration() {
  logMessage("+++ Starting deregistration from Q330\n");
  lib330Interface_changeState(LIBSTATE_IDLE, LIBERR_NOERR);
}

/**
 * Send a ping to the station
 **/
void lib330Interface_ping() {
  lib_unregistered_ping(stationContext, &(registrationInfo));
}

/**
 * Wait for a particular state to arrive, or timeout after maxSecondsToWait.
 * return value indicated whether we reached the desired state or not
 **/
int lib330Interface_waitForState(enum tlibstate waitFor, int maxSecondsToWait) {
  int i;
  for(i=0; i < maxSecondsToWait; i++) {
    if(lib330Interface_getLibState() != waitFor) {
      sleep_ew(1000);
    } else {
      return 1;
    }
  }
  return 0;
}

/**
 * Below here are several callbacks for lib330 to use for various events
 **/

void lib330Interface_stateCallback(pointer p){
  tstate_call *state;

  state = (tstate_call *)p;

  if(state->state_type == ST_STATE) {
    lib330Interface_libStateChanged((enum tlibstate)state->info);
  }
}

void lib330Interface_msgCallback(pointer p){
  tmsg_call *msg = (tmsg_call *) p;
  string95 msgText;
  char dataTime[32];

  lib_get_msg(msg->code, &msgText);
  
  // we don't need to worry about current time, the log system handles that
  //jul_string(msg->timestamp, &currentTime);
  if(!msg->datatime) {
    logMessage("{%d} %s %s\n", msg->code, msgText, msg->suffix);
  } else {
    jul_string(msg->datatime, (char *) &dataTime);
    logMessage("{%d} [%s] %s %s\n", msg->code, dataTime, msgText, msg->suffix);
  }    
}

void lib330Interface_1SecCallback(pointer p){
  tonesec_call *data = (tonesec_call *) p;
  TracePacket ewPacket;
  TRACE2_HEADER *tpHeaderPtr = &(ewPacket.trh2);
  void *tpDataPtr =  &(ewPacket.msg[sizeof(TRACE2_HEADER)]);
  char *sta, *net;
  string9 netsta;
  int32 messageSize;
  /**
   * set up the trace header
   **/
  tpHeaderPtr->pinno = 0;
  
  // handle the sub 1hz channels differently
  if(data->rate > 0) {
    tpHeaderPtr->nsamp = data->rate;
    tpHeaderPtr->samprate = data->rate;
  } else {
    tpHeaderPtr->nsamp = 1;
    tpHeaderPtr->samprate = -1.0 * ((double)1.0 / data->rate);
  }
  
  tpHeaderPtr->starttime = janFirst2000 + data->timestamp;
  //figure out the time of the last sample
  tpHeaderPtr->endtime = tpHeaderPtr->starttime + ((tpHeaderPtr->nsamp-1) / tpHeaderPtr->samprate);
  
  // seperate the station from the net
  strcpy(netsta, data->station_name);
  net = netsta;
  sta = netsta;
  while(*sta != '-' && *sta != '\0') {
    sta++;
  }
  if(*sta == '-') {
    *sta = '\0';
    sta++;
  } else {
    char *tmp;
    tmp = sta;
    sta = net;
    net = tmp;
  }
  strcpy(tpHeaderPtr->sta, sta);
  strcpy(tpHeaderPtr->net, net);
  strcpy(tpHeaderPtr->chan, data->channel);
  /* if our location is "  " or "", make it "--" */
  if( (data->location[0] == ' ' && data->location[1] == ' ') || 
       data->location[0] == '\0') {
      strcpy(tpHeaderPtr->loc, "--");
  } else {
      strcpy(tpHeaderPtr->loc, data->location);
  }
  tpHeaderPtr->version[0] = TRACE2_VERSION0;
  tpHeaderPtr->version[1] = TRACE2_VERSION1;
#ifdef ENDIAN_LITTLE
  strcpy(tpHeaderPtr->datatype,"i4");
#else
  strcpy(tpHeaderPtr->datatype, "s4");
#endif
  memcpy(tpHeaderPtr->quality, "  ", 2); /* FILL THIS IN */

  // copy the data
  memcpy(tpDataPtr, data->samples, tpHeaderPtr->nsamp * sizeof(longint));
  
  // put the data into the ring
  messageSize = (tpHeaderPtr->nsamp*sizeof(longint)) + sizeof(TRACE2_HEADER);
  if(tport_putmsg(&Region, &DataLogo, messageSize, (char *)(&ewPacket)) != PUT_OK) {
    logMessage("XXX Error putting packet into ring\n");
  }
}

