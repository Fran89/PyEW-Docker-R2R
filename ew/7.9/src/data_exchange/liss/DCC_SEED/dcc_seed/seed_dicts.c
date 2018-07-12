/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_dicts.c 1248 2003-06-16 22:08:11Z patton $
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

/**************Handle Abbreviation Structure and Linked Lists*************/

/*
 *	
 *	Search the abbreviation table for a given key
 *	
 */

_SUB ABBREV *SearchAbbrev(char *inkey)
{

	ABBREV *looper;

	for (looper=VI->Root_Abbrev; looper!=NULL; looper=looper->Next)
		if (streq(looper->Abbrev_Key,FixKey(inkey))) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Search the abbreviation table for a given index key
 *	
 */

_SUB ABBREV *SearchAbbrevIdx(DCC_LONG inkey)
{

	ABBREV *looper;

	for (looper=VI->Root_Abbrev; looper!=NULL; looper=looper->Next)
		if (looper->Abbrev_ID==inkey) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Malloc a new abbreviation structure and initialize it 
 *	
 */

_SUB ABBREV *InitAbbrev()
{

	ABBREV *Abbrev;

	Abbrev = (ABBREV *) SafeAlloc(sizeof (ABBREV));

	Abbrev->Abbrev_Key = NULL;
	Abbrev->Abbrev_ID = 0;
	Abbrev->Comment = NULL;
	Abbrev->Use_Count = 0;
	Abbrev->Next = NULL;

	return(Abbrev);
}

/*
 *	
 *	Add a new abbreviation to the link list in alphabetical order
 *	
 */

_SUB void LinkInAbbrev(ABBREV *inabbrev)
{

	int cm;
	ABBREV *looper,*last;

	last = NULL;
	for (looper=VI->Root_Abbrev; looper!=NULL; 
		last=looper,looper=looper->Next) {

		cm = strcmp(looper->Abbrev_Key,inabbrev->Abbrev_Key);
		if (cm==0) return;	/* Key already here! */
		if (cm<0) continue;		/* Key greater... */
		break;				/* Key is less */
	}

	if (looper==NULL) {		/* Add to end of list */
		if (VI->Tail_Abbrev==NULL) 
			VI->Root_Abbrev = VI->Tail_Abbrev = inabbrev;
		else {
			VI->Tail_Abbrev->Next = inabbrev;
			VI->Tail_Abbrev = inabbrev;
		}
		inabbrev->Next = NULL;
	} else if (last==NULL) {	/* Insert to top of list */
		inabbrev->Next = VI->Root_Abbrev;
		VI->Root_Abbrev = inabbrev;
	} else {			/* Link into middle of list */
		last->Next = inabbrev;
		inabbrev->Next = looper;
	}		

	return;
}

/*
 *	
 *	Init, fill, and link in an abbreviation header
 *	
 */

_SUB void AddAbbrev(char *key,char *comment)
{

	ABBREV *newabbrev;

	newabbrev = InitAbbrev();

	newabbrev->Abbrev_Key = SafeAllocString(FixKey(key));

	newabbrev->Comment = SafeAllocString(comment);

	LinkInAbbrev(newabbrev);
}

/*
 *	
 *	Increment the use count of the abbreviation
 *	
 */

_SUB VOID CountAbbrev(ABBREV *inabbrev)
{

	if (inabbrev==NULL) return;	/* Its ok */

	inabbrev->Use_Count++;
}

/**************Handle Unit Structure and Linked Lists*************/

/*
 *	
 *	Search the unit table for a given key
 *	
 */

_SUB UNIT *SearchUnit(char *inkey)
{

	UNIT *looper;

	for (looper=VI->Root_Unit; looper!=NULL; looper=looper->Next)
		if (streq(looper->Unit_Key,FixKey(inkey))) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Search the unit table for a given index key
 *	
 */

_SUB UNIT *SearchUnitIdx(DCC_LONG inkey)
{

	UNIT *looper;

	for (looper=VI->Root_Unit; looper!=NULL; looper=looper->Next)
		if (looper->Unit_ID==inkey) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Malloc a new unit structure and initialize it 
 *	
 */

_SUB UNIT *InitUnit()
{

	UNIT *Unit;

	Unit = (UNIT *) SafeAlloc(sizeof (UNIT));

	Unit->Unit_Key = NULL;
	Unit->Unit_ID = 0;
	Unit->Comment = NULL;
	Unit->Use_Count = 0;
	Unit->Next = NULL;

	return(Unit);
}

/*
 *	
 *	Add a new unit to the link list in alphabetical order
 *	
 */

_SUB void LinkInUnit(UNIT *inunit)
{

	int cm;
	UNIT *looper,*last;

	last = NULL;
	for (looper=VI->Root_Unit; looper!=NULL; 
		last=looper,looper=looper->Next) {

		cm = strcmp(looper->Unit_Key,inunit->Unit_Key);
		if (cm==0) return;	/* Key already here! */
		if (cm<0) continue;		/* Key greater... */
		break;				/* Key is less */
	}

	if (looper==NULL) {		/* Add to end of list */
		if (VI->Tail_Unit==NULL) 
			VI->Root_Unit = VI->Tail_Unit = inunit;
		else {
			VI->Tail_Unit->Next = inunit;
			VI->Tail_Unit = inunit;
		}
		inunit->Next = NULL;
	} else if (last==NULL) {	/* Insert to top of list */
		inunit->Next = VI->Root_Unit;
		VI->Root_Unit = inunit;
	} else {			/* Link into middle of list */
		last->Next = inunit;
		inunit->Next = looper;
	}		

	return;
}

/*
 *	
 *	Init, fill, and link in a unit header
 *	
 */

_SUB void AddUnit(char *key,char *comment)
{

	UNIT *newunit;

	newunit = InitUnit();

	newunit->Unit_Key = SafeAllocString(FixKey(key));

	newunit->Comment = SafeAllocString(comment);

	LinkInUnit(newunit);
}

/*
 *	
 *	Increment use count of the unit
 *	
 */

_SUB VOID CountUnit(UNIT *inunit)
{

	if (inunit==NULL) return;	/* Its ok */

	inunit->Use_Count++;
}

/**************Handle Comment Structure and Linked Lists*************/

/*
 *	
 *	Search the comment table for a given key
 *	
 */

_SUB COMMENT *SearchComment(int inid)
{

	COMMENT *looper;

	for (looper=VI->Root_Comment; looper!=NULL; looper=looper->Next)
		if (looper->Comment_ID==inid) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Malloc a new comment structure and initialize it 
 *	
 */

_SUB COMMENT *InitComment()
{

	COMMENT *Comment;

	Comment = (COMMENT *) SafeAlloc(sizeof (COMMENT));

	Comment->Comment_ID = 0;
	Comment->Class = ' ';
	Comment->Text = NULL;
	Comment->Level = NULL;
	Comment->Use_Count = 0;
	Comment->Next = NULL;

	return(Comment);
}

/*
 *	
 *	Add a new commentiation to the link list in alphabetical order
 *	
 */

_SUB void LinkInComment(COMMENT *incomment)
{

	COMMENT *looper,*last;

	last = NULL;
	for (looper=VI->Root_Comment; looper!=NULL; 
		last=looper,looper=looper->Next) {

		if (looper->Comment_ID==incomment->Comment_ID) return;
		if (looper->Comment_ID<incomment->Comment_ID) continue;
		break;				/* Key is less */
	}

	if (looper==NULL) {		/* Add to end of list */
		if (VI->Tail_Comment==NULL) 
			VI->Root_Comment = VI->Tail_Comment = incomment;
		else {
			VI->Tail_Comment->Next = incomment;
			VI->Tail_Comment = incomment;
		}
		incomment->Next = NULL;
	} else if (last==NULL) {	/* Insert to top of list */
		incomment->Next = VI->Root_Comment;
		VI->Root_Comment = incomment;
	} else {			/* Link into middle of list */
		last->Next = incomment;
		incomment->Next = looper;
	}		

	return;
}

/*
 *	
 *	Init, fill and link a comment header
 *	
 */

_SUB void AddComment(int comnum,char class,char *text,char *level)
{

	UNIT *levcom;
	COMMENT *newcomment;

	newcomment = InitComment();

	newcomment->Comment_ID = comnum;
	newcomment->Class = class;

	newcomment->Text = SafeAllocString(text);

	levcom = SearchUnit(level);
	newcomment->Level = levcom;
	CountUnit(levcom);			/* Increment the count */
	/*if (levcom!=NULL) printf("Found level %s\n",levcom->Unit_Key);*/

	LinkInComment(newcomment);
}

/*
 *	
 *	Increment use count of the comment
 *	
 */

_SUB VOID CountComment(COMMENT *incomment)
{

	if (incomment==NULL) return;	/* Its ok */

	incomment->Use_Count++;
}

/**************Handle Format Structure and Linked Lists*************/

/*
 *	
 *	Search the format table for a given key
 *	
 */

_SUB FORMAT *SearchFormat(char *inkey)
{

	FORMAT *looper;

	for (looper=VI->Root_Format; looper!=NULL; looper=looper->Next)
		if (streq(looper->Format_Key,FixKey(inkey))) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Search the format table for a given index key
 *	
 */

_SUB FORMAT *SearchFormatIdx(DCC_LONG inkey)
{

	FORMAT *looper;

	for (looper=VI->Root_Format; looper!=NULL; looper=looper->Next) {
		if (looper->Format_ID==inkey) return(looper);
	}

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Malloc a new format structure and initialize it 
 *	
 */

_SUB FORMAT *InitFormat()
{

	FORMAT *Format;
	int i;

	Format = (FORMAT *) SafeAlloc(sizeof (FORMAT));

	Format->Format_Key = NULL;
	Format->Format_Name = NULL;
	Format->Format_ID = 0;
	Format->Family = 0;
	Format->NumKeys = 0;
	Format->Use_Count = 0;
	for (i=0; i<MAXKEYS; i++) Format->Keys[i] = NULL;
	Format->Next = NULL;

	return(Format);
}

/*
 *	
 *	Add a new format to the link list in alphabetical order
 *	
 */

_SUB void LinkInFormat(FORMAT *informat)
{

	int cm;
	FORMAT *looper,*last;

	last = NULL;
	for (looper=VI->Root_Format; looper!=NULL; 
		last=looper,looper=looper->Next) {

		cm = strcmp(looper->Format_Key,informat->Format_Key);
		if (cm==0) return;	/* Key already here! */
		if (cm<0) continue;		/* Key greater... */
		break;				/* Key is less */
	}

	if (looper==NULL) {		/* Add to end of list */
		if (VI->Tail_Format==NULL) 
			VI->Root_Format = VI->Tail_Format = informat;
		else {
			VI->Tail_Format->Next = informat;
			VI->Tail_Format = informat;
		}
		informat->Next = NULL;
	} else if (last==NULL) {	/* Insert to top of list */
		informat->Next = VI->Root_Format;
		VI->Root_Format = informat;
	} else {			/* Link into middle of list */
		last->Next = informat;
		informat->Next = looper;
	}		

	return;
}

/*
 *	
 *	Init, fill, and link in a format
 *	
 */

_SUB void AddFormat(char *key,char *name,int family,
		int numkeys,char *keylist[MAXKEYS])
{

	FORMAT *newformat;
	int i;

	newformat = InitFormat();

	newformat->Format_Key = SafeAllocString(FixKey(key));

	newformat->Format_Name = SafeAllocString(name);

	newformat->Family = family;

	newformat->NumKeys = numkeys;
	for (i=0; i<numkeys; i++) {
		newformat->Keys[i] = SafeAllocString(keylist[i]);
	}

	LinkInFormat(newformat);
}

/*
 *	
 *	Increment the use count of the format
 *	
 */

_SUB VOID CountFormat(FORMAT *informat)
{

	if (informat==NULL) return;	/* Its ok */

	informat->Use_Count++;
}

/**************Handle Pole-Zero Structure and Linked Lists*************/

/*
 *	
 *	Search the Zero_Pole table for a given key
 *	
 */

_SUB ZEROS_POLES *SearchZerosPoles(char *inkey)
{

	ZEROS_POLES *looper;

	for (looper=VI->Root_Zeros_Poles; looper!=NULL; looper=looper->Next)
		if (streq(looper->Citation_Name,
			  FixKey(inkey))) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Insert Zero/Pole into dictionary linked list - will return
 *      without inserting if already in table
 *	
 */

_SUB void LinkInZeroPoleDict(ZEROS_POLES *inpz)
{
  int cm;
  ZEROS_POLES *looper,*last;

  for (last=NULL,
       looper=VI->Root_Zeros_Poles; 
       looper!=NULL; 
       last=looper,
       looper=looper->Next) {

    cm = strcmp(looper->Citation_Name,inpz->Citation_Name);
    if (cm==0) return;
    if (cm>0) break;
  }
  
  if (looper==NULL) {		/* Add to end of list */
    if (VI->Tail_Zeros_Poles==NULL) 
      VI->Root_Zeros_Poles = VI->Tail_Zeros_Poles = inpz;
    else {
      VI->Tail_Zeros_Poles->Next = inpz;
      VI->Tail_Zeros_Poles = inpz;
    }
    inpz->Next = NULL;
  } else if (last==NULL) {	/* Insert to top of list */
    inpz->Next = VI->Root_Zeros_Poles;
    VI->Root_Zeros_Poles = inpz;
  } else {			/* Link into middle of list */
    last->Next = inpz;
    inpz->Next = looper;
  }		

}

/**************Handle Coefficients Structure and Linked Lists*************/

/*
 *	
 *	Search the Coefficients table for a given key
 *	
 */

_SUB COEFFICIENTS *SearchCoefficients(char *inkey)
{

	COEFFICIENTS *looper;

	for (looper=VI->Root_Coefficients; looper!=NULL; looper=looper->Next)
		if (streq(looper->Citation_Name,
			  FixKey(inkey))) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Insert Coefficients into dictionary linked list - will return
 *      without inserting if already in table
 *	
 */

_SUB void LinkInCoefficientsDict(COEFFICIENTS *incoeff)
{
  int cm;
  COEFFICIENTS *looper,*last;

  for (last=NULL,
       looper=VI->Root_Coefficients; 
       looper!=NULL; 
       last=looper,
       looper=looper->Next) {

    cm = strcmp(looper->Citation_Name,incoeff->Citation_Name);
    if (cm==0) return;
    if (cm>0) break;
  }
  if (looper==NULL) {		/* Add to end of list */
    if (VI->Tail_Coefficients==NULL) {
      VI->Root_Coefficients = VI->Tail_Coefficients = incoeff;
    }
    else {
      VI->Tail_Coefficients->Next = incoeff;
      VI->Tail_Coefficients = incoeff;
    }
    incoeff->Next = NULL;
  } else if (last==NULL) {	/* Insert to top of list */
    incoeff->Next = VI->Root_Coefficients;
    VI->Root_Coefficients = incoeff;
  } else {			/* Link into middle of list */
    last->Next = incoeff;
    incoeff->Next = looper;
  }		

}

/**************Handle Decimation Structure and Linked Lists*************/

/*
 *	
 *	Search the Decimation table for a given key
 *	
 */

_SUB DECIMATION *SearchDecimation(char *inkey)
{

	DECIMATION *looper;

	for (looper=VI->Root_Decimation; looper!=NULL; looper=looper->Next)
		if (streq(looper->Citation_Name,
			  FixKey(inkey))) return(looper);

	return(NULL);		/* Not Found */
}

/*
 *	
 *	Insert Decimation into dictionary linked list - will return
 *      without inserting if already in table
 *	
 */

_SUB void LinkInDecimationDict(DECIMATION *indm)
{
  int cm;
  DECIMATION *looper,*last;

  for (last=NULL,
       looper=VI->Root_Decimation; 
       looper!=NULL; 
       last=looper,
       looper=looper->Next) {

    cm = strcmp(looper->Citation_Name,indm->Citation_Name);
    if (cm==0) return;
    if (cm>0) break;
  }
  
  if (looper==NULL) {		/* Add to end of list */
    if (VI->Tail_Decimation==NULL) 
      VI->Root_Decimation = VI->Tail_Decimation = indm;
    else {
      VI->Tail_Decimation->Next = indm;
      VI->Tail_Decimation = indm;
    }
    indm->Next = NULL;
  } else if (last==NULL) {	/* Insert to top of list */
    indm->Next = VI->Root_Decimation;
    VI->Root_Decimation = indm;
  } else {			/* Link into middle of list */
    last->Next = indm;
    indm->Next = looper;
  }		

}
