#ifndef EW2MOLEDB_ERROR_H
#define EW2MOLEDB_ERROR_H 1

#ifdef _WINNT
#include <winsock.h>
#endif /* _WINNT */

char *get_sqlstr_from_error_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname);

#endif

