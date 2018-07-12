
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: list.h 1722 2005-01-24 21:17:53Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2005/01/24 21:17:53  dietz
 *     Added TRIG_VERSION definition to track version of output message.
 *
 *     Revision 1.2  2004/05/24 20:37:38  dietz
 *     Added location code; uses TYPE_LPTRIG_SCNL as input,
 *     outputs TYPE_TRIGLIST_SCNL.
 *
 *     Revision 1.1  2000/02/14 17:15:33  lucky
 *     Initial revision
 *
 *
 */

#include <trace_buf.h>

         /*****************************************************
          *                    File list.h                    *
          *****************************************************/

#define TRIG_VERSION   "v2.0"  /* version of TYPE_TRIGLIST_SCNL  */

/* Typedefs
   ********/
typedef struct {
   int     MsgType;
   int     ModId;
   int     InstId;
   short   pin;                   /* Pin number */
   short   flag;                  /* 0 if trigger is from a duplicate station */
   char    sta[TRACE2_STA_LEN];   /* Station name */
   char    comp[TRACE2_CHAN_LEN]; /* Station component */
   char    net[TRACE2_NET_LEN];   /* Network name */
   char    loc[TRACE2_LOC_LEN];   /* Location code */
   double  time;                  /* Seconds since Jan 1, 1970 */
   char    event_type;            /* (B)ig or (N)ormal */
} TRIGGER;

struct list {
   TRIGGER      trg;
   struct list *next;
};

typedef struct list ELEMENT;
typedef ELEMENT *LINK;


/* Function prototypes
   *******************/
void AppendTrg( TRIGGER );
void PurgeList( double );
int  CountSta( void );
void SaveList( void );
void LogSavedList( void );

