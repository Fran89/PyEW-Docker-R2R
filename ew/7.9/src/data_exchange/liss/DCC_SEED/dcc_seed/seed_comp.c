/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_comp.c 4531 2011-08-10 01:29:00Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:06:20  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>
#include <seed_comp.h>
#include <dcc_misc.h>

_SUB UDCC_LONG _SD_SetCtlType(int pos,UDCC_LONG val,UDCC_LONG *outctl)
{

	UDCC_LONG inlong,mask;
	int sf;

	inlong=LocGM68_LONG(*outctl);

	sf=(15-(pos+1))*2;
	mask=3;
	if (sf>0) mask <<= sf;
	inlong &= ~mask;
	if (sf>0) val <<= sf;
	inlong |= val;

	*outctl=M68GLoc_LONG(inlong);
	return(inlong);
}

_SUB VOID _SD_ClearContext(SD_Context *context)
{

	memset(&context->databuf[64], 0, context->blocksz - 64);

	context->samp_pop = 0;
	context->frmpop = 0;
	context->frame = 0;
	context->chkidx = 0;
	context->frmidx = 0;
}

_SUB SD_Context *SD_CreateChannel(UDCC_BYTE *network,
			     UDCC_BYTE *station,
			     UDCC_BYTE *location,
			     UDCC_BYTE *channel,
			     int subchan,
			     VOID (*flushroutine)(VOID *),
			     int format,
			     int len,
			     DCC_LONG ms_sam)
{

	SD_Context *context;

	context = (SD_Context *) SafeAlloc(sizeof(SD_Context));

	context->Network[0] = '\0';
	context->Station[0] = '\0';
	context->Location[0] = '\0';
	context->Channel[0] = '\0';
       
	if (network!=NULL) strcpy((char *) context->Network,(char *) network);
	if (station!=NULL) strcpy((char *) context->Station,(char *) station);
	if (location!=NULL) strcpy((char *) context->Location,(char *) location);
	if (channel!=NULL) strcpy((char *) context->Channel,(char *) channel);

	context->flushoutput = flushroutine;
	context->ms_sam = ms_sam;

	context->outrec = (SEED_DATA *) context->databuf;
	context->out_data = (struct steim_frames *) context->databuf;

	context->blocksz = len;
	if (len>4096)
	    bombout(EXIT_ABORT,
		    "Seed record cannot be larger than 4096 (%d)\n",
		    len);

	memset(context->databuf, 0, len);
	memset(context->databuf, ' ', 20);

	context->sframe = 1;

	_SD_ClearContext(context);

	return(context);
}

_SUB VOID SD_DestroyChannel(SD_Context *context)
{

	free(context);

}

#define nfit8(val) ((val) & (~0x7F))
#define nfit16(val) ((val) & (~0x7FFF))

static int lbi[4] = { 1, 2, 3, 0 };		/* Fast ring buffers */
static int lbv[4][4] = { { 1, 2, 3, 0 },
		         { 2, 3, 0, 1 },
		         { 3, 0, 1, 2 },
		         { 0, 1, 2, 3 } };

_SUB VOID SD_PutSample(SD_Context *context, DCC_LONG delta, DCC_LONG sample, BOOL flush)
{
       
	union long_element *stoel;
	int sto,typ;
	int total,i,j;
	STDTIME nextime;

	total = context->frmpop;

	if (flush) {

		if (context->frmpop<=0) return;	/* Nothing outstanding */
	
		while (context->frmpop<4) {

			if (context->frmidx<0||
			    context->frmidx>3) {
				fprintf(stderr,"SEED_COMP: frmidx was %d\n",
					context->frmidx);
				context->frmidx = 0;
			}
			context->frmidx = lbi[context->frmidx];
			context->frmbuf[lbv[context->frmidx][3]] = 0;
			context->tstbuf[lbv[context->frmidx][3]] = 0;
			context->sambuf[lbv[context->frmidx][3]] = 0;
			context->frmwt[lbv[context->frmidx][3]] = 2;
			context->frmpop++;
		}

	} else {

		if (context->frmidx<0||
		    context->frmidx>3) {
			fprintf(stderr,"SEED_COMP: frmidx was %d\n",
				context->frmidx);
			context->frmidx = 0;
		}
		context->frmidx = lbi[context->frmidx];
		context->frmbuf[lbv[context->frmidx][3]] = delta;
		context->tstbuf[lbv[context->frmidx][3]] = abs(delta);
		context->sambuf[lbv[context->frmidx][3]] = sample;
		context->frmwt[lbv[context->frmidx][3]] = 1;
		context->frmpop++;

		total = 4;

		if (context->frmpop<4) return;		/* Not full yet */
	}


	if (context->samp_pop==0) {
		context->out_data->framestore[context->sframe].
				chunkstore[0].type_3.longval =
			M68GLoc_LONG(context->sambuf[lbv[context->frmidx][0]]);
		context->movic = 0;
		context->frame = context->sframe;
		context->chkidx = 2;	/* Skip ic's */
	}

	stoel = &context->out_data->framestore[context->frame].chunkstore[context->chkidx];

	/* Will they fit in 8 bits? */

	sto = 0;

	if (nfit8(context->tstbuf[lbv[context->frmidx][0]]) ||
	    nfit8(context->tstbuf[lbv[context->frmidx][1]]) ||
	    nfit8(context->tstbuf[lbv[context->frmidx][2]]) ||
	    nfit8(context->tstbuf[lbv[context->frmidx][3]])) {

		/* How about 16 bits */

		if (nfit16(context->tstbuf[lbv[context->frmidx][0]]) ||
		    nfit16(context->tstbuf[lbv[context->frmidx][1]])) {

			/* No, its big */

			stoel->type_3.longval=
				M68GLoc_LONG(context->frmbuf[lbv[context->frmidx][0]]);
			sto = 1;
			typ = 3;
		
		} else {

			for (i=0; i<2; i++) 
				stoel->type_2.wordval[i] = 
					M68GLoc_WORD(context->frmbuf[lbv[context->frmidx][i]]);

			sto = 2;
			typ = 2;
		}
	} else {
		
		for (i=0; i<4; i++) 
			stoel->type_1.byteval[i] = 
				context->frmbuf[lbv[context->frmidx][i]];

		sto = 4;
		typ = 1;
	}
			
        (VOID) _SD_SetCtlType(context->chkidx,typ,
		&context->out_data->framestore[context->frame].ctlflags);

	for (i=0; i<sto; i++) {
		j = context->frmwt[lbv[context->frmidx][i]];
		if (j==1) {
			context->samp_pop++;
			context->movic += context->frmbuf
				[lbv[context->frmidx][i]];
		}
	}

	context->frmpop -= sto;
	if (context->frmpop<0) context->frmpop = 0;
	total -= sto;
	if (total<0) total = 0;

	context->chkidx++;

	if (context->chkidx>14) {		/* Filled the frame */
		context->chkidx = 0;
		context->frame++;
		if (context->frame>=NUMFRAM) {	/* Filled the record */
			context->frame = 0;
			
			context->out_data->framestore[context->sframe].
				chunkstore[1].type_3.longval =
				M68GLoc_LONG(context->movic);

			if (context->ms_sam!=0) 

				nextime = ST_AddToTime(context->begtime,0,0,0,0,
					context->ms_sam * context->samp_pop);

			else nextime = context->marktime;	
					/* Use last known for logs */
			
			SD_FlushRecord(context);

			context->begtime = nextime;			
		}
	}

	if (flush&&total>0) {
		SD_PutSample(context, 0, 0, TRUE);	/*Flush more */
		context->out_data->framestore[context->sframe].
			chunkstore[1].type_3.longval =
			M68GLoc_LONG(context->movic);
	}

}

_SUB VOID SD_FlushRecord(SD_Context *context)
{
	SD_PutSample(context,0,0,TRUE);		/* Flush current frame */

	if (context->samp_pop<=0) return;

	context->outrec->Number_Samps = M68GLoc_WORD(context->samp_pop);

  	context->outrec->Start_Time.year = M68GLoc_WORD(context->begtime.year);
  	context->outrec->Start_Time.day = M68GLoc_WORD(context->begtime.day);
  	context->outrec->Start_Time.hour = context->begtime.hour;
  	context->outrec->Start_Time.minute = context->begtime.minute;
  	context->outrec->Start_Time.seconds = context->begtime.second;
  	context->outrec->Start_Time.dummy = 0;
  	context->outrec->Start_Time.fracs = M68GLoc_WORD(context->begtime.msec * 10);
	
	context->outrec->Data_Start = M68GLoc_WORD(64 * context->sframe);

        (*context->flushoutput)(context->databuf);

	_SD_ClearContext(context);

}

_SUB VOID SD_PutData(SD_Context *context, STDTIME datetime, int numsamps,
	DCC_LONG *dataarray)
{

	STDTIME nowtime;
	int	i;
	DCC_LONG	tsam;

	if (context->samp_pop!=0&&context->ms_sam!=0) {	
				/* We must look for time discontinuities */
				/* ms_sam==0, must be log file */

		nowtime = ST_AddToTime(context->begtime,0,0,0,0,
			context->ms_sam * context->samp_pop);

		if (ST_TimeComp(nowtime,datetime)!=0) {	/* Zero tolerance */
			SD_FlushRecord(context);
		}
	}			

	context->marktime = datetime;

	if (context->samp_pop==0) {
		context->begtime = datetime;
		context->psam = 0;		/* No record continuation */
	}

	for (i=0; i<numsamps; i++) {
		tsam = dataarray[i] - context->psam;
		SD_PutSample(context,tsam,dataarray[i],FALSE);
		context->psam = dataarray[i];
	}
	
} 
