/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_write.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_seed.h>
#include <dcc_misc.h>

extern int	DefBlkLen;		/* Default blockette buffer len */
extern int	DefBlkExt;		/* Default blockette extension */

/*
 *	
 *	Initialize a structure for a logical record (Safe)
 *	
 */

_SUB LOGICAL_RECORD *CreateRecordBuff(int creexp,BOOL (*writertn)())
{

  LOGICAL_RECORD *New_Record;

  New_Record = (LOGICAL_RECORD *) SafeAlloc(sizeof(LOGICAL_RECORD));

  if (creexp>15) creexp = 15;	/* Maximum logical record size */
  if (creexp<8) creexp = 8;	/* Minimum logical record size */

  New_Record->RecBuff = (char *) SafeAlloc(1<<creexp);

  New_Record->RecNum = 0;
  New_Record->NextPos = 0;
  New_Record->RecLen = creexp;
  New_Record->WriteService = writertn;
  New_Record->Continue = FALSE;
  New_Record->RecType = ' ';
	
  FlushRecord(New_Record);

  return(New_Record);
}

/*
 *	
 * Append data to a blockette
 * */

_SUB VOID AppendBlockette(BLOCKETTE *InBlock,char *DataBuf,int DataLen)
{

  int	counter;

  /* See if the data will fit */

  if (InBlock->PackPos + DataLen > InBlock->BufLen) 
    ExtendBlockette(InBlock,
		    ((InBlock->PackPos + DataLen - InBlock->BufLen) /
		     DefBlkExt) + 1);

  for (counter = 0; counter < DataLen; counter++)
    InBlock->BlkBuff[InBlock->PackPos + counter] = 
      DataBuf[counter];

  InBlock->PackPos += DataLen;

}

/*
 *	
 *	Pack a fixed length string onto record
 *	
 */

_SUB VOID PackString(BLOCKETTE *InBlock,char *InStr,int MaxStr,BOOL Upcase)
{

  char	*stobuff,txchar,dummy;
  int	stoidx;

  if (InStr==NULL) {
    dummy = '\0';
    InStr = &dummy;
  }

  while (*InStr==' ') InStr++;	/* Skip leading spaces */

  stobuff = (char *) SafeAlloc(MaxStr);

  for (stoidx=0; stoidx<MaxStr; stoidx++) {
    if (*InStr!='\0') {
      txchar = *InStr;
      if (Upcase&&islower(txchar)) txchar = toupper(txchar);
      stobuff[stoidx] = txchar;
      InStr++;
    } else	
      stobuff[stoidx] = ' '; /* Space pad */
  }

  AppendBlockette(InBlock,stobuff,MaxStr);

  free(stobuff);

}

_SUB VOID PackChar(BLOCKETTE *InBlock,char InChar,BOOL Upcase)
{

  char NewStr[2];

  NewStr[0] = InChar;
  NewStr[1] = '\0';

  PackString(InBlock,NewStr,1,Upcase);
}

/*
 *	
 *	Pack away a variable length string
 *	MaxStr = 0, no maximum length
 *	
 */

_SUB VOID PackVariable(BLOCKETTE *InBlock,char *InStr,int MaxStr,BOOL Upcase)
{

  char	*stobuff,txchar,dummy;
  int	stoidx;

  if (InStr==NULL) {
    dummy = '\0';
    InStr = &dummy;
  }

  while (*InStr==' ') InStr++;	/* Skip leading spaces */

  stobuff = (char *) SafeAlloc((MaxStr!=0)?MaxStr+1:strlen(InStr+1));

  stoidx = 0;
  while (*InStr!='\0') {
    if (MaxStr>0&&stoidx>=MaxStr) break;
    txchar = *InStr;
    if (Upcase&&islower(txchar)) txchar = toupper(txchar);

    stobuff[stoidx++] = txchar;
    InStr++;
  }

  while (stoidx>0&&stobuff[stoidx-1]==' ') stoidx--; /* Trail space */

  stobuff[stoidx++] = '~';	/* Tilde terminator */

  AppendBlockette(InBlock,stobuff,stoidx);

  free(stobuff);

}

/*
 *	
 *	Pack signed or unsigned integer
 *	
 */

_SUB BOOL PackInteger(BLOCKETTE *InBlock,int InNum,int FldLen,BOOL Signed)
{

  char	maskbuff[25];
  char	intbuff[25];
  BOOL	minus=FALSE;		/* Signed Minus? */
  int	start,totlen;

  totlen = Signed?(FldLen-1):FldLen; /* Field without sign */
  start = Signed?1:0;		/* Where does field start */
  if (Signed&&InNum<0) minus = TRUE; /* Number minus */
  if (InNum<0) InNum = -InNum;	

  sprintf(maskbuff,"%%0%dd",totlen); /* Make final mask */
  sprintf(&intbuff[start],maskbuff,InNum);

  if (Signed) intbuff[0] = minus?'-':'+'; /* Place sign */

  if (strlen(intbuff)!=FldLen) return(FALSE); /* Number overflow */
	
  AppendBlockette(InBlock,intbuff,FldLen);
  
  return(TRUE);

}

/*
 *	
 *	Pack signed or unsigned fixed floating point
 *	
 */

static VOLFLT _shfttab[] = { 1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E8, 1E8 };

_SUB BOOL PackFixed(BLOCKETTE *InBlock,VOLFLT InFlt,
		    int FldLen,int Decpls,BOOL Signed)
{

  char	maskbuff[25],finalbuff[30];
  char	intbuff[25];
  BOOL	minus=FALSE;		/* Signed Minus? */
  int	start,totlen,stoidx;
  int	InNum;

  InFlt *= _shfttab[Decpls];
  InNum = InFlt;		/* Convert and round */

  totlen = Signed?(FldLen-1):FldLen; /* Field without sign */
  totlen --;			/* Leave room for decimal */
  start = Signed?1:0;		/* Where does field start */
  if (Signed&&InNum<0) minus = TRUE; /* Number minus */
  if (InNum<0) InNum = -InNum;	

  sprintf(maskbuff,"%%0%dd",totlen); /* Make final mask */
  sprintf(&intbuff[start],maskbuff,InNum);

  if (Signed) intbuff[0] = minus?'-':'+'; /* Place sign */

  if (strlen(intbuff)!=FldLen-1) return(FALSE);	/* Number overflow */
	
  for (stoidx = 0; stoidx < FldLen-Decpls-1; stoidx++) 
    finalbuff[stoidx] = intbuff[stoidx];

  finalbuff[FldLen-Decpls-1] = '.'; /* Decimal point */
  for (stoidx = 0; stoidx<Decpls; stoidx++)
    finalbuff[FldLen-Decpls+stoidx] = 
      intbuff[FldLen-Decpls+stoidx-1];
	
  AppendBlockette(InBlock,finalbuff,FldLen);

  return(TRUE);
}

/*
 *	
 *	Pack an exponential form
 *	
 */

_SUB VOID PackExponent(BLOCKETTE *InBlock,VOLFLT InFlt,int FldLen)
{

  char	fltbuff[30];
  char	fltmask[30];

  sprintf(fltmask,"%%%d.%dE",FldLen,FldLen-7);
  sprintf(fltbuff,fltmask,InFlt);

  AppendBlockette(InBlock,fltbuff,FldLen);

}

/*
 *	
 *	Pack the standard time
 *	
 */

_SUB VOID PackTime(BLOCKETTE *InBlock,STDTIME InTime)
{

  char	timestr[30],msbuf[6];
  int	flgsec;

  flgsec = 0;
  if      (InTime.msec  !=0) flgsec = 6;
  else if (InTime.second!=0) flgsec = 5;
  else if (InTime.minute!=0) flgsec = 4;
  else if (InTime.hour  !=0) flgsec = 3;
  else if (InTime.day   !=0) flgsec = 2;
  else if (InTime.year  !=0) flgsec = 1;
	
  switch(flgsec) {
  case 0:
    sprintf(timestr,"~");
    break;
  case 1:
  case 2:
    sprintf(timestr,"%04d,%03d~",InTime.year,InTime.day);
    break;
  case 3:
    sprintf(timestr,"%04d,%03d,%02d~",InTime.year,
	    InTime.day,InTime.hour);
    break;
  case 4:
    sprintf(timestr,"%04d,%03d,%02d:%02d~",InTime.year,
	    InTime.day,InTime.hour,InTime.minute);
    break;
  case 5:
    sprintf(timestr,"%04d,%03d,%02d:%02d:%02d~",InTime.year,
	    InTime.day,InTime.hour,InTime.minute,InTime.second);
    break;
  case 6:
    sprintf(msbuf,"%03d",InTime.msec);
    if (msbuf[2]=='0') {
      msbuf[2]='\0';
      if (msbuf[1]=='\0')
	msbuf[1]='\0';
    }
    sprintf(timestr,"%04d,%03d,%02d:%02d:%02d.%s~",InTime.year,
	    InTime.day,InTime.hour,InTime.minute,
	    InTime.second,msbuf);
    break;
  }

  AppendBlockette(InBlock,timestr,strlen(timestr));

}

/*
 *	
 *	Empty out record to the write service routine
 *	
 */

_SUB VOID FlushRecord(LOGICAL_RECORD *inrecord)
{	

  int rlen,i;

  rlen = 1<<inrecord->RecLen;

  if (inrecord->NextPos==8) {	/* Don't dump out empty records */
    /* scratch off serial numbers and recycle it */
    inrecord->RecBuff[6] = inrecord->RecType;
    inrecord->RecBuff[7] = inrecord->Continue?'*':' ';
    return;
  }

  if (inrecord->NextPos>0) {
    for (i=inrecord->NextPos; i<rlen; i++)
      inrecord->RecBuff[i] = ' '; /* Flush out */
    (*inrecord->WriteService)(inrecord->RecBuff,
			      rlen,inrecord->RecNum);
  }

  inrecord->RecNum++;		/* Increment ID */

  if (inrecord->RecNum>999999) inrecord->RecNum = 0;
  sprintf(inrecord->RecBuff,"%06d%c%c",
	  inrecord->RecNum,
	  inrecord->RecType,
	  inrecord->Continue?'*':' ');

  inrecord->NextPos = 8;
	
}

/*
 *	
 *	Write out a "noise" record
 *	
 */

_SUB VOID NoiseRecord(LOGICAL_RECORD *inrecord)
{

  if (inrecord->NextPos==8) {
    inrecord->RecBuff[8] = ' ';
    inrecord->NextPos = 9;
    FlushRecord(inrecord);	/* Write it out blanks */
  }

  FlushRecord(inrecord);	/* It had something in it */

}

/*
 *	
 *	Flush headers and setup as new header type
 *	
 */

_SUB VOID NewHeader(LOGICAL_RECORD *inrecord,char headtype)
{

  inrecord->RecType = headtype;
  inrecord->Continue = FALSE;
  FlushRecord(inrecord);
}

/*
 *	
 *	Jam a blockette into a record...  Keep writing records until it fits
 *	
 */

_SUB VOID RecordPack(BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
{

  int rlen,blockavail,startover;

  rlen = 1<<inrecord->RecLen;

  if ((rlen-inrecord->NextPos)<20) {
    inrecord->Continue=TRUE;
    FlushRecord(inrecord);
  }		
	
  sprintf(&inrecord->RecBuff[inrecord->NextPos],
	  "%03d%04d",
	  inblock->BlkType,
	  inblock->PackPos+7);	/* Inclusive */

  inrecord->NextPos += 7;

  startover = 0;

  while (startover<inblock->PackPos) {

    blockavail = rlen-inrecord->NextPos;
    if (blockavail>=(inblock->PackPos-startover)) {
      (VOID) memcpy(&inrecord->RecBuff[inrecord->NextPos],
		    &inblock->BlkBuff[startover],
		    inblock->PackPos-startover);

      inrecord->NextPos += inblock->PackPos-startover;
      return;
    }

    (VOID) memcpy(&inrecord->RecBuff[inrecord->NextPos],
		  &inblock->BlkBuff[startover],
		  blockavail);

    inrecord->NextPos += blockavail; /* Should fill up */
    inrecord->Continue=TRUE;

    FlushRecord(inrecord);	/* Write that out */

    startover += blockavail;
		
  }

  fprintf(stderr,"RecordPack should never exit here\n");
  fprintf(stderr,"Startover is %d, packpos is %d\n",
	  startover,inblock->PackPos);

}

/*
 *	
 *	Write out the proper volume header blockette
 *	
 */

_SUB BOOL VolumeBlockette(BLOCKETTE *inblock,LOGICAL_RECORD *inrecord,
			  BOOL station, char *organization, char *label)
{

  STDTIME ttim;

  if (VI->SeedVersion<10)
    bombout(EXIT_ABORT,"VI->SeedVersion is set to %d",
	    VI->SeedVersion);

  SetupBlockette(inblock,
		 station?SEED_STATION_HEADER:SEED_NETWORK_HEADER,
		 VI->SeedVersion);

  CKP(PackFixed(inblock,2.3,4,1,FALSE));
  CKP(PackInteger(inblock,inrecord->RecLen,2,FALSE));

  ttim = VI->Vol_Begin;
  if (ttim.year<1500) {  /* We don't think that there's any recorded
			    seismology before this date */
    ttim = ST_Zero();
    ttim.year = 1500;
    ttim.day = 1;
  }
  PackTime(inblock,ttim);
  if (!station) {
    ttim = VI->End_Volume;
    if (ttim.year>=2500) {  /* Sufficiently far in the future - for now */
      ttim = ST_Zero();
      ttim.year = 2500;
      ttim.day = 1;
    }
    PackTime(inblock,ttim);
    PackTime(inblock,ST_GetCurrentTime());
    PackVariable(inblock,organization,80,FALSE);
    PackVariable(inblock,label,80,FALSE);
  }

  RecordPack(inblock,inrecord);

  return(TRUE);
}

/*
 *	
 *	Write out a abbreviation dictionary entry blockette
 *	
 */

_SUB BOOL AbbrevBlockette(ABBREV *inabbrev,
			  BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
{

  SetupBlockette(inblock,SEED_ABBREV_DICT,SEEDVER);

  CKP(PackInteger(inblock,inabbrev->Abbrev_ID,3,FALSE));
  PackVariable(inblock,inabbrev->Comment,50,FALSE);

  RecordPack(inblock,inrecord);

  return(TRUE);
}

_SUB BOOL DumpAbbrevBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
{

  ABBREV *looper;

  for (looper=VI->Root_Abbrev; looper!=NULL; looper=looper->Next) {
    if (looper->Abbrev_ID==0) continue;

    CKP(AbbrevBlockette(looper,blockbuf,inrecord));
  }

  return(TRUE);
}

/*
 *	
 *	Put out the abbreviation pointer part of a blockette
 *	
 */

_SUB BOOL PackAbbrev(BLOCKETTE *inblock,ABBREV *inabbrev)
{

  int packid=0;

  if (inabbrev!=NULL) 
    packid = inabbrev->Abbrev_ID;

  return(PackInteger(inblock,packid,3,FALSE));
}

/*
 *	
 *	Form a unit blockette
 *	
 */

_SUB BOOL UnitBlockette(UNIT *inunit,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
{

  SetupBlockette(inblock,SEED_UNIT_DICT,SEEDVER);

  CKP(PackInteger(inblock,inunit->Unit_ID,3,FALSE));
  PackVariable(inblock,inunit->Unit_Key,20,TRUE);
  PackVariable(inblock,inunit->Comment,50,FALSE);

  RecordPack(inblock,inrecord);

  return(TRUE);
}

/*
 *	
 *	Write out all unit blockettes which are in use
 *	
 */

_SUB BOOL DumpUnitBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
{

  UNIT *looper;

  for (looper=VI->Root_Unit; looper!=NULL; looper=looper->Next) {
    if (looper->Unit_ID==0) continue;

    CKP(UnitBlockette(looper,blockbuf,inrecord));
  }

  return(TRUE);
}

/*
 *	
 *	Write out the unit part of a blockette
 *	
 */

_SUB BOOL PackUnit(BLOCKETTE *inblock,UNIT *inunit)
{

  int packid=0;

  if (inunit!=NULL) 
    packid = inunit->Unit_ID;

  return(PackInteger(inblock,packid,3,FALSE));

}

/*
 *	
 *	Form and write a comment dictionary header
 *	
 */

_SUB BOOL CommentBlockette(COMMENT *incomment,
			   BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
{

  if (incomment->Comment_ID>9999) return(TRUE);	/* Internal use only */

  SetupBlockette(inblock,SEED_COMMENT_DICT,SEEDVER);

  CKP(PackInteger(inblock,incomment->Comment_ID,4,FALSE));
  PackChar(inblock,incomment->Class,TRUE);
  PackVariable(inblock,incomment->Text,70,FALSE);
  CKP(PackUnit(inblock,incomment->Level));

  RecordPack(inblock,inrecord);

  return(TRUE);
}

/*
 *	
 *	Dump out all comment blockettes
 *	
 */

_SUB BOOL DumpCommentBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
{

  COMMENT *looper;

  for (looper=VI->Root_Comment; looper!=NULL; looper=looper->Next) {
    if (looper->Comment_ID==0) continue;

    CKP(CommentBlockette(looper,blockbuf,inrecord));
  }

  return(TRUE);
}

/*
 *	
 *	Write out the comment portion of a blockette
 *	
 */

_SUB BOOL PackComment(BLOCKETTE *inblock,COMMENT *incomment)
{

  int packid=0;

  if (incomment!=NULL) 
    packid = incomment->Comment_ID;

  return(PackInteger(inblock,packid,4,FALSE));
}

/*
 *	
 *	Write out a format blockette
 *	
 */

_SUB BOOL FormatBlockette(FORMAT *informat,
			  BLOCKETTE *inblock,LOGICAL_RECORD *inrecord)
{

  int i;

  SetupBlockette(inblock,SEED_FORMAT_DICT,SEEDVER);

  PackVariable(inblock,informat->Format_Name,50,FALSE);
  CKP(PackInteger(inblock,informat->Format_ID,4,FALSE));
  CKP(PackInteger(inblock,informat->Family,3,FALSE));

  CKP(PackInteger(inblock,informat->NumKeys,2,FALSE));
  for (i=0; i<informat->NumKeys; i++) 
    PackVariable(inblock,informat->Keys[i],500,FALSE);

  RecordPack(inblock,inrecord);

  return(TRUE);
}

/*
 *	
 *	Write out all format blockettes which are in use
 *	
 */

_SUB BOOL DumpFormatBlockettes(BLOCKETTE *blockbuf,LOGICAL_RECORD *inrecord)
{

  FORMAT *looper;

  for (looper=VI->Root_Format; looper!=NULL; looper=looper->Next) {
    if (looper->Format_ID==0) continue;

    CKP(FormatBlockette(looper,blockbuf,inrecord));
  }

  return(TRUE);
}

/*
 *	
 *	Write a blockette piece with the format code in it
 *	
 */

_SUB BOOL PackFormat(BLOCKETTE *inblock,FORMAT *informat)
{

  int packid=0;

  if (informat!=NULL) 
    packid = informat->Format_ID;

  return(PackInteger(inblock,packid,4,FALSE));
}

/*
 *
 *	Assign all id numbers 
 *
 */

_SUB BOOL SetupDictionaryIDs()
{

  FORMAT *formlooper;
  ABBREV *abblooper;
  UNIT *unitlooper;
  int blockid;

  blockid = 0;

  for (formlooper=VI->Root_Format; formlooper!=NULL; 
       formlooper=formlooper->Next) {
    /* if (looper->Use_Count<=0) continue; */
    formlooper->Format_ID = ++blockid;
  }

  /*	blockid = 0;
	
	for (comlooper=VI->Root_Comment; comlooper!=NULL; looper=looper->Next) {
	if (looper->Use_Count<=0) continue; 
	looper->Comment_ID = ++blockid; 
	}
	
	*/
  blockid = 0; 

  for (abblooper=VI->Root_Abbrev; abblooper!=NULL; 
       abblooper=abblooper->Next) {
    /* if (looper->Use_Count<=0) continue; */
    abblooper->Abbrev_ID = ++blockid;
  }

  blockid = 0;

  for (unitlooper=VI->Root_Unit; unitlooper!=NULL; 
       unitlooper=unitlooper->Next) {
    /* if (looper->Use_Count<=0) continue; */
    unitlooper->Unit_ID = ++blockid;
  }

  return(TRUE);
}

/*
 *	
 *	Write out a station blockette
 *	
 */

_SUB BOOL StationBlockette(STATION_LIST *instation,
			   STATION_TIMES *intime,
			   BLOCKETTE *WriteBlock,
			   LOGICAL_RECORD *WriteRec)
{

  NewHeader(WriteRec,'S');

  SetupBlockette(WriteBlock,SEED_STATION_RECORD,VI->SeedVersion);

  PackString(WriteBlock,instation->Station,5,TRUE);
  CKP(PackFixed(WriteBlock,intime->Station->Latitude,10,6,TRUE));
  CKP(PackFixed(WriteBlock,intime->Station->Longitude,11,6,TRUE));
  CKP(PackFixed(WriteBlock,intime->Station->Elevation,7,1,TRUE));
  CKP(PackInteger(WriteBlock,0,4,FALSE));
  CKP(PackInteger(WriteBlock,0,3,FALSE));
  PackVariable(WriteBlock,intime->Station->Site_Name,60,FALSE);
  CKP(PackAbbrev(WriteBlock,intime->Station->Station_Owner));
  CKP(PackInteger(WriteBlock,intime->Station->Long_Order,4,FALSE));
  CKP(PackInteger(WriteBlock,intime->Station->Word_Order,2,FALSE));
  PackTime(WriteBlock,intime->Effective_Start);
  if (intime->Effective_End.year>3000) 
    intime->Effective_End = ST_Zero();
  PackTime(WriteBlock,intime->Effective_End);
  PackChar(WriteBlock,intime->Update_Flag,TRUE);
  if (VI->SeedVersion>=23)
    PackString(WriteBlock,instation->Network,2,TRUE);
	
  RecordPack(WriteBlock,WriteRec);

  return(TRUE);
}

/*
 *	
 *	Write out a channel blockette
 *	
 */

_SUB BOOL ChannelBlockette(CHANNEL_LIST *inchannel,
			   CHANNEL_TIMES *intime,
			   STATION_TIMES *stattime,
			   BLOCKETTE *WriteBlock,
			   LOGICAL_RECORD *WriteRec)
{

  char	flags[30],*flg;
  VOLFLT lat,lon,elv;

  SetupBlockette(WriteBlock,SEED_CHANNEL_RECORD,SEEDVER);

  /* Compute the latitude, longitude, and elevation */

  lat = intime->Channel->Latitude;
  lon = intime->Channel->Longitude;
  elv = intime->Channel->Elevation;

  if (stattime==NULL&&intime->Channel->Coord_Set!=0)
    bombout(EXIT_ABORT,"Coordinates are from station and no station rec");

  switch(intime->Channel->Coord_Set) {
  case 1:
    lat = stattime->Station->Latitude;
    lon = stattime->Station->Longitude;
    elv = stattime->Station->Elevation;
    break;
  case 2:
    lat = stattime->Station->Latitude;
    lon = stattime->Station->Longitude;
    elv = stattime->Station->Elevation + intime->Channel->Local_Depth;
    break;
  case 3:
    if (lat==0.0) lat = stattime->Station->Latitude;
    if (lon==0.0) lon = stattime->Station->Longitude;
    if (elv==0.0) elv = stattime->Station->Elevation;
    break;
  case 0:
    break;
  case 4:    /* New coordinate type - all elevations are surface */
    if (lat==0.0) lat = stattime->Station->Latitude;
    if (lon==0.0) lon = stattime->Station->Longitude;
    if (elv==0.0) elv = stattime->Station->Elevation;
    elv -= fabs(intime->Channel->Local_Depth);
    break;
  default:
    bombout(EXIT_ABORT,"Bad Coord_Set Variable %d",
	    intime->Channel->Coord_Set);
  }

  PackString(WriteBlock,inchannel->Location,2,TRUE);
  PackString(WriteBlock,inchannel->Identifier,3,TRUE);
  CKP(PackInteger(WriteBlock,inchannel->SubChannel,4,FALSE));
  CKP(PackAbbrev(WriteBlock,intime->Channel->Instrument));
  PackVariable(WriteBlock,intime->Channel->Optional,30,FALSE);
  CKP(PackUnit(WriteBlock,intime->Channel->Signal_Response));
  CKP(PackUnit(WriteBlock,intime->Channel->Calibration_Input));
  CKP(PackFixed(WriteBlock,lat,10,6,TRUE));
  CKP(PackFixed(WriteBlock,lon,11,6,TRUE));
  CKP(PackFixed(WriteBlock,elv,7,1,TRUE));
  /* Local depth must be abs value because we store negatives
     as depth and overburdens as positive, but seed makes no
     such distinctions */
  CKP(PackFixed(WriteBlock,fabs(intime->Channel->Local_Depth),5,1,FALSE));
  CKP(PackFixed(WriteBlock,intime->Channel->Azimuth,5,1,FALSE));
  CKP(PackFixed(WriteBlock,intime->Channel->Dip,5,1,TRUE));
  CKP(PackFormat(WriteBlock,intime->Channel->Format_Type));
  CKP(PackInteger(WriteBlock,intime->Channel->Data_Exp,2,FALSE));
  PackExponent(WriteBlock,intime->Channel->Sample_Rate,10);
  PackExponent(WriteBlock,intime->Channel->Max_Drift,10);
  CKP(PackInteger(WriteBlock,0,4,FALSE));

  flg = flags;
  if (intime->Channel->Channel_Flags & CFG_TRIGGERED)	*flg++ = 'T';
  if (intime->Channel->Channel_Flags & CFG_CONTINUOUS)	*flg++ = 'C';
  if (intime->Channel->Channel_Flags & CFG_HEALTHCHAN)	*flg++ = 'H';
  if (intime->Channel->Channel_Flags & CFG_GEODATA)	*flg++ = 'G';
  if (intime->Channel->Channel_Flags & CFG_ENVIRON)	*flg++ = 'W';
  if (intime->Channel->Channel_Flags & CFG_FLAGS)	*flg++ = 'F';
  if (intime->Channel->Channel_Flags & CFG_SYNTH)	*flg++ = 'S';
  if (intime->Channel->Channel_Flags & CFG_CALIN)	*flg++ = 'I';
  if (intime->Channel->Channel_Flags & CFG_EXPERIMENT)	*flg++ = 'E';
  if (intime->Channel->Channel_Flags & CFG_MAINTENANCE)	*flg++ = 'M';
  if (intime->Channel->Channel_Flags & CFG_BEAM)	*flg++ = 'B';

  *flg++ = '\0';

  PackVariable(WriteBlock,flags,26,TRUE);
	
  PackTime(WriteBlock,intime->Effective_Start);
  if (intime->Effective_End.year>3000) 
    intime->Effective_End = ST_Zero();
  PackTime(WriteBlock,intime->Effective_End);
  PackChar(WriteBlock,intime->Update_Flag,TRUE);
	
  RecordPack(WriteBlock,WriteRec);

  return(TRUE);
}

/*
 *	
 *	Write a sensitivity blockette
 *	
 */

_SUB BOOL SensBlockette(SENSITIVITY *insens,int stage,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
{

  int i;

  SetupBlockette(inblock,SEED_GAIN_SENSITIVITY,SEEDVER);

  if (stage>50) stage = 0;    /* Final sensitivity */

  CKP(PackInteger(inblock,stage,2,FALSE));
  PackExponent(inblock,insens->Sensitivity,12);
  PackExponent(inblock,insens->Frequency,12);

  CKP(PackInteger(inblock,insens->Num_Cals,2,FALSE));
	
  for (i=0; i<insens->Num_Cals; i++) {
    PackExponent(inblock,insens->Cal_Value[i],12);
    PackExponent(inblock,insens->Cal_Freq[i],12);
    PackTime(inblock,insens->Cal_Time[i]);
  }

  RecordPack(inblock,inrec);

  return(TRUE);
}

/*
 *	
 *	Write a decimation blockette
 *	
 */

_SUB BOOL DeciBlockette(DECIMATION *indeci,int stage,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
{

  SetupBlockette(inblock,SEED_DECIMATION,SEEDVER);

  CKP(PackInteger(inblock,stage,2,FALSE));
  PackExponent(inblock,indeci->Input_Rate,10);
  CKP(PackInteger(inblock,indeci->Factor,5,FALSE));
  CKP(PackInteger(inblock,indeci->Offset,5,FALSE));
  PackExponent(inblock,indeci->Delay,11);
  PackExponent(inblock,indeci->Correction,11);

  RecordPack(inblock,inrec);

  return(TRUE);
}

/*
 *	
 *	Write a zeros and poles blockette
 *	
 */

_SUB BOOL ZerosBlockette(ZEROS_POLES *inzero, int stage,
			 BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
{

  int i;

  SetupBlockette(inblock,SEED_ZEROS_POLES,SEEDVER);

  PackChar(inblock,inzero->Function_Type,TRUE);
  CKP(PackInteger(inblock,stage,2,FALSE));
  CKP(PackUnit(inblock,inzero->Response_In));
  CKP(PackUnit(inblock,inzero->Response_Out));
  PackExponent(inblock,inzero->AO_Normalization,12);
  PackExponent(inblock,inzero->Norm_Frequency,12);

  CKP(PackInteger(inblock,inzero->Complex_Zeros,3,FALSE));

  for(i=0; i<inzero->Complex_Zeros; i++) {
    PackExponent(inblock,inzero->Zero_Real_List[i],12);
    PackExponent(inblock,inzero->Zero_Imag_List[i],12);
    PackExponent(inblock,inzero->Zero_Real_Errors[i],12);
    PackExponent(inblock,inzero->Zero_Imag_Errors[i],12);
  }

  CKP(PackInteger(inblock,inzero->Complex_Poles,3,FALSE));

  for(i=0; i<inzero->Complex_Poles; i++) {
    PackExponent(inblock,inzero->Pole_Real_List[i],12);
    PackExponent(inblock,inzero->Pole_Imag_List[i],12);
    PackExponent(inblock,inzero->Pole_Real_Errors[i],12);
    PackExponent(inblock,inzero->Pole_Imag_Errors[i],12);
  }

  RecordPack(inblock,inrec);

  return(TRUE);
}

/*
 *	
 *	Write a coefficients blockette
 *	
 */

#ifdef OLDSTUPIDCOEF /* Doesn't handle >9999 byte blockettes */
_SUB BOOL CoefBlockette(COEFFICIENTS *incoef, int stage,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
{

  int i;

  SetupBlockette(inblock,SEED_COEFFICIENTS,SEEDVER);

  PackChar(inblock,incoef->Function_Type,TRUE);
  CKP(PackInteger(inblock,stage,2,FALSE));
  CKP(PackUnit(inblock,incoef->Response_In));
  CKP(PackUnit(inblock,incoef->Response_Out));

  CKP(PackInteger(inblock,incoef->Numerators,4,FALSE));

  for(i=0; i<incoef->Numerators; i++) {
    PackExponent(inblock,incoef->Numer_Coeff[i],12);
    PackExponent(inblock,incoef->Numer_Error[i],12);
  }

  CKP(PackInteger(inblock,incoef->Denominators,4,FALSE));

  for(i=0; i<incoef->Denominators; i++) {
    PackExponent(inblock,incoef->Denom_Coeff[i],12);
    PackExponent(inblock,incoef->Denom_Error[i],12);
  }

  RecordPack(inblock,inrec);

  return(TRUE);
}
#endif

BOOL CoefBlockette(COEFFICIENTS *incoef, int stage,
		   BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
{

  int i;

  int total_nums,
    start_nums,
    do_nums,
    total_dens,
    start_dens,
    do_dens,
    budget;

  /* Initial conditions */

  total_nums = incoef->Numerators;
  start_nums = 0;

  total_dens = incoef->Denominators;
  start_dens = 0;

  FOREVER {

    do_nums = total_nums - start_nums;
    do_dens = total_dens - start_dens;

    /* Check that we do not exceed the 9999 byte blockette limit */

    if ((do_nums+do_dens)>400) { /* Exceeded 9600+ */

      budget = 400;		/* Our budget for this blk */

      do_nums = min(budget,total_nums - start_nums);
      budget -= do_nums;
      do_dens = min(budget,total_dens - start_dens);

    }

    SetupBlockette(inblock,SEED_COEFFICIENTS,SEEDVER);

    PackChar(inblock,incoef->Function_Type,TRUE);
    CKP(PackInteger(inblock,stage,2,FALSE));
    CKP(PackUnit(inblock,incoef->Response_In));
    CKP(PackUnit(inblock,incoef->Response_Out));

    CKP(PackInteger(inblock,do_nums,4,FALSE));

    for(i=start_nums; i<do_nums+start_nums; i++) {
      PackExponent(inblock,incoef->Numer_Coeff[i],12);
      PackExponent(inblock,incoef->Numer_Error[i],12);
    }

    CKP(PackInteger(inblock,do_dens,4,FALSE));

    for(i=start_dens; i<do_dens+start_dens; i++) {
      PackExponent(inblock,incoef->Denom_Coeff[i],12);
      PackExponent(inblock,incoef->Denom_Error[i],12);
    }

    RecordPack(inblock,inrec);

    start_nums += do_nums;
    start_dens += do_dens;

    /* See if we have finished the volume */

    if ((start_nums>=total_nums)    &&
	(start_dens>=total_dens)) break;

  }

  return(TRUE);

}
 
/*
 *	
 *	Write out whichever response is appropriate
 *	
 */

_SUB BOOL RespBlockette(RESPONSE *inresp,
			BLOCKETTE *inblock,LOGICAL_RECORD *inrec)
{

  switch (inresp->type) {
  case RESP_SENSITIVITY:
    return(SensBlockette(inresp->ptr.SENS,inresp->Stage,
			 inblock,inrec));
  case RESP_ZEROS_POLES:
    return(ZerosBlockette(inresp->ptr.PZ,inresp->Stage,
			  inblock,inrec));
  case RESP_COEFFICIENTS:
    return(CoefBlockette(inresp->ptr.CO,inresp->Stage,
			 inblock,inrec));
  case RESP_DECIMATION:
    return(DeciBlockette(inresp->ptr.DM,inresp->Stage,
			 inblock,inrec));
#ifdef BAROUQUE_RESPONSES
  case RESP_RESP_LIST:
  case RESP_GENERIC:
#endif
  default:
    return(FALSE);		/* Not yet implemented */
  }
}

/*
 *	
 *	Write comment entry blockette entry out
 *	
 */

_SUB BOOL ComBlockette(COMMENT_ENTRY *incom,BLOCKETTE *inblock,
		       LOGICAL_RECORD *inrec,BOOL station)
{

  if (incom->Comment->Comment_ID<0) return(TRUE);
  if (incom->Comment->Comment_ID>9999) return(TRUE);

  SetupBlockette(inblock,
		 station?SEED_STATION_COMMENT:SEED_CHANNEL_COMMENT,SEEDVER);

  PackTime(inblock,incom->Start_Comment);
  PackTime(inblock,incom->End_Comment);
  CKP(PackComment(inblock,incom->Comment));
  CKP(PackInteger(inblock,incom->Comment_Level,6,FALSE));

  RecordPack(inblock,inrec);

  return(TRUE);
}
