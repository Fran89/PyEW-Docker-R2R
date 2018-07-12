#ifndef __LOGGING_H__
#define __LOGGING_H__

void logInitialize(char *configFile, int logFlag);
void logMessage(char *fmt, ...);

#endif
