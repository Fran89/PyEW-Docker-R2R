#ifndef EW2MOLEDB_HEARTBEAT_H
#define EW2MOLEDB_HEARTBEAT_H 1

#ifdef _WINNT
#include <winsock.h>
#endif /* _WINNT */

char *get_sqlstr_from_heartbeat_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname);

#endif

