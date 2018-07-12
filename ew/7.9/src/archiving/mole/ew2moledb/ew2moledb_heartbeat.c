#include <stdio.h>
#include <string.h>

#include "ew2moledb_heartbeat.h"
#include "ew2moledb_mysql.h"

#define EW2MOLEDB_MAXLEN_HEARTBEAT_MSG 4096

char *get_sqlstr_from_heartbeat_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    static char sqlstr[EW2MOLEDB_MAXLEN_HEARTBEAT_MSG];
    long int time_heartbeat;
    char mysqlstr_datetime_heartbeat[EW2MOLEDB_DATETIME_MAXLEN_STR];
    long pid;

    sqlstr[EW2MOLEDB_MAXLEN_HEARTBEAT_MSG - 1] = 0;

    sscanf(msg, "%ld %ld", &time_heartbeat, &pid);
    epoch_to_mysql_datetime_str(mysqlstr_datetime_heartbeat, EW2MOLEDB_DATETIME_MAXLEN_STR, time_heartbeat);

    snprintf(sqlstr, EW2MOLEDB_MAXLEN_HEARTBEAT_MSG - 1, "CALL sp_ins_ew_heartbeat('%s', '%s', '%s', %ld);",
	    ewinstancename,
	    modname,
	    mysqlstr_datetime_heartbeat,
	    pid
	    );

    return sqlstr;
}

