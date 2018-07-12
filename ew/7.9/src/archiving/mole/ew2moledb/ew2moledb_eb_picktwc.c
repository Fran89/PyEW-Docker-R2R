#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <earlybirdlib.h>
#include <time_ew.h>
#include <chron3.h>

#include "ew2moledb_eb_picktwc.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"


/*
int sendmail_eb_picktwc_struct(struct Hsum *arcSum, struct Hpck *arcPck, int n_arcPck, char *ewinstancename, char *modname) {
  int ret = 0;

  ret = ew2moledb_SendMail(subject, mail_msg_body);

  return ret;
}
*/

#define MAX_SQLSTR_PICKTWC 65536
char *get_sqlstr_from_eb_picktwc_struct(STATION *Station, char *ewinstancename, char *modname) {
  static char sqlstr[MAX_SQLSTR_PICKTWC];
  char mysqlstr_datetime_dPTime[EW2MOLEDB_DATETIME_MAXLEN_STR];
  char mysqlstr_datetime_dMbTime[EW2MOLEDB_DATETIME_MAXLEN_STR];
  char mysqlstr_datetime_dMlTime[EW2MOLEDB_DATETIME_MAXLEN_STR];
  char mysqlstr_datetime_dMSTime[EW2MOLEDB_DATETIME_MAXLEN_STR];

  sqlstr[MAX_SQLSTR_PICKTWC - 1] = 0;

  epoch_to_mysql_datetime_str(mysqlstr_datetime_dPTime, EW2MOLEDB_DATETIME_MAXLEN_STR, Station->dPTime);
  epoch_to_mysql_datetime_str(mysqlstr_datetime_dMbTime, EW2MOLEDB_DATETIME_MAXLEN_STR, Station->dMbTime);
  epoch_to_mysql_datetime_str(mysqlstr_datetime_dMlTime, EW2MOLEDB_DATETIME_MAXLEN_STR, Station->dMlTime);
  epoch_to_mysql_datetime_str(mysqlstr_datetime_dMSTime, EW2MOLEDB_DATETIME_MAXLEN_STR, Station->dMSTime);

  /**************************************************
   * similar sprintf code in function ReportPick()  *
   * in earthworm/src/libsrc/earlybird/report.c     *
   **************************************************/
  snprintf(sqlstr, MAX_SQLSTR_PICKTWC - 1,
      "CALL sp_ins_eb_picktwc('%s', '%s', "
      "'%s', '%s', '%s', '%s', "
      "%ld, %d, '%s', '%c', "
      "'%s', %lf, %lf, '%s', %lf, "
      "%lf, '%s', %lf, %lf, '%s', "
      "%lE, %lf, '%s', %lf, %lf);",

      ewinstancename,
      modname,

      Station->szStation, Station->szChannel, Station->szNetID, Station->szLocation,
      Station->lPickIndex, Station->iUseMe, mysqlstr_datetime_dPTime, Station->cFirstMotion,
      Station->szPhase, Station->dMbAmpGM, Station->dMbPer, mysqlstr_datetime_dMbTime, Station->dMlAmpGM,
      Station->dMlPer, mysqlstr_datetime_dMlTime, Station->dMSAmpGM, Station->dMSPer, mysqlstr_datetime_dMSTime,
      Station->dMwpIntDisp, Station->dMwpTime, Station->szHypoID, Station->dPStrength, Station->dFreq );

  return sqlstr;
}

#define MAX_LEN_TEXT 2048

char *get_sqlstr_from_eb_picktwc_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    int error = 0;
    int nfields = 0;
    int  iMessageType, iModId, iInst;  /* Incoming logo */
    STATION Station;

    /* Initialize the temp buffer 
     ************************** */
    Station.dLat = 0.;
    Station.dLon = 0.;
    InitP( &Station );

    /**************************************************
    * Fill in PPICK structure from PickTWC message.   *
    * Parsing code copied from function PPickStruct() *
    * in earthworm/src/libsrc/earlybird/get_pick.c    *
    **************************************************/
    nfields = sscanf( msg, "%d %d %d %s %s %s %s %ld %d %lf %c %s %lf %lf %lf %lf "
	"%lf %lf %lf %lf %lf %lE %lf %s %lf %lf",
	&iMessageType, &iModId, &iInst, Station.szStation, Station.szChannel,
	Station.szNetID, Station.szLocation, &Station.lPickIndex,
	&Station.iUseMe, &Station.dPTime, &Station.cFirstMotion, Station.szPhase,
	&Station.dMbAmpGM, &Station.dMbPer, &Station.dMbTime,
	&Station.dMlAmpGM, &Station.dMlPer, &Station.dMlTime,
	&Station.dMSAmpGM, &Station.dMSPer, &Station.dMSTime,
	&Station.dMwpIntDisp, &Station.dMwpTime, Station.szHypoID,
	&Station.dPStrength, &Station.dFreq );

    /* Fill structure HypoTWC from HYPOTWC message */
    if ( nfields != 26 )
    {                  
      logit( "et", "Problem reading PickTWC message (nfields = %d)\n", nfields );
      error = -1;
    }

    if(error != -1) {

	ret = get_sqlstr_from_eb_picktwc_struct(&Station, ewinstancename, modname);

	if(ew2moledb_nmailRecipients > 0) {
	  /* sendmail_eb_picktwc_struct(&arcSum, arcPck, n_arcPck, ewinstancename, modname); */
	}
    }

    return ret;
}

