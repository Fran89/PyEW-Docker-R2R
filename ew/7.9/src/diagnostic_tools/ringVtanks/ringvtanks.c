

/*
  ringVtanks.c

  Get the menu from wave_serverV
*/
/* Modified for Y2K compliance: PNL, 11/5/1998 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <platform.h> /* includes system-dependent socket header files */
#include <chron3.h>
#include <earthworm.h>
#include <ws_clientII.h>
#include <kom.h>
#include <transport.h>
#include <swap.h>
#include <time_ew.h>
#include <trace_buf.h>

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff  /* should be in <netinet/in.h> */
#endif

#define MAX_WAVESERVERS   100
#define MAX_ADRLEN        20
#define MAXTXT           150
#define TABLELEN 1000


/* Structures
 ************/
typedef struct 
{
	char cWsIp[MAX_WAVESERVERS][MAX_ADRLEN];   /* IP number of a waveserver(s) */
	char cWsPort[MAX_WAVESERVERS][MAX_ADRLEN]; /* Port Number of a waveserver(s) */
	char cWsLabel[MAX_WAVESERVERS][MAXTXT];    /* Label for a waveserver */
	int  iNumWs;                               /* Number of waveservers read */
} GPARM;

typedef struct 
{
	char cStation[10];
	char cComponent[10];
	char cNetwork[10];
} SCN;


typedef struct 
{
	TRACE_HEADER TrHdr;
	int maxMsg;
	int minMsg;
	double totalMsgTime;
	int nMsg;
} TABLE ;

TABLE MissingScns[TABLELEN];
int TableLast = 0;	/*Next available entry */



/* Function prototypes
   *******************/
int  GetConfig( char *, GPARM * );
void LogWsErr( char [], int );
void Add2ServerList( char *, GPARM * );
int  FindSCN( TABLE* , TRACE_HEADER* );
int  AddSCN(TABLE* , TRACE_HEADER*, int );
void UpdateSCN(TABLE*, TRACE_HEADER* , int , int );
int  compare( const void*, const void* );

/* Globals between threads 
 *************************/
SCN		pWsScns[2000];
int		iNumWsScns = 0;
  
/*SCN		MissingScns[1000];
int		iNumMissing = 0;*/

SHM_INFO Region;
char	cRingName[20];
long	lRingKey;         /* Key to the transport ring to read from */
MSG_LOGO getlogo;

/* Constants
   *********/
const long wsTimeout = 30000;                /* milliSeconds to wait for reply */

/* Thread stuff
 **************/
#define THREAD_STACK 8192
static unsigned tidProcessor;      /* Message processing thread id */

thr_ret MessageProcessor( void * );
int MessageProcessorStatus = 0;        /* 0=> Snipper thread ok. <0 => dead */

/****************************************************************************/

int main( int argc, char *argv[] )
{
	
	int		rc;                  /* Function return code */
	int		i, k;
	int		days_to_save = 10;

	long	lGotSize;

	char	cMsg[MAX_TRACEBUF_SIZ];
	char	line[100];
	char    index_size[10];

	double bytesPerDay;
	double total_bytes;

	FILE	*pOut_File;
	GPARM	config_parameters;
	MSG_LOGO logo;
	WS_MENU_QUEUE_REC CountQueue;

	CountQueue.head = (WS_MENU) NULL;
	CountQueue.tail = (WS_MENU) NULL;

	/* Check command line arguments
	*******************************/
	if ( argc == 1 )
	{
		printf( "Usage: ringvtanks <configfile> <ring name> (days to save)\n" );
		printf( "Where:\n");
		printf( "      <configfile>   = A file of waveserver addresses in the form:\n");
		printf( "                       Waveserver aaa.bbb.ccc.ddd:ppppp.\n");
		printf( "      <ring name>    = The earthworm ring to listen to.\n");
		printf( "      (days to save) = Optional command used in formating waveserver output, defaults to 10.\n");
		return -1;
	}
	if ( argc < 3 )
	{
		printf( "ringvtanks: Not enough inputs.\n");
		return -1;
	}
	if ( argc > 4 )
	{
		printf( "ringvtanks: Too many inputs.\n");
		return -1;
	}
	if ( argc == 4 )
	{
		days_to_save = atoi(argv[3]);
	}

	strcpy( cRingName, argv[2]);

	/* Since our routines call logit, we must initialize it,
	 *  although we don't want to us it!
	 *******************************************************/
	logit_init( "ringVtanks", (short) 0, 256, 0 );

	/* Zero the wave server arrays
	***************************/
	for ( i = 0; i < MAX_WAVESERVERS; i++ )
	{
		memset( config_parameters.cWsIp[i], 0, MAX_ADRLEN );
		memset( config_parameters.cWsPort[i], 0, MAX_ADRLEN );
	}
	config_parameters.iNumWs = 0;

	strcpy(index_size, "10000");

	/* read waveservers from config file 
	************************************/
	GetConfig( argv[1], &config_parameters );

	/* Print out startup Info
	 ************************/
	printf ("\nringVtanks: Listening to the following Ring:\n");
	printf ("%s\n\n", cRingName);

	printf ("ringVtanks: Read in the following %d waveserver(s):\n", config_parameters.iNumWs);
	for ( i = 0; i < config_parameters.iNumWs; i++)
	{
		printf("\t%s:%s\n", config_parameters.cWsIp[i], config_parameters.cWsPort[i]);
	}
	printf("\n");

	/* Initialize the socket system
	 ******************************/
	SocketSysInit();

	/* Build the current wave server menus
	 *************************************/
	for ( i = 0; i < config_parameters.iNumWs; i++ )
	{
		rc = wsAppendMenu( config_parameters.cWsIp[i], config_parameters.cWsPort[i], &CountQueue, wsTimeout );
		if ( rc != WS_ERR_NONE )
		{
			LogWsErr( "wsAppendMenu", rc );
			return -1;
		}
	}
	if (CountQueue.head != NULL )
	{
		/* Get the SCNs from the waveservers
		 ***********************************/
		iNumWsScns = 0;
		for ( i = 0; i < config_parameters.iNumWs; i++ )
		{
			WS_PSCN scnp;

			rc = wsGetServerPSCN( config_parameters.cWsIp[i], config_parameters.cWsPort[i], &scnp, &CountQueue );

			if ( rc == WS_ERR_EMPTY_MENU ) 
				continue;

			while ( 1 )
			{
				iNumWsScns++;

				strcpy(pWsScns[iNumWsScns].cStation, scnp->sta);
				strcpy(pWsScns[iNumWsScns].cComponent, scnp->chan);
				strcpy(pWsScns[iNumWsScns].cNetwork, scnp->net);		  

				if ( scnp->next == NULL )
					break;
				else
					scnp = scnp->next;
			}
		}
		wsKillMenu( &CountQueue );
	}

	/* Attach to ring
	*****************/
	if ((lRingKey = GetKey( cRingName )) == -1 )
	{
		printf("Invalid RingName; exiting!\n" );
		exit( -1 );
	}
	tport_attach( &Region, lRingKey );

	/* Specify logos to get
	***********************/
	GetType(  "TYPE_TRACEBUF", &getlogo.type   );
	GetModId( "MOD_WILDCARD",  &getlogo.mod    );
	GetInst(  "INST_WILDCARD", &getlogo.instid );

	/* Flush the ring
	*****************/
	while ( tport_getmsg( &Region, &getlogo, (short)1, &logo, &lGotSize,
	  	   (char *)&cMsg, MAX_TRACEBUF_SIZ ) != GET_NONE );
	printf("ringVtanks: inRing flushed.\n");

	/* Start the  message processing thread
	***************************************/
	if ( StartThread(  MessageProcessor, (unsigned)THREAD_STACK, &tidProcessor ) == -1 )
	{
		printf("ringVtanks: Error starting  MessageProcessor thread. "
			"Exitting.\n" );
		tport_detach( &Region );
		exit (-1);
	}
	MessageProcessorStatus = 0; /*assume the best*/

	/* main loop
	 ***********/
	do
	{
		printf( "\n   Type quit<cr> to stop ringVtanks.\n\n" );
		gets( line );
		/* print out missing so far
		 **************************/
		if ( strlen( line ) == 0 )
		{
			printf("\nThe following %d station(s) are missing:\n",TableLast );
			for(k = 0; k < TableLast; k++)
			{
				printf("SCN %s.%s.%s was not found in the Waveserver list.\n", 
						MissingScns[k].TrHdr.sta, MissingScns[k].TrHdr.chan, 
						MissingScns[k].TrHdr.net);
			}
		}

		for ( i = 0; i < (int)strlen( line ); i++ )
			line[i] = tolower( line[i] );

		line[4] = '\0';
		sleep_ew(1000);

	}
	while (( strcmp( line, "quit" ) != 0 ) && ( tport_getflag( &Region ) != TERMINATE )
		   && (MessageProcessorStatus==0));

    /* Time to quit, make a nice file of missing stations
	 ****************************************************/
	printf ("\nQuitting, writing output file missing_stations.txt.\n\n\n");

	pOut_File = fopen("missing_stations.txt", "w");

	/* First tell a little about this run
	 ************************************/
	fprintf (pOut_File, "\n# ringVtanks: Listened to the following Ring:\n");
	fprintf (pOut_File, "#\t%s\n\n", cRingName);
	fprintf (pOut_File, "# ringVtanks: Read in the following %d waveserver(s):\n", config_parameters.iNumWs);
	for ( i = 0; i < config_parameters.iNumWs; i++)
	{
		fprintf(pOut_File, "#\t%s:%s\n", config_parameters.cWsIp[i], config_parameters.cWsPort[i]);
	}
	fprintf(pOut_File, "\n\n");

	/* Print out the missing stations
	 ********************************/
	fprintf(pOut_File, "# Found %d SCNs missing.\n", TableLast);
	fprintf(pOut_File, "# Generating waveserverV output:\n#\n");
	fprintf(pOut_File, "#       SCN           Record           Logo            File Size   Index Size       File Name\n");
	fprintf(pOut_File, "#       names          size     (TYPE_TRACEBUF only)  (megabytes) (max breaks)     (full path)\n\n");
	for(k = 0; k < TableLast; k++)
	{
		/* compute bytes per day of a WaveServer tank
		 ********************************************/
		bytesPerDay = ( (double)(24*60*60) / (MissingScns[k].totalMsgTime / MissingScns[k].nMsg)) * MissingScns[k].maxMsg;
		bytesPerDay = bytesPerDay / 1000000.;
		total_bytes = bytesPerDay * days_to_save;

		fprintf(pOut_File, "Tank %5s  %5s  %2s  %d\tINST_WILDCARD MOD_WILDCARD %5.0f %s <insert path here>%s_%s_%s.tnk\n", MissingScns[k].TrHdr.sta, 
				MissingScns[k].TrHdr.chan, MissingScns[k].TrHdr.net, MissingScns[k].maxMsg, total_bytes, index_size, MissingScns[k].TrHdr.sta,
				MissingScns[k].TrHdr.chan, MissingScns[k].TrHdr.net);
	}

	fclose (pOut_File);
	return 0;
}


/****************************************************************************/

thr_ret MessageProcessor( void *dummy )
{
	long	lGotSize;
	char	cMsg[MAX_TRACEBUF_SIZ];
	

	int		i, rc, ret;
	int		iFound = 0;
	int		iMissingInList = 0;
	int     index;
	
	long	writeInterval = 5;

	time_t	timeLastReport;
	time_t	timeNow;
   

	MSG_LOGO logo;
	TRACE_HEADER *trh;

    trh  = (TRACE_HEADER *) cMsg;

	/* Tell the main thread we're ok
	********************************/
	MessageProcessorStatus=0;
	
	while ( tport_getflag(&Region) != TERMINATE )
	{
		rc = tport_getmsg(&Region, &getlogo, (short)1,
						  &logo, &lGotSize, cMsg, MAX_TRACEBUF_SIZ );

		if ( rc == GET_NONE )
		{
			sleep_ew( 200 );
			continue;
		}
		if ( rc == GET_TOOBIG )
		{
			printf("ringVtanks: retrieved message too big (%d) for msg\n",
				   lGotSize );
			continue;
		}
		if ( rc == GET_NOTRACK )
			printf("ringVtanks: Tracking error.\n");
		if ( rc == GET_MISS_LAPPED )
			printf("ringVtanks: Got lapped on the ring.\n");
		if ( rc == GET_MISS_SEQGAP )
			printf("ringVtanks: Gap in sequence numbers\n");
		if ( rc == GET_MISS )
			printf("ringVtanks: Missed messages\n");

		/* Search the SCN list for this scn
		 **********************************/
		iFound = 0;
		for (i = 0; i < iNumWsScns+1; i++)
		{
			if ((strcmp( pWsScns[i].cStation, trh->sta) == 0) && (strcmp( pWsScns[i].cComponent, trh->chan) == 0)
				&& (strcmp( pWsScns[i].cNetwork, trh->net) == 0))
			{
				iFound = 1;
				break;
			}
			else
			{
				iFound = 0;
			}
		}
		/* Check to see if the missing SCN has already
		 * been added to the missing list
		 *********************************************/
		if (iFound == 0)
		{
			WaveMsgMakeLocal( trh );

			/* Have we seen this SCN before?
			 ********************************/
			index = FindSCN( MissingScns, trh);
			if (index == -1 ) /* it's a new one */
			{
				if (ret = AddSCN(MissingScns, trh, lGotSize) <0)
				{
					printf( "ringvtanks: Table overflow. More than %d SCNs found\n",TABLELEN);
				}

				if  ( time(&timeNow) - timeLastReport  >=  writeInterval ) 
				{
					timeLastReport = timeNow;
					printf("%d SCNs missing, press enter for list\n",TableLast );
				}
			}
			else
			{
				UpdateSCN(MissingScns, trh, index, lGotSize);
			}
		}
	}

	/* Quitting */
    MessageProcessorStatus=-1; /* file a complaint to the main thread */
    KillSelfThread(); /* we'll count on the restart mechanism */
}

/****************************************************************************/

void LogWsErr( char fun[], int rc )
{
	switch ( rc )
	{
	case WS_ERR_INPUT:
		printf( "%s: Bad input parameters.\n", fun );
		break;
	case WS_ERR_EMPTY_MENU:
		printf( "%s: Empty menu.\n", fun );
		break;
	case WS_ERR_SERVER_NOT_IN_MENU:
		printf( "%s: Empty menu.\n", fun );
		break;
	case WS_ERR_SCN_NOT_IN_MENU:
		printf( "%s: SCN not in menu.\n", fun );
		break;
	case WS_ERR_BUFFER_OVERFLOW:
		printf( "%s: Buffer overflow.\n", fun );
		break;
	case WS_ERR_MEMORY:
		printf( "%s: Out of memory.\n", fun );
		break;
	case WS_ERR_BROKEN_CONNECTION:
		printf( "%s: The connection broke.\n", fun );
		break;
	case WS_ERR_SOCKET:
		printf( "%s: Could not get a connection.\n", fun );
		break;
	case WS_ERR_NO_CONNECTION:
		printf( "%s: Could not get a connection.\n", fun );
		break;
	default:
		printf( "%s: unknown ws_client error: %d.\n", fun, rc );
		break;
	}
	return;
}

/****************************************************************************/

void Add2ServerList( char * addr_port, GPARM * Gparm )
{
	char	c;
	char	addr[MAXTXT], port[MAX_ADRLEN];
	char	*addr_p;
	int		is_port = 0;   /* flag to say we are looking at the port number */
	int		i = 0;
	int		a = 0, p = 0;
	struct	hostent *host;
	struct	in_addr addr_s;

	/* scan the input string for format address:port */
	while ((c = addr_port[i++]) != (char) 0 ) 
	{
		if (a == MAXTXT) 
		{
			fprintf(stderr, "ringVtanks: hostname <%s> too long; max is %d\n", addr,
					MAXTXT - 1);
			return;
		}
		if ( is_port == 0 ) 
		{
			if ( c == ':') 
			{   /* end of address, start of port# */
				addr[a++] = '\0';
				is_port = 1;
				continue;
			} 
			else 
			{    /* a hostname or IP address */
				addr[a++] = c;
				continue;
			}
		} 
		else 
		{     /* looking at port number */
			if ( isdigit(c) ) 
			{   /* looks ok for port number */
				port[p++] = c;
				continue;
			} 
			else 
			{              /* oops! Something's wrong! */
				fprintf(stderr, "ringVtanks: bad server address:port <%s>.\n", addr_port );
				return;
			}
		}   /* if is_port  */
	}     /* while(1)  */
	port[p++] = '\0';

	/* Assume we have an IP address and try to convert to network format.
	 * If that fails, assume a domain name and look up its IP address.
	 * Can't trust reverse name lookup, since some place may not have their
	 * name server properly configured.
	 */
	addr_p = addr;
	if ( inet_addr(addr) == INADDR_NONE )
	{        /* it's not a dotted quad address */
		if ( (host = gethostbyname(addr)) == NULL)
		{
			logit("e", "ringVtanks: bad server address <%s>\n", addr );
			return;
		}
		memcpy((char *) &addr_s, host->h_addr,  host->h_length);
		addr_p = inet_ntoa(addr_s);
	}


	if (strlen(addr_p) > 15 || strlen(port) > 5) 
	{
		fprintf(stderr, "ringVtanks: ignoring dubious server <%s:%s>\n", addr_p,
				port);
		return;
	}
	strcpy( Gparm->cWsIp[Gparm->iNumWs], addr_p );
	strcpy( Gparm->cWsPort[Gparm->iNumWs], port );
	Gparm->iNumWs++;

	return;
}

/****************************************************************************/

int FindSCN( TABLE* SCNtable, TRACE_HEADER* msg)
{
	int i;
	
	for(i=0;i<TABLELEN;i++)
	{
		if( strcmp(SCNtable[i].TrHdr.sta, msg->sta)==0 &&
			strcmp(SCNtable[i].TrHdr.chan, msg->chan)==0 &&
			strcmp(SCNtable[i].TrHdr.net, msg->net)==0 )
			return(i);
	}
	return(-1);
}

/****************************************************************************/

int AddSCN( TABLE* SCNtable, TRACE_HEADER* msg, int msgSize)
{
	if(TableLast == TABLELEN) return(-1); /* no more room */
	
	memcpy( (void*)&(SCNtable[TableLast].TrHdr), msg, sizeof(TRACE_HEADER));
	SCNtable[TableLast].maxMsg = msgSize;
	SCNtable[TableLast].minMsg = msgSize;
	SCNtable[TableLast].totalMsgTime = SCNtable[TableLast].TrHdr.endtime - /* time of last data point */
									SCNtable[TableLast].TrHdr.starttime		/* time of first data point */
									+ 1/SCNtable[TableLast].TrHdr.samprate; /* one sample period */
	SCNtable[TableLast].nMsg = 1;
	TableLast++;

	/* Sort the table 
	*****************/
	qsort( (void*)SCNtable, TableLast, sizeof(TABLE), compare);

	return(0);
}

/****************************************************************************/

int compare( const void* s1, const void* s2 )
{
	int rc;
	TABLE* t1 = (TABLE*) s1;
	TABLE* t2 = (TABLE*) s2;

   /* Compare all of both strings: */
   rc = strcmp( t1->TrHdr.sta, t2->TrHdr.sta);
   if (rc != 0) 
	   return(rc);

   rc = strcmp( t1->TrHdr.chan, t2->TrHdr.chan);
   if (rc != 0) 
	   return(rc);

   rc = strcmp( t1->TrHdr.net, t2->TrHdr.net);

   return(rc);
}


/****************************************************************************/

void UpdateSCN(TABLE* SCNtable, TRACE_HEADER* msg, int index, int msgSize)
{
	if( SCNtable[index].maxMsg < msgSize) 
		SCNtable[index].maxMsg = msgSize;
	if( SCNtable[index].minMsg > msgSize) 
		SCNtable[index].minMsg = msgSize;

	/* Update average message time
	******************************/
	SCNtable[index].totalMsgTime =	SCNtable[index].totalMsgTime + 
										SCNtable[index].TrHdr.endtime - 
										SCNtable[index].TrHdr.starttime +
										1/SCNtable[index].TrHdr.samprate;
	(SCNtable[index].nMsg)++;

	return;
}

/****************************************************************************/

#define ncommand 1           /* Number of commands in the config file */

int GetConfig( char *config_file, GPARM *Gparm )
{
	char	init[ncommand];     /* Flags, one for each command */
	int		nmiss;              /* Number of commands that were missed */
	int		nfiles;
	int		i;

	/* Set to zero one init flag for each required command
     ***************************************************/
	for ( i = 0; i < ncommand; i++ ) init[i] = 0;

	/* Initialize Configuration parameters
	 ***********************************/
	Gparm->iNumWs = 0;

	/* Open the main configuration file
     ********************************/
	nfiles = k_open( config_file );
	if ( nfiles == 0 )
	{
		printf( "ringVtanks: Error opening configuration file <%s> Exiting.\n",
		config_file );
		return( -1 );
	}

	/* Process all nested configuration files
     **************************************/
	while ( nfiles > 0 )          /* While there are config files open */
	{
		while ( k_rd() )           /* Read next line from active file  */
		{
			int  success;
			char *com;
			char *str;

			com = k_str();          /* Get the first token from line */

			if ( !com ) continue;             /* Ignore blank lines */
			if ( com[0] == '#' ) continue;    /* Ignore comments */

			/* Open another configuration file
			 *******************************/
			if ( com[0] == '@' )
			{
				success = nfiles + 1;
				nfiles  = k_open( &com[1] );
				if ( nfiles != success )
				{
					printf( "ringVtanks: Error opening command file <%s>. Exiting.\n",
							&com[1] );
					return( -1 );
				}
			continue;
			}

			/* Read configuration parameters
			*****************************/
			else if ( k_its( "Waveserver" ) )
			{
				if ((Gparm->iNumWs + 1) < MAX_WAVESERVERS)
				{
					if ( str = k_str() )
					{
						Add2ServerList( str, Gparm );
					}
					init[0] = 1;
				}
				else
				{
					printf ("ringVtanks: Max number of waveservers reached, ignoring this server.\n");
				}
			}

			/* An unknown parameter was encountered
			************************************/
			else
			{
				printf( "ringVtanks: <%s> unknown parameter in <%s>\n",
						com, config_file );
				continue;
			}

			/* See if there were any errors processing the command
			***************************************************/
			if ( k_err() )
			{
				printf( "ringVtanks: Bad <%s> command in <%s>. Exiting.\n", com,
				config_file );
				exit( -1 );
			}
		}
		nfiles = k_close();
	}

	/* After all files are closed, check flags for missed commands
	***********************************************************/
	nmiss = 0;
	for ( i = 0; i < ncommand; i++ )
		if ( !init[i] )
			nmiss++;

	if ( nmiss > 0 )
	{
		printf( "ringVtanks: ERROR, no " );
		if ( !init[0] ) 
			printf( "<Waveserver> " );
		exit( -1 );
	}
	return (1);
}









