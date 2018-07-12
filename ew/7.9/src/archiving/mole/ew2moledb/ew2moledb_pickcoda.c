#include <stdio.h>
#include <earthworm.h>
#include <rdpickcoda.h>

#include "ew2moledb_pickcoda.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"

int sendmail_pick_scnl_ew_struct(EWPICK *pick) {
    int ret = 0;
    char tpick_mysql_datetime[EW2MOLEDB_DATETIME_MAXLEN_STR];

#define MAX_SUBJECT_LEN 1024
    static char subject[MAX_SUBJECT_LEN] = "pick_scnl message";
#define MAIL_MSG_BODY_SIZE 32000
    char mail_msg_body[MAIL_MSG_BODY_SIZE];

    char *msgtype_name;
    char *modid_name;
    char *instid_name;

    subject[MAX_SUBJECT_LEN - 1] = 0;
    mail_msg_body[MAIL_MSG_BODY_SIZE - 1] = 0;

    msgtype_name = GetTypeName(pick->msgtype);
    modid_name = GetModIdName(pick->modid);
    instid_name = GetInstName(pick->instid);

    epoch_to_mysql_datetime_str(tpick_mysql_datetime, EW2MOLEDB_DATETIME_MAXLEN_STR, pick->tpick);

    /*
    snprintf(mail_msg_body, MAIL_MSG_BODY_SIZE - 1, "\n\
Message: %s; Module: %s; Installation: %s\n\
*/
    snprintf(mail_msg_body, MAIL_MSG_BODY_SIZE - 1, "\n\
Sequence: %d\n\
Station: %s.%s.%s.%s\n\
FM: %c\n\
Weight: %d\n\
Time pick: %s\n\
Time pick: %f\n\
Pamp: %ld, %ld, %ld",
	    /*
	    msgtype_name,
	    modid_name,
	    instid_name,
	    */
	    /*
	       pick->msgtype,
	       pick->modid,
	       pick->instid,
	       */
	    pick->seq,
	    pick->site,
	    pick->net,
	    pick->comp,
	    pick->loc,
	    pick->fm,
	    pick->wt,
	    tpick_mysql_datetime,
	    pick->tpick,
	    pick->pamp[0],
	    pick->pamp[1],
	    pick->pamp[2]
		);


	    // logit("t", "%s\n", mail_msg_body);

	    ret = ew2moledb_SendMail(subject, mail_msg_body);

	    return ret;
}


#define MAX_SQLSTR_PICKCODA 4096
char *get_sqlstr_from_pick_scnl_ew_struct(EWPICK *pick, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_PICKCODA];
    char tpick_mysql_datetime[EW2MOLEDB_DATETIME_MAXLEN_STR];

    /*
    char *msgtype_name;
    char *modid_name;
    char *instid_name;

    msgtype_name = GetTypeName(pick->msgtype);
    modid_name = GetModIdName(pick->modid);
    instid_name = GetInstName(pick->instid);
    */

    sqlstr[MAX_SQLSTR_PICKCODA - 1] = 0;

    epoch_to_mysql_datetime_str(tpick_mysql_datetime, EW2MOLEDB_DATETIME_MAXLEN_STR, pick->tpick);

    snprintf(sqlstr, MAX_SQLSTR_PICKCODA - 1, "CALL sp_ins_ew_pick_scnl('%s', '%s', %d, '%s', '%s', '%s', '%s', '%c', %c, '%s', %ld, %ld, %ld);",
	    /*
	    msgtype_name,
	    modid_name,
	    instid_name,
	    */
	    ewinstancename,
	    modname,
	    pick->seq,
	    pick->net,
	    pick->site,
	    pick->comp,
	    pick->loc,
	    pick->fm,
	    pick->wt,
	    /*
	    pick->tpick,
	    */
	    tpick_mysql_datetime,
	    pick->pamp[0],
	    pick->pamp[1],
	    pick->pamp[2]
	   );

    return sqlstr;
}

char *get_sqlstr_from_coda_scnl_ew_struct(EWCODA *coda, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_PICKCODA];

    /*
    char *msgtype_name;
    char *modid_name;
    char *instid_name;

    msgtype_name = GetTypeName(coda->msgtype);
    modid_name = GetModIdName(coda->modid);
    instid_name = GetInstName(coda->instid);
    */

    sqlstr[MAX_SQLSTR_PICKCODA - 1] = 0;

    snprintf(sqlstr, MAX_SQLSTR_PICKCODA - 1, "CALL sp_ins_ew_coda_scnl('%s', '%s', %d, '%s', '%s', '%s', '%s', %ld, %ld, %ld, %ld, %ld, %ld, %d);",
	    /*
	    msgtype_name,
	    modid_name,
	    instid_name,
	    */
	    ewinstancename,
	    modname,
	    coda->seq,
	    coda->net,
	    coda->site,
	    coda->comp,
	    coda->loc,
	    coda->caav[0],
	    coda->caav[1],
	    coda->caav[2],
	    coda->caav[3],
	    coda->caav[4],
	    coda->caav[5],
	    coda->dur
	   );

    return sqlstr;
}


char *get_sqlstr_from_pick_scnl_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    char Text[1024];
    EWPICK pick;

    /* Read the pick into an EWPICK struct
     *  *************************************/
    if( rd_pick_scnl( msg, msg_size, &pick ) != EW_SUCCESS )
    {
	sprintf( Text, "Error reading pick: %s", msg );
	logit("e", "%s\n", Text);
    } else {
	ret = get_sqlstr_from_pick_scnl_ew_struct(&pick, ewinstancename, modname);
	if(ew2moledb_nmailRecipients > 0) {
	    // sendmail_pick_scnl_ew_struct(&pick);
	}
    }

    return ret;
}

char *get_sqlstr_from_coda_scnl_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    char Text[1024];
    EWCODA coda;

    /* Read the coda into an EWCODA struct
     *  *************************************/
    if( rd_coda_scnl( msg, msg_size, &coda ) != EW_SUCCESS )
    {
	sprintf( Text, "Error reading coda: %s", msg );
	logit("e", "%s\n", Text);
    } else {
	ret = get_sqlstr_from_coda_scnl_ew_struct(&coda, ewinstancename, modname);
    }

    return ret;
}

