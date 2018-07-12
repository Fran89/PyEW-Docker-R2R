#ifndef GLASSSTATE_H
# define GLASSSTATE_H

#define MAXACT			100
#define MAXTRN			40
#define MAXTSK			20

enum ActionTypes
{
  ACTION_TYPE_PICK,
  ACTION_TYPE_ORIGIN
};

typedef struct {
	int		iAct;
  int   iActType;
	char	sTsk[32];
} TASK;
typedef struct {
	int		iAct;	// Old action
	int		iRes;	// Result
	int		iTrn;	// New action to schedule
	int		iPer;	// Percentage of times transition taken (0-100).
  int   iNumEval;   // Number of times rule evaluated
  int   iNumSuccess; // Number of times rule prob successfull  (iNumSuccess/iNumEval ~ iPer/100)
} TRANS;
typedef struct {
	int		iAct;
	int		iCycle;
	char	sEnt[18];
} ACTION;


int nCycle;
int nScheduledActions;

int nTrn;
TRANS Trn[MAXTRN];
int nAct;
ACTION Act[MAXACT];


// DK 011404  WARNING:  This enum array should be synced 
//            with the Tsk[] array.  There are sections of
//            code that assume that they are synced,
//            and will assume i=Tsk[i].iAct
enum GlassStates 
{
  // STATE STATES

  // Unknown
  STATE_UNKNOWN,
  // Finished
  STATE_FINISHED,

  // Pick states
  STATE_NEW_UNASSIGNED_PICK,
  STATE_PICK_UNASSIGNABLE,
  STATE_PICK_UNASSOCIABLE,
  STATE_PICK_ASSOCIATED,

  // Origin States
  STATE_ORIGIN_DELETED,
  STATE_ORIGIN_CHANGED,
  STATE_ORIGIN_LOCATED,
  STATE_ORIGIN_ASSOCIATIONS_CHANGED,
  STATE_ORIGIN_VALIDATED,
  STATE_ORIGIN_STABILIZED,
  STATE_ORIGIN_INVALIDATED,

  // ACTION STATES
    
  // Pick states
  ACT_ASSIGN,
  ACT_ASSOCIATE,

  // Origin States
  ACT_LOCATE,
  ACT_LOCATE_FIX,
  ACT_FOCUS,
  ACT_WAIF,
  ACT_PRUNE,
  ACT_SCAVENGE,
  ACT_PAU,
  ACT_RECAST,
  ACT_DELETE,
  ACT_UPDATE_GUI,
  ACT_VALIDATE
} ;


TASK Tsk[]=
{
  // Unknown
  {STATE_UNKNOWN,    0,   "Unknown"},

  // Finished (with this branch of processing)
  {STATE_FINISHED,    0,   "Finished"},

  // Pick states
  {STATE_NEW_UNASSIGNED_PICK, ACTION_TYPE_PICK,   "Unassigned_Pick"},
  {STATE_PICK_UNASSIGNABLE,   ACTION_TYPE_PICK,   "Unassignable_Pick"},
  {STATE_PICK_UNASSOCIABLE,   ACTION_TYPE_PICK,   "Unassociable_Pick"},
  {STATE_PICK_ASSOCIATED,     ACTION_TYPE_PICK,   "Associated_Pick"},

  // Origin States
  {STATE_ORIGIN_DELETED,      ACTION_TYPE_ORIGIN, "Deleted_Origin"},
  {STATE_ORIGIN_CHANGED,      ACTION_TYPE_ORIGIN, "Changed_Origin"},
  {STATE_ORIGIN_LOCATED,      ACTION_TYPE_ORIGIN, "Located_Origin"},
  {STATE_ORIGIN_ASSOCIATIONS_CHANGED,  
                              ACTION_TYPE_ORIGIN, "Origin_w_Changed_Assocs"},
  {STATE_ORIGIN_VALIDATED,    ACTION_TYPE_ORIGIN, "Validated_Origin"},
  {STATE_ORIGIN_STABILIZED,   ACTION_TYPE_ORIGIN, "Stabilized_Origin"},
  {STATE_ORIGIN_INVALIDATED,  ACTION_TYPE_ORIGIN, "Invalidated_Origin"},

  // Pick States
  {ACT_ASSIGN,    ACTION_TYPE_PICK,   "Assign"},
  {ACT_ASSOCIATE,  ACTION_TYPE_PICK, "Associate"},

  // Origin States
  {ACT_LOCATE,     ACTION_TYPE_ORIGIN, "Locate"},
  {ACT_LOCATE_FIX, ACTION_TYPE_ORIGIN, "LocateFix"},
  {ACT_FOCUS,      ACTION_TYPE_ORIGIN, "Focus"},
  {ACT_WAIF,       ACTION_TYPE_ORIGIN, "Waif"},
  {ACT_PRUNE,      ACTION_TYPE_ORIGIN, "Prune"},
  {ACT_SCAVENGE,   ACTION_TYPE_ORIGIN, "Scavenge"},
  {ACT_PAU,        ACTION_TYPE_ORIGIN, "Terminate"},
  {ACT_RECAST,     ACTION_TYPE_ORIGIN, "Recast"},
  {ACT_DELETE,     ACTION_TYPE_ORIGIN, "Delete"},
  {ACT_UPDATE_GUI, ACTION_TYPE_ORIGIN, "Update_GUI"},
  {ACT_VALIDATE,   ACTION_TYPE_ORIGIN, "Validate_Origin"}
  
};
int nTsk;


#endif // GLASSSTATE_H

