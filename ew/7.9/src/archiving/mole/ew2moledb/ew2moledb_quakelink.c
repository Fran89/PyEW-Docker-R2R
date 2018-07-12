#include <stdio.h>

#include <earthworm.h>
#include <chron3.h>

#include "ew2moledb_quakelink.h"
#include "ew2moledb_mysql.h"


#define MAX_SQLSTR_QUAKE2K 4096
char *get_sqlstr_from_quake2k_ew_struct(QUAKE2K *quake, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_QUAKE2K];
    char mysqlstr_datetime[EW2MOLEDB_DATETIME_MAXLEN_STR];

    sqlstr[MAX_SQLSTR_QUAKE2K - 1] = 0;

    timestr_to_mysql_datetime_str(mysqlstr_datetime, quake->timestr);

    snprintf(sqlstr, MAX_SQLSTR_QUAKE2K - 1, "CALL sp_ins_ew_quake2k('%s','%s', %ld, '%s', %lf, %lf, %lf, %f, %f, %f, %f, %d);",
	    ewinstancename,
	    modname,
	    quake->qkseq,
	    /*
	    quake->timestr,
	    */
	    mysqlstr_datetime,
	    quake->lat,
	    quake->lon,
	    quake->z,
	    quake->rms,
	    quake->dmin,
	    quake->ravg,
	    quake->gap,
	    quake->nph
	    );

    /* logit("t", "DEBUG: timestr (%s)   mysqlstr_datetime (%s)\n", quake->timestr, mysqlstr_datetime); */

    return sqlstr;
}

char *get_sqlstr_from_link_ew_struct(LINK *link, char *ewinstancename, char *modname) {
    static char sqlstr[MAX_SQLSTR_QUAKE2K];

    sqlstr[MAX_SQLSTR_QUAKE2K - 1] = 0;

    snprintf(sqlstr, MAX_SQLSTR_QUAKE2K - 1, "CALL sp_ins_ew_link('%s', '%s', %ld, %d, %d);",
	    ewinstancename,
	    modname,
	    link->qkseq,
	    /*
	    link->iinstid,
	    link->isrc,
	    */
	    link->pkseq,
	    link->iphs
	    );

    return sqlstr;
}

/****************************************************************************/
/*  from eqassemple eqas_quake2k() processes a TYPE_QUAKE2K message from binder              */
/****************************************************************************/
int read_quake2k(char *msg, int msg_size, QUAKE2K *quake) {
    double     tOrigin, tNow;
    int        narg;
    char       Text[4096];

    /* Read info from ascii message
     ******************************/
    narg =  sscanf( msg,
	    "%*d %*d %ld %s %lf %lf %lf %f %f %f %f %d",
	    &(quake->qkseq), quake->timestr, &(quake->lat), &(quake->lon), &(quake->z),
	    &(quake->rms), &(quake->dmin), &(quake->ravg), &(quake->gap), &(quake->nph) );

    if ( narg < 10 ) {
	sprintf( Text, "eqas_quake2k: Error reading ascii quake msg: %s", msg );
	logit("e", "%s", Text);
	return EW_FAILURE;
    }

    tNow = tnow();
    tOrigin = julsec17( quake->timestr );
    if ( tOrigin == 0. ) {
	sprintf( Text, "eqas_quake2k: Error decoding quake time: %s", quake->timestr );
	logit("e", "%s", Text);
	return EW_FAILURE;
    }

    return EW_SUCCESS;
}

/****************************************************************************/
/* from eqassemble eqas_link() processes a TYPE_LINK message                                */
/****************************************************************************/
int read_link(char *msg, int msg_size, LINK *link) {
    int           narg;
    char          Text[4096];

    narg  = sscanf( msg, "%ld %d %d %d %d",
	    &(link->qkseq), &(link->iinstid), &(link->isrc), &(link->pkseq), &(link->iphs) );

    if ( narg < 5 ) {
	sprintf( Text, "eqas_link: Error reading ascii link msg: %s", msg );
	logit("e", "%s", Text);
	return EW_FAILURE;
    }

    return EW_SUCCESS;
}

char *get_sqlstr_from_quake2k_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    char Text[1024];
    QUAKE2K quake;

    /* Read the pick into an EWPICK struct
     *  *************************************/
    if( read_quake2k( msg, msg_size, &quake ) != EW_SUCCESS )
    {
	sprintf( Text, "Error reading pick: %s", msg );
	logit("e", "%s", Text);
    } else {
	ret = get_sqlstr_from_quake2k_ew_struct(&quake, ewinstancename, modname);
    }

    return ret;
}

char *get_sqlstr_from_link_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    char Text[1024];
    LINK link;

    /* Read the coda into an EWCODA struct
     *  *************************************/
    if( read_link( msg, msg_size, &link ) != EW_SUCCESS )
    {
	sprintf( Text, "Error reading coda: %s", msg );
	logit("e", "%s", Text);
    } else {
	ret = get_sqlstr_from_link_ew_struct(&link, ewinstancename, modname);
    }

    return ret;
}

