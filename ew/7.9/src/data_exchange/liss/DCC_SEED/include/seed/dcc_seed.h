/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_seed.h 23 2000-03-05 21:49:40Z lombard $
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

#include <dcc_time.h>

/*
 *	
 *	Blockette manipulation structures
 *	
 */

#define SEEDVER 23	/* Version 2.3 of the SEED format */

#define CKP(a) if (!(a)) return(diemsg(__FILE__,__LINE__,__DATE__))
         /* Procedure to unwind on error */

typedef double VOLFLT;

#include <seed/itemlist.h>
#include <seed/seed_abbrevs.h>
#include <seed/seed_responses.h>
#include <seed/seed_spans.h>
#include <seed/seed_statchan.h>
#include <seed/seed_blockettes.h>

typedef struct _volume_id {
  int  SeedVersion;

  char		LenExp;		/* Record length exponent */
  STDTIME		Vol_Begin;	/* Beginning time of volume */
  STDTIME		End_Volume;	/* End time of volume */
  STDTIME 	Effective_Start;
  STDTIME		Effective_End;
  
  /* Station/Channel Tree */
  STATION_LIST	*Root_Station,  *Tail_Station;

  /* Time Span Tree */
  TIME_SPAN	*Root_Span,  *Tail_Span;

  /* Dictionary Tables */
  ABBREV		*Root_Abbrev,  *Tail_Abbrev;
  UNIT		*Root_Unit,  *Tail_Unit;
  FORMAT		*Root_Format,  *Tail_Format;
  COMMENT		*Root_Comment,  *Tail_Comment;
  SOURCE		*Root_Source,  *Tail_Source;

  /* Response Dictionary Tables */
  ZEROS_POLES	*Root_Zeros_Poles,  *Tail_Zeros_Poles;
  COEFFICIENTS	*Root_Coefficients,  *Tail_Coefficients;
#ifdef BAROUQUE_RESPONSES
  RESP_LIST	*Root_Resp_List,  *Tail_Resp_List;
  RESP_GENERIC	*Root_Resp_Generic,  *Tail_Resp_Generic;
#endif
  DECIMATION	*Root_Decimation,  *Tail_Decimation;
  
  /* Map Information */
  int		VolumeHdr_Rec;
  int		Dict_Start,Dict_End;
  int		Stat_Start,Stat_End;
  
} VOLUME_ID;

extern VOLUME_ID 	*VI;

/* Blockette types */

#define SEED_STATION_HEADER	5
#define SEED_NETWORK_HEADER	10
#define SEED_STATION_INDEX	11
#define SEED_SPAN_INDEX		12
#define SEED_FORMAT_DICT	30
#define SEED_COMMENT_DICT	31
#define SEED_SOURCE_DICT	32
#define SEED_ABBREV_DICT	33
#define SEED_UNIT_DICT		34
#define SEED_BEAM_DICT		35
#define SEED_STATION_RECORD	50
#define SEED_STATION_COMMENT	51
#define SEED_CHANNEL_RECORD	52
#define SEED_ZEROS_POLES	53
#define SEED_COEFFICIENTS	54
#define SEED_RESPONSE_LIST	55
#define SEED_GENERIC_RESPONSE	56
#define SEED_DECIMATION		57
#define SEED_GAIN_SENSITIVITY	58
#define SEED_CHANNEL_COMMENT	59
#define SEED_TIME_SPAN_HEADER	70
#define SEED_HYPOCENTER		71
#define SEED_EVENT_PHASES	72
#define SEED_TIME_SPAN_INDEX	73

#include <seed/seed_proto.h>
