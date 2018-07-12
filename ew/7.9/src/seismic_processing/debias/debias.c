#include <earthworm.h>
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */
#include "debias.h"

#define VERSION_NUM "0.0.2 2014-02-20"

static DBWORLD db;
static DBPARAM pDb;
static DBEWH eDb;
static int MTB_Command_ID;
static int GWF_Command_ID;
static int AW_Command_ID;

#define NUM_MY_COMMANDS 2
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
     "RIMinTraceBuf",
     "RIAvgWindow"};
static void *ParamTargets[NUM_MY_COMMANDS] = {
    /* Add targets for your commands here
     *
     * If you specify a type of S, I or V above, that command must have
     * the address of where the value is to go specified here
     * Otherwise, ProcessCommand will be called, and what you put here
     * (and what you do with it) is up to you */
     &(pDb.minTraceBufLen),
     &(pDb.avgWindowSecs)};

/*************************************************************************'

    SetupXfrm creates/initializes the world, param and EWH structures, 
    as well as the information about commands to be read from the config 
    file(s)

**************************************************************************/
void SetupXfrm( XFRMWORLD **pXfrm, 
            char **Cmds[], int *CmdCount, void **ParamTarget[] ) 
{    
    *pXfrm = (XFRMWORLD*)&db;
    db.mod_name = MOD_STR;
    db.version = VERSION_ID;
    db.version_magic = VERSION_MAGIC;
    db.xfrmEWH = &eDb; 
    db.xfrmParam = &pDb;
    db.scnlRecSize = sizeof(DBSCNL);
    
    MTB_Command_ID = *CmdCount;
    AW_Command_ID = *CmdCount + 1;
    
    *CmdCount += NUM_MY_COMMANDS;
    *Cmds = Commands;
    *ParamTarget = ParamTargets;
    
}

/*************************************************************************'

    ConfigureXfrm gets called at the end of Configure, after the config
    file(s) and earthworm.h table info have been read in, so any 
    module-specific processing of that data can be done

**************************************************************************/
int ConfigureXfrm( char *init )
{  
  return EW_SUCCESS;
}

/*************************************************************************'

    InitializeXfrmParameters gets called after standard parameters have 
    been initialized, so your module-specific parameters can be initialized

**************************************************************************/
void InitializeXfrmParameters()
{
  int i;
  for( i=0; i<MAX_LOGO; i++ ) {
     eDb.readInstId[i]  = 0;
     eDb.readModId[i]   = 0;
     eDb.readMsgType[i] = 0;
  }

  pDb.minTraceBufLen = 10;
}

/*************************************************************************'

    SpecifyXfrmLogos gets called after standard incomming and outgoing logos
    have been specified, so any specific to this module can be added

**************************************************************************/
void SpecifyXfrmLogos()
{
  db.trcLogo.instid = eDb.myInstId;
  db.trcLogo.mod    = eDb.myModId;
  db.trcLogo.type   = eDb.typeTrace2;
}

/*************************************************************************'

    ReadXfrmEWH gets called at the end of ReadEWH, so any earthworm.h 
    information specific to this module can be handled

**************************************************************************/
int       ReadXfrmEWH()
{
  return EW_SUCCESS;
}

/*************************************************************************'

    FreeXfrmWorld gets called just before the module exits, so that any 
    module-specific cleanup can be done

**************************************************************************/
void      FreeXfrmWorld()
{
}


/*************************************************************************'

    ProcessXfrmCommand gets called when either the type of a known command is
    unknown (cmd_id will be its index) or when the command itself is unknown
    (cmd_id will be -1).  This allows any module-specific command processing
    that isn't a simple parse (string, int, double or flag) to be handled.

**************************************************************************/
int ProcessXfrmCommand( int cmd_id, char *com )
{
  return -1;  
}

/*************************************************************************'

    ReadXfrmConfig after the config file is processed, so any module-specific
    processing of the config file(s) can be handled

**************************************************************************/
int ReadXfrmConfig( char *init ) 
{
  int i;
/* Make sure that InSCNL and OutSCNL are different */
  for( i=0; i<db.nSCNL; i++ )
  {
    DBSCNL *scnls = (DBSCNL *)(db.scnls);
    if( (strcmp(scnls[i].inSta,  scnls[i].outSta)  == 0) && 
        (strcmp(scnls[i].inChan, scnls[i].outChan) == 0) && 
        (strcmp(scnls[i].inNet,  scnls[i].outNet)  == 0) &&
        (strcmp(scnls[i].inLoc,  scnls[i].outLoc)  == 0)     )
    {
      logit ("e", "debias: WARNING: %s.%s.%s.%s will have same "
                  "SCNL after mean-removal!\n",
                   scnls[i].inSta, scnls[i].inChan, 
                   scnls[i].inNet, scnls[i].inLoc );
    }
  }

  return EW_SUCCESS;
}


/*************************************************************************'

    PreprocessXfrmBuffer gets called before the packet is sent off to the 
    worker thread.  If you want the (possibly modified) packet to be passed
    to the thread, return 0; if you don't want it passed on, return -1;
    any other negative value will signify an error and terminate the module.

**************************************************************************/
int PreprocessXfrmBuffer( TracePacket *TracePkt, MSG_LOGO logoMsg, char *inBuf )
{
  return 0;
}


/*************************************************************************'

    ProcessXfrmRejected gets for each packet not matched to a GetSCNL command.
    If it returns EW_FAILURE, module will abort.

**************************************************************************/
int ProcessXfrmRejected( TracePacket *TracePkt, MSG_LOGO logoMsg, char *inBuf )
{
  return EW_SUCCESS;
}

thr_ret XfrmThread (void* db)
{
#ifdef _WINNT
  BaseXfrmThread( db );
  return;
#else
  return BaseXfrmThread( db );
#endif
}
