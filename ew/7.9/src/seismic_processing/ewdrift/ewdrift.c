#include <earthworm.h>
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */
#include "ewdrift.h" /* your module's header */

static INTWORLD ewi;
static INTPARAM pEwi;
static INTEWH eEwi;

#define NUM_MY_COMMANDS 1
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
     "OSMyModId"
};
static void *ParamTargets[NUM_MY_COMMANDS] = {
    /* Add targets for your commands here
     *
     * If you specify a type of S, I or V above, that command must have
     * the address of where the value is to go specified here
     * Otherwise, ProcessCommand will be called, and what you put here
     * (and what you do with it) is up to you */
     NULL
};

/* ID#s of our added commands */

void SetupXfrm( XFRMWORLD **pXfrm,
            char **Cmds[], int *CmdCount, void **ParamTarget[] )
{
    *pXfrm = (XFRMWORLD*)&ewi;
    ewi.mod_name = MOD_STR;
    ewi.xfrmEWH = &eEwi;
    ewi.xfrmParam = &pEwi;
    ewi.scnlRecSize = sizeof(INTSCNL);
    ewi.useInBufPerSCNL = 1;  /* use input buffer per SCNL */
    ewi.nInSCNL = 2;  /* 2 input SCNLs */

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
#ifndef _WINNT
  return
#endif
          BaseXfrmThread(ewi);
}
