/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_time_proto.h 2530 2006-11-29 21:22:24Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/11/29 21:22:24  stefan
 *     added missing type specifier for timparse and parstart
 *
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:55  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:26  mark
 *     Initial checkin
 *
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

#ifndef DCC_TIME_PROTO_H
#define DCC_TIME_PROTO_H

/* Processing file st_addtodelta.c */

	DELTA_T ST_AddToDelta(DELTA_T intime,DCC_LONG dd,DCC_LONG dh,
		DCC_LONG dm,DCC_LONG ds,DCC_LONG dms)
;

/* Found 1 functions */

/* Processing file st_addtotime.c */

	STDTIME	ST_AddToTime(STDTIME intime,DCC_LONG dd,DCC_LONG dh,
		DCC_LONG dm,DCC_LONG ds,DCC_LONG dms)
;

/* Found 1 functions */

/* Processing file st_cleandate.c */

	STDTIME	ST_CleanDate(STDTIME indate,DCC_WORD epoch,UDCC_LONG timflgs)
;

/* Found 1 functions */

/* Processing file st_deltacomp.c */

	int ST_DeltaComp(DELTA_T intime,DELTA_T insect)
;

/* Found 1 functions */

/* Processing file st_deltaprint.c */

	UDCC_BYTE *ST_DeltaPrint(DELTA_T delta,BOOL printplusflag,BOOL fixms)
;

/* Found 1 functions */

/* Processing file st_deltatoms.c */

	DCC_LONG ST_DeltaToMS(DELTA_T indelta)
;

/* Found 1 functions */

/* Processing file st_difftimes.c */

	DELTA_T	ST_DiffTimes(STDTIME intime,STDTIME insect)
;

/* Found 1 functions */

/* Processing file st_flttotime.c */

	STDTIME	ST_FLTToTime(FLTTIME intime)
;

/* Found 1 functions */

/* Processing file st_formatdate.c */


/* Found 0 functions */

/* Processing file st_formdelta.c */

	DELTA_T ST_FormDelta(DCC_LONG dd,DCC_LONG dh,DCC_LONG dm,DCC_LONG ds,DCC_LONG dms)
;

/* Found 1 functions */

/* Processing file st_getcurrent.c */

	STDTIME ST_GetCurrentTime()
;

/* Found 1 functions */

/* Processing file st_getjulian.c */

	DCC_LONG	ST_GetJulian(STDTIME intime)
;

/* Found 1 functions */

/* Processing file st_getlocal.c */

	STDTIME ST_GetLocalTime()
;

/* Found 1 functions */

/* Processing file st_julian.c */

 DCC_LONG _julday(DCC_LONG year,DCC_LONG mon,DCC_LONG day)
;
 DCC_LONG ST_Julian(DCC_LONG year, DCC_LONG mon, DCC_LONG day)
;
 VOID ST_CnvJulToCal(DCC_LONG injul,DCC_WORD *outyr,DCC_WORD *outmon,
	DCC_WORD *outday,DCC_WORD *outjday)
;
 STDTIME ST_CnvJulToSTD(JULIAN injul)
;

/* Found 4 functions */

/* Processing file st_limits.c */

 STDTIME ST_Zero()
;
 DELTA_T ST_ZeroDelta()
;
 STDTIME ST_End()
;

/* Found 3 functions */

/* Processing file st_minmax.c */

 	STDTIME ST_TimeMin(STDTIME intime,STDTIME insect)
;
 	STDTIME ST_TimeMax(STDTIME intime,STDTIME insect)
;

/* Found 2 functions */

/* Processing file st_minusdelta.c */

	DELTA_T	ST_MinusDelta(DELTA_T indelta)
;

/* Found 1 functions */

/* Processing file st_oracle.c */

 STDTIME ST_GetOracleTime(char *instr)
;
 char *ST_PutOracleTime(STDTIME intime)
;

/* Found 2 functions */

/* Processing file st_parsetime.c */

 	STDTIME	ST_ParseTime(UDCC_BYTE *inbuff)
;

void juldateset(int year,int day);

void nordate(int month,int day,int year);

void timeload(int hour,int minute,int second,int msec);


/* Found 1 functions */

/* Processing file st_printcal.c */

	char *ST_PrintCalDate(STDTIME odate,BOOL longfmt)
;
	char *ST_PrintFullDate(STDTIME odate)
;

/* Found 2 functions */

/* Processing file st_printdate.c */

	UDCC_BYTE *ST_PrintDate(STDTIME odate,BOOL fixfmt)
;

/* Found 1 functions */

/* Processing file st_printdec.c */

	UDCC_BYTE *ST_PrintDECDate(STDTIME odate,BOOL printtime)
;

/* Found 1 functions */

/* Processing file st_setupdate.c */

	BOOL _tleap(DCC_WORD year)		/* Gregorian leap rules */
;
	DCC_LONG _calyear(DCC_LONG dy,DCC_LONG epoch,UDCC_LONG timflgs)
;
	STDTIME	ST_SetupDate(DCC_LONG dy,DCC_LONG dd,DCC_LONG dh,DCC_LONG dm,
		DCC_LONG ds,DCC_LONG dms,DCC_WORD epoch,UDCC_LONG timflgs)
;

/* Found 3 functions */

/* Processing file st_spanprint.c */

	UDCC_BYTE *ST_SpanPrint(STDTIME ftime,STDTIME etime,BOOL fixfmt)
;

/* Found 1 functions */

/* Processing file st_timecomp.c */

	int ST_TimeComp(STDTIME intime,STDTIME insect)
;

/* Found 1 functions */

/* Processing file st_timeminusd.c */

	STDTIME	ST_TimeMinusDelta(STDTIME intime,DELTA_T indelta)
;

/* Found 1 functions */

/* Processing file st_timenorm.c */

	VOID timenorm(DCC_LONG *dy,DCC_LONG *dd,DCC_LONG *dh,DCC_LONG *dm,DCC_LONG *ds,DCC_LONG *dms)
;
	VOID timenormd(DCC_LONG *dd,DCC_LONG *dh,DCC_LONG *dm,DCC_LONG *ds,DCC_LONG *dms)
;

/* Found 2 functions */

/* Processing file st_timepar.c */

int timparse(void);

void parstart(char *string, int stomax);

/* Found 0 functions */

/* Processing file st_timeplusd.c */

	STDTIME	ST_TimePlusDelta(STDTIME intime,DELTA_T indelta)
;

/* Found 1 functions */

/* Processing file st_timespan.c */

 int ST_TimeSpan(STDTIME starta,STDTIME enda,
		     STDTIME startb,STDTIME endb,
		     STDTIME *retstart,STDTIME *retend)
;

/* Found 1 functions */

/* Processing file st_timetoflt.c */

	FLTTIME	ST_TimeToFLT(STDTIME intime)
;

/* Found 1 functions */

/* Processing file st_unixtimes.c */

 long ST_GetUnix(STDTIME instd)
;
 long ST_GetUnixTest(STDTIME instd)
;
 double ST_GetDblUnix(STDTIME instd)
;
 STDTIME ST_CnvUnixtoSTD(long intim)
;
 STDTIME ST_CnvUnixDbltoSTD(double intim)
;

/* Found 5 functions */


#endif /* DCC_TIME_PROTO_H */
