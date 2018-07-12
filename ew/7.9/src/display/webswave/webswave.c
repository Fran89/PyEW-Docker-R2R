

#define VERSION_STR "0.0.01 - November 4, 2012"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <trace_buf.h>
#include <swap.h>
#include <rdpickcoda.h>
//#include <sema_ew.h>
#include "mongoose.h"
#include "webpage.h"


/* Functions in this file */
void StartupErrorMsg( FILE *std );
void addwebopt(char **webopt, int *nwebopt, char *opt, char *val );
int stafilter( char *msg, unsigned char type, char *sta, 
		char *chan, char *net, char *loc );
int strcmpwild( char *a, char *b );
int fifoadd( char *msg, int msglen, unsigned char type );
int fiforead( char **msg, int *msglen, long *lastseq );
static void *web_callback( enum mg_event event, struct mg_connection *conn );
int tracehead2json( char *msg, char *buf, int buflen );


#define MAX_RINGS	5
#define NUMLOGOS	2
#define MAXMSGLEN	4096
#define MINARGS		1


/* Globals */
unsigned char	Type_TraceBuf2;
unsigned char	Type_PickSCNL;
unsigned char	ModWildcard;
unsigned char	InstWildcard;

/* Main Program */
int main( int argc, char **argv ) 
{
	MSG_LOGO	GetLogo[NUMLOGOS];		/* Message logos to retrieve */
	SHM_INFO	InRegion[MAX_RINGS];
	long		InRingKey[MAX_RINGS];	/* keys of transport input ring */
	unsigned char 	seq[MAX_RINGS];		/* Sequence numbers */
	int			nRings = 0;				/* Number of considered rings */
	char		*msgbuf;				/* Message buffer */
	long		recsize;				/* Length of received message */
	MSG_LOGO	reclogo;				/* Logo of received message */
	int			rc;						/* Receive code */
	int			i;						/* Generic counter */
	struct mg_context	*webserver;		/* Webserver */
	int 		seqno;					/* Internal msg sequence number */
	char		staf[10];				/* Station filter */
	char		chanf[10];				/* Channel filter */
	char		netf[10];				/* Network filter */
	char		locf[10];				/* Location filter */
	char		*argps, *argpe;			/* For parsing arguments */
	int			cargs = 0;				/* Counting mandatory arguments */
	char		*webopt[11];			/* Web options */
	int			nwebopt = 0;				/* Number of weboptions */

	
	/* Set default input arguments */
	strcpy( staf, "*" );
	strcpy( chanf, "*" );
	strcpy( netf, "*" );
	strcpy( locf, "*" );
	
	/* Set default web options */
	addwebopt( webopt, &nwebopt, "enable_directory_listing", "no" );
	addwebopt( webopt, &nwebopt, "listening_ports", "127.0.0.1:8080" );
	
	/* Fix possible bug with xxd */
	//web_index_html[web_index_html_len-1] = 0x20;
	
	
	/* Process input arguments */
	if( argc < 2 )
	{
		/* Give error msg */
		StartupErrorMsg( stderr );
		return -1;
	}
	else
	{
		for( i = 1; i < argc; i++ )
		{
			/* Earthworm rings */
			if( strstr( argv[i], "-r" ) != NULL )
			{
				while( ++i < argc && argv[i][0] != '-' )
				{
					if( nRings < MAX_RINGS )
					{
						if( ( InRingKey[nRings++] = GetKey( argv[i] ) ) == -1 )
						{
							/* Invalid ring name */
							fprintf( stderr, "%s: Invalid ring - %s\n", argv[0],
									argv[i] );
							exit( -1 );
						}
						cargs++;
					}
					else
					{
						/* Number of rings exceeds maximum */
						fprintf( stderr, 
								"%s: Maximum number of rings <%d> exceeded.\n",
								argv[0], MAX_RINGS );
						StartupErrorMsg( stderr );
						exit( -1 );
					}
				}
				i--;
			}
			/* Station filter */
			if( strstr( argv[i], "-s" ) != NULL )
			{
				i++;
				/* Parse argument for scnl components */
				argps = argv[i];
				
				argpe = strstr( argps, "." );
				if( argpe == NULL || *( argpe + 1 ) =='\0' )
				{
					fprintf( stderr, "%s: Invalid station filter - %s\n",
						argv[0], argv[i] );
					StartupErrorMsg( stderr );
					exit( -1 );
				}
				strncpy( staf, argv[i], ( int )( argpe - argps ) );
				staf[ ( int )( argpe - argps ) ] = '\0';
				argps = argpe + 1;
				
				argpe = strstr( argps, "." );
				if( argpe == NULL || *( argpe + 1 ) =='\0' )
				{
					fprintf( stderr, "%s: Invalid station filter - %s\n",
						argv[0], argv[i] );
					StartupErrorMsg( stderr );
					exit( -1 );
				}
				strncpy( chanf, argps, ( int )( argpe - argps ) );
				chanf[ ( int )( argpe - argps ) ] = '\0';
				argps = argpe + 1;
				
				argpe = strstr( argps, "." );
				if( argpe == NULL || *( argpe + 1 ) =='\0' )
				{
					fprintf( stderr, "%s: Invalid station filter - %s\n",
						argv[0], argv[i] );
					StartupErrorMsg( stderr );
					exit( -1 );
				}
				strncpy( netf, argps, ( int )( argpe - argps ) );
				netf[ ( int )( argpe - argps ) ] = '\0';
				argps = argpe + 1;
				if( *argps =='\0' )
				{
					fprintf( stderr, "%s: Invalid station filter - %s\n",
						argv[0], argv[i] );
					StartupErrorMsg( stderr );
					exit( -1 );
				}
				strcpy( locf, argps );
			}
			/* Webserver port */
			if( strstr( argv[i], "-p" ) != NULL )
			{
				addwebopt( webopt, &nwebopt, "listening_ports", argv[++i] );
			}
		}
	}
	/* Check mandatory arguments */
	if( cargs < MINARGS )
	{
		fprintf( stderr, "%s: Mandatory arguments are missing.\n",
				argv[0] );
		StartupErrorMsg( stderr );
		exit( -1 );
	}
		

	/* Specify Logos to get */
	if( GetType( "TYPE_TRACEBUF2", &Type_TraceBuf2 ) != 0 )
	{
		fprintf(stderr, 
				"%s: Invalid message type <TYPE_TRACEBUF2>!\n", argv[0] );
		exit( -1 );
	}
	if( GetType( "TYPE_PICK_SCNL", &Type_PickSCNL ) != 0 )
	{
		fprintf(stderr, 
				"%s: Invalid message type <TYPE_PICK_SCNL!\n", argv[0] );
		exit( -1 );
	}
	if( GetModId( "MOD_WILDCARD", &ModWildcard ) != 0 )
	{
		fprintf(stderr, "%s: Invalid moduleid <MOD_WILDCARD>!\n", argv[0] );
		exit( -1 );
	}
	if( GetInst( "INST_WILDCARD", &InstWildcard ) != 0 )
	{
		fprintf(stderr, "%s: Invalid instid <INST_WILDCARD>!\n", argv[0] );
		exit( -1 );
	}
	GetLogo[0].instid = InstWildcard;
	GetLogo[0].mod = ModWildcard;
	GetLogo[0].type = Type_TraceBuf2;
	GetLogo[1].instid = InstWildcard;
	GetLogo[1].mod = ModWildcard;
	GetLogo[1].type = Type_PickSCNL;
	
	
	/* Allocate memory for message buffer */
	msgbuf = ( char* ) malloc( sizeof( char ) * MAXMSGLEN );
	if( msgbuf == NULL )
	{
		fprintf( stderr, 
				"%s: Unable to allocate memory for message buffer\n", argv[0] );
		exit( -1 );
	}
	
	/* Attach to shared memory rings */
	for( i = 0; i < nRings; i++ )
	{
		tport_attach( &InRegion[i], InRingKey[i] );
		
		/* Flush the ring */
		while( tport_copyfrom( &InRegion[i], GetLogo, NUMLOGOS, &reclogo,
				&recsize, msgbuf, MAXMSGLEN, &seq[i] ) != GET_NONE );
	}
	
	
	/* Setup mutex semaphore */
	CreateMutex_ew();
	
	
	/* Start web server */
	fprintf( stdout, "Starting web server...\n" );
	webserver = mg_start( &web_callback, NULL, (const char*) webopt );
	if( webserver == NULL )
	{
		fprintf( stderr, "%s: Unable to start web server\n", argv[0] );
		return -1;
	}
	fprintf( stdout, "Web server started. "
			"Use the address below to visualize the traces.\n"
			"http://%s\n",
			mg_get_option(webserver, "listening_ports") );

	
	/* Start main cycle */
	while( tport_getflag( &InRegion[0] ) != TERMINATE )
	{
		int nmsgs = 0;
		
		
		/* Check for messages */
		for( i = 0; i < nRings; i++ )
		{
			rc = tport_copyfrom( &InRegion[i], GetLogo, NUMLOGOS, &reclogo, 
					&recsize, msgbuf, MAXMSGLEN, &seq[i] );
					
			
			/* Decide what to do with message */
			switch( rc )
			{
				case GET_OK:      // got a message, no errors or warnings
					break;
               
				case GET_NONE:    // no messages of interest, check again later
					continue;
               
				case GET_NOTRACK: // got a msg, but can't tell if any were missed
					fprintf( stderr,
							"Msg received (i%u m%u t%u); "
							"transport.h NTRACK_GET exceeded\n",
							reclogo.instid, reclogo.mod, reclogo.type );
					break;

				case GET_MISS_LAPPED:     // got a msg, but also missed lots
					fprintf( stderr,
							"Missed msg(s) from logo (i%u m%u t%u)\n",
							reclogo.instid, reclogo.mod, reclogo.type );
					break;

				case GET_MISS_SEQGAP:     // got a msg, but seq gap
					fprintf( stderr,
							"Saw sequence# gap for logo (i%u m%u t%u s%u)\n",
							reclogo.instid, reclogo.mod, reclogo.type, seq[i] );
					break;

				case GET_TOOBIG:  // next message was too big
					fprintf( stderr,
							"Retrieved msg[%ld] (i%u m%u t%u) "
							"too big for msgbuf[%d]\n",
							recsize, reclogo.instid, reclogo.mod, reclogo.type,
							MAXMSGLEN );
					continue;

				default:         // Unknown result
					fprintf( stderr, "Unknown tport_copyfrom result:%d\n", rc );
					continue;
			}
			msgbuf[recsize] = '\0'; // Null terminate for ease of printing
			nmsgs++;
			
			
			/* Filter Message */
			if( stafilter( msgbuf, reclogo.type, 
					staf, chanf, netf, locf ) == -1 )
				continue;
			
			/* Process Message */
			seqno = fifoadd( msgbuf, recsize, reclogo.type );
			if( seqno == -1 )
			{
				fprintf( stderr, "%s: Error adding message to FIFO.\n", 
						argv[0] );
			}
			
			
		} // End of for cycle
		
		
		/* Wait for new messages */
		if( nmsgs == 0 ) sleep_ew( 200 );
	} // End of while cycle
	
	
	/* detach from shared memory */
	for( i=0; i<nRings; i++ ) tport_detach( &InRegion[i] );
	
	/* Free msg buffer */
	free( msgbuf );
	
	/* Free web options */
	for( i = 0; i < nwebopt; i++ )
		free( webopt[i] );
	
	
	/* Destroy semaphore */
	CloseMutex();
	
	return 0;
}




void StartupErrorMsg( FILE *std )
{
	fprintf( std, "WebSWave - Version: %s\n"
			"Usage: webswave -r [Earthworm Rings] [options]\n"
			"Arguments:\n"
			"  -r <EW rings>  : Mandatory list of earthworm ring names. At \n"
			"                   least one ring must be given.\n"
			"  -s <S.C.N.L>   : Optional station filter. Wildcards ? and * \n"
			"                   can be used to select channels.\n"
			"  -p <ip>:<port> : Optional ip address and/or port for the\n"
			"                   webserver.\n"
			"                   Default setting is 127.0.0.1:8080\n"
			"Examples:\n"
			"  webswave -r WAVE_RING\n"
			"      Listens to WAVE_RING for any TYPE_TRACEBUF2 and \n"
			"      TYPE_PICK_SCNL messages\n"
			"  webswave -r WAVE_RING PICK_RING\n"
			"      The same as previous but listens to WAVE_RING and \n"
			"      PICK_RING simultaneously\n"
			"  webwave -r WAVE_RING PICK_RING -s *.*Z.*.*\n"
			"      Same as previous, but selects only channels with component\n"
			"      name terminated in Z\n\n"
			"For questions and comments, use the Earthworm community "
			"forum \nhttps://groups.google.com/forum/?fromgroups#!forum/"
			"earthworm_forum\n",
			VERSION_STR );
	return;
}


void addwebopt(char **webopt, int *nwebopt, char *opt, char *val )
{
	
	/* Allocate memory for individual options */
	webopt[*nwebopt] = ( char* ) malloc( sizeof( char ) * strlen( opt ) );
	*nwebopt = *nwebopt + 1;
	webopt[*nwebopt] = ( char* ) malloc( sizeof( char ) * strlen( val ) );
	*nwebopt = *nwebopt + 1;
	webopt[*nwebopt] = NULL;
	
	/* Add option */
	strcpy( webopt[(*nwebopt) - 2], opt );
	strcpy( webopt[(*nwebopt) - 1], val );
}

/* Station Filter */
int stafilter( char *msg, unsigned char type, char *sta, 
		char *chan, char *net, char *loc )
{
	TRACE2_HEADER 	*theader;
	EWPICK			pheader;
	
	/* Filter tracebuf messages */
	if( type == Type_TraceBuf2 )
	{
		theader = ( TRACE2_HEADER* ) msg;
		if( strcmpwild( sta, theader->sta ) == -1 ||
				strcmpwild( chan, theader->chan ) == -1 ||
				strcmpwild( net, theader->net ) == -1 ||
				strcmpwild( loc, theader->loc ) == -1 )
			return -1;
	}
	else if( type == Type_PickSCNL )
	{
		rd_pick_scnl( msg, strlen( msg ), &pheader );
		if( strcmpwild( sta, pheader.site ) == -1 ||
				strcmpwild( chan, pheader.comp ) == -1 ||
				strcmpwild( net, pheader.net ) == -1 ||
				strcmpwild( loc, pheader.loc ) == -1 )
			return -1;
	}
	else
	{
		return -1;
	}
	return 1;
}


/* String comparison with wildcards
 * String a may contain wildcards to process */
int strcmpwild( char *a, char *b )
{
	unsigned int match = 0;
	char *ap, *bp;
	/* Forward direction */
	ap = a;
	bp = b;
	while( ap < ( a + strlen( a ) ) && bp < ( b + strlen( b ) ) )
	{
		if( *ap == '?' || *ap == *bp )
		{
			match++;
			ap++;
			bp++;
		}
		else if( *ap == '*' )
		{
			match++;
			bp++;
		}
		else
		{
			return -1;
		}
	}
	if( match != strlen( b ) ) return -1;
	
	/* backwards direction */
	ap = a + strlen( a ) - 1;
	bp = b + strlen( b ) - 1;
	while( ap >= a && bp >= b )
	{
		if( *ap == '?' || *ap == *bp )
		{
			match++;
			ap--;
			bp--;
		}
		else if( *ap == '*' )
		{
			match++;
			bp--;
		}
		else
		{
			return -1;
		}
	}
	return ( match == ( 2 * strlen( b ) ) ) ? 1 : -1;
}




/* FIFO management Functions */
typedef struct _FIFOMSG {
	char			*msg;
	int				msglen;
	long			seq;
	int				type;
	struct _FIFOMSG	*next;
} FIFOMSG;

FIFOMSG		*fifoStart = NULL;
FIFOMSG		*fifoEnd = NULL;
int			fifoC = 0;
int			fifokey = 0;
#define MAXFIFO 200


/* Add msg to fifo */
int fifoadd( char *msg, int msglen, unsigned char type )
{

	/* Allocate new message */
	FIFOMSG *newmsg = ( FIFOMSG* ) malloc( sizeof( FIFOMSG ) );
	if( newmsg == NULL )
		return -1;
			
	/* Allocate msg contents */
	newmsg->msg = ( char* ) malloc( msglen * sizeof( char ) );
	if( newmsg->msg == NULL )
	{
		free( newmsg );
		fprintf( stderr, "fifoadd: Unable to allocate fifo memory\n" );
		return -1;
	}
	
	
	/* Make msg local */
	if( type == Type_TraceBuf2 )
	{
		if( WaveMsg2MakeLocal( ( TRACE2_HEADER* ) msg ) < 0 )
		{
			fprintf( stderr, "WaveMsg2MakeLocal error.\n" );
			return -1;
		}
	}
	
	/* Copy msg contents */
	memcpy( newmsg->msg, msg, msglen );
	newmsg->msglen = msglen;

	/* Set sequence number and type*/
	newmsg->seq = ( fifoEnd == NULL ) ? 0 : ( fifoEnd->seq + 1 );
	newmsg->type = ( int ) type;
	
	
	/* Block until mutex is available */
	RequestMutex();
	
	
	/* Set fifo end */
	if( fifoEnd != NULL ) fifoEnd->next = newmsg;
	fifoEnd = newmsg;
	fifoEnd->next = NULL;
	
	/* Set fifo start */
	if( fifoStart == NULL )
	{
		fifoStart = newmsg;
		fifoC++;
	}
	else
	{
		if( fifoC == MAXFIFO )
		{
			/* fifo is already at maximum capacity, rotate it */
			FIFOMSG *temp = fifoStart->next;
			free( fifoStart->msg );
			free( fifoStart );
			fifoStart = temp;
		}
		else
		{
			fifoC++;
		}
	}
	
	/* Release Mutex */
	ReleaseMutex_ew();
	
	return fifoEnd->seq;
}

/* Read messages from FIFO sequencially */
int fiforead( char **msg, int *msglen, long *lastseq )
{
	FIFOMSG *curmsg;
	FIFOMSG *tgtmsg = NULL;
	
	//printf("Searching for number: %ld\n", *lastseq);
	
	/* Start at begining of fifo */
	if( fifoStart == NULL ) return -1;
	
	/* Check sequence number */
	if( *lastseq == -1 )
	{
		//printf("Starting new...\n");
		// We want the first available message (lowest sequence number)
		for( curmsg = fifoStart; curmsg != NULL; curmsg = curmsg->next )
		{
			if( tgtmsg == NULL )
			{
				tgtmsg = curmsg;
				continue;
			}
			if( curmsg->seq < tgtmsg->seq ) tgtmsg = curmsg;
		}
		*lastseq = tgtmsg->seq;
		*msglen = tgtmsg->msglen;
		*msg = tgtmsg->msg;
		
		//printf("Selected %ld\n", *lastseq );
		return tgtmsg->type;
	}
	
	/* Find first message with sequence number above request */
	for( curmsg = fifoStart; curmsg != NULL; curmsg = curmsg->next )
	{
		if( curmsg->seq <= *lastseq ) continue;
		if( tgtmsg == NULL )
		{
			tgtmsg = curmsg;
			continue;
		}
		if( curmsg->seq < tgtmsg->seq ) tgtmsg = curmsg;
	}
	if( tgtmsg == NULL ) return -1;
	*lastseq = tgtmsg->seq;
	*msglen = tgtmsg->msglen;
	*msg = tgtmsg->msg;
	return tgtmsg->type;
}









/* Callback function for webserver */
static void *web_callback( enum mg_event event, struct mg_connection *conn )
{
	long seq = -1;
	char *msg;
	int msglen;
	int status = 1;
	unsigned char buffer[MAXMSGLEN + 100];
	int				msgtype;
	short			shortlen;
	char			*msgtypestr;
	char			msgtypestrlen;

	//printf("Event: %d\n", event );
	if( event == MG_WEBSOCKET_READY )
	{
		/* This is a websocket request - set while cycle to send messages */
		while( status > 0 )
		{
			/* Block until mutex is available */
			RequestMutex();
			
			/* Read fifo messages */
			while( ( msgtype = fiforead( &msg, &msglen, &seq ) ) != -1 
					&& status > 0 )
			{
				/* Prepare message type string */
				msgtypestr = GetTypeName( ( unsigned char ) msgtype );
				msgtypestrlen = strlen( msgtypestr );
				
				
				/* Encode msg in buffer */
				if( msglen+msgtypestrlen <= 125 )
				{
					
					/* Short messages */
					buffer[0] = 0x82; // Binary messages
					buffer[1] = ( unsigned char ) ( 1 + msgtypestrlen + msglen );
					buffer[2] = msgtypestrlen;
					memcpy( buffer + 3, msgtypestr, msgtypestrlen );
					memcpy( buffer + 3 + msgtypestrlen, msg, msglen );
					status = mg_write( conn, buffer, 
							3 + msglen + msgtypestrlen);
				}
				else if( msglen+msgtypestrlen > 125 )
				{
					/* Medium sized messages */
				
					/* Prepare message for sending */
					buffer[0] = 0x82; // Binary message
					buffer[1] = 0x7e;
					shortlen = ( short ) ( 1 + msglen + msgtypestrlen );
#if defined( _INTEL )
					SwapShort( &shortlen );
#endif
					memcpy( buffer + 2, ( char* ) ( &shortlen ), 2 );
					buffer[2 + 2] = msgtypestrlen;
					memcpy( buffer + 2 + 2 + 1,
							msgtypestr, msgtypestrlen );
					memcpy( buffer + 2 + 2 + 1 + msgtypestrlen,
							msg, msglen );
					/* Send to client */
					status = mg_write( conn, buffer,
							2 + 2 + 1 + msgtypestrlen + msglen );
					//int i;	
					//for(i=0;i<7;i++) printf("%02x ", *(msg+i+32));
					//printf("\n");
				}
				
				
				//fprintf( stdout, "." );
			}
			fflush( stdout );
			/* Release Mutex */
			ReleaseMutex_ew();
			
			
			/* Rest a little before checking for messages again */
			if( status != -1 )
				sleep_ew( 1000 );
		}
		//printf("\n");
		return "";
	}
	else if( event == MG_NEW_REQUEST )
	{
		/*
		struct mg_request_info *info = mg_get_request_info( conn );
		int i;
		for( i = 0; i < info->num_headers; i++ )
			printf( "%s : %s\n", info->http_headers[i].name, 
					info->http_headers[i].value ),
		*/
		
		
		/* HTTP Header */
		mg_printf( conn, "HTTP/1.1 200 OK\r\n"
				//"Vary: Accept-Encoding\r\n"
				"Content-Length: %d\r\n"
				"Content-Encoding: gzip\r\n"
				//"Content-Type: text/html\r\n"
				"\r\n", _HTMLVARLEN);
				
		//mg_printf( conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html"
		//		"\r\n\r\n");
		
		//mg_printf( conn, "%*s", _HTMLVARLEN, _HTMLVAR );
		mg_write( conn, _HTMLVAR, _HTMLVARLEN );
		//mg_send_file(conn, "test.html" );
		//mg_printf( conn, "<html>Teste</html>" );
		return "yes";
	}
	
	
	
	return NULL;
}


/*
 * tracehead2json : Was supposed to generate a json header to include 
 * but sending the raw message may be better so this is not used for now.
 */
int tracehead2json( char *msg, char *buf, int buflen )
{
	TRACE2_HEADER 	*thead = ( TRACE2_HEADER* ) msg;
	
	return snprintf( buf, buflen,
			"{"
			"\"typ\":\"tbuf2\","
			"\"sta\":\"%s\",\"cha\":\"%s\",\"net\":\"%s\",\"loc\":\"%s\","
			"\"st\":%.3f,\"et\":%.3f,\"sr\":%.3f,\"ns\":%d"
			"}",
			thead->sta, thead->chan, thead->net, thead->loc,
			thead->starttime, thead->endtime, thead->samprate, thead->nsamp );
}




