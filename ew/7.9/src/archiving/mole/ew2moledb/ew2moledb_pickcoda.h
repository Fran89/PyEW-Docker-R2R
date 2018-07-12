#ifndef EW2MOLEDB_PICKCODA_H
#define EW2MOLEDB_PICKCODA_H 1

char *get_sqlstr_from_pick_scnl_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname);
char *get_sqlstr_from_coda_scnl_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname);

#endif

