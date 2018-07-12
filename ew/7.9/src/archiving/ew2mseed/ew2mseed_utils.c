/*=====================================================================
// Copyright (C) 2000,2001 Instrumental Software Technologies, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code, or portions of this source code,
//    must retain the above copyright notice, this list of conditions
//    and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. All advertising materials mentioning features or use of this
//    software must display the following acknowledgment:
//    "This product includes software developed by Instrumental
//    Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
//    product, or is used in other commercial software products the
//    customer must be informed that "This product includes software
//    developed by Instrumental Software Technologies, Inc.
//    (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
//    must not be used to endorse or promote products derived from
//    this software without prior written permission. For written
//    permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
//    nor may "ISTI" appear in their names without prior written
//    permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
//    acknowledgment:
//    "This product includes software developed by Instrumental
//    Software Technologies, Inc. (http://www.isti.com/)."
// THIS SOFTWARE IS PROVIDED BY INSTRUMENTAL SOFTWARE
// TECHNOLOGIES, INC. "AS IS" AND ANY EXPRESSED OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.  IN NO EVENT SHALL INSTRUMENTAL SOFTWARE TECHNOLOGIES,
// INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//=====================================================================
//  A current version of the software can be found at
//                http://www.isti.com
//  Bug reports and comments should be directed to
//  Instrumental Software Technologies, Inc. at info@isti.com
//=====================================================================
// This work was funded by the IRIS Data Management Center
// http://www.iris.washington.edu
//===================================================================== 
*/
/*
**   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
**   CHECKED IT OUT USING THE COMMAND CHECKOUT.
**
**    $Id: ew2mseed_utils.c 5391 2013-03-04 18:55:57Z paulf $
** 
**	Revision history:
**	$Log$
**	Revision 1.1  2010/03/10 17:14:58  paulf
**	first check in of ew2mseed to EW proper
**
**	Revision 1.5  2007/04/13 19:43:22  hal
**	use flock instead of lockf when compiling under cygwin (which lacks lockf)
**	
**	Revision 1.4  2007/04/12 19:21:04  hal
**	Added ew7.0+ support (-dSUPPORT_SCNL) and linux port (-D_LINUX)
**	
**	-Hal
**	
**	Revision 1.3  2002/12/19 00:35:00  ilya
**	Byteswapping in shorts and doubles is fixed
**	
**	Revision 1.2  2002/04/02 14:42:01  ilya
**	Version 02-Apr-2002: catchup algorithm
**	
**	Revision 1.1.1.1  2002/01/24 18:32:05  ilya
**	Exporting only ew2mseed!
**	
**	Revision 1.1.1.1  2001/11/20 21:47:00  ilya
**	First CVS commit
**	
 * Revision 1.7  2001/03/15  16:42:52  comserv
 * 	void signal_handler() modified to accomodate SIGALRM
 *
 * Revision 1.6  2001/02/06  17:19:32  comserv
 * Functions logFailedSCN() resetWsTrace() are moved to ew2mseed_utils.c
 * Functions  logFailedSCN() and  checkLock() are changed, so you can actually
 * write data into them, and still use them for locking. int lockfd is now a global
 * variable
 *
 * Revision 1.5  2001/02/06  05:53:29  comserv
 * Added code  to make sure is that the content of the lockfile is truncated to zero.
 * We now write SCNs which we cannot process in this file
 *
 * Revision 1.4  2001/01/18  21:58:20  comserv
 * checkForGaps is modified to detect snippet overlaps
 *
 * Revision 1.4  2001/01/18  21:58:20  comserv
 * checkForGaps is modified to detect snippet overlaps
 *
 * Revision 1.3  2001/01/17  21:56:54  comserv
 * prerelease version ; copyright statement
 *
 * Revision 1.2  2001/01/08  22:55:31  comserv
 * locking function is added
 *
 * Revision 1.1  2000/11/17  06:56:57  comserv
 * Initial revision
 *
*/

/* Standard includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#ifdef _CYGWIN
#  include <sys/file.h>
#endif

/* Includes from Earthworm distribution */
#include "earthworm.h"

/* Local includes */
#include "ew2mseed.h"
#include "swap.h"
#include "qlib2.h"

/* Global variable */
int lockfd = -1;

/***************************************************************************/
  double getReqEndTime (double startTime, double SampRate, int samples)
  {
  	double endTime;
	endTime = startTime + (double)samples/SampRate;
	return endTime;	
  }


/*****************************************************************************/
/* Define how many UNCOMPRESSED bytes from WS we have to request to fill     */
/* for sure a single MiniSEED record                                         */ 
/* IGD 03/28/02 highWaterMark is assuming possible increase in the size of   */
/* the requested snippet                                                     */
/****************************************************************************/
  long int assumeSnippetSize (int logRec, short CompAlg, short RecNum,  
		int highWaterMark)
  { 
	long int bytes;
	
	/*Must be larger than the maximum */
	if (CompAlg == STEIM1)
		bytes = 2 * logRec * RecNum * highWaterMark * sizeof(int);
	else if(CompAlg == STEIM2)
		bytes = 4 * logRec * RecNum * highWaterMark * sizeof(int);
	else	{
		fprintf(stderr, "fatal error in assumeSnippetSize: %d is not supported compression algorithm\n", CompAlg);
		logit("pt", "fatal error in assumeSnippetSize: %d is not supported compression algorithm\n", CompAlg);
		return -1;
		}
	return bytes;
  }

/***************************************************************************/

  void makeSNCLoop (RINGS *rn)
  {
	int i; 
 	struct SCN_ring *scn_head;

	scn_head = rn->scnRing;	

	for (i=0; i < rn->SCN_avail-1; i++)
		rn->scnRing = rn->scnRing->next;
	
	rn->scnRing->next = scn_head;	


  }  
/***************************************************************************/
/************************************************************************/
/*  checkForGaps:                                                       */
/*      Reads in the sampling rate, time of the last sample of the      */ 
/*	previous snippet and the first sample of the current snippet.   */
/*      Checks for the data consistency                                 */
/*  returns:                                                            */
/*      0 if there is no gaps (tears) or overlaps                       */
/*      positive double gap in seconds or negative overlap in seconds   */
/*                                                                      */
/************************************************************************/ 
  double checkForGaps (double prevEndTime, double startTime, double sampRate)
  {
	double smallValue, invSampRate;

	invSampRate = 1./sampRate;
	smallValue = invSampRate/10.;

	/* Here is the gap 
	 ****************************/ 
	if ((prevEndTime + invSampRate + smallValue) < startTime )
		return (startTime - prevEndTime - invSampRate);  /*positive */

	/* Here is the overlap
	 ***************************/
	if ((prevEndTime + invSampRate - smallValue) > startTime )
		return (startTime - prevEndTime - invSampRate); /* negative */

	return 0;
  }
/****************************************************************************/
  long ew2mseedAddData (int *buffer, char *msg_p, long position, int bufLen, char swap, int bytesPerSample)
  {	float *fBuf = NULL;
	double *dBuf = NULL;
	int *iBuf = NULL;
	short *sBuf= NULL;
	int i;

	if (swap == 't' && bytesPerSample == 8) /*t8 SUN IEEE double precision real */
	{
		dBuf = (double *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			double tmp;
			tmp = *(sBuf + i);
#if defined _INTEL
			ew2mseed_SwapDouble(&tmp, 't');	
#endif
			*(buffer + i+ position) = (int)tmp;	
		}
	}	
	else if (swap == 't' && bytesPerSample == 4) /*t4 SUN IEEE single precision real */
	{
		fBuf = (float *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			*(buffer + i+ position) = (int) *(fBuf + i);
#if defined _INTEL
			SwapInt(buffer+i+position);
#endif	
		}	
	}
	else if (swap == 's' && bytesPerSample == 4) /*s4 SUN IEEE integer */
	{
		iBuf = (int *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			*(buffer + i+ position) = (int) *(iBuf + i);
#if defined _INTEL
			SwapInt(buffer+i+position);
#endif
		}		
	}
	else if (swap == 's' && bytesPerSample == 2) /*s2 SUN IEEE short integer */
	{	
		sBuf = (short *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			short tmp;
			tmp = *(sBuf + i);
#if defined _INTEL
			ew2mseed_SwapShort(&tmp, 's');
#endif	
			*(buffer + i+ position) = (int) tmp;
		}
	}
	else if (swap == 'f' && bytesPerSample == 8) /*f8 VAX/Intel IEEE double precision real */
	{
		dBuf = (double *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			double tmp;
			tmp = *(sBuf + i);
#if defined _SPARC
			ew2mseed_SwapDouble(&tmp, 'f');
#endif	
			*(buffer + i+ position) = (int) tmp;
		}	
	}
	else if (swap == 'f' && bytesPerSample == 4) /*f4 VAX/Intel IEEE single precision real */
	{
		fBuf = (float *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			*(buffer + i+ position) = (int) *(fBuf + i);
#if defined _SPARC
			SwapInt(buffer+i+position);
#endif		
		}
	}
	else if (swap == 'i' && bytesPerSample == 4) /*i4 VAX/Intel IEEE integer */
	{
		iBuf = (int *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			*(buffer + i+ position) = (int) *(iBuf + i);
#if defined _SPARC
			SwapInt(buffer+i+position);
#endif
		}		
	}
	else if (swap == 'i' && bytesPerSample == 2) /*i2 VAX/Intel IEEE short integer */	
	{
		sBuf = (short *) msg_p;
		for (i = 0; i< bufLen; i ++)
		{
			short tmp;
			tmp = *(sBuf + i);
#if defined _SPARC
			ew2mseed_SwapShort(&tmp, 'i');
#endif	
			*(buffer + i+ position) = (int) tmp; 
		}		
	}
	else
	{
		return -1;
	}

	return (position + bufLen);
  }

/****************************************************************************/
  int getBufferShift (double reqStarttime, double actStarttime, double samprate)
  {
	int myshift;
	double timeDiff;
	double verySmall = 0.000001;

	if (reqStarttime < actStarttime)
	{
		if (reqStarttime + 0.5/samprate >= actStarttime )
			return 0; 	
		else
			return -1;
	}

	timeDiff = reqStarttime - actStarttime;
	myshift = (int) nearbyint(timeDiff * samprate);
	while (1)
	{
		if ( actStarttime + myshift/samprate - reqStarttime >= (0.5/samprate))
		{
			if (fabs(actStarttime + myshift/samprate - reqStarttime - 0.5/samprate) < verySmall )
			{
				printf("fabs = %f\n", fabs(actStarttime + myshift/samprate - reqStarttime - 0.5/samprate));
				break;
			}
			myshift--;
		}
		else if (actStarttime + myshift/samprate - reqStarttime < -0.5/samprate)
		{
			myshift++;
		}
		else
			break;
	}	
/*	printf("%d %f %f %f\n", myshift, actStarttime + myshift/samprate - reqStarttime, samprate,  timeDiff); */
	return myshift;
	
  } 

/****************************************************************************/

  int checkCompilationFlags (char *prog)
  {	int retVal = -2;
#ifdef _SPARC
	retVal = 1;
#endif
#ifdef  _INTEL
        retVal = 1;
#endif
	if(retVal == -2)
	{
	fprintf(stderr, "-------------------------------------------------------\n");
	fprintf(stderr, "%s should be compiled with -D_SPARC or -D_INTEL flags only!\n", prog);
	fprintf(stderr, "Please recompile the program with this flag\n");
	fprintf(stderr, "-------------------------------------------------------\n");
	}

	return(retVal);
  }

  char *strDate( double epochTime, char *dd)
  {
	/* Warning: dd should be allocated outside ! */


	/* SEC1970 is a convertion from epoch to julian seconds */
	date20(epochTime + SEC1970, dd);
	return dd;
  }

/**********************************************************/

  double findNewFileJulTime (double jtime)
  {	
	double nextDayJtime;
	EXT_TIME ext_time;
 
	ext_time = int_to_ext (tepoch_to_int(jtime));
	ext_time.hour = 0;
	ext_time.minute = 0;
	ext_time.second = 0;
	ext_time.usec = 0;
	/* next day */
	nextDayJtime = int_to_tepoch(ext_to_int(ext_time)) + 24 * 60 *60;
 	return (nextDayJtime);
 }
  /* IGD 03/14/01 Modified for SIGALRM and 
   *********************************/
  void signal_handler (int sig)
  {
	if (sig == SIGUSR1)	
		ew_signal_flag = EW2MSEED_MAKE_REPORT; 
	else if (sig == SIGALRM)
		ew_signal_flag = EW2MSEED_ALARM; 
	else
		ew_signal_flag = EW2MSEED_TERMINATE;
	signal(sig, signal_handler); /*Re-install handler */	
  }

  void finish_handler (RINGS *rn)
  {
	fprintf(stderr, "TERMINATION SIGNAL ARRIVED\n");
	fprintf(stderr, "PROGRAM EXITS NOW\n");
	logit("pt", "TERMINATION SIGNAL ARRIVED\n");
	logit("pt", "PROGRAM EXITS NOW\n");
	ew_signal_flag = EW2MSEED_OK;
	exit(0);	
  }
  
  void ew2mseedGenReport (RINGS *p_rn)
  {


	fprintf(stderr, "STATE-OF-HEALTH REPORT REQUEST RECEIVED\n");
	ew2mseed_logconfig(p_rn);

	fprintf(stderr, "REPORT IS WRITTEN INTO THE LOG FILE\n");
	ew_signal_flag = EW2MSEED_OK;
	
	
  }

  int checkLock (char * lockfile)
  {
	int status;
	if (lockfile == NULL)
	{
		fprintf(stderr, "This instance of ew2mseed does not use the locking mechanism!\n");
		fprintf(stderr, "Multiple copies of ew2mseed are allowed to write into a single data file\n");
		fprintf(stderr, "To enable locking set LockFile parameter in the configuration file\n");
		logit ("", "This instance of ew2mseed does not use the locking mechanism!\n");
		logit("", "Multiple copies of ew2mseed are allowed to write into a single data file\n");
		logit("", "To enable locking set LockFile parameter in the configuration file\n");
		return(0);
	}

	if ((lockfd = open (lockfile, O_RDWR|O_CREAT,0644)) < 0) 
	{
		fprintf (stderr, "Error in ew2mseed: Unable to open lockfile: %s\n", lockfile);
		logit ("pt", "Error in ew2mseed: Unable to open lockfile: %s\n", lockfile);
		return (-1);
	}	
#ifdef _CYGWIN
	if ((status=flock(lockfd, LOCK_EX|LOCK_NB)) < 0)
#else
	if ((status=lockf (lockfd, F_TLOCK, 0)) < 0)
#endif
	{
		fprintf (stderr, "Error in ew2mseed: Unable to lock lockfile: %s status=%d errno=%d\n", 
				lockfile, status, errno);
	 	logit ("pt", "Error in ew2mseed Unable to lock daemon lockfile: %s status=%d errno=%d\n", 
				lockfile, status, errno);
		return(-2);
	}
	close(lockfd);
	
	if ((lockfd = open (lockfile, O_RDWR|O_CREAT|O_TRUNC,0644)) < 0) 
	{
		fprintf (stderr, "Error in ew2mseed: Unable to open lockfile: %s\n", lockfile);
		logit ("pt", "Error in ew2mseed: Unable to open lockfile: %s\n", lockfile);
		return (-1);
	}

#ifdef _CYGWIN
	if ((status=flock(lockfd, LOCK_EX|LOCK_NB)) < 0)
#else
	if ((status=lockf (lockfd, F_TLOCK, 0)) < 0)
#endif
	{
		fprintf (stderr, "Error in ew2mseed: Unable to lock lockfile: %s status=%d errno=%d\n", 
				lockfile, status, errno);
	 	logit ("pt", "Error in ew2mseed Unable to lock daemon lockfile: %s status=%d errno=%d\n", 
				lockfile, status, errno);
		return(-2);
	}
	return(1); /*Success */
	
  }
/*---------------------------------------------------------*/
/* IGD 02/05/01 a function to reset TRACE_REQ * strucuture */

	int resetWsTrace (TRACE_REQ* trace, double startTime, double endTime)
	{
		trace->bufLen = 0;
		trace->reqStarttime = startTime;
		trace->reqEndtime = endTime;
		free(trace->pBuf);
		trace->pBuf = (char *)NULL;
		trace->timeout = 0;
		trace->actStarttime = 0;
		trace->actEndtime = 0;
		trace->actLen = 0;
		trace->retFlag = 0;
		return 0;
	}

/* IGD this function is added on 02/05/01 */

	void logFailedSCN (struct SCN_ring *failedTrace, char *lockFN)	
	{
		
		char line[100];
		char locId[5];
		char alg[7];
		memset(&locId[0], '\0', 5);
		if (failedTrace->locId[0] == '\000')
			strcpy(locId, "NONE");
		else
			strncpy(locId, failedTrace->locId, 2);	
		if (failedTrace->CompAlg == 11)
			strcpy(alg, "STEIM2");
		else if  (failedTrace->CompAlg == 10)
			strcpy(alg, "STEIM1");
		else
			strcpy(alg, "ALG???");
		sprintf(line, "SCNLocSZ %s %s %s %s %d %s\n",
			failedTrace->traceRec.sta, failedTrace->traceRec.chan,
			failedTrace->traceRec.net, locId, failedTrace->logRec, alg);

		logit("pt", "Trace %s is removed from ew2mseed loop\n", line);

		if (lockfd == -1 )
			fprintf(stderr,  "Trace %s is removed from ew2mseed loop\n", line);
		else	
			write(lockfd, line, strlen(line));
		return;
	}

 
/* IGD 03/29/02 Computes priority for catchup algorithm */
	
void  updatePriorityValues(RINGS *rn)
{
 struct SCN_ring *scn_head;
 int i; 
 int j = 0;
 double meanSystemTime = 0;
 double timeFromTank = 0;	
 int priorityFactor = 3600 * 24; /* 1 day */
 int priority;
 int TankPriority;
 int previous_priority;
		
	/* Get the ring start */
	scn_head = rn->scnRing;

	for (i = 0; i< rn->SCN_avail; i++)  	
	{
		/* Process uninitialized cases */
		if (rn->scnRing->prevStarttime <= 0 ||
			rn->scnRing->prevEndtime <= 0)
		{
			rn->scnRing->lastRequestTime = -1;
		} 
		else /* Normal cases */
		{
			rn->scnRing->lastRequestTime =
			  		rn->scnRing->prevEndtime;
			meanSystemTime += rn->scnRing->lastRequestTime;
			j++;
		}
		rn->scnRing = rn->scnRing->next;
	}
	
	meanSystemTime /= j;

	/* distribute priorities */
	for (i = 0; i< rn->SCN_avail; i++)  	
	{	
		/* See how far away we are from tank start */
		timeFromTank = rn->scnRing->lastRequestTime -
			rn->scnRing->tankStarttime;
		if (timeFromTank < 900)	/* 15 mins */
			TankPriority = rn->PriorityHighWater;
		else if (timeFromTank < 1800)
			TankPriority = rn->PriorityHighWater/2;
		else 
			TankPriority = 1;

		previous_priority = rn->scnRing->priority;

		priority = (meanSystemTime - rn->scnRing->lastRequestTime) /
				priorityFactor;
		if (TankPriority > priority)
			priority = TankPriority;
 
		if (priority < 0)
			rn->scnRing->priority  = 1;
		else if (priority > rn->PriorityHighWater)
			rn->scnRing->priority = rn->PriorityHighWater;
		else
			rn->scnRing->priority = priority;

		rn->scnRing->timeInterval = rn->scnRing->timeInterval / previous_priority * rn->scnRing->priority;	

		if (rn->verbosity > 3)
		{
			logit("pt", "		SCN-%d   = %7s %7s %7s %7s : priority (%1d) %10.1f secs later than mean\n",
				i, rn->scnRing->traceRec.sta, 
				rn->scnRing->traceRec.chan, 
				rn->scnRing->traceRec.net,
			   	rn->scnRing->locId, 
				rn->scnRing->priority,
				meanSystemTime - rn->scnRing->lastRequestTime);

			logit("pt", "		SCN-%d   = %7s %7s %7s %7s:  time interval is set to %f s\n",
				i, rn->scnRing->traceRec.sta, 
				rn->scnRing->traceRec.chan, 
				rn->scnRing->traceRec.net,
			   	rn->scnRing->locId, 
				rn->scnRing->timeInterval);

		}	
		rn->scnRing = rn->scnRing->next;

	
	}

	/* Set back the start of the ring */
	rn->scnRing = scn_head;

	return;
}
