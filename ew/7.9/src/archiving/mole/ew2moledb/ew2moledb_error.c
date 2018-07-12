#include <stdio.h>
#include <string.h>

#include "ew2moledb_error.h"
#include "ew2moledb_mysql.h"

#define EW2MOLEDB_MAXLEN_ERROR_MSG 4096

char *get_sqlstr_from_error_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    static char sqlstr[EW2MOLEDB_MAXLEN_ERROR_MSG];
    long int time_error;
    char mysqlstr_datetime_error[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char *p = msg;
    int l = msg_size;
    int i;

    sqlstr[EW2MOLEDB_MAXLEN_ERROR_MSG - 1] = 0;

    sscanf(msg, "%ld", &time_error);
    epoch_to_mysql_datetime_str(mysqlstr_datetime_error, EW2MOLEDB_DATETIME_MAXLEN_STR, time_error);

    while(l>0 && *p != ' ' && *p != 0) {
	p++;
	l--;
    }
    if(l<=0) {
	p = NULL;
    } else if(l==1) {
	if(*p == ' ') {
	    p = NULL;
	}
    } else {
	p++;
    }
    if(p) {
	p[strlen(p)-1] = 0;
    }

    /* TODO work-around, replace ';' with ','
     * Fix that when the separator ';' will be customizable */
    for(i=0; i < strlen(p); i++) {
	if(p[i] == ';') {
	    p[i] = ',';
	}
    }

    snprintf(sqlstr, EW2MOLEDB_MAXLEN_ERROR_MSG - 1, "CALL sp_ins_ew_error('%s', '%s', '%s', '%s');",
	    ewinstancename,
	    modname,
	    mysqlstr_datetime_error,
	    (p)? p : "Unknown"
	    );

    return sqlstr;
}

