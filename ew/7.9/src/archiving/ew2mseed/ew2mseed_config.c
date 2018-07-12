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
**    $Id: ew2mseed_config.c 6803 2016-09-09 06:06:39Z et $
** 
**	Revision history:
**	$Log$
**	Revision 1.1  2010/03/10 17:14:58  paulf
**	first check in of ew2mseed to EW proper
**
**	Revision 1.5  2007/04/12 19:21:04  hal
**	Added ew7.0+ support (-dSUPPORT_SCNL) and linux port (-D_LINUX)
**	
**	-Hal
**	
**	Revision 1.4  2003/05/16 01:04:18  ilya
**	Added StartLatency
**	
**	Revision 1.3  2002/04/11 20:07:13  ilya
**	Add disabling of catchup mechanism
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
 * Revision 1.4  2001/03/15  16:39:51  comserv
 * Modifications to add configurable SocketReconnect and to
 * reduce a frequency of menu updates
 *
 * reduce a frequency of menu updates
 * New function  void ew2mseedUpdateTimeout ()
 *
 * Revision 1.3  2001/01/17  21:50:05  comserv
 * Prerelease version; new copyright; new parameters
 *
 * Revision 1.2  2001/01/08  22:54:29  comserv
 * Added the parameters
 *
 * Revision 1.1  2000/11/17  06:55:17  comserv
 * Initial revision
 *
**
*/

/* Standard includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Includes from Earthworm distribution */
#include "earthworm.h"
#include "kom.h"

/* Local includes */
#include "ew2mseed.h"

#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff  /* should be in <netinet/in.h> */
#endif


  int ew2mseed_config (RINGS *rn)	
  {
	char *com;		
	char *algLine;	
	char *tmpWord;
	char tmpWord1[1000];
	char tmpWord2[10];
	int nfiles;
	int success;

	FILE *fid;
	struct WS_ring *ws_head;    /* this holds the head of the WS linked_list */
	struct SCN_ring *scn_head; /* this holds the head of the SCH linked_list */

	/* Open the main configuration file 
 	**********************************/
	
	nfiles = k_open(rn->configFile);
	if ( nfiles == 0 ) 
	{
		fprintf( stderr,
			"ew2mseed: Error opening command file <%s>; exitting!\n", 
							rn->configFile );
		return EW_FAILURE;
	}


/* Initialize the RINGS main structure  
 **********************************/

	/* Initialize WaveServer structure */
	rn->wsRing = (struct WS_ring *) calloc(1, sizeof (struct WS_ring));
	ws_head = rn->wsRing;   /* Reference to the head of the linked list */
	rn->wsRing->wsIP = (char *) NULL;
	rn->wsRing->wsPort = (char *) NULL;
	rn->wsRing->next = (struct WS_ring *) NULL;
	
	/* Initialize SCN structure */
	
	rn->scnRing = (struct SCN_ring *) calloc(1, sizeof (struct SCN_ring));
	scn_head = rn->scnRing;   /* refrence to the head of linked list */
	rn->scnRing->next = (struct SCN_ring *) NULL;		

	rn->WS_num = 0;    /* Set Wave server counter */
	rn->SCN_num = 0;   /*Set SCN counter */ 
	
	/* set default values for various parameters */
	rn->MseedDir = (char *)calloc(1, 3);
	rn->verbosity = 1;
	strcpy(rn->MseedDir, "./");
	rn->logSwitch = TRUE;
	rn->GapThresh = 20;
	rn->TimeoutSeconds = 3600;  /* IGD 03/14/01 This is the period of
					 socket reconnections by default*/
	rn->TravelTimeout = 30;
	rn->LoopsBeforeService = 50;
	rn->usePriority = 0;	/* default : NO */
	rn->PriorityHighWater = 5;
	rn->StartTime = (char *)calloc (1, 18);
	strcpy (rn->StartTime, STR1970); 
	rn->julStartTime = julsec17(rn->StartTime) - SEC1970 ;
	rn->LockFile = NULL;
	rn->RecNum = 1; /* A single record */

	/* Process all command files
	 ***************************/
	while(nfiles > 0)	/* While there are command files open */
	{  
  
        	while(k_rd())  	      /* Read next line from active file  */
        	{
			com = k_str();         /* Get the first token from line */

 			/* Ignore blank lines & comments
			*******************************/
    		        if( !com )           continue;
			if( com[0] == '#' )  continue;	

			/* Open a nested configuration file 
			**********************************/
			if( com[0] == '@' ) 
			{
				success = nfiles+1;
				nfiles  = k_open(&com[1]);
				if ( nfiles != success )
				{
					fprintf( stderr, 
						"ew2mseed: Error opening command file <%s>; exitting!\n",
							&com[1] );
                  			return EW_FAILURE;
				}
               			continue;
			}


			/* Process anything else as a command 
			************************************/
  /*0*/     		if( k_its("LogFile") ) 
            		{
				rn->logSwitch = k_int();
			}
/*1*/			else if( k_its("SCNLocSz") ) 	/* Filling the SCN linked list */
			{
				strcpy (rn->scnRing->traceRec.sta, k_str());
				strcpy (rn->scnRing->traceRec.chan, k_str());
				strcpy (rn->scnRing->traceRec.net, k_str());
				rn->scnRing->traceRec.pinno = -1;
				rn->scnRing->tankStarttime = -1;
				rn->scnRing->tankEndtime = -1;
				rn->scnRing->prevStarttime = -1;
				rn->scnRing->prevEndtime = -1;
				rn->scnRing->lastRequestTime = -1;	/* IGD 03/29/02 for catchup */
				rn->scnRing->priority = 1;		/*  IGD 03/29/02 For catchup */
				rn->scnRing->mseedSeqNo = 1;
				rn->scnRing->gapsCount = 0;
				rn->scnRing->CompAlg = STEIM1;
				rn->scnRing->dir = (char *) NULL; 
				rn->scnRing->curMseedFile = (char *) NULL; 
  
				strcpy (tmpWord2, k_str());
				/* Replace a dummy location code with blank 
				 ***************************************/
				if (strlen(tmpWord2) > 3) { /* That is NONE */
					strcpy(rn->scnRing->locId, ""); /* Empty location ID */
#ifdef SUPPORT_SCNL
					strcpy(rn->scnRing->traceRec.loc, "");
#endif
				} else {
					strcpy(rn->scnRing->locId, tmpWord2); /* Location ID is real */
#ifdef SUPPORT_SCNL
					strcpy(rn->scnRing->traceRec.loc, tmpWord2);
#endif
				}
				rn->scnRing->logRec = k_int();
				algLine = k_str();  /* Get next token */
				if (k_its("STEIM1"))
					rn->scnRing->CompAlg = STEIM1;
				else if (k_its("STEIM2"))
					rn->scnRing->CompAlg = STEIM2;
				else 
				{
					fprintf (stderr,
			  			"ew2mseed: Compression Algorithm %s is not supported\n",
			  			 algLine );
					exit(-11);
					/* must exit now */
				}
			
	
				rn->SCN_num++;
				rn->scnRing->next = (struct SCN_ring *) calloc(1, sizeof (struct SCN_ring ));
				rn->scnRing = rn->scnRing->next;
				rn->scnRing->next = (struct SCN_ring *)	 NULL;				
    		        }
/*2*/			else if( k_its("StartTime") ) 
			{

				/*	IGD 11/27/2012 No need to free			if (rn->StartTime)
				{
					free (rn->StartTime);
				}*/
				rn->StartTime = k_str();
				rn->julStartTime = julsec17(rn->StartTime) - SEC1970;
			 	if (strlen(rn->StartTime) != 17)
				{
					fprintf(stderr, "StartTime must contain 17 symbols, not %d", (int)strlen(rn->StartTime));
					exit(-15);
				}
			}
			/* IGD 05/15/03 Added latency at the DMC request */
/*2a*/			else if( k_its("StartLatency") ) 
			{
				int latency;
				latency = k_int() * 3600;  /* Latency in hours converted to seconds*/
				rn->julStartTime =  time(NULL) - latency;
				
			}	
/*3*/			else if( k_its("GapThresh") ) 
			{
				rn->GapThresh = k_int();
			}
/*4*/			else if( k_its("LockFile") ) 
			{
				tmpWord = k_str();
				rn->LockFile = (char *) calloc (strlen(tmpWord)+1, 1);
				strcpy(rn->LockFile, tmpWord);
			}
            
/*5*/			else if( k_its("WaveServer") )  /* Filling WS linked list */
			{	
				rn->wsRing->wsIP = format_IP(k_str());
				if (rn->wsRing->wsIP == NULL)
					exit(-2);		
				tmpWord = k_str();
				rn->wsRing->wsPort = (char *) calloc(1, strlen(tmpWord) + 1);
				strcpy(rn->wsRing->wsPort, tmpWord) ;
				rn->WS_num++;
				rn->wsRing->next = (struct WS_ring *) calloc(1, sizeof (struct WS_ring ));
				rn->wsRing = rn->wsRing->next;
				rn->wsRing->wsIP = (char *) NULL;
				rn->wsRing->wsPort = (char *) NULL;
				rn->wsRing->next = (struct WS_ring *) NULL;
			}

/*6*/ 			else if( k_its("SocketReconnect") )  /* IGD 03/14/01 Used to be "TimeoutSeconds" */
			{
       			         rn->TimeoutSeconds = k_int();
			}

/*7*/			else if( k_its("TravelTimeout") ) 
			{
				rn->TravelTimeout = k_int();
			}
/*8*/			else if( k_its("Verbosity") ) 
			{
				rn->verbosity = k_int();
			}
/*9*/			else if( k_its("UsePriority") ) 
			{
				rn->usePriority = k_int();
				if (rn->usePriority != 1)
					rn->usePriority = 0;
			}
/*10*/			else if( k_its("RecordsNumber") ) 
			{
				rn->RecNum = k_int();
			}
/*11*/                  else if(k_its("PriorityHighWater") )
			{
                                rn->PriorityHighWater = k_int();
                        }
/*12*/                  else if(k_its("LoopsBeforeService") )
                        {
                                rn->LoopsBeforeService = k_int();
                        }

/*13*/			else if (k_its ("MseedDir") )
			{
				tmpWord = k_str();
				rn->MseedDir = realloc(rn->MseedDir, strlen(tmpWord) +1);
				strcpy(rn->MseedDir, tmpWord);	
				/* Do a simple test: see if we can read and write into the dir 
				****************************************************************/	
 				sprintf(tmpWord1, "%s/%s", rn->MseedDir, ".IGDtest");
				fid = fopen(tmpWord1, "w+");
				if (fid == NULL)	
				{
					perror("fopen: ");
					printf("ew2mseed_config fatal error: cannot open files in %s\n",
						rn->MseedDir);
					return EW_FAILURE;
				}
				else
				{
					fclose(fid); unlink(tmpWord1);
				}	
			}	 	

			/* Unknown command
			*****************/ 
			else 
			{
				fprintf( stderr, "ew2mseed_config warning: <%s> Unknown command in <%s>.\n", 
    			                     com, rn->configFile );
				continue;
			}
	
			/* See if there were any errors processing the command 
			*****************************************************/
			if( k_err() ) 
			{
				fprintf( stderr, 
					"ew2mseed: Bad <%s> command in <%s>; exitting!\n",
					com, rn->configFile );
				return EW_FAILURE;
			}
		}  /* End of k_rd() loop */

		nfiles = k_close();

	} /* End of config files loop */

	/*IGD 04/11/02 If we don't use priority, HighWater is 1 and the 
	 * values in the computation of buffer size are unaffected by High Water parameter */

	if (rn->usePriority != 0)
		rn->PriorityHighWater = 1;

	rn->wsRing = ws_head; /* the head of wsRing address */
	rn->scnRing = scn_head; /* the head of scnRing address*/

	return EW_SUCCESS; 
  } 

  char *  format_IP(char *addr_p)	
  {  
	char *addr;
	struct hostent  *host;
	struct in_addr addr_s;
 
	if ( inet_addr(addr_p) == INADDR_NONE )
	{ 
		/* it's not a dotted quad address 
		********************************/
		if ( (host = gethostbyname(addr_p)) == NULL)
		{
			fprintf(stderr, "ew2mseed_config fatal error: bad EW server address <%s>\n", 
				addr_p );
			return 0x0;
		}
		memcpy((char *) &addr_s, host->h_addr,  host->h_length);
		addr_p = inet_ntoa(addr_s);
 	}

	if (strlen(addr_p) > 15) 
 	{
		fprintf(stderr, "ew2mseed_config fatal error: bad EW server address <%s>\n", addr_p);
		return 0x0;
 	}
 	else 
 	{	
		addr = (char *) calloc(1, strlen(addr_p) +1) ;
		strcpy(addr, addr_p);
  		return (addr);
	}
  }
   /*********************************************************************  
   * IGD 03/14/01
   * We put the value of rn->TravelTimeout in each rn->scnRing
   **********************************************************************/
  void ew2mseedUpdateTimeout (RINGS *rn)
  {
	int i;
	struct SCN_ring *scn_head;

	scn_head = rn->scnRing;
	for (i=0; i < rn->SCN_num; i++)
	{
		rn->scnRing->traceRec.timeout = rn->TravelTimeout;
		rn->scnRing = rn->scnRing->next;
	}
	rn->scnRing = scn_head;
	return;
  }
