#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <rw_mag.h>

#include "ew2moledb_magnitude.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"

void ingv_mag_quality_magnitude_ew_struct(MAG_INFO *magSum, MAG_CHAN_INFO *magChan, char *ingv_mag_quality1, char *ingv_mag_quality2) {
    /* Initialization */
    *ingv_mag_quality1 = ' ';
    *ingv_mag_quality2 = ' ';

    if(magSum->quality <= 0.25) {
	*ingv_mag_quality1 = 'D';
    } else if(magSum->quality <= 0.5) {
	*ingv_mag_quality1 = 'C';
    } else if(magSum->quality <= 0.75) {
	*ingv_mag_quality1 = 'B';
    } else {
	*ingv_mag_quality1 = 'A';
    }

}

int sendmail_magnitude_ew_struct(MAG_INFO *magSum, MAG_CHAN_INFO *magChan, char *ewinstancename, char *modname) {
    int ret = 0;
    int i;

#define MAX_SUBJECT_LEN 256
	    static char subject[MAX_SUBJECT_LEN] = "Localmag message";

#define MAIL_MSG_BODY_SIZE 32000
#define MAIL_MSG_LINE_SIZE 4096
    char mail_msg_body[MAIL_MSG_BODY_SIZE];
    char mail_msg_line[MAIL_MSG_LINE_SIZE];

    subject[MAX_SUBJECT_LEN - 1] = 0;
    mail_msg_body[MAIL_MSG_BODY_SIZE - 1] = 0;
    mail_msg_line[MAIL_MSG_LINE_SIZE - 1] = 0;

    snprintf(subject, MAX_SUBJECT_LEN - 1, "ID %s.%u - %s %.2lf -  Err: %.2lf - Quality: %.2lf (%s)",
	    magSum->qid,
	    magSum->origin_version,
	    magSum->szmagtype,
	    magSum->mag,
	    magSum->error,
	    magSum->quality,
	    modname
	    );

    snprintf(mail_msg_body, MAIL_MSG_BODY_SIZE - 1, "\
Magnitude: %lf\n\
Error: %lf\n\
Quality: %lf\n\
Mindist: %lf\n\
Azimuth: %d\n\
Number stations: %d\n\
Number channels: %d\n\
Quake ID: %s\n\
Quake Author: %s\n\
Origin version: %u\n\
QDDS version: %u\n\
IMagType: %d\n\
SZMagType: %s\n\
Algorithm: %s\n\
TLoad: %lf\n\n",
	    magSum->mag,
	    magSum->error,
	    magSum->quality,
	    magSum->mindist,
	    magSum->azimuth,
	    magSum->nstations,
	    magSum->nchannels,
	    magSum->qid,
	    magSum->qauthor,
	    magSum->origin_version,
	    magSum->qdds_version,
	    magSum->imagtype,
	    magSum->szmagtype,
	    magSum->algorithm,
	    magSum->tload);

	    for(i=0; i < magSum->nchannels; i++) {
		/* TODO */
		snprintf(mail_msg_line, MAIL_MSG_LINE_SIZE - 1, "\n\n\
Station: %s.%s.%s.%s\n\
Magnitude: %lf\n\
Distance: %lf\n\
Corr: %lf\n\
Time1: %lf\n\
Amp1: %f\n\
Period1: %f\n\
Time2: %lf\n\
Amp2: %f\n\
Period2: %f)",
			magChan[i].sta,
			magChan[i].comp,
			magChan[i].net,
			magChan[i].loc,
			magChan[i].mag,
			magChan[i].dist,
			magChan[i].corr,
			magChan[i].Time1,
			magChan[i].Amp1,
			magChan[i].Period1,
			magChan[i].Time2,
			magChan[i].Amp2,
			magChan[i].Period2
			    );

			if (strlen(mail_msg_line) + 1 > MAIL_MSG_BODY_SIZE - strlen(mail_msg_body)) {
			    logit("e", "mail_msg_body for magnitude messge would be truncated.\n");
			} else {
			    strncat(mail_msg_body, mail_msg_line, MAIL_MSG_BODY_SIZE - strlen(mail_msg_body) - 1);
			}


	    }

	    // logit("t", "%s\n", mail_msg_body);

	    ret = ew2moledb_SendMail(subject, mail_msg_body);

	    return ret;
}

#define MAX_SQLSTR_MAGNITUDE 65536*4
#define MAX_SQLSTR_TMP 4096
char *get_sqlstr_from_magnitude_ew_struct(MAG_INFO *magSum, MAG_CHAN_INFO *magChan, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_MAGNITUDE];
    char sqlstr_tmp[MAX_SQLSTR_TMP];
    char mysqlstr_datetime_Time1[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_Time2[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char ingv_mag_quality1;
    char ingv_mag_quality2;
    int i;

    sqlstr[MAX_SQLSTR_MAGNITUDE - 1] = 0;
    sqlstr_tmp[MAX_SQLSTR_TMP - 1] = 0;

    ingv_mag_quality_magnitude_ew_struct(magSum, magChan, &ingv_mag_quality1, &ingv_mag_quality2);

    snprintf(sqlstr, MAX_SQLSTR_MAGNITUDE - 1, "CALL sp_ins_ew_magnitude_summary('%s', '%s', %s, %u, %lf, %lf, %lf, %lf, %d, %d, %d, '%s', %u, %d, '%s', '%s', '%c%c');",
	    ewinstancename,
	    modname,
	    magSum->qid,
	    magSum->origin_version,
	    magSum->mag,
	    magSum->error,
	    magSum->quality,
	    magSum->mindist,
	    magSum->azimuth,
	    magSum->nstations,
	    magSum->nchannels,
	    magSum->qauthor,
	    magSum->qdds_version,
	    magSum->imagtype,
	    magSum->szmagtype,
	    magSum->algorithm,
	    ingv_mag_quality1,
	    ingv_mag_quality2
	    );

    for(i=0; i < magSum->nchannels; i++) {

	epoch_to_mysql_datetime_str(mysqlstr_datetime_Time1, EW2MOLEDB_DATETIME_MAXLEN_STR, magChan[i].Time1);
	epoch_to_mysql_datetime_str(mysqlstr_datetime_Time2, EW2MOLEDB_DATETIME_MAXLEN_STR, magChan[i].Time2);

	snprintf(sqlstr_tmp, MAX_SQLSTR_TMP - 1, "CALL sp_ins_ew_magnitude_phase('%s', '%s', %s, %u, '%s', '%s', '%s', '%s', %lf, %lf, %lf, '%s', %f, %f, '%s', %f, %f);",
		ewinstancename,
		modname,
		magSum->qid,
		magSum->origin_version,
		magChan[i].sta,
		magChan[i].comp,
		magChan[i].net,
		magChan[i].loc,
		magChan[i].mag,
		magChan[i].dist,
		magChan[i].corr,
		/*
		magChan[i].Time1,
		*/
		mysqlstr_datetime_Time1,
		magChan[i].Amp1,
		magChan[i].Period1,
		/*
		magChan[i].Time2,
		*/
		mysqlstr_datetime_Time2,
		magChan[i].Amp2,
		magChan[i].Period2
		);

		if (strlen(sqlstr_tmp) + 1 > MAX_SQLSTR_MAGNITUDE - strlen(sqlstr)) {
		    logit("e", "sqlstr for magnitude message would be truncated.\n");
		} else {
		    strncat(sqlstr, sqlstr_tmp, MAX_SQLSTR_MAGNITUDE - strlen(sqlstr) - 1);
		}

    }

    return sqlstr;
}


char *get_sqlstr_from_magnitude_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    int error = 0;
    char Text[1024];
    MAG_INFO magSum;
    MAG_CHAN_INFO *magChan = NULL;
    int magChanSize;

    /* Before calling rd_mag() set this value */
    magSum.size_aux = 0;
    magSum.pMagAux = NULL;

    /* Fill the MAG_INFO struct from the received message */
    if (rd_mag (msg, msg_size, &magSum) < 0)
    {
	sprintf( Text, "Error reading magnitude summary: %s", msg );
	logit("e", "%s", Text);
	error = -1;
    } else {
	magChanSize = 4 * MAX_PHS_PER_EQ * sizeof (MAG_CHAN_INFO);
	if ((magChan = (MAG_CHAN_INFO *) malloc (magChanSize)) == NULL)
	{
	    sprintf( Text, "Error allocating memory for magnitude stations");
	    logit("e", "%s", Text);
	    error = -1;
	} else {
	    /* Fill the MAG_CHAN_INFO struct from the received message */
	    if (rd_chan_mag (msg, msg_size, magChan, magChanSize) < 0)
	    {
		sprintf( Text, "Error reading magnitude station info: %s", msg );
		logit("e", "%s", Text);
		error = -1;
	    } else {
		ret = get_sqlstr_from_magnitude_ew_struct(&magSum, magChan, ewinstancename, modname);
		if(ew2moledb_nmailRecipients > 0) {
		    sendmail_magnitude_ew_struct(&magSum, magChan, ewinstancename, modname);
		}
	    }
	    free(magChan);
	}
    }

    return ret;
}

