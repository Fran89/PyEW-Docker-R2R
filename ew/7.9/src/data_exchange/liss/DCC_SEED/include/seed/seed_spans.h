/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_spans.h 23 2000-03-05 21:49:40Z lombard $
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

typedef struct _hypocenter_info {

	STDTIME		Origin_Time;
	SOURCE		*Hypo_Source;
	STDFLT		Event_Latitude;
	STDFLT		Event_Longitude;
	STDFLT		Event_Depth;

	int		Number_Magnitudes;

	int		Region_Number;	/* Flinn-Engdahl Region Name Code */
	char		*Region_Name;

	struct	magnitudes {
		STDFLT		Magnitude;
		char		*Mag_Type;
		SOURCE		*Mag_Source;
	} mags[1];

} HYPO_INFO;

typedef struct _event_phases {

	char		SeedKey[11];		/* Stat/Loc/Chan */
	STDTIME		Arrival;
	STDFLT		Amplitude;
	STDFLT		Period;
	STDFLT		SNR;
	char		*Phase_Name;

	struct _event_phases	*Next;
} EVENT_PHASES;

typedef struct _time_span_index {

	char		SeedKey[11];		/* Stat/Loc/Chan */

	STDTIME		Start_Span;		/* Start interval of span */
	STDTIME		End_Span;		/* End of span */
	
	int		Record_Begin;		/* Ftell first rec in data */
	int		numrec;			/* number of records */

	char		*Filename;		/* Copy of file name ptr */

	int		Start_Rec;		/* Record number on seed vol */
	int		Start_Sub;
	int		End_Rec;		/* End rec on seed vol */
	int		End_Sub;

	ITEMLIST	*taglist;

	struct _time_span_index	*Next;

} SPAN_INDEX;

typedef struct _time_span_header {

	STDTIME		Start_Span;		/* Start interval of span */
	STDTIME		End_Span;		/* End of span */

	int		Span_Rec;		/* Time span hdr rec */

	int		Start_Rec;		/* Record # of first data */
	int		End_Rec;		/* Record # of last data */

	char		Span_Flag;		/* Span type */
#define SPAN_TYPE_EVENT 	'E'
#define SPAN_TYPE_PERIODIC 	'P'

	SPAN_INDEX	*Root_Span,*Tail_Span;	/* Linked list of indexes */

	ITEMLIST	*taglist;
	BOOL		setrange;		/* Set start time from data? */

	HYPO_INFO	*Hypocenter;		/* Event Hypocenter */
	EVENT_PHASES	*Phase_List;		/* Event phase list */

	struct _time_span_header *Next;	
} TIME_SPAN;

