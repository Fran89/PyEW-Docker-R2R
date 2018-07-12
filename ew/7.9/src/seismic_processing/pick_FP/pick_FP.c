/*
 * pick_FP.c
 * Modified from pick_ew.c, Revision 1.14 in src/seismic_processing/pick_ew
 *
 * This file is part of pick_FP, Earthworm module for the FilterPicker phase detector and picker.
 *
 * (C) 2008-2012 Claudio Satriano <satriano@ipgp.fr>,
 * under the same license terms of the Earthworm software system. 
 *
 * versioning added in 0.0.2 release in EW SVN
 * 0.0.3 Feb 13, 2012 - made sample rate array static in call_FilterPicker
 * 0.0.4 2014-05-27 - fixed flush of ring at start up
 */

#define PICK_FP_VERSION "0.0.5 - 2016-08-04"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>
#include <swap.h>
#include <trheadconv.h>
#include "pick_FP.h"

/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM * );
void LogConfig( GPARM * );
int  GetStaList( STATION **, int *, GPARM * );
void LogStaList( STATION *, int );
int  CompareSCNL( const void *, const void * );
int  Restart( STATION *, GPARM *, int, int );
void Interpolate( STATION *, char *, int );
int  GetEwh( EWH * );
//void Sample( int, STATION * );

void call_FilterPicker (STATION *Sta, char *TraceBuf, GPARM *Gparm, EWH *Ewh);

   
      /***********************************************************
       *              The main program starts here.              *
       *                                                         *
       *  Arguments:                                             *
       *     argv[1] = Name of picker configuration file         *
       *     argv[2] = (Optional) Tracebuffer file for offline   *
       *                          mode                           *
       ***********************************************************/

int offline_mode;  /* Global flag to determine whether we work in offline mode or not*/
int main (int argc, char **argv)
{
	int           i;                /* Loop counter */
	STATION       *StaArray = NULL; /* Station array */
	char          *TraceBuf;        /* Pointer to waveform buffer */
	TRACE_HEADER  *TraceHead;       /* Pointer to trace header w/o loc code */
	TRACE2_HEADER *Trace2Head;      /* Pointer to header with loc code */
	int           *TraceLong;       /* Long pointer to waveform data */
	short         *TraceShort;      /* Short pointer to waveform data */
	long          MsgLen;           /* Size of retrieved message */
	MSG_LOGO      logo;             /* Logo of retrieved msg */
	MSG_LOGO      hrtlogo;          /* Logo of outgoing heartbeats */
	int           Nsta = 0;         /* Number of stations in list */
	time_t        then;             /* Previous heartbeat time */
	long          InBufl;           /* Maximum message size in bytes */
	GPARM         Gparm;            /* Configuration file parameters */
	EWH           Ewh;              /* Parameters from earthworm.h */
	char          *configfile;      /* Pointer to name of config file */
	pid_t         myPid;            /* Process id of this process */

	char    	type[3];
	STATION 	key;              /* Key for binary search */
	STATION 	*Sta;             /* Pointer to the station being processed */
	int		rc;               /* Return code from tport_copyfrom() */
	time_t  	now;              /* Current time */
	double  	GapSizeD;         /* Number of missing samples (double) */
	int  		GapSize;          /* Number of missing samples (integer) */
	unsigned char 	seq;        /* msg sequence number from tport_copyfrom() */

	/* file interface - c. satriano */
	char	*tracename;
	FILE 	*tracefile;
	int 	loop_condition,count;


	/* Check command line arguments */
	if (argc < 2) {
		fprintf( stderr, "Usage: pick_FP <configfile> [tracebuf_file]\n" );
		fprintf( stderr, "       This module can be used as a standalone program (offline mode)\n" );
		fprintf( stderr, "       by specifing the path to a tracebuf file as second argument.\n" );
 		fprintf( stderr, " Version: %s\n", PICK_FP_VERSION);
		return -1;
	}
	configfile = argv[1];

	offline_mode = 0;
	if (argc == 3) {
   		offline_mode = 1;
	   	tracename = argv[2];

		if ( (tracefile = fopen (tracename, "r")) == NULL ) {
			perror (tracename);
			exit (-1);
   		}
	}



	/* Initialize name of log-file & open it */
	logit_init (configfile, 0, 256, 1);

	/* Get parameters from the configuration files */
	if ( GetConfig (configfile, &Gparm) == -1 ) {
		logit( "e", "pick_FP: GetConfig() failed. Exiting.\n" );
		return -1;
	}

	/* Look up info in the earthworm.h tables */
	if ( GetEwh( &Ewh ) < 0 ) {
		logit( "e", "pick_FP: GetEwh() failed. Exiting.\n" );
		return -1;
	}

	/* Specify logos of incoming waveforms and outgoing heartbeats */
	if( Gparm.nGetLogo == 0 ) {
		Gparm.nGetLogo = 2;
		Gparm.GetLogo  = (MSG_LOGO *) calloc( Gparm.nGetLogo, sizeof(MSG_LOGO) );
		if( Gparm.GetLogo == NULL ) {
			logit( "e", "pick_FP: Error allocating space for GetLogo. Exiting\n" );
			return -1;
		}
		Gparm.GetLogo[0].instid = Ewh.InstIdWild;
		Gparm.GetLogo[0].mod    = Ewh.ModIdWild;
		Gparm.GetLogo[0].type   = Ewh.TypeTracebuf2;

		Gparm.GetLogo[1].instid = Ewh.InstIdWild;
		Gparm.GetLogo[1].mod    = Ewh.ModIdWild;
		Gparm.GetLogo[1].type   = Ewh.TypeTracebuf;
	}

	hrtlogo.instid = Ewh.MyInstId;
	hrtlogo.mod    = Gparm.MyModId;
	hrtlogo.type   = Ewh.TypeHeartBeat;

	/* Get our own pid for restart purposes */
	myPid = getpid();
	if ( myPid == -1 ) {
		logit( "e", "pick_FP: Can't get my pid. Exiting.\n" );
		free( Gparm.GetLogo );
		free( Gparm.StaFile );
		return -1;
	}

	/* Log the configuration parameters */
	LogConfig (&Gparm);

	/* Allocate the waveform buffer */
	InBufl = MAX_TRACEBUF_SIZ*2 + sizeof(int)*(Gparm.MaxGap-1);
	TraceBuf = (char *) malloc( (size_t) InBufl );
	if ( TraceBuf == NULL ) {
		logit( "et", "pick_FP: Cannot allocate waveform buffer\n" );
		free( Gparm.GetLogo );
		free( Gparm.StaFile );
		return -1;
	}

	/* Point to header and data portions of waveform message */
	TraceHead  = (TRACE_HEADER *)TraceBuf;
	Trace2Head = (TRACE2_HEADER *)TraceBuf;
	TraceLong  = (int *) (TraceBuf + sizeof(TRACE_HEADER));
	TraceShort = (short *) (TraceBuf + sizeof(TRACE_HEADER));

	/* Read the station list and return the number of stations found.
		Allocate the station list array. */
	if ( GetStaList( &StaArray, &Nsta, &Gparm ) == -1 ) {
		logit( "e", "pick_FP: GetStaList() failed. Exiting.\n" );
		free( Gparm.GetLogo );
		free( Gparm.StaFile );
		free( StaArray );
		return -1;
	}

	if ( Nsta == 0 ) {
		logit( "et", "pick_FP: Empty station list(s). Exiting." );
		free( Gparm.GetLogo );
		free( Gparm.StaFile );
		free( StaArray );
		return -1;
	}

	/* Sort the station list by SCNL */
	qsort (StaArray, Nsta, sizeof(STATION), CompareSCNL);

	/* Log the station list */
	LogStaList( StaArray, Nsta );

	if (!offline_mode) { /* if not in offline mode */
		/* Attach to existing transport rings */
	   	if ( Gparm.OutKey != Gparm.InKey ) {
			tport_attach( &Gparm.InRegion,  Gparm.InKey );
			tport_attach( &Gparm.OutRegion, Gparm.OutKey );
		} else {
			tport_attach( &Gparm.InRegion, Gparm.InKey );
			Gparm.OutRegion = Gparm.InRegion;
		}

		/* Flush the input ring */
   		while ( tport_copyfrom( &Gparm.InRegion, Gparm.GetLogo, (short)Gparm.nGetLogo, 
                         &logo, &MsgLen, TraceBuf, MAX_TRACEBUF_SIZ, &seq) != 
                         GET_NONE );

		/* Get the time when we start reading messages.
		   This is for issuing heartbeats. */
   		time( &then );
   	}

	count=0;
	loop_condition=1; /* The loop condition depends on whether we are in offline_mode or not*/
	/* Loop to read waveform messages and invoke the picker */
	while (loop_condition) {

		if (offline_mode) {
			// TODO: check if the file is a valid trace buffer
			loop_condition = (fread (Trace2Head, sizeof(TRACE2_HEADER), 1, tracefile) > 0) &&
					 (fread (TraceLong, TraceHead->nsamp*sizeof(int32_t), 1, tracefile) > 0);
		} else {
			loop_condition = tport_getflag( &Gparm.InRegion ) != TERMINATE &&
					 tport_getflag( &Gparm.InRegion ) != myPid;
		}

		if (!offline_mode) {
			/* Get tracebuf or tracebuf2 message from ring */
			rc = tport_copyfrom( &Gparm.InRegion, Gparm.GetLogo, (short)Gparm.nGetLogo, 
					     &logo, &MsgLen, TraceBuf, MAX_TRACEBUF_SIZ, &seq );

			if ( rc == GET_NONE ) {
				sleep_ew( 100 );
				continue;
			}

			if ( rc == GET_NOTRACK )
				logit( "et", "pick_FP: Tracking error (NTRACK_GET exceeded)\n");

			if ( rc == GET_MISS_LAPPED )
				logit( "et", "pick_FP: Missed msgs (lapped on ring) "
					"before i:%d m:%d t:%d seq:%d\n",
					(int)logo.instid, (int)logo.mod, (int)logo.type, (int)seq );

			if ( rc == GET_MISS_SEQGAP )
				logit( "et", "pick_FP: Gap in sequence# before i:%d m:%d t:%d seq:%d\n",
					(int)logo.instid, (int)logo.mod, (int)logo.type, (int)seq );

			if ( rc == GET_TOOBIG ) {
				logit( "et", "pick_FP: Retrieved msg is too big: i:%d m:%d t:%d len:%d\n",
					(int)logo.instid, (int)logo.mod, (int)logo.type, MsgLen );
				continue;
			}
		}

		/* If necessary, swap bytes in tracebuf message */
		if ( logo.type == Ewh.TypeTracebuf ) {
			if ( WaveMsgMakeLocal (TraceHead) < 0 ) {
				logit( "et", "pick_FP: WaveMsgMakeLocal() error.\n" );
				continue;
			}
		} else {
			if ( WaveMsg2MakeLocal (Trace2Head) < 0 ) {
				logit( "et", "pick_FP: WaveMsg2MakeLocal error.\n" );
				continue;
			}
		}

		/* Convert TYPE_TRACEBUF messages to TYPE_TRACEBUF2 */
		if ( logo.type == Ewh.TypeTracebuf )
			Trace2Head = TrHeadConv (TraceHead);

		/* Look up SCNL number in the station list */
      		{
			int j;
			for ( j = 0; j < 5; j++ ) key.sta[j]  = Trace2Head->sta[j];
			key.sta[5] = '\0';
			for ( j = 0; j < 3; j++ ) key.chan[j] = Trace2Head->chan[j];
			key.chan[3] = '\0';
			for ( j = 0; j < 2; j++ ) key.net[j]  = Trace2Head->net[j];
			key.net[2] = '\0';
			for ( j = 0; j < 2; j++ ) key.loc[j]  = Trace2Head->loc[j];
			key.loc[2] = '\0';
		}

		Sta = (STATION *) bsearch (&key, StaArray, Nsta, sizeof(STATION),
						CompareSCNL);

		if ( Sta == NULL )      /* SCNL not found */
			continue;

		/* Do this the first time we get a message with this SCNL */
		if ( Sta->first == 1 ) {
			Sta->endtime = Trace2Head->endtime;
			Sta->first = 0;
			continue;
		}

		/* If the samples are shorts, make them longs */
		strcpy (type, Trace2Head->datatype);

		if ( (strcmp(type,"i2")==0) || (strcmp(type,"s2")==0) ) {
			for ( i = Trace2Head->nsamp - 1; i > -1; i-- )
				TraceLong[i] = (int)TraceShort[i];
		}

		/* 
		 * Compute the number of samples since the end of the previous message.
		 * If (GapSize == 1), no data has been lost between messages.
		 * If (1 < GapSize <= Gparm.MaxGap), data will be interpolated.
		 * If (GapSize > Gparm.MaxGap), the picker will go into restart mode.
		*/
      		GapSizeD = Trace2Head->samprate * (Trace2Head->starttime - Sta->endtime);

		if ( GapSizeD < 0. )          /* Invalid. Time going backwards. */
			GapSize = 0;
		else
			GapSize  = (int) (GapSizeD + 0.5);

		/* Interpolate missing samples and prepend them to the current message */
		if ( (GapSize > 1) && (GapSize <= Gparm.MaxGap) )
			Interpolate (Sta, TraceBuf, GapSize);

		/* Announce large sample gaps */
		if ( GapSize > Gparm.MaxGap ) {
			int      lineLen;
			time_t   errTime;
			char     errmsg[80];
			MSG_LOGO logo;

			time( &errTime );
			sprintf( errmsg,
				"%ld %d Found %4d sample gap. Restarting channel %s.%s.%s.%s\n",
				(long) errTime, PK_RESTART, GapSize, Sta->sta, Sta->chan, Sta->net, Sta->loc );
			lineLen = strlen( errmsg );
			logo.type   = Ewh.TypeError;
			logo.mod    = Gparm.MyModId;
			logo.instid = Ewh.MyInstId;
			tport_putmsg (&Gparm.OutRegion, &logo, lineLen, errmsg);
		}

		/* 
		 * For big gaps, enter restart mode. In restart mode, calculate
		 * STAs and LTAs without picking.  Start picking again after a 
		 * specified number of samples has been processed.
		 */
		if ( Restart (Sta, &Gparm, Trace2Head->nsamp, GapSize) ) {
			// TODO: shall we implement this ??? - c.satriano
			for ( i = 0; i < Trace2Head->nsamp; i++ )
				;
				//Sample( TraceLong[i], Sta );
		} else {
			/*
			 * Function to call Filter Picker
			 * picker parameters are passed through the structure Sta.Parm
			 * Gparm : configuration file parameters
			 * Ewh : earthworm.h parameters (INST_ID, etc.)
			 */
			call_FilterPicker (Sta, TraceBuf, &Gparm, &Ewh);
		}

		/* Save time and amplitude of the end of the current message */
		Sta->enddata = TraceLong[Trace2Head->nsamp - 1];
		Sta->endtime = Trace2Head->endtime;

		if (!offline_mode) {
			/* Send a heartbeat to the transport ring */
			time (&now);
			if ( (now - then) >= Gparm.HeartbeatInt ) {
				int  lineLen;
				char line[40];

				then = now;

				sprintf (line, "%ld %d\n", (long) now, (int) myPid);
				lineLen = strlen (line);

				if ( tport_putmsg (&Gparm.OutRegion, &hrtlogo, lineLen, line) !=
					PUT_OK ) {
					logit ("et", "pick_FP: Error sending heartbeat. Exiting.");
					break;
				}
			}
		}
	} /* End of loop on waveform messages */

	if (!offline_mode) { /* if not in offline_mode */
		/* Detach from the ring buffers */
		if ( Gparm.OutKey != Gparm.InKey ) {
			tport_detach( &Gparm.InRegion );
			tport_detach( &Gparm.OutRegion );
		} else {
			tport_detach( &Gparm.InRegion );
		}

		logit( "t", "Termination requested. Exiting.\n" );
	}

	free (Gparm.GetLogo);
	free (Gparm.StaFile);
	free (StaArray);
	free (TraceBuf);

	return 0;
}


      /*******************************************************
       *                      GetEwh()                       *
       *                                                     *
       *      Get parameters from the earthworm.h file.      *
       *******************************************************/

int GetEwh( EWH *Ewh )
{
   if ( GetLocalInst( &Ewh->MyInstId ) != 0 )
   {
      logit( "e", "pick_FP: Error getting MyInstId.\n" );
      return -1;
   }

   if ( GetInst( "INST_WILDCARD", &Ewh->InstIdWild ) != 0 )
   {
      logit( "e", "pick_FP: Error getting InstIdWild.\n" );
      return -2;
   }
   if ( GetModId( "MOD_WILDCARD", &Ewh->ModIdWild ) != 0 )
   {
      logit( "e", "pick_FP: Error getting ModIdWild.\n" );
      return -3;
   }
   if ( GetType( "TYPE_HEARTBEAT", &Ewh->TypeHeartBeat ) != 0 )
   {
      logit( "e", "pick_FP: Error getting TypeHeartbeat.\n" );
      return -4;
   }
   if ( GetType( "TYPE_ERROR", &Ewh->TypeError ) != 0 )
   {
      logit( "e", "pick_FP: Error getting TypeError.\n" );
      return -5;
   }
   if ( GetType( "TYPE_PICK_SCNL", &Ewh->TypePickScnl ) != 0 )
   {
      logit( "e", "pick_FP: Error getting TypePickScnl.\n" );
      return -6;
   }
   if ( GetType( "TYPE_CODA_SCNL", &Ewh->TypeCodaScnl ) != 0 )
   {
      logit( "e", "pick_FP: Error getting TypeCodaScnl.\n" );
      return -7;
   }
   if ( GetType( "TYPE_TRACEBUF", &Ewh->TypeTracebuf ) != 0 )
   {
      logit( "e", "pick_FP: Error getting TYPE_TRACEBUF.\n" );
      return -8;
   }
   if ( GetType( "TYPE_TRACEBUF2", &Ewh->TypeTracebuf2 ) != 0 )
   {
      logit( "e", "pick_FP: Error getting TYPE_TRACEBUF2.\n" );
      return -9;
   }
   return 0;
}

