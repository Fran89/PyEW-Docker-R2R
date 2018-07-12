#include <stdio.h>
#include <string.h>
#include <earthworm.h>

#include "ew2moledb_sendmail.h"

#define MAX_SUBJECT_LEN 256
int  ew2moledb_nmailRecipients = 0;
char ew2moledb_mailRecipients[EW2MOLEDB_MAXRECIP][EW2MOLEDB_MAXRECIPLEN] =  { "" };
char ew2moledb_mailProg[MAX_SIZE_STRING_SENDMAIL] = "/usr/ucb/Mail";
char ew2moledb_mailServer[MAX_SIZE_STRING_SENDMAIL] = "mail.server.com";
char ew2moledb_mailFrom[MAX_SIZE_STRING_SENDMAIL] = "ew2moledb";

static char subject_default[MAX_SUBJECT_LEN] = "ew2moledb message";
static char *msgPrefix = NULL;
static char *msgSuffix = NULL;

int ew2moledb_SendMail(char *subject, char *msg) {

    int ret = 0;

    if(subject == NULL) {
	subject = subject_default;
    }

    if(ew2moledb_nmailRecipients > 0) {
	ret = SendMail(ew2moledb_mailRecipients, ew2moledb_nmailRecipients, ew2moledb_mailProg,
		subject, msg, msgPrefix,
		msgSuffix, ew2moledb_mailServer,
		ew2moledb_mailFrom );
    };

    return ret;
}

