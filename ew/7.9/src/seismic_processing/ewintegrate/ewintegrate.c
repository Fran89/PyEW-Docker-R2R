#include <earthworm.h>
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */
#include "ewintegrate.h" /* your module's header */

static INTWORLD ewi;
static INTPARAM pEwi;
static INTEWH eEwi;

#define NUM_MY_COMMANDS 4
static char *Commands[NUM_MY_COMMANDS] = { 
    /* List your commands here
     *
     * 1st character: R for required, O for optional
     * 2nd character: type
     *     S = string
     *     I = int
     *     V = double
     *     F = boolean int flag (present=1, absent=0)
     *     Otherwise, command will go to ProcessCommand w/ index of command
     * If command isn't understood, will be sent to ProcessCommand
     **************************************************************/
     "OIAvgWindow",
     "OFDoIntegration",
     "OVHighPassFreq", 
     "OIHighPassOrder"
};
static void *ParamTargets[NUM_MY_COMMANDS] = {
    /* Add targets for your commands here
     *
     * If you specify a type of S, I or V above, that command must have
     * the address of where the value is to go specified here
     * Otherwise, ProcessCommand will be called, and what you put here
     * (and what you do with it) is up to you */
     &(pEwi.avgWindowSecs),
     &(pEwi.doIntegration),
     &(pEwi.hpFreqCutoff),
     &(pEwi.hpOrder)
};

/* ID#s of our added commands */
static int AvgWindowID;
static int DoIntegrationID;
static int HPFreqID;
static int HPOrderID;

void SetupXfrm( XFRMWORLD **pXfrm, 
            char **Cmds[], int *CmdCount, void **ParamTarget[] ) 
{    
    *pXfrm = (XFRMWORLD*)&ewi;
    ewi.mod_name = MOD_STR;
    ewi.xfrmEWH = &eEwi; 
    ewi.xfrmParam = &pEwi;
    ewi.scnlRecSize = sizeof(INTSCNL);
    pEwi.avgWindowSecs = 0;
    pEwi.doIntegration = 0;
    pEwi.hpOrder = 0;
    
	AvgWindowID = (*CmdCount)++;
	DoIntegrationID = (*CmdCount)++;
    HPFreqID = (*CmdCount)++;
    HPOrderID = (*CmdCount)++;
    *Cmds = Commands;
    *ParamTarget = ParamTargets;
}

int ConfigureXfrm()
{  
  return EW_SUCCESS;
}

void InitializeXfrmParameters()
{
}

void SpecifyXfrmLogos()
{
  ewi.trcLogo.instid = eEwi.myInstId;
  ewi.trcLogo.mod    = eEwi.myModId;
  ewi.trcLogo.type   = eEwi.typeTrace2;
}

int       ReadXfrmEWH()
{
  return EW_SUCCESS;
}

void      FreeXfrmWorld()
{
}


int ProcessXfrmCommand( int cmd_id, char *com )
{
    return -1;
}

int ReadXfrmConfig( char *init ) 
{ 
  if ( init[HPFreqID] != init[HPOrderID] ) 
  {
    logit( "e", "ewintegrate: use of high-pass filter requires specification of both frequency and order\n" );
    return EW_FAILURE;
  }
  else if ( init[HPOrderID] && pEwi.hpOrder < 1 )
  {
    logit( "e", "ewintegrate: %s must be > 0\n", Commands[0]+2 );
    return EW_FAILURE;
  }
  if ( !init[AvgWindowID] && !init[DoIntegrationID] && !init[HPOrderID] )
  {
    logit( "e", "ewintegrate: at least one stage (debias, integrate, filter) must be specified\n" );
  }
  return EW_SUCCESS;
}


int PreprocessXfrmBuffer( TracePacket *TracePkt, MSG_LOGO logoMsg, char *inBuf )
{
  return 0;
}


int ProcessXfrmRejected( TracePacket *TracePkt, MSG_LOGO logoMsg, char *inBuf )
{
  return EW_SUCCESS;
}

thr_ret XfrmThread (void* ewi)
{
  return BaseXfrmThread( ewi );
}
