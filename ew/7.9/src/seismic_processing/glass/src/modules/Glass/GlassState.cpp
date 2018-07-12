/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: GlassState.cpp 3212 2007-12-27 18:05:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.2  2006/12/13 17:15:01  davidk
 *     Fixed "Heavy Handed" code in glass state manager, so that it gets heavy handed
 *     with all of the currently scheduled tasks, instead of just the first one in the list.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2005/02/28 18:12:54  davidk
 *     Added code to handle endless-loop pick/origin processing.
 *     Upped the max cycles per pick, since we were getting into some pick processing
 *     cycles where 50 steps wasn't enough.  Made the value a #define constant.
 *     Modified the processing control loop.
 *     Formerly, processing for each pick was limited to 50 steps,
 *     which was anecdotally deemed sufficient.
 *     If processing from a pick lasted more than 50 steps, the processing was truncated
 *     at 50.  There are two sections to Origin processing:  the first section is where
 *     the origin runs through several processors( Pick, Waif, Scavenge, Prune, Focus)
 *     that attempt to modify the attributes of the origin to make it more accurate;
 *     once an origin has become stable, it moves to the second section it is not changed
 *     but is evaluated for quality and published to the display GUIs.
 *     The hard stop at 50 was causing situations where an origin would never go through
 *     validation because it was in an oscilating state.  The origin would keep changing
 *     back and forth between two points and processing would be hard stopped before
 *     the origin ever was ever stabilized.  As a result, the origin would never get
 *     deleted or updated in the display.  The origin would also get published A LOT,
 *     because publications are based upon the number of changes an origin went through,
 *     and the origin would go through many changes as a result of the oscilation.
 *     In very few cases, there was a lot of changes going on with respect to an origin,
 *     and all of the required processing could not be accomplished in the 50 steps.
 *     Code was changed to extend the number of steps, and add an additional heavy handed
 *     processing stage, where all origins are moved to a stabilized state after the
 *     initial allotment of processing steps.  Where a second allotment of processing
 *     steps are allowed before processing is finally hard-stopped.
 *     As a result, even if something goes wrong and an origin is locked in an oscilation,
 *     the processing engine, guarantees that it will still be evaluated for quality.
 *     (NOTE:  many of the oscilations were a result of origins that were on the edge between
 *      the embryonic limited-number-of-picks state for newly formed origins, where quality
 *      criteria is much more relaxed, and the life-is-hard state in which standard origins exist.)
 *
 *     Revision 1.4  2005/02/15 19:42:05  davidk
 *     Changed state behavior such that now
 *     Focus, Scavenge, Waif, and Prune
 *     all cause an Origin to attempt to go into the STATE_ORIGIN_STABILIZED
 *     state.  When an origin attempts to go into the Stabilized state, a test is invoked
 *     that checks to see if the Origin is scheduled for anymore processing.
 *     If the origin doesn't have anymore actions scheduled, then it will become Stabilized,
 *     and at that point it can be scheduled for Validation or some other task.
 *     This affectively applies a mutex at the stabilized state, to prevent origins from
 *     being declared as stabilized just because one branch of processing has finished.
 *
 *     Revision 1.3  2004/11/02 19:12:22  davidk
 *     changed the severity on a logging statement.
 *
 *     Revision 1.2  2004/09/16 00:58:23  davidk
 *     Cleaned up logging messages.
 *     Downgraded some debugging messages.
 *
 *     Revision 1.1  2004/04/01 22:05:43  davidk
 *     v1.11 update
 *     Added internal state-based processing.
 *
 *
 *
 **********************************************************/

// GlassState.cpp
#include <stdlib.h>
#include <comfile.h>
#include <string.h>
#include <Debug.h>
#include "glass.h"
#include "GlassState.h"

#define MAX_CYCLES_PER_PICK 80

/***************
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <ITravelTime.h>
#include "spock.h"
#include "monitor.h"
#include "GlassMod.h"
#include "IGlint.h"
#include "date.h"
#include "comfile.h"
#include "sphere.h"
#include "rank.h"
#include "str.h"
#include <Debug.h>
#include <GlassState.h>
#include <opcalc.h>
************/

/************* DK REMOVE
//---------------------------------------------------------------------------------------Task
// Define relationship between task name and action value
void CGlass::Task(char *task, int iact, int iActType) {
	if(nTsk < MAXTSK) {
		strcpy(Tsk[nTsk].sTsk, task);
		Tsk[nTsk].iAct = iact;
    Tsk[nTsk].iActType = iActType;
		nTsk++;
	}
}
*******************/
//---------------------------------------------------------------------------------------Action
// Execute scheduled tasking
// At present limited to 100 actiona per pick, which is mostly likely adequate, might
// have to implement round-robin, but for now doesn't seem so.
void CGlass::Action() {
	char ent[18];
	int res;
	int iact;
	int i;
	int iprob;
  bool bOriginChanged;
  int iNextState;
  int iTempCycle=0;

  ORIGIN * pOrg;
  PICK *   pPick;

  bool bHeavyHanded = false;

  // STEP 1:   Make sure there's something to do.  Otherwise return
	if(nScheduledActions <= 0)
  {
    if(nScheduledActions < 0)
    {
      CDebug::Log(DEBUG_MAJOR_WARNING,
                  "Action() nScheduledActions is (%d).  Something's amiss!\n",
                  nScheduledActions);
      nScheduledActions = 0;
    }
		return;
  }

  // STEP 2:   While there's something to do, do it.
	while(nScheduledActions > 0) 
  {
    // Step 2.0.1: Reset flags
    bOriginChanged = false;

    // STEP 2.1: Increment the cycle count  (DK CLEANUP  why???)
		nCycle++;
    if(iTempCycle++ > MAX_CYCLES_PER_PICK)
    {
      if(!bHeavyHanded)
      {
        CDebug::Log(DEBUG_MINOR_WARNING,
          "Action()loop.  Cycling (%d) times. Something's amiss!\n",iTempCycle);
        // Get Heavy Handed.  Change all remaining states to 
        //   ORIGIN_STABILIZED/STATE_PICK_UNASSOCIABLE.
        for(i=0; i < nScheduledActions; i++)
        {
          if(Act[i].iAct  == STATE_NEW_UNASSIGNED_PICK ||
            Act[i].iAct  == STATE_PICK_UNASSIGNABLE ||
            Act[i].iAct  == STATE_PICK_UNASSOCIABLE)
          {
            Act[i].iAct  = STATE_PICK_UNASSOCIABLE;
          }
          else
          {
            Act[i].iAct  = STATE_ORIGIN_STABILIZED;
          }
        }
        // Mark bHeavyHanded as true
        bHeavyHanded = true;
        
        // Reset the temp cycle counter
        iTempCycle = 0;

        // Hopefully the state machine will wrap everything up in short order
        // but we need to put in a 'hand of god' wrap-up just in case.
      }  // end if ! bHeavyHanded
      else
      {
        CDebug::Log(DEBUG_MAJOR_WARNING,
          "Action()loop.  Cycling (%d) times in Heavy Handed Mode! GLASS Possibly in Trouble!\n",iTempCycle);
        // Get VERY Heavy Handed.  Clean the deck!
        nScheduledActions = 0;

        return;
      }// end else bHeavyHanded
    }  // end if(iTempCycle++ > MAX_CYCLES_PER_PICK)



    // Step 2.2:  Copy the Entity and action values to local storage.
		strcpy(ent, Act[0].sEnt);
		iact = Act[0].iAct;

   
    // Step 2.3:  Initialize task to unknown
		iNextState = STATE_UNKNOWN;

    // Step 2.4:  Figure out which task to perform, by going through
    //            the list of tasks until we find a match.
    //            PERFORM THE TASK, and RECORD THE RESULT
		switch(iact) 
    {
    // dummy states
    case STATE_NEW_UNASSIGNED_PICK:
      //  There is a new unassigned pick  (could be recycled)
			res = 1;  // Dummy State
			break;
    case STATE_PICK_UNASSIGNABLE:
      //  There is a new pick that could not be assigned
			res = 1;  // Dummy State
			break;
    case STATE_PICK_UNASSOCIABLE:
      //  There is a new pick that could not be associated
      //CDebug::Log(DEBUG_MINOR_INFO,"Pick Unused(%s)\n", ent);
			res = 1;  // Dummy State
			break;

    case STATE_ORIGIN_DELETED:
      //  The origin was deleted
			res = 1;  // Dummy State
			break;
		case STATE_ORIGIN_CHANGED:
      //  The origin parameters were changed by something other than the 
      //  locator.
			res = 1;  // Dummy State
			break;

    case STATE_ORIGIN_LOCATED:
      //  The location of the origin was changed
			res = 1;  // Dummy State
			break;

		case STATE_ORIGIN_ASSOCIATIONS_CHANGED:
      //  The pick associations for an origin were changed
			res = 1;  // Dummy State
			break;

		case STATE_ORIGIN_STABILIZED:
      //  An Origin was stabilized
			res = 1;  // Dummy State
			break;

		case STATE_ORIGIN_VALIDATED:
      //  An Origin was validated
			res = 1;  // Dummy State
			break;

		case STATE_ORIGIN_INVALIDATED:
      //  An Origin was invalidated
			res = 1;  // Dummy State
			break;

    case ACT_ASSIGN:
      //CDebug::Log(DEBUG_MINOR_WARNING,"Attempting to Assign new pick(%s)\n", ent);
      //  Associate a Pick with one of the existing quakes.
      res = Assign(ent);
      if(res == 1)
      {
        iNextState = STATE_ORIGIN_ASSOCIATIONS_CHANGED;
        // we are jumping to a dummy state.  Schedule it and continue!
        pPick = pGlint->getPickFromidPick(ent);
        if(!pPick)
        {
          CDebug::Log(DEBUG_MINOR_ERROR,"Action(ACT_ASSIGN) Couldn't get pick(%s), in transition %d.\n",
            ent, i);
          break;
        }
        
        pOrg = pGlint->getOriginForPick(pPick); 
        if(!pOrg)
        {
          CDebug::Log(DEBUG_MINOR_ERROR,"Action(ACT_ASSIGN) Couldn't get origin for pick(%s) (iOrigin=%d), in transition %d.\n",
            ent, pPick->iOrigin);
          break;
        }
        strcpy(ent,pOrg->idOrigin);
      }
      else
      {
        iNextState = STATE_PICK_UNASSIGNABLE;
      }
			break;

		case ACT_ASSOCIATE:
      //  Associate a new quake, from  a new pick and the 
      //  existing unassociated picks.
			res = Associate(ent);
      if(res == 1)
      {
        iNextState = STATE_ORIGIN_ASSOCIATIONS_CHANGED;
        // we are jumping to a dummy state.  Schedule it and continue!
        pPick = pGlint->getPickFromidPick(ent);
        if(!pPick)
        {
          CDebug::Log(DEBUG_MINOR_ERROR,"Action(ACT_ASSOCIATE) Couldn't get pick(%s), in transition %d.\n",
            ent, i);
          break;
        }
        
        pOrg = pGlint->getOriginForPick(pPick);  
        if(!pOrg)
        {
          CDebug::Log(DEBUG_MINOR_ERROR,"Action(ACT_ASSOCIATE) Couldn't get origin for pick(%s) (iOrigin=%d), in transition %d.\n",
            ent, pPick->iOrigin);
          break;
        }
        strcpy(ent,pOrg->idOrigin);
      }
      else
        iNextState = STATE_PICK_UNASSOCIABLE;
			break;

		case ACT_LOCATE:
      //CDebug::Log(DEBUG_MINOR_WARNING,"Locating quake(%s)\n", ent);
      //  "Locate" an origin.  (Send to the locator)
			res = Locate(ent, "TNEZ");
      iNextState = STATE_ORIGIN_LOCATED;
			break;

		case ACT_LOCATE_FIX:
      //  Calculate residuals for an origin. (Send to the locator, but with fixed location)
			res = Locate(ent, 0);
      iNextState = STATE_ORIGIN_LOCATED;
			break;

		case ACT_PRUNE:
      //  Remove picks from an origin that don't have the requisite affinity level.
      //  Return them to the unassociated pick(waif) pool.
			res = Prune(ent);
      if(res == 1)
      {
        //CDebug::Log(DEBUG_MINOR_WARNING,"Pruned quake(%s)\n", ent);
        iNextState = STATE_ORIGIN_ASSOCIATIONS_CHANGED;
      }
      else
        iNextState = STATE_ORIGIN_STABILIZED;
			break;

		case ACT_VALIDATE:
      //  Remove picks from an origin that don't have the requisite affinity level.
      //  Return them to the unassociated pick(waif) pool.
			res = ValidateOrigin(ent);
      if(res == 1)
      {
        CDebug::Log(DEBUG_MINOR_WARNING,"Validated quake(%s)\n", ent);
        iNextState = STATE_ORIGIN_VALIDATED;
      }
      else
      {
        CDebug::Log(DEBUG_MINOR_WARNING,"Invalidated quake(%s)\n", ent);
        iNextState = STATE_ORIGIN_INVALIDATED;
      }
			break;

		case ACT_SCAVENGE:
      //  Search other Origins for Picks that better fit the given Origin.
			res = Scavenge(ent);
      if(res == 1)
      {
        //CDebug::Log(DEBUG_MINOR_WARNING,"Scavenged quake(%s)\n", ent);
        iNextState = STATE_ORIGIN_ASSOCIATIONS_CHANGED;
      }
      else
        iNextState = STATE_ORIGIN_STABILIZED;
			break;

		case ACT_FOCUS:
      //  Refocus an Origin.  (trick to attempt to break out of bad local minimum)
			res = Focus(ent);


      if(res == 1)
      {
        iNextState = STATE_ORIGIN_ASSOCIATIONS_CHANGED;
        //CDebug::Log(DEBUG_MINOR_WARNING,"Focused quake(%s)\n", ent);
      }
      else
        iNextState = STATE_ORIGIN_STABILIZED;
      break;

		case ACT_DELETE:
      //  Delete the Origin.  (it has falled into a pitiful state)
			res = DeleteOrigin(ent);
      if(res == 1)
        iNextState = STATE_ORIGIN_DELETED;
      else
      {
        CDebug::Log(DEBUG_MAJOR_INFO,"Origin %s: Delete Could not delete Origin!\n",
                    ent);
      }
			break;

		case ACT_UPDATE_GUI:
      //  Update the GUI
			res = UpdateGUI(ent);
        iNextState = STATE_FINISHED;
			break;

		case ACT_WAIF:
      //  Scan the unassociated-pick pool
			res = Waif(ent);
      if(res == 1)
      {
        iNextState = STATE_ORIGIN_ASSOCIATIONS_CHANGED;
        //CDebug::Log(DEBUG_MINOR_WARNING,"Waifed quake(%s)\n", ent);
      }
      else
        iNextState = STATE_ORIGIN_STABILIZED;
			break;

		default:
      // Unknown Action, record error!
      CDebug::Log(DEBUG_MINOR_ERROR,
                  "Action() ERROR: unrecognized action(%d)\n",
                  iact);
			res = -1;
    } // end switch(action)


    // Step 2.4.2:  Delete the "just performed" task from the task
    //            list.  (moving each remaining task forward, and
    //            decreasing the number of tasks.
		for(i=1; i<nScheduledActions; i++) {
			Act[i-1] = Act[i];
		}
		nScheduledActions--;

    // Step 2.4.3:  If we operated on an origin, and it changed, then
    //              we need to revalidate it.
    if(iNextState == STATE_FINISHED)
    {
      // we're done with this processing tree
      continue;
    }
    else if(iNextState != STATE_UNKNOWN)
    {
      // we are jumping to a dummy state.  Schedule it and continue!
      Action(iNextState, ent);
    }
    else
    {

    // Step 2.5:  Schedule tranistions
    //            Check the transition table with the results of
    //            the previous task, to see if any other tasks 
    //            should be scheduled.
		for(i=0; i<nTrn; i++) 
    {
      // Step 2.5.1:  If the current transition rule matches task/result
      //              of the just completed task, evaluate the transition
      //              rule.
			if(Trn[i].iAct == iact && Trn[i].iRes == res)
      {
        // Step 2.5.1.1:  Calculate a random value to test against the
        //                probability of the current transition rule firing.
        //                (THIS CODE CAUSES A SCHEDULED TASK TO BE FIRED
        //                 a percentage of the time, as indicated by the 
        //                 transition rule probability)
        iprob = (int)(100.0*rand()/RAND_MAX);

        // increment the iNumEval counter
        Trn[i].iNumEval++;

        // Step 2.5.1.2:  If the rule's probablity threshhold is above the
        //                random value, schedule the resulting 
        //                task (from the transition rule)
        if(iprob < Trn[i].iPer) 
        {

          // increment the iNumSuccess counter
          Trn[i].iNumSuccess++;
          
          // make the proper transition
          if(Tsk[Trn[i].iAct].iActType == Tsk[Trn[i].iTrn].iActType)
          {
            // Straight forward transition
            Action(Trn[i].iTrn, ent);
          }
          else if(Tsk[Trn[i].iAct].iActType == ACTION_TYPE_PICK && 
                  Tsk[Trn[i].iTrn].iActType == ACTION_TYPE_ORIGIN)
          {
            // Transition from Pick entity to Origin entity  (associate/assign)
            pPick = pGlint->getPickFromidPick(ent);
            if(!pPick)
            {
              CDebug::Log(DEBUG_MINOR_ERROR,"Action() Couldn't get pick(%s), in transition %d.\n",
                          ent, i);
              break;
            }

            pOrg = pGlint->getOriginForPick(pPick);
            if(!pOrg)
            {
              CDebug::Log(DEBUG_MINOR_ERROR,"Action() Couldn't get origin for pick(%s) (iOrigin=%d), in transition %d.\n",
                          ent, pPick->iOrigin);
              break;
            }
            Action(Trn[i].iTrn, pOrg->idOrigin);
          }
          else
          {
            CDebug::Log(DEBUG_MINOR_ERROR,"Action() Improper transition(%d %d %s - rule %d)\n",
                        Tsk[Trn[i].iAct].iActType, Tsk[Trn[i].iTrn].iActType, 
                        ent, i);
            return;
          }
          // DK REMOVE  011404 there can be multiple transition rules for each state.
          // remove Carl's break call.    break;
        }
			}
    }  // end for transitions
    }  // end else (state = STATE_UNKNOWN)

    /************* DK REMOVE *******************
		// List new postings
		for(i=0; i<nScheduledActions; i++) {
			if(Act[i].iCycle == nCycle) {
				for(j=0; j<nTsk; j++) {
					if(Tsk[j].iAct == Act[i].iAct) {
            CDebug::Log(DEBUG_MINOR_INFO, "    %s >>> %s\n", Act[i].sEnt, Tsk[j].sTsk);
						break;
					}
				}
			}
		}
    ********** END DK REMOVE *******************/
  }  // end while(nScheduledActions)
}  // end Action()


//---------------------------------------------------------------------------------------Action
// Schedule defered action. Over-rides any previous action scheduled for that entity.
// In current configuration, and even or pick can only be in one state, any new posting
// overrides previous one.
void CGlass::Action(int iact, char *ent) {
	int i;

  // DK CLEANUP is there anything that keeps and Origin and a pick from
  //            having matching entity IDs?????

// DK  Modified this check that prevents an event from being in multiple
//     states at one time......
// Changed it so that the same entity does not get scheduled for the same
//   action more than once at a time.
  for(i=0; i<nScheduledActions; i++) 
  {
    // if the entity AND the action match, then ignore the new request
    if(strcmp(ent, Act[i].sEnt) == 0)
    {
      // Don't post duplicate states.
      // Don't allow origins to be considered "stabilized" if there's
      // more processing scheduled for them
      if(Act[i].iAct == iact || iact == STATE_ORIGIN_STABILIZED) 
      {
        return;
      }
    }
  }
	if(nScheduledActions >= MAXACT)
		return;
	Act[nScheduledActions].iAct = iact;
	Act[nScheduledActions].iCycle = nCycle;
	strcpy(Act[nScheduledActions].sEnt, ent);
	nScheduledActions++;
}


void CGlass::InitGlassStateData()
{
  // CARL 120303
  nCycle = 0;
  nTsk   = 0;
  nTrn   = 0;
  nScheduledActions   = 0;
  // DK CLEANUP pSummary = new CSummary();

  /******** DK REMOVE tasks are now hardcoded 011404
	Task("Assign",		ACT_ASSIGN,     ACTION_TYPE_PICK);
	Task("Associate",	ACT_ASSOCIATE,  ACTION_TYPE_PICK);
	Task("Locate",		ACT_LOCATE,     ACTION_TYPE_ORIGIN);
	Task("LocateFix",	ACT_LOCATE_FIX, ACTION_TYPE_ORIGIN);
	Task("Focus",		  ACT_FOCUS,      ACTION_TYPE_ORIGIN);
	Task("Waif",		  ACT_WAIF,       ACTION_TYPE_ORIGIN);
	Task("Prune",		  ACT_PRUNE,      ACTION_TYPE_ORIGIN);
	Task("Scavenge",	ACT_SCAVENGE,   ACTION_TYPE_ORIGIN);
	Task("Scavenged",	ACT_SCAVENGED,  ACTION_TYPE_ORIGIN);
  ****************/

  // Calculate the size by checking the size of the Tsk array.
  nTsk = sizeof(Tsk) / sizeof(TASK);

}  // end InitGlassStateData()


bool CGlass::ConfigureTransitionRule(CComFile *pcf)
{
	CStr cold;
	CStr cnew;
	double prob1;

  CDebug::Log(DEBUG_MAJOR_INFO,"TRAN-Config\n");
  if(nTrn >= MAXTRN)
  {
    CDebug::Log(DEBUG_MAJOR_ERROR, "Error parsing Glass State Transition Rule[%d] "
                                   "Max Rules(%d) already parsed.!\n",
                nTrn+1, MAXTRN);
    exit(-1);
    return(false);
  }
  cold = pcf->String();
  memset(&Trn[nTrn], 0, sizeof(Trn[nTrn]));
  Trn[nTrn].iAct = Decode(cold.GetBuffer());
  Trn[nTrn].iRes = pcf->Long();
  cnew = pcf->String();
  Trn[nTrn].iTrn = Decode(cnew.GetBuffer());
  prob1 = 0.01*pcf->Double();
  CDebug::Log(DEBUG_MAJOR_INFO,"TRAN-Config: string[%d] = %s %d %s %.2f\n",
    nTrn, cold.String(), Trn[nTrn].iRes, cnew.String(), prob1);
  
  Trn[nTrn].iPer = (int)(100.0*prob1 + 0.5);
  CDebug::Log(DEBUG_MAJOR_INFO,"Config:Trn: %-12s %1d %-12s %3d (%d)\n",
				cold.GetBuffer(), Trn[nTrn].iRes, cnew.GetBuffer(),
        (int)(100.0*prob1+0.5), Trn[nTrn].iPer);
  nTrn++;
  return(true);
}  // end ConfigureTransitionRule()

//---------------------------------------------------------------------------------------Decode
// Program the transition table
int CGlass::Decode(char *act) {
	if(!act)
		return -1;
	for(int itsk=0; itsk<nTsk; itsk++) {
		if(!strcmp(act, Tsk[itsk].sTsk))	return Tsk[itsk].iAct;
	}
	return -2;
}


int CGlass::RegisterUnassociatedPick(char * idPick)
{
	Action(STATE_NEW_UNASSIGNED_PICK, idPick);
  return(0);
}

int CGlass::MarkOriginAsScavenged(char * idOrigin)
{
	Action(STATE_ORIGIN_ASSOCIATIONS_CHANGED, idOrigin);
  return(0);
}

