/* smb2tb.c - console based conversion utility to convert
 Seismic Monitor Buffer data to tank data( series of TRACE2_HEADER)
 records. 

 052507 - ronb created this module.
*/


/* skipped a couple of versions. Note that version 0.0.4 added the -m mapfile switch and processing*/
/* version 0.0.5 fixed sq wave bug by changing datatype to 'i4' */
#define SMB2TB_VERSION "v0.0.6 2007-06-22"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <trace_buf.h>
#include <chron3.h>
#include <kom.h>
#include "hashtable.h"

#pragma pack(1)	/* fix alignment for now. */

/* macros and constant, some pilfered from playback source. */
#define DEC_MULTIPLIER		1	
#define SAMPLE_SIZE			2 /*sizeof(int)*/
#define SAMPLES_PER_REC		256
#define Q_DATA_SIZE	(SAMPLES_PER_REC * DEC_MULTIPLIER * SAMPLE_SIZE)	/* size of data portion of smb window(record) */
#define MAX_FILE_ARGS		256
#define MAX_PATHSIZE		256
#define MAX_TIME_STR		64
#define	MAX_S_Q				4096	/* max number of records per station */
#define	MAX_SP_TIM			360		/* max short period CLOK records */
#define	MAX_LP_TIM			360		/* max long period CLOK records */
#define EWSECS_PER_SPREC	2.56
#define EWSECS_PER_LPREC	25.6
#define EWSECS_PER_TBUF_SPREC	2.55
#define EWSECS_PER_TBUF_LPREC	25.5
#define MIN_GAP_WARNING		2.56
#define TANKFILE_SUFFIX		".tnk"
#define SMB_STA_LEN			3		/* smb station names are first 3 chars of channel*/
#define SMB_CHAN_ARG_LEN	2		/*  only two chars from command line for channel names*/
#define MAX_CFG_LINE		256		/* max length of config file line. */
/* error reporting levels */
#define ERR_MSG		0	
#define WARNING_MSG	1
#define DEBUG_MSG	2

/* define the channel and clock version nibbles for the buffer file	*/
#define BUFFER_VERSION 2

#if BUFFER_VERSION == 1
#define	CHANNEL_NIBBLE				0x00
#define	CLOCK_10HZ					0xfe
#define	CLOCK_100HZ					0xff
#define	CLOCK_SIZE					8
#define	CLOCK_OFFSET				1
#define	CLOCK_IDX					7
#endif

#if BUFFER_VERSION == 2
#define	CHANNEL_NIBBLE				0x40
#define	CLOCK_10HZ					0xee
#define	CLOCK_100HZ					0xef
#define	CLOCK_SIZE					10
#define	CLOCK_OFFSET				0
#define	CLOCK_IDX					8
#endif

#define IS_100HZ_CLOCK(q) ((q)->id == CLOCK_100HZ)
#define IS_10HZ_CLOCK(q) ((q)->id == CLOCK_10HZ)
#define IS_TIMING_REC(q) ((IS_100HZ_CLOCK(q) || IS_10HZ_CLOCK(q)) && !strcmp((q)->name, "CLOK"))
#define IS_VALID_DATA_CHANNEL(q)	(((q)->id & ~CHANNEL_NIBBLE) < 0x14)
#define IS_100HZ_DATA(q) ((q)->id < 0x10)
#define IS_10HZ_DATA(q) (!IS_100HZ_DATA(q))

#define SMB_NAME_SIZE		5	/* size of name field in SMB_HDR */


/* channel data record */
typedef struct smb_rec {
	unsigned char	id;
	char			name[SMB_NAME_SIZE];
	unsigned long	window_no;
	unsigned char	data[Q_DATA_SIZE];
} SMB_REC;
#define SMB_REC_SIZE	522	/* hardcode size (avoid byte alignment issue) */

typedef struct smb_timestamp {
	unsigned long	window_no;		/* sequence number */
	double			tm;				/* time stamp in epoch seconds*/
} SMB_TIMESTAMP;

typedef struct smb_elem {
	unsigned char	 id;			/* channel id / slew? */
	unsigned long	 window_no;		/*  sequence number*/
	double			 starttime;		/*  start time */
	long			 filepos;		/*  file pos */
} SMB_ELEM;

typedef struct smb_station {
	struct smb_station* pPrev;					/* previous station in list */
	struct smb_station* pNext;					/* next station in list */
	unsigned char	 id;						/* channel id*/
	char			 station[SMB_NAME_SIZE];	/* station name */
	double			 NominalSampleRate;			/* nominal sample rate */
	int			     NumRecs;					/* number of elements in DataSet*/
	SMB_ELEM*		 DataSet;					/* array of SMB_ELEM*/
}SMB_STATION;

typedef struct smb_sncl_map {
	char	smb_name[SMB_NAME_SIZE];
	char	sta[TRACE2_STA_LEN];
	char	chan[TRACE2_CHAN_LEN];
	char	net[TRACE2_NET_LEN];
	char	loc[TRACE2_LOC_LEN];
} SMB_SNCL_MAP;

static int nVerbose = 0;
static int bQuiet = 0;
static char FileArgs[MAX_FILE_ARGS][MAX_PATHSIZE];
static int FileArgCount = 0;
static char DfltStationName[TRACE2_STA_LEN];
static int  DfltPinno = 0;
static char DfltNetwork[TRACE_NET_LEN] = "XX";
static char DfltLocation[TRACE_NET_LEN] = "--";
static char TankFileName[MAX_PATHSIZE];
static SMB_STATION* pFirstStation = NULL;
static SMB_STATION* pLastStation = NULL;
static char ShortPeriodName[SMB_CHAN_ARG_LEN] = "EH";
static char LongPeriodName[SMB_CHAN_ARG_LEN] = "SH";
static int clock_channel = -1;
static int OutputMultipleTanks = 1;
static SMB_TIMESTAMP SPTimeStamps[MAX_SP_TIM];
static SMB_TIMESTAMP LPTimeStamps[MAX_LP_TIM];
static int SPTimeStampCount = 0;
static int LPTimeStampCount = 0;
static char SNCLMapFile[MAX_PATHSIZE] = "smb2tb.d";
static struct hashtable* SNCLMap = NULL;

/* prototyping */
void usage(void);
char* CLOK2Str(char* tm);
double CLOK2ew(char* tm);
SMB_STATION* FindStation(SMB_REC* smbw);
SMB_STATION* CreateStation(SMB_REC* smbw);
int DestroyStation(SMB_STATION* pStation);
int AppendData(SMB_STATION* pStation, SMB_REC* smbw, long filepos);
int CompareTimeStamp(const void* a, const void* b);
int CompareStationRecords(const void* a, const void* b);
int FixTimeStamp(SMB_ELEM* pElem);
void msg(int lvl, char* fmt, ...);
int ReadSNCLMapFile(char* smFile);
unsigned int HashFromKey(void *ky);
int HashKeyCompare(void *k1, void *k2);

int main(int argc, char** argv)
{
	FILE* fp;
	FILE* fpOut;
	SMB_REC smbw;
	int bailOut = 0;
	int i,j;
	int fi;
	int NumCLOK = 0;
	int shortPeriodCount = 0;
	int longPeriodCount = 0;
	int id;
	SMB_STATION* pStation;
	SMB_STATION* pTmp;
	long fpos;
	TRACE2_HEADER trh;
	double ewsec_per_rec;
	double lastEndTime;
	SMB_SNCL_MAP *snclMapping;
	unsigned long i4sample;
	TankFileName[0] = '\0';

	/*Parse command line args */
	for(i=1; i<argc; i++) {
		/*check switches */
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {

				case 'c':
					strncpy(ShortPeriodName, argv[++i], SMB_CHAN_ARG_LEN); /* default CHAN name for short period clok*/
					break;
				case 'C':
					strncpy(LongPeriodName, argv[++i], SMB_CHAN_ARG_LEN); /* default CHAN name for long period clok*/
					break;
				case 'l':
					strncpy(DfltLocation, argv[++i], TRACE2_LOC_LEN); /* default LOC setting*/
					break;
				case 'm':
					strncpy(SNCLMapFile, argv[++i], MAX_PATHSIZE); /* default LOC setting*/
					break;
				case 'n':
					strncpy(DfltNetwork, argv[++i], TRACE2_NET_LEN); /* default NET name*/
					break;
				case 'p':
					DfltPinno = atoi(argv[++i]);	/* default PINNO */
					break;
				case 't':
					strncpy(TankFileName, argv[++i], MAX_PATHSIZE);
					OutputMultipleTanks = 0;	/* clear multitank flag*/
					break;
				case 'v':
					nVerbose = atoi(argv[++i]);
					if(nVerbose < 0 || nVerbose > 2) {
						usage();
						return(-1);
					}
					break;
				case 'q':
					bQuiet = 1;
					break;
				default:
					msg(ERR_MSG,"don't understand request.\n");
					usage();
					return(-1);
			}
		} else {
			/* otherwise assume filename args*/
			strncpy(FileArgs[FileArgCount++], argv[i], MAX_PATHSIZE);
		}

	}

	if(FileArgCount == 0) {
		msg(ERR_MSG, "no files to process.\n");
		usage();
		return(-1);
	}

	/* create hashtable */
	if(!(SNCLMap = create_hashtable(16, HashFromKey, HashKeyCompare))) {
		msg(ERR_MSG,"Could not create hashtable.\n");
	}

	/* read map file in*/
	if(!ReadSNCLMapFile(SNCLMapFile)) {
		msg(ERR_MSG, "could not read map file\n");
	}

	/* loop through file arguments */
	for(fi=0; fi<FileArgCount; fi++) {
		if(fp = fopen(FileArgs[fi], "rb")) {

#ifdef CHECK_ALIGN
			printf("sizeof SMB_WINDOW.ChannelId %ld\n", sizeof(smbw.id));
			printf("sizeof SMB_WINDOW.ChannelName %ld\n", sizeof(smbw.name));
			printf("sizeof SMB_WINDOW.WindowNum %ld\n", sizeof(smbw.window_no));
			printf("sizeof SMB_WINDOW.Data %ld\n", sizeof(smbw.data));
			printf("sizeof SMB_WINDOW %ld\n", sizeof(SMB_REC));
			printf("sizeof(int) %d\n", sizeof(int));
			printf("Q_DATA_SIZE %d\n", Q_DATA_SIZE);
#endif

			msg(DEBUG_MSG, "Reading Raw SMB data.\n");
			while((fread(&smbw, SMB_REC_SIZE, 1, fp) == 1) && !bailOut) {

				fpos = ftell(fp) - SMB_REC_SIZE;
				id = (int)smbw.id & ~CHANNEL_NIBBLE;

				if(nVerbose >= DEBUG_MSG) {
					msg(DEBUG_MSG, "Channel id: %d\n", smbw.id);
					msg(DEBUG_MSG, "Channel Nibble: %d\n", id);
					msg(DEBUG_MSG, "Slew offset = %d\n", (id < 16) ? ((id - clock_channel + 15) / 16): 0); /* slew correction */
					msg(DEBUG_MSG, "Channel Name: %5.5s\n", smbw.name);
					msg(DEBUG_MSG, "Window Number: %ld\n", smbw.window_no);
				}

				if(IS_TIMING_REC(&smbw)) {
					char* s = CLOK2Str((char*)smbw.data);
					++NumCLOK;
					if (!(smbw.data[CLOCK_IDX] & 0x80)) {
					     clock_channel = smbw.data[CLOCK_IDX];
					}

					if(IS_100HZ_CLOCK(&smbw)) {
						msg(DEBUG_MSG, "Short period 100HZ CLOC record[%s].\n:", s);
						SPTimeStamps[SPTimeStampCount].window_no = smbw.window_no;
						SPTimeStamps[SPTimeStampCount].tm = CLOK2ew((char*) smbw.data);
						SPTimeStampCount++;
					} else if(IS_10HZ_CLOCK(&smbw)) {
						msg(DEBUG_MSG,"Long period 10HZ CLOC record[%s].\n:", s);
						LPTimeStamps[SPTimeStampCount].window_no = smbw.window_no;
						LPTimeStamps[SPTimeStampCount].tm = CLOK2ew((char*) smbw.data);
						LPTimeStampCount++;
					} else {
						msg(ERR_MSG,"Cannot determine CLOK type.\n");
					}
					msg(DEBUG_MSG,"CLOK Epoch Seconds: %lf\n", CLOK2ew((char*) smbw.data));
					if(s) {
					 free(s);
					}
				} else if(IS_VALID_DATA_CHANNEL(&smbw)) {

					if((nVerbose >= DEBUG_MSG) && !bQuiet) {
						msg(DEBUG_MSG,"Valid Data Channel");
						msg(DEBUG_MSG,"[");
						for(i=0; i < 256; i++) {
							msg(DEBUG_MSG,"%c%d",((i)?',':'\0') , (unsigned short)((unsigned short*)smbw.data)[i]);
						}
						msg(DEBUG_MSG,"]\n"); 
					}

					/* if no matching station found then create one*/
					if(!(pStation = FindStation(&smbw))) {
						pStation = CreateStation(&smbw);
					}
					
					if(!pStation) {
						fclose(fp);
						msg(ERR_MSG,"Error: Cannot create SMB_STATION list.\n");
						exit(0);
					}
					
					/* append data to station*/
					if(!AppendData(pStation, &smbw, fpos)) {
						msg(ERR_MSG, "Error: Cannot add data record to station.\n");
					}

				}

				if(ferror(fp)) {
					bailOut = 1;
				}
			}

			msg(DEBUG_MSG, "Number of LP CLOK Records: %d\n", LPTimeStampCount);
			msg(DEBUG_MSG, "Number of SP CLOK Records: %d\n", SPTimeStampCount);

			/* check CLOK records min 2 of either required */
			if(SPTimeStampCount < 2 && LPTimeStampCount < 2) {
				msg(ERR_MSG, "Error insufficient CLOK records to \n");
				break;
			}

			/* process ShortPeriod CLOK records */
			if(SPTimeStampCount >= 2) {
				qsort(SPTimeStamps, SPTimeStampCount, sizeof(SMB_TIMESTAMP), CompareTimeStamp);
				if(nVerbose >= DEBUG_MSG) {
					msg(DEBUG_MSG, "SP CLOK records*****************\n");
					for(i=0; i < SPTimeStampCount; i++) {
						msg(DEBUG_MSG, "SP_CLOK window_no[%ld] timestamp[%lf]\n", SPTimeStamps[i].window_no, SPTimeStamps[i].tm);
					}
				}
			}
			/* process Long Period CLOK records */
			if(LPTimeStampCount >= 2) {
				qsort(SPTimeStamps, SPTimeStampCount, sizeof(SMB_TIMESTAMP), CompareTimeStamp);
				if(nVerbose >= DEBUG_MSG) {
					msg(DEBUG_MSG, "SP CLOK records*****************\n");
					for(i=0; i < LPTimeStampCount; i++) {
						msg(DEBUG_MSG, "LP_CLOK window_no[%ld] timestamp[%lf]\n", LPTimeStamps[i].window_no, LPTimeStamps[i].tm);
					}
				}
			}
	
			/* process stations: fix timestamps */
			
			pStation = pFirstStation;
			while(pStation) {
				qsort(pStation->DataSet, pStation->NumRecs, sizeof(SMB_ELEM), CompareStationRecords);
				msg(DEBUG_MSG, "****\n");
				msg(DEBUG_MSG, "Processing Station [%.5s] id[%d] NomSR[%lf] NumRecs[%ld]\n", pStation->station, pStation->id, pStation->NominalSampleRate, pStation->NumRecs);
				//epochsec17(&tm,"20041230170700.00");
				for(i=0; i < pStation->NumRecs; i++) {
					/* fix time stamps */					
					// SetTimeStamp(&(pStation->DataSet[i]));
					// pStation->DataSet[i].starttime = 
					FixTimeStamp(&(pStation->DataSet[i]));
					msg(DEBUG_MSG, "Record [%d] station[%.5s] id[%d] window_no[%d] Start[%lf] filepos[%ld]\n", i, pStation->station, pStation->DataSet[i].id, pStation->DataSet[i].window_no, pStation->DataSet[i].starttime, pStation->DataSet[i].filepos);

					//tm += 2.56;
				}
				pStation = pStation->pNext;
			}

			/* if tank file name not specified then set
			TankFileName from SMB file name and add suffix*/
			if(OutputMultipleTanks) {
				strncpy(TankFileName, FileArgs[fi], MAX_PATHSIZE);
				strncat(TankFileName, TANKFILE_SUFFIX, MAX_PATHSIZE);
			}

			/* write tracebuf*/
			if(fpOut = fopen(TankFileName , OutputMultipleTanks ? "wb" : "ab" )) {
				msg(DEBUG_MSG, "Writing Tank file [%s]\n", TankFileName);
				pStation = pFirstStation;
				while(pStation) {
					memset(&trh, 0, sizeof(TRACE2_HEADER));
//					trh.pinno = DfltPinno;									/* Pin number */
					trh.samprate = pStation->NominalSampleRate;			    /* Sample rate; nominal */

					if(snclMapping = hashtable_search(SNCLMap, pStation->station)) {

						strncpy(trh.sta, snclMapping->sta, TRACE2_STA_LEN);
						strncpy(trh.chan, snclMapping->chan, TRACE2_CHAN_LEN);
						strncpy(trh.net, snclMapping->net, TRACE2_NET_LEN);
						strncpy(trh.loc, snclMapping->loc, TRACE2_LOC_LEN);

					} else {
						strncpy(trh.sta, pStation->station, SMB_STA_LEN);	/* Site name (NULL-terminated) */

						strncpy(trh.chan, (IS_100HZ_DATA(pStation) ? ShortPeriodName: LongPeriodName ), SMB_CHAN_ARG_LEN);	/* Component/channel code (NULL-terminated)*/
						trh.chan[2] = pStation->station[3] == 'V' ? 'Z' : pStation->station[3];

						strncpy(trh.net, DfltNetwork, TRACE2_NET_LEN);			/* Network name (NULL-terminated) */
						strncpy(trh.loc, DfltLocation, TRACE2_LOC_LEN);			/* Location code (NULL-terminated) */
					}
					trh.version[0] = TRACE2_VERSION0;    /* version field */
					trh.version[1] = TRACE2_VERSION1;    /* version field */
					strcpy(trh.datatype,"i4");           /* Data format code (NULL-terminated) */
					/* trh.quality[2];*/            /* Data-quality field */
					/* trh.pad[2]; */               /* padding */ 
						
					ewsec_per_rec = IS_100HZ_DATA(pStation) ? EWSECS_PER_TBUF_SPREC : EWSECS_PER_TBUF_LPREC ;
					lastEndTime = pStation->DataSet[0].starttime;
					for(i=0; i<pStation->NumRecs; i++) {
						
						trh.starttime = pStation->DataSet[i].starttime;  /* time of first sample in epoch seconds (seconds since midnight 1/1/1970) */
						trh.endtime = trh.starttime + ewsec_per_rec;     /* Time of last sample in epoch seconds */
						trh.nsamp = 256;
						/*check gappage */
						if((trh.starttime - lastEndTime) > MIN_GAP_WARNING){
							msg(WARNING_MSG, "Warning time gap [%lf] exceeded %lf secs seq bracket[%d - %d]\n", 
								trh.starttime - lastEndTime, MIN_GAP_WARNING,
								pStation->DataSet[i-1].window_no, pStation->DataSet[i].window_no);
						}
					
						/*check overlap*/
						if(trh.starttime < lastEndTime){
							msg(WARNING_MSG, "Warning overlap [%lf] seconds at %lf seq bracket[%d - %d]\n", 
								trh.starttime - lastEndTime, trh.starttime, pStation->DataSet[i-1].window_no, pStation->DataSet[i].window_no);
						}

						lastEndTime = trh.endtime;
						if(pStation->id >= 16) {
							printf(" LP Channel\n");
						}
						msg(DEBUG_MSG, "Writing Tank Record: windowNo[%ld] SCNL[%.5s.%.3s.%.2s.%.2s] Pinno[%d] StartTime[%lf] EndTime[%lf] SampRate[%lf] Version[%.2s] DataType[%.2s]\n",
							pStation->DataSet[i].window_no, trh.sta, trh.chan, trh.net, trh.loc, trh.pinno, trh.starttime, trh.endtime, trh.samprate, trh.version, trh.datatype);
						if(fwrite(&trh, sizeof(TRACE2_HEADER), 1, fpOut) == 1) {					
							fseek(fp, pStation->DataSet[i].filepos, SEEK_SET);
							if(fread(&smbw, SMB_REC_SIZE, 1, fp) == 1) {

								/* promote to 32 bit*/
								for(j=0; j < SAMPLES_PER_REC; j++) {
									i4sample = (unsigned long)((unsigned short*)smbw.data)[j];
									fwrite(&i4sample, sizeof(i4sample),  1, fpOut);
								}

								/* write data */
/*								if(fwrite(&(smbw.data), Q_DATA_SIZE, 1, fpOut) != 1) {
									msg(ERR_MSG, "Error: Writing data buffer to tank.\n");
								}
*/							} else {
								msg(ERR_MSG, "Error: Reading input data record.\n");
							}
						}
					}
					pStation = pStation->pNext;
				}

				fclose(fpOut);
				msg(DEBUG_MSG,"Done writing to tankfile [%s]\n", TankFileName);
			}

			/* delete station structures*/
			pTmp = pLastStation;
			while(pTmp) {
				pStation = pTmp->pPrev;
				DestroyStation(pTmp);
				pTmp = pStation;
			}
			pFirstStation = pLastStation = NULL;

			fclose(fp);
		}
	}

	if(SNCLMap) {
		hashtable_destroy(SNCLMap, 1);
		SNCLMap = NULL;
	}

	return 0;
}

/*********************************************************
 CompareStationRecords - callback func for qsort
**********************************************************/
int CompareStationRecords(const void* a, const void* b) {
	
	SMB_ELEM* pA = (SMB_ELEM*) a;
	SMB_ELEM* pB = (SMB_ELEM*) b;
	return (pA->window_no == pB->window_no) ? 0 : ((pA->window_no < pB->window_no) ? -1 : 1 );
}

/**********************************************************
CompareTimeStamp - callback func for qsort to use
***********************************************************/
int CompareTimeStamp(const void* a, const void* b) {
	
	SMB_TIMESTAMP* pA = (SMB_TIMESTAMP*) a;
	SMB_TIMESTAMP* pB = (SMB_TIMESTAMP*) b;
	return (pA->tm == pB->tm) ? 0 : ((pA->tm < pB->tm) ? -1 : 1 );
}

/**********************************************************
 AppendData - Append SMB_REC to DataSet array of SMB_STATION
 
 input:
 pStation - ptr to SMB_STATION to append data to.
 smbw - ptr to SMB_REC to append.
 pos -  file postion that smbw references.
***********************************************************/
int AppendData(SMB_STATION* pStation, SMB_REC* smbw, long pos) {

	int rc = 0;

	if(pStation) {
		if(pStation->NumRecs < MAX_S_Q) {
			pStation->DataSet[pStation->NumRecs].id = smbw->id & ~CHANNEL_NIBBLE;
			pStation->DataSet[pStation->NumRecs].window_no = smbw->window_no;
			pStation->DataSet[pStation->NumRecs].filepos = pos;
			pStation->DataSet[pStation->NumRecs].starttime = 0.0;
			pStation->NumRecs++;
			rc = 1;
		} else {
			/* error number of records exceeded */
		}
	} else {
		/* error invalid pStation*/
	}
	return rc;
}

/**********************************************************
 CreateStation - Create a SMB_STATON struct from SMB_REC.
  and append it to station linked list defined by 
  pFirstStation and pLastStation.

 input:
 smbw - ptr to SMB_REC
***********************************************************/
SMB_STATION* CreateStation(SMB_REC* smbw) {
	SMB_STATION* pStation = NULL;

	if(pStation = (SMB_STATION*) malloc(sizeof(SMB_STATION))) {

		memset(pStation, 0, sizeof(SMB_STATION));
		pStation->id = smbw->id & ~CHANNEL_NIBBLE;
		strncpy(pStation->station, smbw->name, SMB_NAME_SIZE);
		pStation->NumRecs = 0;
		pStation->NominalSampleRate = IS_100HZ_DATA(pStation) ? 100 : 10;
		if(pStation->DataSet = (SMB_ELEM*) calloc(MAX_S_Q, sizeof(SMB_ELEM))) {
			/* append to station list*/
			if(pLastStation) {
				pLastStation->pNext = pStation;
			}
			pStation->pPrev = pLastStation;
			pStation->pNext = NULL;
			pLastStation = pStation;
			if(!pFirstStation) {
				pFirstStation = pLastStation;
			}
	
		} else {
			free(pStation);
			pStation = NULL;

		}
	}

	return pStation;
}

/**********************************************************
 DestroyStation - Free memory allocated for SMB_STATION
 and it's DataSet field.
***********************************************************/
int DestroyStation(SMB_STATION* pStation) {
	int rc = 0;
	if(pStation) {
		if(pStation->DataSet) {
			free(pStation->DataSet);
		}
		free(pStation);
		rc = 1;
	}
	return rc;
}

/**********************************************************
 FindStation - Locate Station in matching SMB_REC in list
 defined by pFirstStation and pLastStation.
***********************************************************/
SMB_STATION* FindStation(SMB_REC* smbw) {

	SMB_STATION* pStation = pFirstStation;

	while(pStation) {
		if((strncmp(pStation->station, smbw->name, SMB_NAME_SIZE) == 0) && (pStation->id == (smbw->id& ~CHANNEL_NIBBLE))) {
			return pStation;
		}
		pStation = pStation->pNext;
	}
	return pStation;
}

/**********************************************************
 usage - print user instructions to stdout
***********************************************************/
void usage() {

	msg(0, "smt2tb.exe - Converts Seismic Monitor Buffer data to TankPlayer format.\n");
	msg(0, "Version %s.\n", SMB2TB_VERSION);
	msg(0, "usage: smb2tb [-cCnlqtv] smbfile [filelist]\n");
	msg(0, "\n");
	msg(0, "  -c cc    100HZ SCNL channel prefix. Defaults to 'EHn' (n is SMB channel id).\n");
	msg(0, "  -C cc    10HZ SCNL channel prefix. Defaults to 'SHn' (n is SMB channel id).\n");
	msg(0, "  -l ll    Location name component of SCNL. Defaults to '--'.\n");
	msg(0, "  -m file  Mapping file name. Default is smb2tb.d\n");
	msg(0, "  -n nn    Network name component of SCNL. Defaults to '%s'. \n", DfltNetwork);
	msg(0, "  -p n     pinno to use. Default is 0.\n");
	msg(0, "  -q       Quiet mode, No text output.\n");
	msg(0, "  -t tfile name of tankfile to write to.\n");
	msg(0, "  -v n     Verbosity level. 0:Error msgs, 1:Warning msgs, 2:Debug msgs.\n");
	
}

/**********************************************************
CLO2Str - convert clock data to string
***********************************************************/
char* CLOK2Str(char* tm) {

	char* sCLOKStr = NULL;
	int y,m,d,h,min,sec,ssec;

	if(sCLOKStr = (char*) malloc(MAX_TIME_STR)) {
		y = 100 * *tm + *(tm +1);
		m = *(tm+2);
		d = *(tm+3);
		h = *(tm+4);
		min = *(tm+5);
		sec = *(tm+6);
		ssec = *(tm+7);

		sprintf(sCLOKStr,"%d/%d/%d %d:%d:%d.%d",y, m, d, h, min, sec, ssec);
	}
	return sCLOKStr;
}

/***********************************************************
CLOK2ew - convert CLOK record to epoc seconds.
input: tm - array of bytes forming date/time.centiseconds.
************************************************************/
double CLOK2ew(char* tm) {

	char sCLOK[64];
	double ewtm;
	int y,m,d,h,min,sec,csec;

	y = 100 * *tm + *(tm +1);
	m = *(tm+2);
	d = *(tm+3);
	h = *(tm+4);
	min = *(tm+5);
	sec = *(tm+6);
	csec = *(tm+7);

	sprintf(sCLOK,"%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d.%2.2d",y, m, d, h, min, sec, csec);
	epochsec17(&ewtm, sCLOK);	
	return ewtm;
}

/**********************************************************
FixTimeStamp - used to calculate starttime for a given
SMB_ELEM from either the SPTimeStamps or LPTimeStamps array
based on SMB_ELEM.id. 

based on the following code from INPDIR.C in original data viewer
long	relative_time(wno, id)
long	wno;
int	id;
{
	int	u;
	int	l = 0;
	int	i;
	long	csecs;
	int	offset;
	long	size;
	int	tcount;
	T_ELEM	*t_elem;

	 first find window number in list	
	if (id < 16)	{
		t_elem = sp_tim;
		u = tcount = sp_count;
		offset = (id - clock_channel + 15) / 16;  // slew correction 
		size = 256L;
	}
	else	{
		t_elem = lp_tim;
		u = tcount = lp_count;
		offset = 0;
		size = 2560L;
	}
	 binary search looks like! ronb053007 comment
	while(u >= l)	{
		i = (u + l) / 2;
		if ((t_elem + i)->window_no == wno)	{
			u = l = i;
			break;
		}
		if ((t_elem + i)->window_no < wno)
			l = i + 1;
		else
			u = i - 1;
	}

	if (u == l)
		csecs = (t_elem + i)->csecs;
	else if (u < 0)
		csecs = t_elem->csecs - size * (t_elem->window_no - wno);
	else if (l >= tcount)
		csecs = (t_elem + tcount - 1)->csecs +
				 size * (wno - (t_elem + tcount - 1)->window_no);
	else
		csecs = (t_elem + u)->csecs + (((t_elem + l)->csecs - (t_elem + u)->csecs) *
		(wno - (t_elem + u)->window_no)) /
				 ((t_elem + l)->window_no - (t_elem + u)->window_no);
	return(csecs + (long) offset);
}

***********************************************************/
int FixTimeStamp(SMB_ELEM* pElem) {
	int rc = 0;
	SMB_TIMESTAMP* ts;
	int tsCount;
	int upper, lower;
	int i;
	double slew;
	double size; 

	/* use correct CLOK channel for data channel sample rate. */
	if(IS_100HZ_DATA(pElem)) {
		ts = SPTimeStamps;
		tsCount = SPTimeStampCount;
		slew = (pElem->id - clock_channel + 15) / 16;
		size = EWSECS_PER_SPREC;
	} else {
		ts = LPTimeStamps;
		tsCount = LPTimeStampCount;
		slew = 0;
		size = EWSECS_PER_LPREC;
	}

	/* locate timestamp based on window_no */
	i = lower = 0;
	upper = tsCount;
	while(upper >= lower) {
		i = (upper + lower) / 2;
		if((ts+i)->window_no == pElem->window_no) {
			upper = lower = i;
			break;
		}
		if((ts + i)->window_no < pElem->window_no) {
			lower = i + 1;
		} else {
			upper = i - 1;
		}
	}

	/* calculate time */
	if (upper == lower)
		pElem->starttime = (ts + i)->tm;
	else if (upper < 0)
		pElem->starttime = ts->tm - (size * (ts->window_no - pElem->window_no));
	else if (lower >= tsCount)
		pElem->starttime = (ts + tsCount - 1)->tm + (size * (pElem->window_no - (ts + tsCount - 1)->window_no));
	else
		pElem->starttime = (ts + upper)->tm + (size * (pElem->window_no - (ts + upper)->window_no));
/*		pElem->starttime = (ts + upper)->tm + (((ts + lower)->tm - (ts + upper)->tm) *
		(pElem->window_no - (ts + upper)->window_no)) /
				 ((ts + lower)->window_no - (ts + upper)->window_no);
*/	
	pElem->starttime += ((double)slew / 100.0);

	return rc;
}

/***********************************************************
 msg - output msg based on flags nQuiet, bVerbose and bLevel
 settings.
 lvl - one of ERR_MSG, WARNING_MSG, DEBUG_MSG
 fmt - same as printf format string 
 ... - variable number of args for insertion into fmt.
************************************************************/
void msg(int lvl, char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	if(!bQuiet && (nVerbose >= lvl)) {	
		vfprintf((lvl==ERR_MSG)? stderr : stdout, fmt, ap);
	}
	va_end(ap);
}

/**********************************************************
 ReadSNCLMapFile - Read file containing mappings for SMB 
 name to SCNL names.
***********************************************************/
int ReadSNCLMapFile(char* smFile) {
	
	int rc = 0;
	int nfiles = 0;
	char* smb;
	char* sncl;
	char* key;
	char* tok;
	int bailout;
	SMB_SNCL_MAP *snclMapping;
	static char* seps = " .\t\n";

	if(nfiles = k_open(smFile)) {

		/* loop through lines*/
		bailout = 0;
		while(k_rd() && !bailout) {
			
			/* get first tok*/
			smb = k_str();
			if(!smb) {
				continue;
			}
			/* skip rest of line for comment*/
			if(smb[0] == '#') {
				continue;
			}

			/* otherwise create key, value and insert
			int hashtable SNCLMap */
			if( key = (char*) malloc(SMB_NAME_SIZE)) {

				/* copy key value */
				strncpy(key, smb, SMB_NAME_SIZE);

				/* get sncl*/
				sncl = k_str();

				if(snclMapping = (SMB_SNCL_MAP*) malloc(sizeof(SMB_SNCL_MAP))) {

					strncpy(snclMapping->smb_name, key, SMB_NAME_SIZE);

					/* parse sncl*/
					tok = strtok(sncl, seps);
					strncpy(snclMapping->sta, tok, TRACE2_STA_LEN);
					tok = strtok(NULL,seps);
					strncpy(snclMapping->chan, tok, TRACE2_CHAN_LEN);
					tok = strtok(NULL,seps);
					strncpy(snclMapping->net, tok, TRACE2_NET_LEN);
					tok = strtok(NULL,seps);
					strncpy(snclMapping->loc, tok, TRACE2_LOC_LEN);



					/* insert into hash table */
					if(!hashtable_insert(SNCLMap, key , snclMapping )) {
						msg(ERR_MSG, "Could not insert SNCL mapping into hashtable.\n");
						rc = 0;
						break;
					}
					rc = 1;
				} else {
					msg(ERR_MSG, "Cannot allocate memory for hash key.\n");
					free(key);
					bailout = 1;
				}
			} else {
				msg(ERR_MSG, "Cannot allocate memory for hash key.\n");
				bailout = 1;
			}
		}
		nfiles = k_close();
	} else {
		msg(ERR_MSG, "error opening map file.\n");
	}

	return rc;
}

/**********************************************************
 HashFromKey - Returns a hash value for a given key.
***********************************************************/
unsigned int HashFromKey(void *key) {

   unsigned long hash = 5381;
   int c;
   char* str = (char*)key;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**********************************************************
HashKeyCompare - Callback function for create_hashtable. 
Used to compare keys for equality.
***********************************************************/
int HashKeyCompare(void *k1, void *k2) {

    return (0 == memcmp(k1,k2,SMB_NAME_SIZE));
}

