#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#include <earlybirdlib.h>
#include <time_ew.h>
#include <chron3.h>

#include "ew2moledb_eb_hypotwc.h"
#include "ew2moledb_sendmail.h"
#include "ew2moledb_mysql.h"


/*
int sendmail_eb_hypotwc_struct(struct Hsum *arcSum, struct Hpck *arcPck, int n_arcPck, char *ewinstancename, char *modname) {
  int ret = 0;

  ret = ew2moledb_SendMail(subject, mail_msg_body);

  return ret;
}
*/

#define MAX_SQLSTR_HYPOTWC 65536
char *get_sqlstr_from_eb_hypotwc_struct(HYPO *HypoTWC, char *ewinstancename, char *modname) {
  static char sqlstr[MAX_SQLSTR_HYPOTWC];
  char mysqlstr_datetime_dOriginTime[EW2MOLEDB_DATETIME_MAXLEN_STR];

  sqlstr[MAX_SQLSTR_HYPOTWC - 1] = 0;

  epoch_to_mysql_datetime_str(mysqlstr_datetime_dOriginTime, EW2MOLEDB_DATETIME_MAXLEN_STR, HypoTWC->dOriginTime);

  snprintf(sqlstr, MAX_SQLSTR_HYPOTWC - 1,
      "CALL sp_ins_eb_hypotwc('%s', '%s', "
      "%lf, %lf, '%s', "
      "%lf, %lf, '%s', "
      "%d, '%s', "
      "%d, %d, %lf, %d, %d, "
      "%lf, %d,"
      "%lf, %d,"
      "%lf, %d,"
      "%lf, %d,"
      "%lf, %d,"
      "%lf, %d, %d"
      ");",

      ewinstancename,
      modname,

      HypoTWC->dLat,
      HypoTWC->dLon,

      mysqlstr_datetime_dOriginTime,

      HypoTWC->dDepth,
      HypoTWC->dPreferredMag,
      HypoTWC->szPMagType,

      HypoTWC->iNumPMags,
      HypoTWC->szQuakeID,

      HypoTWC->iVersion,
      HypoTWC->iNumPs,

      HypoTWC->dAvgRes,
      HypoTWC->iAzm,
      HypoTWC->iGoodSoln,
      HypoTWC->dMbAvg,
      HypoTWC->iNumMb,
      HypoTWC->dMlAvg,
      HypoTWC->iNumMl,
      HypoTWC->dMSAvg,
      HypoTWC->iNumMS,
      HypoTWC->dMwpAvg,
      HypoTWC->iNumMwp,
      HypoTWC->dMwAvg,
      HypoTWC->iNumMw,
      HypoTWC->dTheta,
      HypoTWC->iNumTheta,
      HypoTWC->iMagOnly
	);

  return sqlstr;
}

#define MAX_LEN_TEXT 2048

char *get_sqlstr_from_eb_hypotwc_msg(char *msg, int msg_size, char *ewinstancename, char *modname) {
    char *ret = NULL;
    int error = 0;
    HYPO HypoTWC;
    LATLON llIn, llOut;

    /* Initialize structure HypoTWC */
    InitHypo(&HypoTWC);

    /* Fill structure HypoTWC from HYPOTWC message */
    if ( HypoStruct( msg, &HypoTWC ) < 0 )
    {                  
      logit( "et", "Problem in HypoStruct function - 1\n" );
      error = -1;
    }

    if(error != -1) {

	/*  Converts geocentric lat/lon to geographic lat/lon. */
	llIn.dLat = HypoTWC.dLat;
	llIn.dLon = HypoTWC.dLon;
	GeoGraphic(&llOut, &llIn);

	/* Change longitude range between -180 and 180 */
	if ( llOut.dLon > 180. ) llOut.dLon -= 360.;

	/* Overwrite dLat/dLon with geographic coordinates. */
	HypoTWC.dLat = llOut.dLat;
	HypoTWC.dLon = llOut.dLon;


	ret = get_sqlstr_from_eb_hypotwc_struct(&HypoTWC, ewinstancename, modname);

	if(ew2moledb_nmailRecipients > 0) {
	  /* sendmail_eb_hypotwc_struct(&arcSum, arcPck, n_arcPck, ewinstancename, modname); */
	}
    }

    return ret;
}

