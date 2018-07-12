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
**    $Id: ew2mseed_swap.c 6803 2016-09-09 06:06:39Z et $
** 
**	Revision history:
**	$Log$
**	Revision 1.1  2010/03/10 17:14:58  paulf
**	first check in of ew2mseed to EW proper
**
**	Revision 1.1.1.1  2002/01/24 18:32:05  ilya
**	Exporting only ew2mseed!
**	
**	Revision 1.1.1.1  2001/11/20 21:47:00  ilya
**	First CVS commit
**	
 * Revision 1.2  2001/01/17  21:55:40  comserv
 * prerelease version ; copyright statement
 *
 * Revision 1.1  2000/11/17  06:56:30  comserv
 * Initial revision
 *
 * Revision 1.1  2000/11/17  06:56:30  comserv
 * Initial revision
 *
*/

/* Standard includes */
#include <stdio.h>

#include "qlib2.h"

/* Includes from Earthworm distribution */
#include "earthworm.h"
#include "swap.h"

/* Local includes */
#include "ew2mseed.h"

/**************************************************************************/

int ew2mseed_SwapInt(int * pValue, char cDataType)
{
#if defined (_SPARC)
	if( cDataType == 'i' || cDataType == 'f' )
	  SwapInt(pValue);
  return(EW_SUCCESS);
#elif defined (_INTEL)
	if( cDataType == 's' || cDataType == 't' )
	  SwapInt(pValue);
  return(EW_SUCCESS);
#else
  logit("e","ew2mseed_SwapInt():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  fprintf(stderr,"ew2mseed_SwapInt():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  return(EW_FAILURE);
#endif
}


int ew2mseed_SwapInt32(long * pValue, char cDataType)
{
#if defined (_SPARC)
	if( cDataType == 'i' || cDataType == 'f' )
	  SwapInt32(pValue);
  return(EW_SUCCESS);
#elif defined (_INTEL)
	if( cDataType == 's' || cDataType == 't' )
	  SwapInt32(pValue);
  return(EW_SUCCESS);
#else
  logit("e","ew2mseed_SwapInt32():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  fprintf(stderr,"ew2mseed_SwapInt32():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  return(EW_FAILURE);
#endif
}

int ew2mseed_SwapShort(short * pValue, char cDataType)
{
#if defined (_SPARC)
	if( cDataType == 'i' || cDataType == 'f' )
	  SwapShort(pValue);
  return(EW_SUCCESS);
#elif defined (_INTEL)
	if( cDataType == 's' || cDataType == 't' )
	  SwapShort(pValue);
  return(EW_SUCCESS);
#else
  logit("e","ew2mseed_SwapShort():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  fprintf(stderr,"ew2mseed_SwapShort():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  return(EW_FAILURE);
#endif
}

int ew2mseed_SwapDouble(double * pValue, char cDataType)
{
#if defined (_SPARC)
	if( cDataType == 'i' || cDataType == 'f' )
	  SwapDouble(pValue);
  return(EW_SUCCESS);
#elif defined (_INTEL)
	if( cDataType == 's' || cDataType == 't' )
	  SwapDouble(pValue);
  return(EW_SUCCESS);
#else
  logit("e","ew2mseed_SwapDouble():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  fprintf(stderr,"ew2mseed_SwapDouble():Error! Unable to determine platform! "
        "Please compile w/ _INTEL or _SPARC!\n");
  return(EW_FAILURE);

#endif
}

void ew2mseed_swapMiniSeedHeader(SDR_HDR *mseed_hdr)
{
    SwapShort(&mseed_hdr->time.year);
    SwapShort(&mseed_hdr->time.day);
    SwapShort(&mseed_hdr->time.ticks);
    SwapShort(&mseed_hdr->num_samples);
    SwapShort(&mseed_hdr->sample_rate_factor);
    SwapShort(&mseed_hdr->sample_rate_mult);
    SwapInt(&mseed_hdr->num_ticks_correction);
    SwapShort(&mseed_hdr->first_data);
    SwapShort(&mseed_hdr->first_blockette);
    return;
}
