/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_abbrevs.h 23 2000-03-05 21:49:40Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

typedef struct _abbrev {
	char		*Abbrev_Key;	/* How do we access this? */
	int		Abbrev_ID;	/* Numeric id for writing headers */
	char		*Comment;	/* Text for abbreviation */
	int		Use_Count;	/* How many times refered to */
	struct _abbrev 	*Next;
} ABBREV;

typedef struct _unit {
	char		*Unit_Key;	/* Key for the units */
	int		Unit_ID;	/* What code units? */
	char		*Comment;	/* Description of Unit */
	int		Use_Count;	/* Number of times refered to */
	struct _unit    *Next;
} UNIT;

typedef struct _comment {
	int		Comment_ID;	/* Comment numeric ident */
	char		Class;		/* Class Code */
	char		*Text;		/* Text of comment */
	UNIT		*Level;		/* The unit of the level */
	int		Use_Count;	/* Number of times refered to */
	struct _comment *Next;
} COMMENT;

#define MAXKEYS 60
typedef struct _format {
	char		*Format_Key;	/* What is name of format */
	char		*Format_Name;	/* Description of format */
	int		Format_ID;	/* Id of format */
	int		Family;
	char		NumKeys;
	char		*Keys[MAXKEYS];
	int		Use_Count;	/* Number of times refered to */
	struct _format	*Next;
} FORMAT;

typedef struct _comment_entry {
	STDTIME		Start_Comment;
	STDTIME		End_Comment;
	COMMENT		*Comment;
	int		Comment_Level;
	struct _comment_entry *Next;
} COMMENT_ENTRY;

typedef	struct _cited_source {
	char		*Source_Key;
	int		Source_ID;	
	char		*Author;
	char		*Date_Catalog;
	char		*Publisher;
	int		Use_Count;
	struct _cited_source	*Next;
} SOURCE;

