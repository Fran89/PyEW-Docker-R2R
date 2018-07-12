/* support routines to convert time to sample indices and back */

#include "uwdfif.h"

/* TTOS_M - convert time, in seconds from the minute of the master header
   (i.e., the earliest channel), to sample index of channel */
int ttos_m(int chno, float ts)
{
	struct stime ch_start, mh_start, tss;
	int samp;
	float dtStart;

	/* need time diff between channel and master to get sample count */
	UWDFdhref_stime(&mh_start);	
	UWDFchref_stime(chno, &ch_start);
	dtStart = UWDFtime_diff(ch_start,mh_start);

	/* get minute of channel start */	
	tss = ch_start;
	/* replace seconds of channel start with desired seconds, less
	   diff between channel and master */
	tss.sec = ts - dtStart;
	
	/* convert to samples from start of trace */	
	samp = ROUND(UWDFtime_diff(tss,ch_start) * UWDFchsrate(chno));
	samp = MAX(samp,0);
	samp = MIN(samp,UWDFchlen(chno) - 1);
	
	return( samp );
}

/* TTOS_CH - convert time, in seconds from the minute of the channel header,
   to sample index of channel */
int ttos_ch(int chno, float ts)
{
	struct stime ch_start, tss;
	int samp;

	/* get channel start time structure */	
	UWDFchref_stime (chno, &ch_start);
	
	/* get minutes and seconds of channel start */	
	tss = ch_start;
	/* replace seconds of channel start with desired seconds */
	tss.sec = ts;

	/* convert to samples from start of trace */	
	samp = ROUND(UWDFtime_diff(tss,ch_start) * UWDFchsrate(chno));
	samp = MAX(samp,0);
	samp = MIN(samp,UWDFchlen(chno) - 1);
	
	return( samp );
}

/* STOT_M - convert channel sample index to seconds from minutes of
   the master header */
float stot_m(int chno, int samp)
{
	struct stime ch_start, mh_start;
	float dtStart, ts;
	
	/* need time diff in seconds between channel and master */
	UWDFdhref_stime(&mh_start);	
	UWDFchref_stime(chno, &ch_start);
	dtStart = UWDFtime_diff(ch_start,mh_start);

	/* get seconds from minutes of master header */
	ts = mh_start.sec + dtStart + samp * UWDFchsrate(chno);
	
	return( ts );
}
	
/* STOT_CH - convert channel sample index to seconds from minutes of
   the channel header */
float stot_ch(int chno, int samp)
{
	struct stime ch_start;
	float ts;
	
	/* get channel start time */
	UWDFchref_stime(chno, &ch_start);

	/* get seconds from minutes of channel header */
	ts = ch_start.sec + samp * UWDFchsrate(chno);
	
	return( ts );
}
