/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_comp_proto.h 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:04:03  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

/* Processing file seed_comp.c */

 UDCC_LONG _SD_SetCtlType(int pos,UDCC_LONG val,UDCC_LONG *outctl)
;
 VOID _SD_ClearContext(SD_Context *context)
;
 SD_Context *SD_CreateChannel(UDCC_BYTE *network,
			     UDCC_BYTE *station,
			     UDCC_BYTE *location,
			     UDCC_BYTE *channel,
			     int subchan,
			     VOID (*flushroutine)(VOID *),
			     int format,
			     int len,
			     DCC_LONG ms_sam)
;
 VOID SD_DestroyChannel(SD_Context *context)
;
 VOID SD_PutSample(SD_Context *context, DCC_LONG delta, DCC_LONG sample, BOOL flush)
;
 VOID SD_FlushRecord(SD_Context *context)
;
 VOID SD_PutData(SD_Context *context, STDTIME datetime, int numsamps,
	DCC_LONG *dataarray)
;

/* Found 7 functions */

