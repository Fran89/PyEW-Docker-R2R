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
**    $Id: ew2mseed_log.c 4857 2012-06-19 18:01:15Z ilya $
** 
**	Revision history:
**	$Log$
**	Revision 1.1  2010/03/10 17:14:58  paulf
**	first check in of ew2mseed to EW proper
**
**	Revision 1.4  2007/04/12 19:21:04  hal
**	Added ew7.0+ support (-dSUPPORT_SCNL) and linux port (-D_LINUX)
**	
**	-Hal
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
 * Revision 1.3  2001/01/17  21:53:24  comserv
 * prerelease version; copyright notice
 *
 * Revision 1.2  2001/01/08  22:55:08  comserv
 * New parameters
 *
 * Revision 1.1  2000/11/17  06:53:18  comserv
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



/* Local includes */
#include "ew2mseed.h"


  int ew2mseed_logconfig(RINGS *rn)
  {
	  int i;
	  struct WS_ring *ws_head;	
	  struct SCN_ring *scn_head;

          ws_head = rn->wsRing;
	  scn_head = rn->scnRing;

	  logit("", "		MseedDir          = %s\n", rn->MseedDir); 
	  logit("", "		configFile        = %s\n", rn->configFile);
	  logit("", "		Verbosity         = %d\n", (int) rn->verbosity); 
	  logit("", "		logSwitch         = %d\n", rn->logSwitch);
	  logit("", "		GapThresh         = %d\n", rn->GapThresh); 
 	  logit("", "		SocketReconnect   = %d s\n", rn->TimeoutSeconds); /* IGD 03/15/01 updated */
  	  logit("", "		Travel Tm-out     = %d s\n", rn->TravelTimeout);  
  	  logit("", "		Start Time (s)    = %d\n", rn->julStartTime);
	  logit("", "		LockFile          = %s\n", ((rn->LockFile == NULL) ? "NULL" : rn->LockFile));
	  logit("", "		UsePriority       = %s\n",  ((rn->usePriority == 0) ? "NO" : "YES"));
	  logit("", "		Records num-r     = %d\n", rn->RecNum);
	  if (rn->usePriority == 1)
	  {
  	  	logit("", "           PriorityHighWater = %d\n", rn->PriorityHighWater); 
          	logit("", "           LoopsBeforeService= %d\n", rn->LoopsBeforeService);
	  }
          logit("", "----------------------\n");
          for (i = 0; i< rn->WS_num; i++)  	
          {
  		logit("", "		WaveServer-%d   = %s:%s \n",
			i, rn->wsRing->wsIP, rn->wsRing->wsPort);
		rn->wsRing = rn->wsRing->next;
	  }
   	  logit("", "-----------------------\n");
	   logit ("", "		              STN    CHAN     NET   Loc.ID Rec. length Compr.\n");
          for (i = 0; i< rn->SCN_num; i++)	
          {
  		logit("", "		SCN-%d   = %7s %7s %7s %7s %5d %2d \n",
			i, rn->scnRing->traceRec.sta, rn->scnRing->traceRec.chan, rn->scnRing->traceRec.net,
			   rn->scnRing->locId, rn->scnRing->logRec,
				rn->scnRing->CompAlg);
		rn->scnRing = rn->scnRing->next;  
	  }
   	  logit("", "=========================\n");
	  
  rn->wsRing = ws_head;
  rn->scnRing = scn_head;

  return EW_SUCCESS;
  }

void LogWsErr( char fun[], int rc )
{
	switch ( rc )
	{
		
		case WS_ERR_NONE:
			logit("pt", "%s: No errors.\n", fun );
			break;

		case WS_ERR_INPUT:
			logit("pt", "%s: Bad input parameters.\n", fun );
			break;

		case WS_ERR_EMPTY_MENU:
			logit("pt", "%s: Empty menu.\n", fun );
			break;

		case WS_ERR_SERVER_NOT_IN_MENU:
			logit("pt", "%s: Empty menu.\n", fun );
			break;

#ifdef SUPPORT_SCNL
		case WS_ERR_SCNL_NOT_IN_MENU:
			logit("pt", "%s: SCNL not in menu.\n", fun );
			break;
#else
		case WS_ERR_SCN_NOT_IN_MENU:
			logit("pt", "%s: SCN not in menu.\n", fun );
			break;
#endif
		case WS_ERR_BUFFER_OVERFLOW:
			logit("pt", "%s: Buffer overflow.\n", fun );
			break;

		case WS_ERR_MEMORY:
			logit("pt", "%s: Out of memory.\n", fun );
			break;

		case WS_ERR_PARSE:
			logit("pt", "%s: Could not parse server reply.\n", fun );
			break;

		case WS_ERR_TIMEOUT:
			logit("pt", "%s: Socket transaction timed out.\n", fun );
			break;

		case WS_ERR_BROKEN_CONNECTION:
			logit("pt", "%s: The connection broke.\n", fun );
			break;

		case WS_ERR_SOCKET:
			logit("pt", "%s: Could not get a connection.\n", fun );
			break;

		case WS_ERR_NO_CONNECTION:
			logit("pt", "%s: Could not get a connection.\n", fun );
			break;

		case EW2MSEED_TOO_EARLY:
			logit("pt", "%s: Requested end time is not in the tank yet.\n", fun );
			break;

		case EW2MSEED_ACT_START_TIME_AFTER_REQ:
			logit("pt", "%s: Actual start time is AFTER requested.\n", fun );
			break;

		case EW2MSEED_ACT_END_TIME_BEFORE_REQ:
			logit("pt", "%s: Actual end time is BEFORE requested.\n", fun );
			break;

		case TRACE_REQ_EMPTY:
			logit("pt", "%s: Trace request buffer is empty.\n", fun );
			break;

		case WS_DATA_BUF_EMPTY:
			logit("pt", "%s:   Wave server data buffer is empty.\n", fun );
			break;	

		case WRONG_DATA_TYPE:
			logit("pt", "%s: Unknown datatype of data from WS.\n", fun );
			break;		
		case WS_WRN_FLAGGED:
			logit ("pt", "%s: Wave server returned error flag\n", fun);
			break;

		default:
			logit("pt", "%s: unknown ws_client error: %d.\n", fun, rc );
			break;
    }

  return;
}
