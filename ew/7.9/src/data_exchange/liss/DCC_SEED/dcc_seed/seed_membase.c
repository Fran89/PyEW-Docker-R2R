/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_membase.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <string.h>
#include <dcc_seed.h>

VOLUME_ID	Root_Volume,*VI = &Root_Volume;

/*********************Create Standard Structures***********************/

/*
 *	
 *	Initialize the Volume Structure
 *	
 */

_SUB VOID InitializeVolume(STDTIME starttime,STDTIME endtime)
{

	VI = &Root_Volume;		/* Pointer to root structure */

	VI->LenExp = 12;	
	VI->Vol_Begin = starttime;
	VI->End_Volume = endtime;

	VI->Effective_Start = starttime;
	VI->Effective_End = endtime;

	VI->Root_Station = 
	  VI->Tail_Station = NULL;
	VI->Root_Span = 
	  VI->Tail_Span = NULL;
	VI->Root_Abbrev = 
	  VI->Tail_Abbrev = NULL;	/* Init linked lists */
	VI->Root_Unit = 
	  VI->Tail_Unit = NULL;
	VI->Root_Format =
	  VI->Tail_Format = NULL;
	VI->Root_Comment = 
	  VI->Tail_Comment = NULL;
	VI->Root_Source =
	  VI->Tail_Source = NULL;
	VI->Root_Zeros_Poles = 
	  VI->Tail_Zeros_Poles = NULL;
	VI->Root_Coefficients = 
	  VI->Tail_Coefficients = NULL;
#ifdef BAROUQUE_RESPONSES
	VI->Root_Resp_List = 
	  VI->Tail_Resp_List = NULL;
	VI->Root_Resp_Generic = 
	  VI->Tail_Resp_Generic = NULL;
#endif
	VI->Root_Decimation = 
	  VI->Tail_Decimation = NULL;

	return;
}

