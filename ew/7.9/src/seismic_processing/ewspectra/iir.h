/******************************************************************************
 *
 *	File:			iir.h
 *
 *	Function:		Butterworth highpass & lowpass filters.
 *
 *	Author(s):		Scott Hunter, ISTI
 *
 *	Source:			PQL
 *
 *	Notes:			
 *
 *	Change History:
 *			4/26/11	Started source
 *	
 *****************************************************************************/

#include <ew_timeseries.h>

/*****************************************************************************
	iir(EW_TIME_SERIES *ewts, double FH, int NH, double FL, int NL ):  
		Apply highpass filter to data in ewts w/ NH poles w/ cutoff @ FH
		Apply lowpass filter to data in ewts w/ NL poles w/ cutoff @ FL		
		Return 0 on success
*****************************************************************************/
int iir(EW_TIME_SERIES *ewts, double FH, int NH, double FL, int NL );
