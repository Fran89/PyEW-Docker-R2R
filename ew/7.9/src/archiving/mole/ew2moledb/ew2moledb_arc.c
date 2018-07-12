#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <time_ew.h>
#include <chron3.h>
#include <read_arc.h>

#include "ew2moledb_arc.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"

void ingv_quality_arc_ew_struct(struct Hsum *arcSum, struct Hpck *arcPck, int n_arcPck, char *ingv_quality1, char *ingv_quality2) {
    double Nobs;
    double Max_wt;
    double error_depth;
    double error_horizontal;
    double rms;
    double gap;
    double dmin;
    double ev_depth;
    int i;

    /* Save maxima values for Pwt and Swt */
    Max_wt = -100.0;
    for(i=0; i < n_arcPck; i++) {
	if(arcPck[i].Pwt > Max_wt) {
	    Max_wt = arcPck[i].Pwt;
	}
	if(arcPck[i].Swt > Max_wt) {
	    Max_wt = arcPck[i].Swt;
	}
    }

    /* TODO Nobs */
    //  = arcSum->nph;

    /* Compute Nobs */
    Nobs = 0.0;
    for(i=0; i < n_arcPck; i++) {
	Nobs += (arcPck[i].Pwt / Max_wt);
	Nobs += (arcPck[i].Swt / Max_wt);
    }

    error_depth = arcSum->erz;
    error_horizontal = arcSum->erh;
    rms = arcSum->rms;
    gap = arcSum->gap;
    dmin = arcSum->dmin;
    ev_depth = arcSum->z;
    
    *ingv_quality1 = 'D';
    if(
	    Nobs >= 4.0 &&
	    rms <= 1.5 &&
	    error_depth <= 10.0
      ) {
	*ingv_quality1 = 'C';
    }
    if(
	    Nobs >= 4.0 &&
	    rms <= 0.9 &&
	    error_horizontal <= 5.0 &&
	    error_depth <= 10.0
      ) {
	*ingv_quality1 = 'B';
    }
    if(
	    Nobs >= 4.0 &&
	    rms <= 0.45 &&
	    error_horizontal <= 2.0 &&
	    error_depth <= 4.0
      ) {
	*ingv_quality1 = 'A';
    }


    *ingv_quality2 = 'D';
    if(
	    Nobs >= 6.0 &&
	    gap <= 180.0 &&
	    dmin <= 100.0
      ) {
	*ingv_quality2 = 'C';
    }

    if(
	    Nobs >= 6.0 &&
	    gap <= 135.0 &&
	    ( dmin <= 20.0 || dmin <= (2.0 * ev_depth))
      ) {
	*ingv_quality2 = 'B';
    }

    if(
	    Nobs >= 6.0 &&
	    gap <= 90.0 &&
	    ( dmin <= 10.0 || dmin <= ev_depth)
      ) {
	*ingv_quality2 = 'A';
    }

}

int sendmail_arc_ew_struct(struct Hsum *arcSum, struct Hpck *arcPck, int n_arcPck, char *ewinstancename, char *modname) {
    int ret = 0;
    int i;

    char ingv_quality1;
    char ingv_quality2;

#define MAX_SUBJECT_LEN 1024
    static char subject[MAX_SUBJECT_LEN] = "Hypoinverse message";

    char str_ot[256];

#define MAIL_MSG_BODY_SIZE 32000
#define MAIL_MSG_LINE_SIZE 4096
    char mail_msg_body[MAIL_MSG_BODY_SIZE];
    char mail_msg_line[MAIL_MSG_LINE_SIZE];

    ingv_quality_arc_ew_struct(arcSum, arcPck, n_arcPck, &ingv_quality1, &ingv_quality2);

    datestr23 ( (arcSum->ot - GSEC1970) , str_ot, 256);

    subject[MAX_SUBJECT_LEN - 1] = 0;
    mail_msg_body[MAIL_MSG_BODY_SIZE - 1] = 0;
    mail_msg_line[MAIL_MSG_LINE_SIZE - 1] = 0;

    snprintf(subject, MAX_SUBJECT_LEN - 1, "ID %ld.%ld - %c%c - %s - (%.2f, %.2f, %.2f) (%s)",
	    arcSum->qid,
	    arcSum->version,
	    ingv_quality1,
	    ingv_quality2,
	    str_ot,
	    arcSum->lat,
	    arcSum->lon,
	    arcSum->z,
	    modname
	    );

    snprintf(mail_msg_body, MAIL_MSG_BODY_SIZE - 1, "\
http://maps.google.com/maps?q=%f,%f&iwloc=A&hl=en&t=p&z=8\n\n\
Quake ID: %ld\n\
Origin Time: %lf\n\
Latitude: %f\n\
Longitude: %f\n\
Depth: %f\n\
Number phases w/ weight >0.1: %d\n\
Number phases S w/ weight >0.1: %d\n\
Number phases w/ weight >0.0: %d\n\
P first motions: %d\n\
Maximum azimuthal gap: %d\n\
Distance (km) to nearest station: %d\n\
RMS travel time residual: %f\n\
Azimuth of largest principal error: %d\n\
Dip of largest principal error: %d\n\
Magnitude (km) of largest principal error: %f\n\
Azimuth of intermediate principal error: %d\n\
Dip of intermediate principal error: %d\n\
Magnitude (km) of intermed principal error: %f\n\
Magnitude (km) of smallest principal error: %f\n\
Horizontal error (km): %f\n\
Vertical error (km): %f\n\
Duration magnitude: %f\n\
Location region: '%s'\n\
Date character buffer: '%s'\n\
Character describing preferred magnitude: '%c'\n\
Preferred magnitude: %f\n\
Weight (~ # readings) of preferred Mag: %f\n\
Coda duration magnitude type code: '%c'\n\
Median-absolute-difference of duration mags: %f\n\
weight (~ # readings) of Md: %f\n\
Version number of the origin: %ld\n\n",
	    arcSum->lat,
	    arcSum->lon,

	    arcSum->qid,
	    arcSum->ot,
	    arcSum->lat,
	    arcSum->lon,
	    arcSum->z,
	    arcSum->nph,
	    arcSum->nphS,
	    arcSum->nphtot,
	    arcSum->nPfm,
	    arcSum->gap,
	    arcSum->dmin,
	    arcSum->rms,
	    arcSum->e0az,
	    arcSum->e0dp,
	    arcSum->e0,
	    arcSum->e1az,
	    arcSum->e1dp,
	    arcSum->e1,
	    arcSum->e2,
	    arcSum->erh,
	    arcSum->erz,
	    arcSum->Md,
	    arcSum->reg,
	    arcSum->cdate,
	    arcSum->labelpref,
	    arcSum->Mpref,
	    arcSum->wtpref,
	    arcSum->mdtype,
	    arcSum->mdmad,
	    arcSum->mdwt,
	    arcSum->version
		);

	    for(i=0; i < n_arcPck; i++) {
		snprintf(mail_msg_line, MAIL_MSG_LINE_SIZE - 1, "\n\n\
Station: %s.%s.%s.%s\n\
P phase label: '%c'\n\
S phase label: '%c'\n\
P phase onset: '%c'\n\
S phase onset: '%c'\n\
P-arrival-time as sec since 1600: %lf\n\
S-arrival-time as sec since 1600: %lf\n\
P travel time residual: %f\n\
S travel time residual: %f\n\
Assigned P weight code: %d\n\
Assigned S weight code: %d\n\
Coda duration time: %d\n\
Coda weight: %d\n\
P first motion: '%c'\n\
S first motion: '%c'\n\
Date character buffer: '%s'\n\
Data source code: '%c'\n\
Station duration magnitude: %f\n\
Azimuth: %d\n\
Emergence angle at source: %d\n\
epicentral distance (km): %f\n\
P weight actually used: %f\n\
S weight actually used: %f\n\
Peak P-wave half amplitude: %d\n\
Coda duration time (Measured): %d\n\
Window center from P time: %d, %d, %d, %d, %d, %d\n\
Average Amplitude for ccntr[x]: %d, %d, %d, %d, %d, %d",
			arcPck[i].site,
			arcPck[i].net,
			arcPck[i].comp,
			arcPck[i].loc,
			arcPck[i].Plabel,
			arcPck[i].Slabel,
			arcPck[i].Ponset,
			arcPck[i].Sonset,
			arcPck[i].Pat,
			arcPck[i].Sat,
			arcPck[i].Pres,
			arcPck[i].Sres,
			arcPck[i].Pqual,
			arcPck[i].Squal,
			arcPck[i].codalen,
			arcPck[i].codawt,
			arcPck[i].Pfm,
			arcPck[i].Sfm,
			arcPck[i].cdate,
			arcPck[i].datasrc,
			arcPck[i].Md,
			arcPck[i].azm,
			arcPck[i].takeoff,
			arcPck[i].dist,
			arcPck[i].Pwt,
			arcPck[i].Swt,
			arcPck[i].pamp,
			arcPck[i].codalenObs,
			arcPck[i].ccntr[0],
			arcPck[i].ccntr[1],
			arcPck[i].ccntr[2],
			arcPck[i].ccntr[3],
			arcPck[i].ccntr[4],
			arcPck[i].ccntr[5],
			arcPck[i].caav[0],
			arcPck[i].caav[1],
			arcPck[i].caav[2],
			arcPck[i].caav[3],
			arcPck[i].caav[4],
			arcPck[i].caav[5]
			    );

			if (strlen(mail_msg_line) + 1 > MAIL_MSG_BODY_SIZE - strlen(mail_msg_body)) {
			    logit("et", "mail_msg_body for arc messge would be truncated.\n");
			} else {
			    strncat(mail_msg_body, mail_msg_line, MAIL_MSG_BODY_SIZE - strlen(mail_msg_body) - 1);
			}

	    }

	    // logit("t", "%s\n", mail_msg_body);

	    ret = ew2moledb_SendMail(subject, mail_msg_body);

	    return ret;
}

#define MAX_SQLSTR_ARC 65536
char *get_sqlstr_from_arc_ew_struct(struct Hsum *arcSum, struct Hpck *arcPck, int n_arcPck, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_ARC];
    char sqlstr_tmp[MAX_SQLSTR_ARC];
    char mysqlstr_datetime_Pat[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_Sat[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char mysqlstr_datetime_cdate[EW2MOLEDB_DATETIME_MAXLEN_STR];
    char ingv_quality1, ingv_quality2;

    int i;

    sqlstr[MAX_SQLSTR_ARC - 1] = 0;
    sqlstr_tmp[MAX_SQLSTR_ARC - 1] = 0;

    ingv_quality_arc_ew_struct(arcSum, arcPck, n_arcPck, &ingv_quality1, &ingv_quality2);

    timestr_to_mysql_datetime_str(mysqlstr_datetime_cdate, arcSum->cdate);

    snprintf(sqlstr, MAX_SQLSTR_ARC - 1, "CALL sp_ins_ew_arc_summary('%s', '%s', %ld, '%s', %f, %f, %f, %d, %d, %d, %d, %d, %d, %f, %d, %d, %f, %d, %d, %f, %f, %f, %f, %f, '%s',  '%c', %f, %f, '%c', %f, %f, %ld, '%c%c');",
	ewinstancename,
	modname,
        arcSum->qid,
	/*
        arcSum->ot,
	*/
	mysqlstr_datetime_cdate,
        arcSum->lat,
        arcSum->lon,
        arcSum->z,
        arcSum->nph,
        arcSum->nphS,
        arcSum->nphtot,
        arcSum->nPfm,
        arcSum->gap,
        arcSum->dmin,
        arcSum->rms,
        arcSum->e0az,
        arcSum->e0dp,
        arcSum->e0,
        arcSum->e1az,
        arcSum->e1dp,
        arcSum->e1,
        arcSum->e2,
        arcSum->erh,
        arcSum->erz,
        arcSum->Md,
        arcSum->reg,
	/*
        arcSum->cdate,
	*/
        arcSum->labelpref,
        arcSum->Mpref,
        arcSum->wtpref,
        arcSum->mdtype,
        arcSum->mdmad,
        arcSum->mdwt,
        arcSum->version,
	ingv_quality1,
	ingv_quality2
	    );

    for(i=0; i < n_arcPck; i++) {

	epoch_to_mysql_datetime_str(mysqlstr_datetime_Pat, EW2MOLEDB_DATETIME_MAXLEN_STR, arcPck[i].Pat - GSEC1970);
	epoch_to_mysql_datetime_str(mysqlstr_datetime_Sat, EW2MOLEDB_DATETIME_MAXLEN_STR, arcPck[i].Sat - GSEC1970);
	timestr_to_mysql_datetime_str(mysqlstr_datetime_cdate, arcPck[i].cdate);

	snprintf(sqlstr_tmp, MAX_SQLSTR_ARC - 1, "CALL sp_ins_ew_arc_phase('%s', '%s', %ld, '%s', '%s', '%s', '%s', '%c', '%c', '%c', '%c', '%s', '%s', %f, %f, %d, %d, %d, %d, '%c', '%c', '%c', %f, %d, %d, %f, %f, %f, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %ld);",
	ewinstancename,
	modname,
        arcSum->qid,
        arcPck[i].site,
        arcPck[i].net,
        arcPck[i].comp,
        arcPck[i].loc,
        arcPck[i].Plabel,
        arcPck[i].Slabel,
        arcPck[i].Ponset,
        arcPck[i].Sonset,
	/*
        arcPck[i].Pat,
        arcPck[i].Sat,
	*/
	mysqlstr_datetime_Pat,
	mysqlstr_datetime_Sat,
        arcPck[i].Pres,
        arcPck[i].Sres,
        arcPck[i].Pqual,
        arcPck[i].Squal,
        arcPck[i].codalen,
        arcPck[i].codawt,
        arcPck[i].Pfm,
        arcPck[i].Sfm,
	/*
        arcPck[i].cdate,
	mysqlstr_datetime_cdate,
	*/
        arcPck[i].datasrc,
        arcPck[i].Md,
        arcPck[i].azm,
        arcPck[i].takeoff,
        arcPck[i].dist,
        arcPck[i].Pwt,
        arcPck[i].Swt,
        arcPck[i].pamp,
        arcPck[i].codalenObs,
        arcPck[i].ccntr[0],
        arcPck[i].ccntr[1],
        arcPck[i].ccntr[2],
        arcPck[i].ccntr[3],
        arcPck[i].ccntr[4],
        arcPck[i].ccntr[5],
        arcPck[i].caav[0],
        arcPck[i].caav[1],
        arcPck[i].caav[2],
        arcPck[i].caav[3],
        arcPck[i].caav[4],
        arcPck[i].caav[5],
        arcSum->version
	);

	if (strlen(sqlstr_tmp) + 1 > MAX_SQLSTR_ARC - strlen(sqlstr)) {
	    logit("et", "sqlstr for arc messge would be truncated.\n");
	} else {
	    strncat(sqlstr, sqlstr_tmp, MAX_SQLSTR_ARC - strlen(sqlstr) - 1);
	}
    }

    return sqlstr;
}

#define MAX_LEN_TEXT 2048

char *get_sqlstr_from_arc_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    int error = 0;
    char *cur_msg = msg;
    char *cur_sdw = parse_arc_next_shadow(cur_msg);
    char Text[MAX_LEN_TEXT];
    struct Hsum arcSum;
    struct Hpck *arcPck = NULL;
    int cur_arcPck = -1;
    int n_arcPck = -1;
    int nline = 0;

    Text[MAX_LEN_TEXT - 1] = 0;

    while( error != -1 && cur_msg != NULL && cur_sdw != NULL ) {

	// debugging
	// logit("t", "%d cur_msg: %s\ncur_sdw: %s\n", nline, cur_msg, cur_sdw);

	if(cur_msg[0] != '$') {
	    cur_sdw = parse_arc_next_shadow(cur_msg);
	    nline++;

	    if(cur_sdw[0] != '$') {
		snprintf( Text, MAX_LEN_TEXT - 1, "Error reading arc shadow line\n");
		logit("et", "%s", Text);
		error = -1;
	    } else {

		if (nline == 1) {
		    /* Summary ARC line */
		    if (read_hyp (cur_msg, cur_sdw, &arcSum) < 0) {
			snprintf( Text, MAX_LEN_TEXT - 1, "Error reading arc summary: %s\n", msg );
			logit("et", "%s", Text);
			error = -1;
		    } else {
			cur_arcPck = 0;
			n_arcPck = arcSum.nphtot;
			arcPck = (struct Hpck *) malloc( n_arcPck * sizeof(struct Hpck));
			if(arcPck == NULL) {
			    snprintf( Text, MAX_LEN_TEXT - 1, "Error allocating memory for arc station\n");
			    logit("et", "%s", Text);
			    error = -1;
			} else {
			    snprintf( Text, MAX_LEN_TEXT - 1, "Allocated %d arc picks\n", n_arcPck);
			    logit("t", "%s", Text);
			}
		    }
		} else {
		    /* Station ARC line */
		    if(cur_arcPck < n_arcPck) {
			if (read_phs (cur_msg, cur_sdw, &(arcPck[cur_arcPck])) < 0) {
			    snprintf( Text, MAX_LEN_TEXT - 1, "Error reading arc station info: %s\n", msg );
			    logit("et", "%s", Text);
			    error = -1;
			} else {
			    cur_arcPck++;
			}
		    } else {
			/* At the end of the ARC file there are two more lines containing the event id */
			/*
			snprintf( Text, MAX_LEN_TEXT - 1, "Warning number of arc phases execeds number of allocated %d\n", n_arcPck );
			logit("et", "%s", Text);
			*/
			cur_msg = NULL;
			cur_sdw = NULL;
		    }
		}

	    }
	} else {
	    snprintf( Text, MAX_LEN_TEXT - 1, "Error reading arc no shadow line\n");
	    logit("et", "%s", Text);
	    error = -1;
	}

	if(error != -1) {
	    cur_msg = parse_arc_next_shadow(cur_sdw);
	    nline++;
	}
    }

    if(error != -1) {
	if(cur_arcPck != n_arcPck) {
	    snprintf( Text, MAX_LEN_TEXT - 1, "Warning cur_arcPck != n_arcPck %d != %d\n", cur_arcPck, n_arcPck );
	    logit("et", "%s", Text);
	    n_arcPck = cur_arcPck;
	}
	ret = get_sqlstr_from_arc_ew_struct(&arcSum, arcPck, n_arcPck, ewinstancename, modname);
	if(ew2moledb_nmailRecipients > 0) {
	    sendmail_arc_ew_struct(&arcSum, arcPck, n_arcPck, ewinstancename, modname);
	}
    }

    if(arcPck) {
	free(arcPck);
    }

    return ret;
}

