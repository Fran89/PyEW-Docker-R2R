/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: getstation.c 1674 2004-08-09 23:29:19Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/08/09 23:29:19  dietz
 *     modified to work with location code
 *
 */
 
/*
  getstation.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <chron3.h>
#include <earthworm.h>
#include <ws_clientII.h>
#include <kom.h>

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff  /* should be in <netinet/in.h> */
#endif

#define MAX_WAVESERVERS   100
#define MAX_ADRLEN        20
#define MAXTXT           150

typedef struct 
{
	char cWsIp[MAX_WAVESERVERS][MAX_ADRLEN];   /* IP number of a waveserver(s) */
	char cWsPort[MAX_WAVESERVERS][MAX_ADRLEN]; /* Port Number of a waveserver(s) */
	char cWsLabel[MAX_WAVESERVERS][MAXTXT];    /* Label for a waveserver */
	int  iNumWs;                               /* Number of waveservers read */
} GPARM;


/* Function prototypes
 *********************/
void GetConfig( char *, GPARM * );
void LogWsErr( char [], int );
void Add2ServerList( char *, GPARM * );

/* Constants
 ***********/
const long wsTimeout = 30000;                /* milliSeconds to wait for reply */

/****************************************************************************/

int main( int argc, char *argv[] )
{
	int	rc;                  /* Function return code */
	int	i;
	int	iSCNLFound;

	char	*pStaToSearchFor;
	char	*pCompToSearchFor;
	char	*pNetToSearchFor;
	char	*pLocToSearchFor;

	GPARM config_parameters;
	WS_MENU_QUEUE_REC queue;

	queue.head = (WS_MENU) NULL;
	queue.tail = (WS_MENU) NULL;

	/* Check command line arguments
	 ****************************/
	if ( argc != 6 )
	{
		printf( "Usage: getstation <wsv_file> <station> <component> <network> <location>\n" );
		printf( "Where:\n");
		printf( "       <wsv_file>  = file of waveserver addresses in the form:\n");
		printf( "                     Waveserver aaa.bbb.ccc.ddd:ppppp\n");
		printf( "       <station>   = station code,   or wild as a wildcard\n");
		printf( "       <component> = component code, or wild as a wildcard\n");
		printf( "       <network>   = network code,   or wild as a wildcard\n");
		printf( "       <location>  = location code,  or wild as a wildcard\n");
		return -1;
	}

	/* Since our routines call logit, we must initialize it, although we don't
	* want to!
	*/
	logit_init( "getstation", (short) 0, 256, 0 );

	pStaToSearchFor  = argv[2];
	pCompToSearchFor = argv[3];
	pNetToSearchFor  = argv[4];
	pLocToSearchFor  = argv[5];

	/* Zero the wave server arrays
	***************************/
	for ( i = 0; i < MAX_WAVESERVERS; i++ )
	{
		memset( config_parameters.cWsIp[i], 0, MAX_ADRLEN );
		memset( config_parameters.cWsPort[i], 0, MAX_ADRLEN );
	}

	/* read waveservers from config file 
	************************************/
	GetConfig( argv[1], &config_parameters );

	/* Print out startup Info
	 ************************/
	printf ("\ngetstation: Searching for the following SCNL:\n");
	printf ("%s.%s.%s.%s\n\n", pStaToSearchFor, pCompToSearchFor, 
                                   pNetToSearchFor, pLocToSearchFor );

	printf ("getstation: Read in the following %d waveserver(s):\n", 
                 config_parameters.iNumWs);
	for ( i = 0; i < config_parameters.iNumWs; i++)
	{
		printf("\t%s:%s\n", config_parameters.cWsIp[i], config_parameters.cWsPort[i]);
	}
	printf("\n");

	/* Initialize the socket system
	 ****************************/
	SocketSysInit();

	/* Build the current wave server menus
	 ***********************************/
	for ( i = 0; i < config_parameters.iNumWs; i++ )
	{
		rc = wsAppendMenu( config_parameters.cWsIp[i], config_parameters.cWsPort[i], 
                                   &queue, wsTimeout );
		if ( rc != WS_ERR_NONE )
		{
			LogWsErr( "wsAppendMenu", rc );
			return -1;
		}
	}
	if (queue.head == NULL )
	{
		logit("e", "getstation: nothing in server\n");
		exit( 0 );
	}

	/* Print contents of all tanks
 	 ***************************/
	for ( i = 0; i < config_parameters.iNumWs; i++ )
	{
		WS_PSCNL scnlp;

		rc = wsGetServerPSCNL( config_parameters.cWsIp[i], config_parameters.cWsPort[i], 
                                       &scnlp, &queue );
		if ( rc == WS_ERR_EMPTY_MENU ) 
			continue;

		while ( 1 )
		{
			if( strcmp( pStaToSearchFor, scnlp->sta) == 0  ||
			    strcmp( pStaToSearchFor, "wild") == 0         )
			{
				if( strcmp( pCompToSearchFor, scnlp->chan) == 0  ||
				    strcmp( pCompToSearchFor, "wild") == 0          )
				{
					if( strcmp( pNetToSearchFor, scnlp->net) == 0  || 
					    strcmp( pNetToSearchFor, "wild") == 0          )
					{
					   if( strcmp( pLocToSearchFor, scnlp->loc) == 0  || 
					       strcmp( pLocToSearchFor, "wild") == 0          )
					   {

						printf("Found SCNL: ");
						printf("%s.%s.%s.%s ", scnlp->sta, scnlp->chan, 
						        scnlp->net, scnlp->loc);
						printf("on Waveserver %s:%s\n", config_parameters.cWsIp[i], 
						       config_parameters.cWsPort[i]);
						iSCNLFound = 1;
					   }
					}
				}
			}
			if ( scnlp->next == NULL )
				break;
			else
				scnlp = scnlp->next;
		}
	}
	if (iSCNLFound == 0)
	printf("\nSCNL %s.%s.%s.%s not found.\n", pStaToSearchFor, pCompToSearchFor, 
               pNetToSearchFor,pLocToSearchFor);

	/* Release the linked list created by wsAppendMenu
	 ***********************************************/
	wsKillMenu( &queue );

	return 0;
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
	case WS_ERR_SCNL_NOT_IN_MENU:
		printf( "%s: SCNL not in menu.\n", fun );
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
			fprintf(stderr, "getstation: hostname <%s> too long; max is %d\n", addr,
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
				fprintf(stderr, "getstation: bad server address:port <%s>.\n", addr_port );
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
			logit("e", "getstation: bad server address <%s>\n", addr );
			return;
		}
		memcpy((char *) &addr_s, host->h_addr,  host->h_length);
		addr_p = inet_ntoa(addr_s);
	}


	if (strlen(addr_p) > 15 || strlen(port) > 5) 
	{
		fprintf(stderr, "getstation: ignoring dubious server <%s:%s>\n", addr_p,
				port);
		return;
	}
	strcpy( Gparm->cWsIp[Gparm->iNumWs], addr_p );
	strcpy( Gparm->cWsPort[Gparm->iNumWs], port );
	Gparm->iNumWs++;

	return;
}

/****************************************************************************/

#define ncommand 1           /* Number of commands in the config file */

void GetConfig( char *config_file, GPARM *Gparm )
{
	char	init[ncommand];     /* Flags, one for each command */
	int		nmiss;              /* Number of commands that were missed */
	int		nfiles;
	int		i;

	/* Set to zero one init flag for each required command
	 ***************************************************/
	for ( i = 0; i < ncommand; i++ ) 
		init[i] = 0;

	/* Initialize Configuration parameters
	 ***********************************/
	Gparm->iNumWs = 0;

	/* Open the main configuration file
	 ********************************/
	nfiles = k_open( config_file );
	if ( nfiles == 0 )
	{
		printf( "getstation: Error opening configuration file <%s> Exiting.\n",
		config_file );
		exit( -1 );
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

			if ( !com ) 
				continue;             /* Ignore blank lines */
			if ( com[0] == '#' ) 
				continue;    /* Ignore comments */

			/* Open another configuration file
			*******************************/
			if ( com[0] == '@' )
			{
				success = nfiles + 1;
				nfiles  = k_open( &com[1] );
				if ( nfiles != success )
				{
					printf( "getstation: Error opening command file <%s>. Exiting.\n",
					&com[1] );
					exit( -1 );
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
					printf ("getstation: Max number of waveservers reached, ignoring this server.\n");
				}
			}

			/* An unknown parameter was encountered
			 ************************************/
			else
			{
				printf( "getstation: <%s> unknown parameter in <%s>\n",
				com, config_file );
				continue;
			}

			/* See if there were any errors processing the command
			 ***************************************************/
			if ( k_err() )
			{
				printf( "getstation: Bad <%s> command in <%s>. Exiting.\n", com,
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
		printf( "getstation: ERROR, no " );
		if ( !init[0] ) 
			printf( "<Waveserver> " );
			exit( -1 );
	}
	return;
}





