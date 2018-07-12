#ifndef _CONFIG_H_
#define _CONFIG_H_
#include "transport.h"
#include "q3302ew_platform.h"


/* what is in our config */
typedef struct {
  char ConfigFileName[255];
  char ModuleId[40];
  char RingName[40];
  long RingKey;
  int32  HeartbeatInt;
  int32  LogFile;
  char IPAddress[16];
  int32  baseport;
  int32  dataport;
  char serialnumber[40];
  char authcode[40];
  char ContFileDir[255];
  int32  statusinterval;
  int32 LogLevel;
  int32 SourcePortControl;
  int32 SourcePortData;
  int32 FailedRegistrationsBeforeSleep;
  int32 MinutesToSleepBeforeRetry;
  int32 Dutycycle_MaxConnectTime;
  int32 Dutycycle_SleepTime;
  int32 Dutycycle_BufferLevel;
  int32 RegistrationCyclesLimit;
} Configuration;

extern Configuration gConfig;


int32 readConfig(char *);

void printConfigStructToLog();
void setupDefaultConfiguration();
#endif
