#include <stdio.h>
#include <string.h>

#include <earthworm.h>
#include <chron3.h>
#include <rw_strongmotionII.h>

#include "ew2moledb_strongmotionII.h"
#include "ew2moledb_mysql.h"


#define MAX_SQLSTR_STRONGMOTIONII 4069
#define MAX_RSA_STRING 512
char *get_sqlstr_from_strongmotionII_ew_struct(SM_INFO *strongmotionII_msg, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_STRONGMOTIONII];
    char RSA_string[MAX_RSA_STRING];
    char tmp[64];
    int i;
    char mysqlstr_datetime_t[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_talt[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_tpga[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_tpgv[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_tpgd[EW2MOLEDB_DATETIME_MAXLEN_STR];

    /* TODO version */
    /* long int version = -1; */

    sqlstr[MAX_SQLSTR_STRONGMOTIONII - 1] = 0;
    RSA_string[MAX_RSA_STRING - 1] = 0;
    tmp[64 - 1] = 0;

    /* Print the response spectrum */
    snprintf( RSA_string, MAX_RSA_STRING - 1, "RSA: %d", strongmotionII_msg->nrsa );
    for( i=0; i<strongmotionII_msg->nrsa; i++ )
    {
	snprintf( tmp, 64 - 1, "/%.2lf %.6lf", strongmotionII_msg->pdrsa[i], strongmotionII_msg->rsa[i] );
	if (strlen(tmp) + 1 > MAX_RSA_STRING - strlen(RSA_string)) {
	    logit("e", "RSA_string from strongmotionII messge would be truncated.\n");
	} else {
	    strncat(RSA_string, tmp, MAX_RSA_STRING - strlen(RSA_string) - 1);
	}
    }

    epoch_to_mysql_datetime_str(mysqlstr_datetime_t, EW2MOLEDB_DATETIME_MAXLEN_STR, strongmotionII_msg->t);
    epoch_to_mysql_datetime_str(mysqlstr_datetime_talt, EW2MOLEDB_DATETIME_MAXLEN_STR, strongmotionII_msg->talt);

    epoch_to_mysql_datetime_str(mysqlstr_datetime_tpga, EW2MOLEDB_DATETIME_MAXLEN_STR, strongmotionII_msg->tpga);
    epoch_to_mysql_datetime_str(mysqlstr_datetime_tpgv, EW2MOLEDB_DATETIME_MAXLEN_STR, strongmotionII_msg->tpgv);
    epoch_to_mysql_datetime_str(mysqlstr_datetime_tpgd, EW2MOLEDB_DATETIME_MAXLEN_STR, strongmotionII_msg->tpgd);

    snprintf(sqlstr, MAX_SQLSTR_STRONGMOTIONII - 1, "CALL sp_ins_ew_strongmotionII('%s', '%s', %s, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %lf, '%s', %lf, '%s', %lf, '%s', '%s');",
	    ewinstancename,
	    modname,
	    strongmotionII_msg->qid,
	    strongmotionII_msg->sta,
	    strongmotionII_msg->comp,
	    strongmotionII_msg->net,
	    strongmotionII_msg->loc,
	    strongmotionII_msg->qauthor,
	    /*
	    strongmotionII_msg->t,
	    strongmotionII_msg->talt,
	    */
	    mysqlstr_datetime_t,
	    mysqlstr_datetime_talt,
	    strongmotionII_msg->altcode,
	    strongmotionII_msg->pga,
	    /*
	    strongmotionII_msg->tpga,
	    */
	    mysqlstr_datetime_tpga,
	    strongmotionII_msg->pgv,
	    /*
	    strongmotionII_msg->tpgv,
	    */
	    mysqlstr_datetime_tpgv,
	    strongmotionII_msg->pgd,
	    /*
	    strongmotionII_msg->tpgd,
	    */
	    mysqlstr_datetime_tpgd,
	    RSA_string
	    );

    return sqlstr;
}


char *get_sqlstr_from_strongmotionII_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    int error = 0;
    char Text[1024];
    SM_INFO strongmotionII_msg;
    char *msgP;

    /* Fill the SM_DATA struct from the received message
     *     ***************************************************/
    msgP = msg;
    while ((error = rd_strongmotionII( &msgP, &strongmotionII_msg, 1 )) != 0 ) {

	// if( rd_strongmotionII( msg, msg_size, &strongmotionII_msg ) != EW_SUCCESS )
	if(error < 1 )
	{
	    sprintf( Text, "Error reading strongmotionII: %s", msg );
	    logit("e", "%s", Text);
	} else {
	    ret = get_sqlstr_from_strongmotionII_ew_struct(&strongmotionII_msg, ewinstancename, modname);
	}

    }

    return ret;
}

