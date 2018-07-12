
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readsubs.c 1458 2004-05-11 17:49:07Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.6  2003/11/17 18:52:22  friberg
 *     Updated carlsubtrig for larger subnets (like Caltech) than just
 *     50 stations. I also added a more intelligent error reporting for
 *     the readsubs.c code when this limit might be reached.
 *
 *     Revision 1.5  2002/07/26 16:32:38  dietz
 *     logging change: add * before channels that don't count toward
 *     subnet trigger threshold.
 *
 *     Revision 1.4  2001/03/27 21:16:56  cjbryan
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/03/21 23:10:15  cjbryan
 *     added CVO subnet trigger
 *
 *     Revision 1.2  2000/06/10 04:15:47  lombard
 *     Fixed bug that caused crash on zero-length lines in config files.
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * readsubs.c: Read the subnet configuration from a file.
 *              1) Create a list of subnets based on the input file.
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: ReadSubnets						*/
/*									*/
/*	Inputs:		Pointer to a string(input filename)		*/
/*			Pointer to a network definition			*/
/*									*/
/*	Outputs:	Updated network definition(above)		*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlsubtrig.h on failure	*/

/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <stdlib.h>	/* malloc					*/
#include <string.h>	/* strcpy					*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

int IsSubnet( char * );

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: ReadSubnets						*/
int ReadSubnets( NETWORK* csuNet )
{
    char          subnetCode[MAXCODELEN]; /* subnet code (name).          */
    char		staCode[TRACE2_STA_LEN];	/* Station code (name).	*/
    char		compCode[TRACE2_CHAN_LEN];	/* Component code.	*/
    char		netCode[TRACE2_NET_LEN];	/* Network code.	*/
    char		locCode[TRACE2_LOC_LEN];	/* Location code	*/
    char		inBuf[MAXLINELEN];	/* Input buffer.		*/
    char		staList[MAXLINELEN];	/* Stations remaining to be	*/
    /*   parsed in stations string.	*/
    char*		nxttok;		/* token pointer to scan station list	*/
    STATION*	station;	/* station of interest			*/
    SUBNET*	subnet;		/* subnet of interest			*/
    FILE*		subFile;	/* Pointer to the subnet file stream.	*/
    int		retVal = 0;	/* Return value for this function.	*/
    int		i, j, nsub;
    int		triggerableFlag; /* CVO flag for subnet triggers	*/
    int		line_no;	/* for echoing line number during parsing */

    /*	Validate the input parameter					*/
    if ( csuNet->csuParam.subFile ) {
	/*	Open the subnets file						*/
	if ( subFile = fopen( csuNet->csuParam.subFile, "r" ) ) {
	    /* Count subnets in the file. */
	    nsub = 0;
	    while ( fgets( inBuf, MAXLINELEN - 1, subFile ) != NULL )
		if ( IsSubnet( inBuf ) ) nsub++;
	    
	    rewind( subFile );
	    
	    /* 	Allocate the subnet array				*/
	    if ( subnet = (SUBNET *) calloc( (size_t) nsub, sizeof(SUBNET) ) ) {
		/*	Properly initialize all the subnet structures		*/
		InitializeSubnet( subnet, nsub );
		
		/*	Read the subnets from the file				*/
		i = 0;
		line_no = 0;     /* line number for intelligent error message */
		while ( fgets( inBuf, MAXLINELEN - 1, subFile ) ) {
		    line_no++;
		    /* 	Look for Subnet lines 					*/
		    if ( ! IsSubnet( inBuf ) ) continue;
		    
		    /*	Process the line					*/
		    if ( 3 == sscanf( inBuf, "Subnet %s %d %[^\n]", subnetCode,
				      &(subnet[i].minToTrigger), staList ) ) {
			/* changed by Carol 3/21/01 to copy MAX_SUBNET_LEN chars 
			   of subnetCode into SUBNET structure			*/
			/* Copy first 3 chars of subnetCode into SUBNET structure */
			strncpy( subnet[i].subnetCode, subnetCode, MAX_SUBNET_LEN );
			if (strlen(subnet[i].subnetCode) == MAX_SUBNET_LEN)
			    subnet[i].subnetCode[MAX_SUBNET_LEN - 1] = '\0';
			
			/* Changed by Eugene Lublinsky, 3/31/Y2K */
			/* added CVO subnet trigger flag */
			triggerableFlag = 1;
			
			/* Scan the SCN list */
			nxttok = strtok( staList, " " );
			j = 0;
			while ( nxttok ) {
			    if (*nxttok == '|') triggerableFlag = 0;  /* divider reached */
			    else {
				if ( sscanf( nxttok, "%[^.].%[^.].%[^.].%s", staCode, 
					     compCode, netCode, locCode ) == 4 ) {
				    /* Look for staCode in the list 			*/
				    station = FindStation( staCode, compCode, netCode, 
							   locCode, csuNet );
				    if ( station ) {
					/* Using pointer arithmetic, add the station index	*/
					/* to the subnet's station array			*/
					subnet[i].stations[j] = (int) (station - csuNet->stations);
					subnet[i].triggerable[j] = triggerableFlag;
					j++;
					if ( j >= NSUBSTA ) {
					    logit( "e", "Too many stations for subnet %s; max is %d\n",
						   subnet[i].subnetCode, NSUBSTA );
					    retVal = ERR_SUB_READ;
					}
				    } else {
					/*	Station not found, report it		*/
					logit( "e", "Unable to find <%s.%s.%s.%s>, subnet %s\n",
					       staCode, compCode, netCode, locCode, subnet[i].subnetCode);
					retVal = ERR_SUB_READ;
				    }
				} else {	    /*	Encountered a parsing problem		*/
				    logit( "e", "ReadSubnets: Failed to parse next station "
					   "from line %d subnet list: '%s'.\n",
					   line_no, subnetCode );
				    retVal = ERR_SUB_READ;
				}
				/* Continue only if no error during station list parsing	*/
				if ( CT_FAILED( retVal ) )
				    break;
			    }  /* if '|' */
			    nxttok = strtok( (char*)NULL, " " );
			}  /* while( nxttok ) */
			subnet[i].nStas = j;
			i++;
		    } else {  /* if ( 3 == scanf... */
			logit( "e", "Unable to parse subnet parameters in %s\n", inBuf );
			retVal = ERR_SUB_READ;
			break;
		    }
		}	/* while ( fgets... */
		csuNet->nSub = nsub;
		csuNet->subnets = subnet;
		
		/* Adjust numSubAll if it wasn't set by config file */
		if ( csuNet->numSubAll == 0 ) csuNet->numSubAll = nsub;
		
		if ( csuNet->csuParam.debug ) {
		    logit( "", " Read %d subnets:\n", csuNet->nSub );
		    logit( "", " Channels preceded by * do not count toward subnet trigger threshold\n" );
		    for ( i = 0; i < csuNet->nSub; i++ ) {
			logit( "", "%4d   %-3s   %2d   ", 
			       i, csuNet->subnets[i].subnetCode, 
			       csuNet->subnets[i].minToTrigger );
			for ( j = 0; j < csuNet->subnets[i].nStas; j++ ) {
			    if( !csuNet->subnets[i].triggerable[j] ) logit( "", "*" );
			    logit( "", "%s.%s.%s.%s ", 
				   csuNet->stations[csuNet->subnets[i].stations[j]].staCode,
				   csuNet->stations[csuNet->subnets[i].stations[j]].compCode,
				   csuNet->stations[csuNet->subnets[i].stations[j]].netCode, 
				   csuNet->stations[csuNet->subnets[i].stations[j]].locCode );
			}
			logit( "", "\n" );
		    }
		}
		
	    } else {
		logit( "e", "carlsubtrig: Error allocating subnet memory." );
		retVal = ERR_MALLOC;
	    }
	    /*	Close the input file					*/
	    fclose( subFile );
	} else {
	    logit( "e", "carlsubtrig: Error attempting to open '%s'.\n", 
		   csuNet->csuParam.subFile );
	    retVal = ERR_SUB_OPEN;
	}
    } else {
	logit( "e", "carlsubtrig: No subnets file to open.\n" );
	retVal = ERR_SUB_OPEN;
    }
    
    return ( retVal );
}

/***************************************************************************/

int IsSubnet( char string[] )
{
    int i;
    char Subnet[] = "Subnet";
    char space = ' ';
    char tab = '\t';
  
    if (strlen(string) < strlen(Subnet)) return 0;

    for ( i = 0; i < (int)strlen( Subnet ); i++ )
	if ( string[i] != Subnet[i] ) return 0;

    /* Make sure it's not SubnetContrib or some such...			*/
    if ( string[i] == space || string[i] == tab )
	return 1;

    return 0;
}
