#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <earlybirdlib.h>
#include <time_ew.h>
#include <chron3.h>

#include "ew2moledb_eb_alarm.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"


/*
int sendmail_eb_alarm_struct(struct Hsum *arcSum, struct Hpck *arcPck, int n_arcPck, char *ewinstancename, char *modname) {
  int ret = 0;

  ret = ew2moledb_SendMail(subject, mail_msg_body);

  return ret;
}
*/

#define MAX_SQLSTR_ALARM 65536
char *get_sqlstr_from_eb_alarm_struct(char *message, char *ewinstancename, char *modname) {
  static char sqlstr[MAX_SQLSTR_ALARM];

  sqlstr[MAX_SQLSTR_ALARM - 1] = 0;

  snprintf(sqlstr, MAX_SQLSTR_ALARM - 1,
      "CALL sp_ins_eb_alarm('%s', '%s', "
      "'%s');",

      ewinstancename,
      modname,
      message
      );

  return sqlstr;
}

char *get_sqlstr_from_eb_alarm_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    int error = 0;
    char *message = msg;
    int nf;
    long  iMessageType, iModId, iInst;  /* Incoming logo */

    nf = sscanf(msg, "%5ld %5ld %5ld", &iMessageType, &iModId, &iInst);
    if(nf == 3  &&  msg_size > 18) {
      message = message+18;
    }

    if(error != -1) {

	ret = get_sqlstr_from_eb_alarm_struct(message, ewinstancename, modname);

	if(ew2moledb_nmailRecipients > 0) {
	  /* sendmail_eb_alarm_struct(&arcSum, arcPck, n_arcPck, ewinstancename, modname); */
	}
    }

    return ret;
}

