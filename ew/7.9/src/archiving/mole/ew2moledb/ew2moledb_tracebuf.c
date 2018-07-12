#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <swap.h>
#include <trace_buf.h>

#include "ew2moledb_tracebuf.h"
#include "ew2moledb_mysql.h"


#define MAX_SQLSTR_TRACEBUF 4096
char *get_sqlstr_from_tracebuf_ew_struct(TRACE2_HEADER *trh, char *p_data, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_TRACEBUF];
    char wst_mysql_datetime[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char wet_mysql_datetime[EW2MOLEDB_DATETIME_MAXLEN_STR];

    /*
    char *msgtype_name;
    char *modid_name;
    char *instid_name;

    msgtype_name = GetTypeName(pick->msgtype);
    modid_name = GetModIdName(pick->modid);
    instid_name = GetInstName(pick->instid);
    */

    sqlstr[MAX_SQLSTR_TRACEBUF - 1] = 0;

    epoch_to_mysql_datetime_str(wst_mysql_datetime, EW2MOLEDB_DATETIME_MAXLEN_STR, trh->starttime);
    epoch_to_mysql_datetime_str(wet_mysql_datetime, EW2MOLEDB_DATETIME_MAXLEN_STR, trh->endtime);

    snprintf(sqlstr, MAX_SQLSTR_TRACEBUF - 1, "CALL sp_ins_ew_tracebuf('%s', '%s', %d, %d, '%s', '%s', %f, '%s', '%s', '%s', '%s', '%d-%d', '%s', '%d-%d');",
	    /*
	    msgtype_name,
	    modid_name,
	    instid_name,
	    */
	    ewinstancename,
	    modname,
	    trh->pinno,
	    trh->nsamp,
	    wst_mysql_datetime,
	    wet_mysql_datetime,
	    trh->samprate,
	    trh->net,
	    trh->sta,
	    trh->chan,
	    trh->loc,
	    trh->version[0],
	    trh->version[1],
	    trh->datatype,
	    trh->quality[0],
	    trh->quality[1]
	   );

    return sqlstr;
}


char *get_sqlstr_from_tracebuf_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    char Text[1024];
    char  *new_msg = NULL;
    TRACE2_HEADER  *trh;
    char *p_data=NULL;

    /* Read the pick into an EWPICK struct
     *  *************************************/
    new_msg = (char *) malloc(msg_size);

    if(new_msg == NULL) {
	sprintf( Text, "Error allocating memory for TRACEBUF message: %s", msg );
	logit("e", "%s\n", Text);
    } else {

	memmove(new_msg, msg, msg_size);

	trh = (TRACE2_HEADER *) new_msg;
	p_data = (void *)( new_msg + sizeof(TRACE2_HEADER) );

	if(WaveMsg2MakeLocal( trh ) == 0) {
	  ret = get_sqlstr_from_tracebuf_ew_struct(trh, p_data, ewinstancename, modname);
	}

	free(new_msg);
    }

    return ret;
}

