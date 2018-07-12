/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_proto.h 1248 2003-06-16 22:08:11Z patton $
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

/* Processing file diemsg.c */

 BOOL diemsg(char *file, int line, char *date)
;

/* Found 1 functions */

/* Processing file seed_comments.c */

 COMMENT_ENTRY *InitCom()
;
 void LinkInStatCom(STATION_LIST *instat,COMMENT_ENTRY *incom)
;
 void LinkInChanCom(CHANNEL_LIST *inchan,COMMENT_ENTRY *incom)
;
 void AddStatCom(STATION_LIST *inroot,STDTIME startime,STDTIME endtime,
	int comnum,UDCC_LONG level)
;
 void AddChanCom(CHANNEL_LIST *inroot,STDTIME startime,STDTIME endtime,
	int comnum,UDCC_LONG level)
;

/* Found 5 functions */

/* Processing file seed_dicts.c */

 ABBREV *SearchAbbrev(char *inkey)
;
 ABBREV *SearchAbbrevIdx(DCC_LONG inkey)
;
 ABBREV *InitAbbrev()
;
 void LinkInAbbrev(ABBREV *inabbrev)
;
 void AddAbbrev(char *key,char *comment)
;
 VOID CountAbbrev(ABBREV *inabbrev)
;
 UNIT *SearchUnit(char *inkey)
;
 UNIT *SearchUnitIdx(DCC_LONG inkey)
;
 UNIT *InitUnit()
;
 void LinkInUnit(UNIT *inunit)
;
 void AddUnit(char *key,char *comment)
;
 VOID CountUnit(UNIT *inunit)
;
 COMMENT *SearchComment(int inid)
;
 COMMENT *InitComment()
;
 void LinkInComment(COMMENT *incomment)
;
 void AddComment(int comnum,char class,char *text,char *level)
;
 VOID CountComment(COMMENT *incomment)
;
 FORMAT *SearchFormat(char *inkey)
;
 FORMAT *SearchFormatIdx(DCC_LONG inkey)
;
 FORMAT *InitFormat()
;
 void LinkInFormat(FORMAT *informat)
;
 void AddFormat(char *key,char *name,int family,
		int numkeys,char *keylist[MAXKEYS])
;
 VOID CountFormat(FORMAT *informat)
;
 ZEROS_POLES *SearchZerosPoles(char *inkey)
;
 void LinkInZeroPoleDict(ZEROS_POLES *inpz)
;
 COEFFICIENTS *SearchCoefficients(char *inkey)
;
 void LinkInCoefficientsDict(COEFFICIENTS *incoeff)
;
 DECIMATION *SearchDecimation(char *inkey)
;
 void LinkInDecimationDict(DECIMATION *indm)
;

/* Found 29 functions */

/* Processing file seed_membase.c */

 VOID InitializeVolume(STDTIME starttime,STDTIME endtime)
;

/* Found 1 functions */

/* Processing file seed_misc.c */

 char *FixKey(char *inkey)
;
 char *OKNull(char *inc)
;

/* Found 2 functions */

/* Processing file seed_responses.c */

 ZEROS_POLES	*InitZerosPoles(int numzeros,int numpoles)
;
 COEFFICIENTS	*InitCoefficients(int numnum,int numden)
;
 DECIMATION	*InitDecimation()
;
 SENSITIVITY	*InitSensitivity(int numcals)
;
 void DeleteResponses(CHANNEL_TIMES *inchan, RESPONSE *inresp)
;
 void InsertResponse(CHANNEL_TIMES *inchan, RESPONSE *inresp)
;
 RESPONSE *AddResponse(CHANNEL_TIMES *inchannel,char intype,
			   void *inresp, int stage)
;
 void LinkInZerosPoles(CHANNEL_TIMES *inchannel,ZEROS_POLES *inzero, 
			   int stage)
;
 void LinkInCoefficients(CHANNEL_TIMES *inchannel,COEFFICIENTS *incoef,
			     int stage)
;
 void LinkInDecimation(CHANNEL_TIMES *inchannel,DECIMATION *indeci,
			   int stage)
;
 void LinkInSensitivity(CHANNEL_TIMES *inchannel,SENSITIVITY *insens,
			    int stage)
;
 RESPONSE *DupResponse(RESPONSE *inresp)
;
 VOID CopyResponses(RESPONSE *instart,
			RESPONSE **root, RESPONSE **tail)
;
 int ResponseValue(RESPONSE *inresp)
;

/* Found 14 functions */

/* Processing file seed_statchan.c */

 STATION_LIST *SearchStation(char *innet,char *instat)
;
 STATION_TIMES *SearchStationTime(STATION_LIST *instation, 
				      STDTIME findtime)
;
 STATION_ENTRY *InitStationEntry()
;
 STATION_LIST *CreateStation(char *net, char *stat)
;
 VOID BreakStationTimes(STATION_LIST *instation, STDTIME timetobreak)
;
 STATION_TIMES *LinkInStation(STATION_LIST *instation,
				  STDTIME start, STDTIME end)
;
 CHANNEL_LIST *SearchChannel(STATION_LIST *station,char *loc,char *chan)
;
 CHANNEL_TIMES *SearchChannelTime(CHANNEL_LIST *inchannel, 
				      STDTIME findtime)
;
 CHANNEL_LIST *CreateChannel(STATION_LIST *stat, char *loc, char *chan)
;
 CHANNEL_ENTRY *InitChannelEntry()
;
 VOID ChanText(CHANNEL_LIST *chan,char *buf)
;
 BOOL ChanComp(CHANNEL_LIST *chan1,CHANNEL_LIST *chan2)
;
 void BreakChannelTimes(CHANNEL_LIST *inchannel, STDTIME timetobreak)
;
 CHANNEL_TIMES *LinkInChannel(CHANNEL_LIST *inchannel,
				  STDTIME start, STDTIME end)
;
 UDCC_LONG CnvChanFlags(char *inflags)
;
 VOID ChannelModifiedDate(CHANNEL_LIST *chan, STDTIME mod, 
			 STDTIME start, STDTIME end)
;

/* Found 16 functions */

/* Processing file seed_write.c */

 LOGICAL_RECORD *CreateRecordBuff(int creexp,BOOL (*writertn)())
;
 VOID AppendBlockette(BLOCKETTE *InBlock,char *DataBuf,int DataLen)
;
 VOID PackString(BLOCKETTE *InBlock,char *InStr,int MaxStr,BOOL Upcase)
;
 VOID PackChar(BLOCKETTE *InBlock,char InChar,BOOL Upcase)
;
 VOID PackVariable(BLOCKETTE *InBlock,char *InStr,int MaxStr,BOOL Upcase)
;
 BOOL PackInteger(BLOCKETTE *InBlock,int InNum,int FldLen,BOOL Signed)
;
 BOOL PackFixed(BLOCKETTE *InBlock,VOLFLT InFlt,
		    int FldLen,int Decpls,BOOL Signed)
;
 VOID PackExponent(BLOCKETTE *InBlock,VOLFLT InFlt,int FldLen)
;
 VOID PackTime(BLOCKETTE *InBlock,STDTIME InTime)
;
 VOID FlushRecord(LOGICAL_RECORD *inrecord)
;
 VOID NoiseRecord(LOGICAL_RECORD *inrecord)
;
 VOID NewHeader(LOGICAL_RECORD *inrecord,char headtype)
;
 VOID RecordPack(BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
;
 BOOL VolumeBlockette(BLOCKETTE *inblock,LOGICAL_RECORD *inrecord,
			  BOOL station, char *organization, char *label)
;
 BOOL AbbrevBlockette(ABBREV *inabbrev,
			  BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
;
 BOOL DumpAbbrevBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
;
 BOOL PackAbbrev(BLOCKETTE *inblock,ABBREV *inabbrev)
;
 BOOL UnitBlockette(UNIT *inunit,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
;
 BOOL DumpUnitBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
;
 BOOL PackUnit(BLOCKETTE *inblock,UNIT *inunit)
;
 BOOL CommentBlockette(COMMENT *incomment,
			   BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
;
 BOOL DumpCommentBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
;
 BOOL PackComment(BLOCKETTE *inblock,COMMENT *incomment)
;
 BOOL FormatBlockette(FORMAT *informat,
			  BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
;
 BOOL DumpFormatBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
;
 BOOL PackFormat(BLOCKETTE *inblock,FORMAT *informat)
;
 BOOL SetupDictionaryIDs()
;
 BOOL StationBlockette(STATION_LIST *instation,
			   STATION_TIMES *intime,
			   BLOCKETTE *WriteBlock,
			   LOGICAL_RECORD *WriteRec)
;
 BOOL ChannelBlockette(CHANNEL_LIST *inchannel,
			   CHANNEL_TIMES *intime,
			   STATION_TIMES *stattime,
			   BLOCKETTE *WriteBlock,
			   LOGICAL_RECORD *WriteRec)
;
 BOOL SensBlockette(SENSITIVITY *insens,int stage,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
;
 BOOL DeciBlockette(DECIMATION *indeci,int stage,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
;
 BOOL ZerosBlockette(ZEROS_POLES *inzero, int stage,
			 BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
;
 BOOL CoefBlockette(COEFFICIENTS *incoef, int stage,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
;
 BOOL RespBlockette(RESPONSE *inresp,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
;
 BOOL ComBlockette(COMMENT_ENTRY *incom,BLOCKETTE *inblock,
		       LOGICAL_RECORD *inrec,BOOL station)
;

/* Found 35 functions */

/* Processing file seed_blockettes.c */

 BLOCKETTE *CreateBlockette()
;
 VOID SetupBlockette(BLOCKETTE *InBlock,int BlockType,int BlockVer)
;
 VOID ExtendBlockette(BLOCKETTE *InBlock,UDCC_LONG Increments)
;

/* Found 3 functions */

