/* 
// ======================================================================
// Copyright (C) 2000-2003 Instrumental Software Technologies, Inc. (ISTI)
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. If modifications are performed to this code, please enter your own 
// copyright, name and organization after that of ISTI.
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in
// the documentation and/or other materials provided with the
// distribution.
// 3. All advertising materials mentioning features or use of this
// software must display the following acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com)"
// 4. If the software is provided with, or as part of a commercial
// product, or is used in other commercial software products the
// customer must be informed that "This product includes software
// developed by Instrumental Software Technologies, Inc.
// (http://www.isti.com)"
// 5. The names "Instrumental Software Technologies, Inc." and "ISTI"
// must not be used to endorse or promote products derived from
// this software without prior written permission. For written
// permission, please contact "info@isti.com".
// 6. Products derived from this software may not be called "ISTI"
// nor may "ISTI" appear in their names without prior written
// permission of Instrumental Software Technologies, Inc.
// 7. Redistributions of any form whatsoever must retain the following
// acknowledgment:
// "This product includes software developed by Instrumental
// Software Technologies, Inc. (http://www.isti.com/)."
// 8. Redistributions of source code, or portions of this source code,
// must retain the above copyright notice, this list of conditions
// and the following disclaimer.
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
*/ 

/*
  Changes:
    7-28-2003 - HJS 
      * added tankProps datatype, and populate one for the tank we're
        looking at.
      * switched to dynamic (and more intelligent) memory allocation for
        buffer size
      * made all non-error output go to stdout.  Memory allocation errors
        and server errors still go to stderr
*/
      
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <earthworm.h>
#include "ew2mseed.h"

#define CHUNKSIZE 200
#define TIME_SANITY 100
#define GAP_FILL -9999
#define MIN_GAP_SIZE 3
/* account for ascii values, spaces, negative sign etc... */
#define MALLOC_BYTES_PER_SAMPLE 10 
typedef struct s_config {
  int chunkSize;
  int sanityThreshold;
  int gapFiller;
  int minGapSize;
  int verbose;
  char ipAddress[16];
  char port[10];
  char station[7];
  char channel[9];
} config;

typedef struct s_tankProps {
  double tankStart;
  double tankEnd;
  double samprate;
  char   station[7];
  char   channel[9];
} tankProps;

config globalConfig;

void handleError(int errCode, const char *errFunc) {
    switch(errCode) {
        case WS_ERR_NO_CONNECTION:
            fprintf(stderr, "%s() : Could not connect to server\n", errFunc);
            break;
        case WS_ERR_SOCKET:
            fprintf(stderr, "%s() : Could not create socket or socket error\n", errFunc);
            break;
        case WS_ERR_BROKEN_CONNECTION:
            fprintf(stderr, "%s() : Connection broken\n", errFunc);
            break;
        case WS_ERR_TIMEOUT:
            fprintf(stderr, "%s() : Connection timed out\n", errFunc);
            break;
        case WS_ERR_MEMORY:
            fprintf(stderr, "%s() : Out of memory\n", errFunc);
            break;
        case WS_ERR_INPUT:
            fprintf(stderr, "%s() : Bad or missing input params\n", errFunc);
            break;
        case WS_ERR_PARSE:
            fprintf(stderr, "%s() : Error parsing server output\n", errFunc);
            break;
        case WS_WRN_FLAGGED:
            fprintf(stderr, "%s() : Wave server returned error flag\n", errFunc);
            break;
        case WS_ERR_EMPTY_MENU:
            fprintf(stderr, "%s() : Menu empty\n", errFunc);
            break;
#ifdef SUPPORT_SCNL
        case WS_ERR_SCNL_NOT_IN_MENU:
            fprintf(stderr, "%s() : Requested SCNL not in menu\n", errFunc);
            break;
#else
        case WS_ERR_SCN_NOT_IN_MENU:
            fprintf(stderr, "%s() : Requested SCN not in menu\n", errFunc);
            break;
#endif
        case WS_ERR_BUFFER_OVERFLOW:
            fprintf(stderr, "%s() : Trace buffer overflow\n", errFunc);
            break;
        default:
            fprintf(stderr, "%s() : Unknown error\n", errFunc);
            break;
    }
}


/*
 * search for gaps in the data
 */
void gapCheck(TRACE_REQ data) {
  char gapStr[10];
  char *curPos = data.pBuf;
  char curValue[20];
  long bytesScanned = 0;
  long samplesScanned = 0;
  long gapSize = 0;

  sprintf(gapStr, "%d", globalConfig.gapFiller);
  while(bytesScanned < data.actLen) {
    if(*curPos == ' ') {
      // evaluate curValue, checking for gaps
      if(!strcmp(curValue, gapStr)) {
	// this is a gap
	gapSize++;
      } else {
	// this isn't a gap, or it's the end of a gap
	if(gapSize) {
	  // this is the end of a gap
	  if(gapSize >= globalConfig.minGapSize) {
	    // it's a gap we care about
	    double timeOfGapStart = data.actStarttime + ( (samplesScanned - gapSize) / data.samprate);
	    time_t when = timeOfGapStart;
	    fprintf(stdout, "Gap of %ld samples starting at %s\n", gapSize, ctime(&when));
	  }
	  gapSize = 0;
	} 
      }
      samplesScanned++;
      curValue[0] = '\0';
    } else {
      // continue to build the current value
      strncat(curValue, curPos, 1);
    }
    bytesScanned++;
    curPos++;
  }
}

/*
 * Kick off a sanity check of the particular menu item.
 * We make several requests through the data, and look
 * for gaps, as well as look for very strange deltas between
 * the data we asked for, and the data we got.
 */
#ifdef SUPPORT_SCNL
void sanityCheck(WS_MENU_QUEUE_REC thisMenu, WS_PSCNL thisPSCN, tankProps thisTank) {
#else
void sanityCheck(WS_MENU_QUEUE_REC thisMenu, WS_PSCN thisPSCN, tankProps thisTank) {
#endif
    TRACE_REQ request;
    int result;
    char *traceBuf = NULL;
    long traceBufSize;
    double prevChunkEndtime = thisPSCN->tankStarttime + 15;
    double timeDelta;
    time_t rStart;
    time_t rEnd;
    time_t aStart;
    time_t aEnd;

    char rStartStr[30];
    char rEndStr[30];
    char aStartStr[30];
    char aEndStr[30];

    char tankStartStr[30];
    char tankEndStr[30];
    time_t tankStart = thisTank.tankStart;
    time_t tankEnd = thisTank.tankEnd;
    strncpy(tankStartStr, ctime(&tankStart), 24);
    strncpy(tankEndStr, ctime(&tankEnd), 24);

    printf("SCN: %s %s %s\n", thisPSCN->sta, 
                              thisPSCN->chan, 
                              thisPSCN->net);
    printf("Start: %s\nEnd: %s\nSPS: %f\n", 
           tankStartStr, 
           tankEndStr, 
           thisTank.samprate);

    while(prevChunkEndtime <= thisPSCN->tankEndtime) {
        /*
         * set up the request
         */
        memcpy(request.sta, thisPSCN->sta, 7);
        memcpy(request.chan, thisPSCN->chan, 9);
        memcpy(request.net, thisPSCN->net, 9);
        request.pinno = thisPSCN->pinno;
        request.reqStarttime = prevChunkEndtime;
        prevChunkEndtime += globalConfig.chunkSize;
        request.reqEndtime = (prevChunkEndtime > thisPSCN->tankEndtime ? thisPSCN->tankEndtime : prevChunkEndtime);
        request.partial = 1; 
        if(traceBuf) {
          free(traceBuf);
        }
        /* allocate twice the required memory, to account for wandering
           "actual" start/end times
        */
        traceBufSize = (request.reqEndtime - request.reqStarttime) * MALLOC_BYTES_PER_SAMPLE * thisTank.samprate * 2;
        traceBuf = malloc(traceBufSize);
        if(!traceBuf) {
          /* malloc failed */
          fprintf(stderr, "Unable to allocate buffer memory, exiting\n");
          exit(1);
        }
    
        request.pBuf = traceBuf;
        request.bufLen = traceBufSize;
        request.timeout = 30;
        request.fill = GAP_FILL;

        /*
         * only wsGetTraceBin() gives us the actual endtime, so we'll
         * need to use that here.
         */
        result = wsGetTraceBin(&request, &thisMenu, 30000);
        if(result != WS_ERR_NONE) {
            handleError(result, "wsGetTraceBin");
            wsKillMenu(&thisMenu);
            exit(1);
        }  

	/*
         * Sanity checking.  Requested times vs. Actual times
	 */
	rStart = request.reqStarttime;
	rEnd = request.reqEndtime;
	aStart = request.actStarttime;
	aEnd = request.actEndtime;
       
	strcpy(rStartStr, ctime(&rStart)); 
	strcpy(rEndStr, ctime(&rEnd)); 
	strcpy(aStartStr, ctime(&aStart)); 
	strcpy(aEndStr, ctime(&aEnd)); 

	if(globalConfig.verbose) {
	  printf("reqStart: %sactStart: %s\n", rStartStr, aStartStr); 
	  printf("reqEnd: %sactEnd: %s\n", rEndStr, aEndStr); 
	  
	}

        timeDelta = request.reqStarttime - request.actStarttime;
        if(timeDelta > globalConfig.sanityThreshold || timeDelta < (0 - globalConfig.sanityThreshold)) {
            fprintf(stdout, "Sanity error: \n\tDelta: %f\n\t reqStart: %s\n\t actStart: %s\n", 
		    timeDelta, rStartStr, aStartStr); 
        }

        timeDelta = request.reqEndtime - request.actEndtime;
        if(timeDelta > globalConfig.sanityThreshold || timeDelta < (0 - globalConfig.sanityThreshold)) {
            fprintf(stdout, "Sanity error: \n\tDelta: %f\n\t reqEnd: %s\n\t actEnd: %s\n",
		    timeDelta, rEndStr, aEndStr); 
        }


        /*
         * only wsGetTraceAscii() allows a fill value, so we need to
         * use that for gap detection
         */
        result = wsGetTraceAscii(&request, &thisMenu, 30000);
        if(result != WS_ERR_NONE) {
            handleError(result, "wsGetTraceAscii");
            wsKillMenu(&thisMenu);
            if(traceBuf) {
              free(traceBuf);
            }
            exit(1);
        }   

        if(traceBuf) {
          free(traceBuf);
        }
	gapCheck(request);
    }  

}

void usageAndExit() {
  printf("wsSniffer [-sane SANITY_SECONDS] [-mingap MINIMUM_GAP] [-v] IP Port Sta Chan\n");
  exit(0);
}

void parseCommandLine(int argc, char *argv[]) {
  int i;
  char *thisArg;

  if(argc < 5) {
    usageAndExit();
  }

  strncpy(globalConfig.ipAddress, argv[argc-4], 16);
  strncpy(globalConfig.port, argv[argc-3], 8);
  strncpy(globalConfig.station, argv[argc-2], 7);
  strncpy(globalConfig.channel, argv[argc-1], 9);
  for(i=1; i < argc-4; i++) {
    thisArg = argv[i];
    if(thisArg[0] != '-') {
      usageAndExit();
    } else if(!strcmp(thisArg, "-v")) {
      globalConfig.verbose = 1;
    } else if(!strncmp(thisArg, "-sane", 5)) {
      int saneval;
      if(strlen(thisArg) == 5) {
	i++;
	if(sscanf(argv[i], "%d", &saneval) != 1) {
	  usageAndExit();
	}
      } else {
	if(sscanf(&thisArg[5], "%d", &saneval) != 1) {
	  usageAndExit();
	}
      }
      globalConfig.sanityThreshold = saneval;
    } else if(!strncmp(thisArg, "-mingap", 7)) {
      int mingap;
      if(strlen(thisArg) == 7) {
	i++;
	if(sscanf(argv[i], "%d", &mingap) != 1) {
	  usageAndExit();
	}
      } else {
	if(sscanf(&thisArg[5], "%d", &mingap) != 1) {
	  usageAndExit();
	}
      }
      globalConfig.minGapSize = mingap;
    }
  }

  if(globalConfig.verbose) {
    printf("Using sanity threshold of %d seconds\n", globalConfig.sanityThreshold);
  }
  
  if(globalConfig.verbose) {
    printf("Using minimum gap size of %d samples\n", globalConfig.minGapSize);
  }
  

}

int main(int argc, char *argv[]) {

    WS_MENU_QUEUE_REC ourMenu;
#ifdef SUPPORT_SCNL
    WS_PSCNL thisMenuItem;
#else
    WS_PSCN thisMenuItem;
#endif
    int result;

    /*
    char rStartStr[30];
    char rEndStr[30];
    time_t tankStart;
    time_t tankEnd;
    */

    globalConfig.chunkSize = CHUNKSIZE;
    globalConfig.sanityThreshold = TIME_SANITY;
    globalConfig.gapFiller = GAP_FILL;
    globalConfig.minGapSize = MIN_GAP_SIZE;
    globalConfig.verbose = 0;
    parseCommandLine(argc, argv);

    ourMenu.head = NULL;
    ourMenu.tail = NULL;

    logit_init("wsSniffer", 0, 1024, 0);

    setWsClient_ewDebug(1);
    result = wsAppendMenu(globalConfig.ipAddress, globalConfig.port, &ourMenu, 30000);

    if(result != WS_ERR_NONE) {
        handleError(result, "wsAppendMenu");
        wsKillMenu(&ourMenu);
        exit(1);
    }
    
#ifdef SUPPORT_SCNL
    thisMenuItem = ourMenu.head->pscnl;
#else
    thisMenuItem = ourMenu.head->pscn;
#endif
    /* IGD Let's print tank start/end time */
    /* 
       HS - We can't print these out yet, we dont know which
       tank we're supposed to be looking at yet.
    */
    /*
      tankStart = (time_t)thisMenuItem->tankStarttime;
      tankEnd = (time_t)thisMenuItem->tankEndtime; 
      strcpy(rStartStr, ctime((const time_t *)&(tankStart))); 
      strcpy(rEndStr, ctime((const time_t *)&(tankEnd))); 
      printf("Tank start time = %sTank end time = %s\n", rStartStr, rEndStr);
    */


    /* traverse through the menu */
    while(thisMenuItem) {
	if(!strcmp(thisMenuItem->sta, globalConfig.station) &&
	   !strcmp(thisMenuItem->chan, globalConfig.channel)) {

          tankProps thisTank;

          // lets get the sample rate of this tank                                                           
          TRACE_REQ request;
	  // insanely large buffer, so we don't overflow
          char buf[1024 * 10];
          strcat(thisTank.station, thisMenuItem->sta);
          strcat(thisTank.channel, thisMenuItem->chan);
          thisTank.tankStart = thisMenuItem->tankStarttime;
          thisTank.tankEnd   = thisMenuItem->tankEndtime;

          memcpy(request.sta, thisMenuItem->sta, 7);
          memcpy(request.chan, thisMenuItem->chan, 9);
          memcpy(request.net, thisMenuItem->net, 9);
          request.pinno = thisMenuItem->pinno;
          request.reqStarttime = thisTank.tankStart + 15;
          request.reqEndtime = thisTank.tankStart + 17;
          request.partial = 1;
          request.pBuf = buf;
          request.bufLen = 1024*10;
          request.timeout = 30;
          request.fill = GAP_FILL;
          result = wsGetTraceAscii(&request, &ourMenu, 30000);
          if(result != WS_ERR_NONE) {
            handleError(result, "wsGetTraceBin");
            wsKillMenu(&ourMenu);
            exit(1);
          }
          thisTank.samprate = request.samprate;
          sanityCheck(ourMenu, thisMenuItem, thisTank);
	}
        thisMenuItem = thisMenuItem->next; 
    }

    wsKillMenu(&ourMenu);
    exit(0);

}
