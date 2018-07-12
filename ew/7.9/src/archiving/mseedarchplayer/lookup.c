/*********************************************************************
 *  lookup( )   Look up important info from earthworm.h tables       *
 *********************************************************************/
 
// Standard includes
#include <stdlib.h>
#include <earthworm.h>

// Local Includes
#include "mseedarchplayer.h" 
 
void lookup(EWH *Ewh, PARAMS *Parm) {
    /* Look up installations of interest
     *********************************/
    if (GetLocalInst(&(Parm->InstId)) != 0) {
        logit("e",
                "ewhtmlemail: error getting local installation id; exiting!\n");
        exit(-1);
    }

    /* Look up message types of interest
     *********************************/
    if (GetType("TYPE_HEARTBEAT", &(Ewh->TypeHeartBeat)) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_ERROR", &(Ewh->TypeError)) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_ERROR>; exiting!\n");
        exit(-1);
    }
    if (GetType("TYPE_TRACEBUF2", &(Ewh->TypeTraceBuf2)) != 0) {
        logit("e",
                "ewhtmlemail: Invalid message type <TYPE_TRACEBUF2>; exiting!\n");
        exit(-1);
    }
    
    // Process ID
   if( (Ewh->MyPid = getpid()) == -1 )
   {
      logit ("e", "mseedarchplayer: Call to getpid failed. Exiting.\n");
      exit( -1 );
   }
   
   /*
   // Message logos - Hearbeat
   Ewh->htblogo.instid = Parm->InstId;
   Ewh->htblogo.mod = Parm->MyModId;
   Ewh->htblogo.type = Ewh->TypeHeartBeat;
   
   // Message logos - Error
   Ewh->errlogo.instid = Parm.InstId;
   Ewh->errlogo.mod = Parm.MyModId;
   Ewh->errlogo.type = Ewh->TypeError;
   */
   
   // Message logos - tracebuf2
   Ewh->trb2logo.instid = Parm->InstId;
   Ewh->trb2logo.mod = Parm->MyModId;
   Ewh->trb2logo.type = Ewh->TypeTraceBuf2;

   return;
}
