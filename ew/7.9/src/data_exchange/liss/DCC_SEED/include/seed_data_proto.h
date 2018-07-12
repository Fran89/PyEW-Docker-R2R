/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_data_proto.h 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

/* Processing file seed_data.c */

 void SH_Big_Endian()
;
 void SH_Little_Endian()
;
 DCC_WORD SH_SWAP_WORD(DCC_WORD i)
;
 DCC_LONG SH_SWAP_LONG(DCC_LONG i)
;
 STDFLT SH_Cvt_Float(NETFLT inflt)
;
 STDTIME SH_Cvt_Time(NETTIME intim)
;
 STDTIME SH_Start_Time(SEED_DATA *pblk)
;
 int SH_Number_Samples(SEED_DATA *pblk)
;
 int SH_Start_Data(SEED_DATA *pblk)
;
 STDFLT SH_Record_Rate(SEED_DATA *pblk)
;
 DCC_LONG SH_Record_Time(SEED_DATA *pblk)
;
 DCC_LONG SH_Sample_Time(SEED_DATA *pblk)
;
 STDFLT SH_Sample_FltTime(SEED_DATA *pblk)
;
 STDTIME SH_Cal_Samp_Time(SEED_DATA *inrec, int samn)
;
 STDTIME SH_End_Time(SEED_DATA *pblk)
;
 DCC_LONG SH_Record_Number(SEED_DATA *pblk)
;
 VOID SH_Get_Idents(SEED_DATA *pblk,
			char *net, char *stat, 
			char *loc, char *chan)
;
 VOID SH_Get_Spankey(SEED_DATA *pblk, char *retspan)
;

/* Found 18 functions */

/* Processing file seed_data_dec.c */

 int SH_Data_Decode(char *format, long data[], SEED_DATA *inrec,int swdata)
;
 DCC_WORD SH_DEC_SWAP_WORD(DCC_WORD i,int swdata)
;
 DCC_LONG SH_DEC_SWAP_LONG(DCC_LONG i,int swdata)
;

/* Found 3 functions */

/* Processing file seed_dec_16.c */

 int SH_Decode_16(long data[], SEED_DATA *inrec,int swdata)
;

/* Found 1 functions */

/* Processing file seed_dec_32.c */

 int SH_Decode_32(long data[], SEED_DATA *inrec,int swdata)
;

/* Found 1 functions */

/* Processing file seed_dec_sro.c */

 int SH_Decode_SRO(long data[], SEED_DATA *inrec,int swdata)
;

/* Found 1 functions */

/* Processing file seed_dec_cdsn.c */

 int SH_Decode_CDSN(long data[], SEED_DATA *inrec,int swdata)
;

/* Found 1 functions */

/* Processing file seed_dec_seed.c */

 int SH_Decode_SEED(long data[], SEED_DATA *inrec,int swdata,int level)
;
 int SH_Decode_SEED_S2(long data[], SEED_DATA *inrec,int swdata)
;
 int SH_Decode_SEED_S3(long data[], SEED_DATA *inrec,int swdata)
;
 int SH_Decode_SEED_S1(long data[], SEED_DATA *inrec,int swdata)
;

/* Found 4 functions */

/* Processing file seed_dec_ascii.c */

 int SH_Decode_ASCII(long data[], SEED_DATA *inrec,int swdata)
;

/* Found 1 functions */

