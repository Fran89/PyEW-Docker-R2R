
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: readstas.c 6318 2015-04-26 16:54:01Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/11 17:49:07  lombard
 *     Added support for location code, TYPE_CARLSTATRIG_SCNL and TYPE_TRIGLIST_SCNL
 *     messages.
 *     Removed OS2 support
 *
 *     Revision 1.1  2000/02/14 16:14:42  lucky
 *     Initial revision
 *
 *
 */

/*
 * readstas.c: Read the network stations from a file.
 *              1) Create a list of stations based on the input file.
 *		2) Sort the station array for calls to bsearch()
 */

/*******							*********/
/*	Functions defined in this source file				*/
/*******							*********/

/*	Function: ReadStations						*/
/*									*/
/*	Inputs:		Pointer to the Network structure		*/
/*									*/
/*	Outputs:	Updated station list(above)			*/
/*									*/
/*	Returns:	0 on success					*/
/*			Error code as defined in carlstatrig.h on 	*/
/*                      failure						*/
/*									*/
/*      Auxiliary function: IsComment					*/
/*									*/
/*	Inputs:		Pointer to a string				*/
/*									*/
/*	Outputs:	Comment determination				*/
/*									*/
/*	Returns:	1 if it's a comment line                        */
/*			0 if it's not a comment line			*/


/*******							*********/
/*	System Includes							*/
/*******							*********/
#include <stdio.h>
#include <stdlib.h>	/* malloc, qsort				*/
#include <string.h>	/* strcpy					*/

/*******							*********/
/*	Earthworm Includes						*/
/*******							*********/
#include <earthworm.h>	/* logit					*/

/*******							*********/
/*	CarlSubTrig Includes						*/
/*******							*********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*	Function: ReadStations						*/
int ReadStations( NETWORK* csuNet )
{
  char          keyword[MAXLINELEN];
  char		inBuf[MAXLINELEN];	/* Input buffer.		*/
  FILE*		staFile;	/* Pointer to the station file stream.	*/
  STATION       *sta;
  int		retVal = 0;	/* Return value for this function.	*/
  int           i, j, nsta;
  
  /*	Validate the input parameter					*/
  if ( csuNet->csuParam.staFile )
  {
    /*	Open the stations file						*/
    if ( staFile = fopen( csuNet->csuParam.staFile, "r" ) )
    {
      /* Count channels in the station file.
	 Ignore comment lines and lines consisting of all whitespace.
	 ***********************************************************/
      nsta = 0;
      while ( fgets( inBuf, MAXLINELEN - 1, staFile ) != NULL )
	if ( !IsComment( inBuf ) ) nsta++;

      rewind( staFile );

      /* 	Allocate the station list				*/
      if ( sta = (STATION *) calloc( (size_t) nsta, sizeof(STATION) ) )
      {
	/*	Properly initialize all the station structures		*/
	for ( i = 0; i < nsta; i++ )
	{
	  retVal = InitializeStation( &(sta[i]) );
	  if ( CT_FAILED( retVal ) )
	    break;
	}

	/* Read stations from the station list file into station array */
	i = 0;
	while ( fgets( inBuf, MAXLINELEN, staFile ) != NULL )
	{
	  /* 	Check for comments 					*/
	  if ( IsComment( inBuf ) ) continue;
	    
	  /*	Process the line					*/
	  if ( 6 != sscanf( inBuf, "%s %s %s %s %s %d", keyword, 
			    sta[i].staCode, sta[i].compCode, sta[i].netCode, 
			    sta[i].locCode, &(sta[i].timeToLive) ) )
	  {
	    logit( "et",
		   "carlSubTrig: Unable to find station parameters in '%s'.\n",
		   inBuf );
	    retVal = ERR_STA_READ;
	    break;
	  }
	  if ( sta[i].timeToLive > 0 )
	  {
	    /* Grab some space for the station trigger stack		*/
	    sta[i].triggers = 
	      ( STATRIG * ) malloc( (size_t)(sizeof( STATRIG ) * 
					     csuNet->nSlots ) );
	    if ( sta[i].triggers == ( STATRIG *) NULL )
	    {
	      logit( "et", "carlSubTrig: Cannot allocate a trigger stack\n" );
	      retVal = ERR_MALLOC;
	      break;
	    }
	    for ( j = 0; j < csuNet->nSlots; j++ )
	    {	      
	      sta[i].triggers[j].onTime = 0.0;
	      sta[i].triggers[j].offTime = 0.0;
	      sta[i].triggers[j].onEta = 0.0;
	      sta[i].triggers[j].sequence = -1;
	    }
	  }
	  /* ok, get ready for the next station */
	  i++;
	}
      }
      
      else 
      {
	logit( "et", "carlSubTrig: Cannot allocate the station array\n" );
	retVal = ERR_MALLOC;
      }
      
      /*	Close the input file					*/
      fclose( staFile );
      
      /*	Sort the station array so bsearch() will work		*/
      qsort( sta, (size_t) nsta, sizeof(STATION), CompareSCNs );

      logit("", "Sorted station list:\n");
      for ( i = 0; i < nsta; i++ )
	logit( "", "\t%s.%s.%s.%s %d\n", 
	       sta[i].staCode, sta[i].compCode, sta[i].netCode, sta[i].locCode,
	       sta[i].timeToLive );
      csuNet->stations = sta;
      csuNet->nSta = nsta;
      
    }
    else
    {
      logit( "et", "carlSubTrig: Error attempting to open '%s'.\n", 
	     csuNet->csuParam.staFile );
      retVal = ERR_STA_OPEN;
    }
  }
  else
  {
    logit( "et", "carlSubTrig: No stations file to open.\n" );
    retVal = ERR_STA_OPEN;
  }

  return ( retVal );
}


/***************************************************************************/

int IsComment( char string[] )
{
  int i;

  if ( string[0] == '\n' || string[0] == '\r') return 1; /* It's an empty line */
  
  for ( i = 0; i < (int)strlen( string ); i++ )
  {
    char test = string[i];

    if ( test!=' ' && test!='\t')
    {
      if ( test == '#' || test == '\r' || test == '\n' )
	return 1;          /* It's a comment line */
      else
	return 0;          /* It's not a comment line */
    }
  }
  return 1;                   /* It contains only whitespace */
}
