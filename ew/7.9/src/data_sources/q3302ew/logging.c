#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "earthworm.h"
/* default log flags */
char loggingFlag[4] = "tpo";

char logBuffer[512];

void logInitialize(char *configFile, int logFlag) {
  logit_init(configFile, 0, 1024, logFlag);
  logBuffer[0] = '\0';
}


void logMessage(char *fmt, ...) {
  char tmpBuffer[512];
  auto va_list l;
  va_start(l, fmt);

  /* lets see if we have something in the buffer */
  if(strlen(logBuffer)) {
    /* yes we do, lets combine what we just got with the buffer */
    /* handle the format and args */
    vsprintf(tmpBuffer, fmt, l);
    strcat(logBuffer, tmpBuffer);
  } else {
    /* This is the start of the buffer */
    vsprintf(logBuffer, fmt, l);
  }

  va_end(l);

  /* If we have a newline at the end, we'll send it to the logging system */
  /* and clear the buffer */ 
  if(logBuffer[strlen(logBuffer)-1] == '\n') {
    logit("tpo", logBuffer);
    logBuffer[0] = '\0';
  } 
}
