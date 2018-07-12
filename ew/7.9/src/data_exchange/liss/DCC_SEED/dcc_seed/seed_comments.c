/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_comments.c 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:06:20  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <string.h>
#include <dcc_seed.h>
#include <dcc_misc.h>

/*********************Handle Station/Channel Comments**********************/

/*
 *	
 *	Alloc memory for comment and zero out
 *	
 */

_SUB COMMENT_ENTRY *InitCom()
{

	COMMENT_ENTRY *newcom;

	newcom = (COMMENT_ENTRY *) SafeAlloc(sizeof (COMMENT_ENTRY));

	newcom->Start_Comment = ST_Zero();
	newcom->End_Comment = ST_Zero();
	newcom->Comment = NULL;
	newcom->Comment_Level = 0;
	newcom->Next = NULL;

	return(newcom);
}

/*
 *	
 *	Link a comment onto a station record
 *	
 */

_SUB void LinkInStatCom(STATION_LIST *instat,COMMENT_ENTRY *incom)
{

	if (instat->Tail_Comment==NULL) 
		instat->Tail_Comment = instat->Root_Comment = incom;
	else	{
		instat->Tail_Comment->Next = incom;
		instat->Tail_Comment = incom;
	}
}

/*
 *	
 *	Link comment onto a channel record
 *	
 */

_SUB void LinkInChanCom(CHANNEL_LIST *inchan,COMMENT_ENTRY *incom)
{

	if (inchan->Tail_Comment==NULL) 
		inchan->Tail_Comment = inchan->Root_Comment = incom;
	else	{
		inchan->Tail_Comment->Next = incom;
		inchan->Tail_Comment = incom;
	}
}

/*
 *	
 *	Init, fill, and link comment onto station record
 *	
 */

_SUB void AddStatCom(STATION_LIST *inroot,STDTIME startime,STDTIME endtime,
	int comnum,UDCC_LONG level)
{

	COMMENT_ENTRY *newcom;

	newcom = InitCom();

	newcom->Start_Comment = startime;
	newcom->End_Comment = endtime;
	newcom->Comment_Level = level;
	
	newcom->Comment = SearchComment(comnum);
	if (newcom->Comment==NULL) {
		printf("Couldn't find comment number %d in database\n",
			comnum);
		return;
	}
	CountComment(newcom->Comment);

	LinkInStatCom(inroot,newcom);
}

/*
 *	
 *	Init, fill, and link comment onto channel record
 *	
 */

_SUB void AddChanCom(CHANNEL_LIST *inroot,STDTIME startime,STDTIME endtime,
	int comnum,UDCC_LONG level)
{

	COMMENT_ENTRY *newcom;

	newcom = InitCom();

	newcom->Start_Comment = startime;
	newcom->End_Comment = endtime;
	newcom->Comment_Level = level;
	
	newcom->Comment = SearchComment(comnum);
	if (newcom->Comment==NULL) {
		printf("Couldn't find comment number %d in database\n",
			comnum);
		return;
	}
	CountComment(newcom->Comment);

	LinkInChanCom(inroot,newcom);
}
