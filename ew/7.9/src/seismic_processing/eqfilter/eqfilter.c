/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqfilter.c 6488 2016-04-19 09:12:00Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2007/02/26 19:45:48  paulf
 *     yet another time_t fix
 *
 *     Revision 1.9  2007/02/26 13:44:40  paulf
 *     fixed heartbeat sprintf() to cast time_t as long
 *
 *     Revision 1.8  2004/07/23 16:34:39  dietz
 *     removed unused #defines
 *
 *     Revision 1.7  2002/12/10 19:12:59  dietz
 *     Added new test on total number of phases with weight > 0.0
 *
 *     Revision 1.6  2002/06/05 15:08:52  patton
 *     Made Logit changes.
 *
 *     Revision 1.5  2001/05/09 18:36:33  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPid.
 *
 *     Revision 1.4  2001/04/26 00:43:06  dietz
 *     Fixed bug where events would be ignored if tport_getmsg returned GET_MISS or GET_NOTRACK.
 *
 *     Revision 1.3  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.2  2000/07/09 16:52:44  lombard
 *     Fixed return status of several functions; replaced `return(-1)' in
 *     Processor() with logit() calls and KillSelfThread().
 *
 *     Revision 1.1  2000/02/14 17:08:44  lucky
 *     Initial revision
 *
 *
 */

/*
 * eqfilter.c:     blame: Mitch Withers September, 1999
 *
 *   hacked from Lucky Vidmar's authreg module. Essentially operates the same
 *     with added functionality (Lucky's description is below).  Added tests are:

     Depth test
      is hypo between MinDepth and MaxDepth km
     Keyword    InstID         MinDepth  MaxDepth
     DepthTest   INST_MEMPHIS   0.0       25.0
     
     number phase (high weight) test
      are there at least NPhase phases with phase weight greater than 0.1
     Keyword    InstID         NPhase    PhWeight
     NPhaseTest  INST_MEMPHIS   9         0.1 
     
     number phase (any weight) test
      are there at least NPhase phases with phase weight greater than 0.0
     Keyword          InstID         NPhaseTotal   PhWeight
     NPhaseTotalTest  INST_MEMPHIS   9            0.0 
    
     gap test
      is the gap < MaxGap  degrees
     Keyword    InstID         MaxGap
     GapTest     INST_MEMPHIS   270.0
     
     dmin test
      is distance to nearest station at no greater than MaxDmin km
     Keyword    InstID         MaxDmin
     DminTest    INST_MEMPHIS   50.0
      
     rms test
      is the rms less than MaxRMS seconds
     Keyword    InstID         MaxRMS
     RMSTest     INST_MEMPHIS   0.5
      
     e0 test
      is the largest principal error less than MaxE0 km
     Keyword    InstID         MaxE0
     E0Test      INST_MEMPHIS   2.0
     erh test
      is the horizontal error less than MaxERH km
     Keyword    InstID         MaxERH
     ERHTest     INST_MEMPHIS   2.0
      
     erz test
      is the vertical error less than MaxERZ km
     Keyword    InstID         MaxERZ
     ERZTest     INST_MEMPHIS   MaxERZ
      
     Mag test
      is the magnitude greater than MinMag
     Keyword    InstID         MinMag
     MagTest     INST_MEMPHIS   2.0
      
     Ncoda test
      given an event with Magnitude Mag, are there at least MinC coda picks
        kind of kludgy, but we get the number of coda picks by reading the
        phase lines, then just counting the number of coda lengths > 0
     Keyword    InstID         MinC  Mag
     NcodaTest   INST_MEMPHIS   4     2.0
     NcodaTest   INST_MEMPHIS   10    3.0
     NcodaTest   INST_MEMPHIS   20    4.0
      
 *
 *
 *   This module picks up TYPE_HYP2000ARC messages from the InRing,
 *   and checks the location of the event against the authoritative
 *   regions defined in the configuration file. If the event is 
 *   is located within the authoritative region for the installation
 *   which computed the location, the TYPE_HYP2000ARC message is 
 *   written to the OutRing unchanged. Otherwise, the message is 
 *   not written out.  This, in effect, filters out the locations
 *   computed outside of an installation's region of authority.
 *
 *    Initial version 
 *      Lucky Vidmar Thu Aug 12 12:28:08 MDT 1999  
 *
 */

/* added version numbers starting in 2014!
 *
 * 0.0.2 2014-04-18 added in polarityTester to check for teleseism (which often have same polarity)
 *
 */

#define VERSION_NUM "0.6 2015-11-16"

#ifdef _OS2
#define INCL_DOSMEMMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <swap.h>
#include <transport.h>
#include <trace_buf.h>
#include <mem_circ_queue.h>
#include <sys/types.h>
#include <read_arc.h>
#include <chron3.h>

#include "eqfilter.h"
#include "dist.h"

static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;     /* shared memory region to use for i/o    */


#define   NUM_COMMANDS 	6 	/* how many required commands in the config file */
#define   MAX_STR 255

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;
 
/* The message queue
 *******************/
#define	QUEUE_SIZE		1000	    /* How many msgs can we queue */
QUEUE 	MsgQueue;				/* from queue.h */

/* Thread stuff */
#define THREAD_STACK 8192
static unsigned tidProcessor;    /* Processor thread id */
static unsigned tidStacker;      /* Thread moving messages from InRing */
                                 /* to MsgQueue */
int MessageStackerStatus = 0;      /* 0=> Stacker thread ok. <0 => dead */
int ProcessorStatus = 0;           /* 0=> Processor thread ok. <0 => dead */



/* Things to read or derive from configuration file
 **************************************************/
static char    InRingName[MAX_RING_STR];      /* name of transport ring for i/o    */
static char    OutRingName[MAX_RING_STR];     /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* this module's given name          */
static char    MyProgName[256];     /* this module's program name        */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */
static int     Debug = 0;   			/* 1- print debug, 0- no debug       */
static int     AllowUndefInst = 0; 		/* 1- write out msgs from unknown installations */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          InKey;         /* key of transport ring for i/o     */
static long          OutKey;        /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeArc;
static unsigned char InstWild;
static unsigned char ModWild;

/* polarity values
 *********************************/
static int polarityPercent = 0.0;
static int minPolarityValues = 10;
static int maxPolarityGap = 180;

/* Error messages used by eqfilter 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/

/* Function Prototypes */
int read_hyp (char *, char *, struct Hsum *);
int read_phs (char *, char *, struct Hpck *);


/* Functions in this source file 
 *******************************/
static	int  	eqfilter_config (char *);
static	int  	eqfilter_lookup (void);
static	void  	eqfilter_status (unsigned char, short, char *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);
int 			area (int *, int, float *, float *, float *, float *);
int 			cntsct (int, float *, float *, float *, float *);
int 			isect (float, float, float, float, float *, float *);
void IsLimit1LTArgLTLimit2(int *, int, PARTEST2 *, unsigned char, float, int *);
void IsLimitLTArg(int *, int, PARTEST1 *, unsigned char, float, int *);
void IsLimitGTArg(int *, int, PARTEST1 *, unsigned char, float, int *);


int main (int argc, char **argv)
{
  time_t		timeNow;       /* current time                  */ 
  time_t		timeLastBeat;  /* time last heartbeat was sent  */
  long			recsize;       /* size of retrieved message     */
  MSG_LOGO		reclogo;       /* logo of retrieved message     */
  char			*flushmsg;
  int			i, j, k;

  /* Check command line arguments 
   ******************************/
  if (argc != 2)
  {
    fprintf (stderr, "Usage: eqfilter configfile\n");
    fprintf (stderr, "Version %s \n", VERSION_NUM);
    return EW_FAILURE;
  }

  /* To be used in logging functions
   *********************************/
  if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
  {
    fprintf (stderr, "eqfilter: Call to get_prog_name failed.\n");
    return EW_FAILURE;
  }

  /* Initialize name of log-file & open it 
   ***************************************/
  logit_init (argv[1], 0, 256, 1);

  /* Read the configuration file(s)
   ********************************/
  if (eqfilter_config(argv[1]) != EW_SUCCESS)
  {
    logit( "e", "eqfilter: Call to eqfilter_config failed \n");
    return EW_FAILURE;
  }
  logit ("" , "%s(%s): Read command file <%s>\n", 
         MyProgName, MyModName, argv[1]);
  logit ("t", "eqfilter Version %s \n", VERSION_NUM);

  /* Look up important info from earthworm.h tables
   ************************************************/
  if (eqfilter_lookup() != EW_SUCCESS)
  {
    logit( "e", "%s(%s): Call to eqfilter_lookup failed \n",
             MyProgName, MyModName);
    return EW_FAILURE;
  }

  /* Reinitialize logit to desired logging level
   *********************************************/
  logit_init (argv[1], 0, 256, LogSwitch);

  /* Get our process ID
   **********************/
  if ((MyPid = getpid ()) == -1)
  {
    logit ("e", "%s(%s): Call to getpid failed. Exiting.\n",
           MyProgName, MyModName);
    return (EW_FAILURE);
  }

  /* Bail if InRing == OutRing
   ***************************/
  if(strcmp(InRingName,OutRingName)==0)
  {
    logit("e","FATAL ERROR: eqfilter InRing=%s == OutRing = %s. Exiting\n",
          InRingName,OutRingName);
    return(EW_FAILURE);
  }

  /* Attach to Input shared memory ring 
   *******************************************/
  tport_attach (&InRegion, InKey);
  logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
         MyProgName, MyModName, InRingName, InKey);

  /* Attach to Output shared memory ring 
   *******************************************/
  tport_attach (&OutRegion, OutKey);
  logit ("", "%s(%s): Attached to public memory region %s: %d\n", 
         MyProgName, MyModName, OutRingName, OutKey);

  /* Force a heartbeat to be issued in first pass thru main loop
   *************************************************************/
  timeLastBeat = time (&timeNow) - HeartBeatInterval - 1;

  /* Flush the incoming transport ring 
   ***********************************/
  if ((flushmsg = (char *) malloc (MAX_BYTES_PER_EQ)) ==  NULL)
  {
    logit ("e", "eqfilter: can't allocate flushmsg; exiting.\n");
    return EW_FAILURE;
  }

  while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
         &recsize, flushmsg, (MAX_BYTES_PER_EQ - 1)) != GET_NONE);

  /* Show off our regions, if Debug is requested
   **********************************************/
  if (Debug == 1)
  {
    logit ("", "eqfilter: Program starting - NumInsts = %d:\n", numInst);
    for (i = 0; i < numInst; i++)
    {
      logit ("", "Inst %d - %d Inclusion regions \n", AuthReg[i].instid,
             AuthReg[i].numIncReg);
      for (j = 0; j < AuthReg[i].numIncReg; j++)
      {
        logit ("", "Reg %d (%d): ", j, AuthReg[i].IncRegion[j].num_sides);
        for (k = 0; k < (AuthReg[i].IncRegion[j].num_sides + 1); k++)
        {
          logit ("", "[%0.2f, %0.2f] ", AuthReg[i].IncRegion[j].x[k],
                 AuthReg[i].IncRegion[j].y[k]);
        }
        logit ("", "\n");
      }

      logit ("", "Inst %d - %d Exclusion regions \n", AuthReg[i].instid,
             AuthReg[i].numExcReg);
      for (j = 0; j < AuthReg[i].numExcReg; j++)
      {
        logit ("", "Reg %d (%d): ", j, AuthReg[i].ExcRegion[j].num_sides);
        for (k = 0; k < (AuthReg[i].ExcRegion[j].num_sides + 1); k++)
        {
          logit ("", "[%0.2f, %0.2f] ", AuthReg[i].ExcRegion[j].x[k],
                 AuthReg[i].ExcRegion[j].y[k]);
        }
        logit ("", "\n");
      }
    }
    if(NDepthTest>0)
    {
      for(i=0; i<NDepthTest; i++)
      {
        logit("","DepthTest: %d %f %f\n",DepthTest[i].instid,
              DepthTest[i].var1,DepthTest[i].var2);
      }
    }
 
    if(NnphTest>0)
    {
      for(i=0; i<NnphTest; i++)
      {
        logit("","nphTest: %d %f\n",nphTest[i].instid,nphTest[i].var);
      }
    }

    if(NnphtotTest>0)
    {
      for(i=0; i<NnphtotTest; i++)
      {
        logit("","nphtotalTest: %d %f\n",nphtotTest[i].instid,nphtotTest[i].var);
      }
    }

    if(NGapTest>0)
    {
      for(i=0; i<NGapTest; i++)
      {
        logit("","GapTest: %d %f\n",GapTest[i].instid,GapTest[i].var);
      }
    }
 
    if(NDminTest>0)
    {
      for(i=0; i<NDminTest; i++)
      {
        logit("","DminTest: %d %f\n",DminTest[i].instid,DminTest[i].var);
      }
    }
 
    if(NRMSTest>0)
    {
      for(i=0; i<NRMSTest; i++)
      {
        logit("","RMSTest: %d %f\n",RMSTest[i].instid,RMSTest[i].var);
      }
    }
 
    if(NMaxE0Test>0)
    {
      for(i=0; i<NMaxE0Test; i++)
      {
        logit("","MaxE0Test: %d %f\n",MaxE0Test[i].instid,MaxE0Test[i].var);
      }
    }
 
    if(NMaxERHTest>0)
    {
      for(i=0; i<NMaxERHTest; i++)
      {
        logit("","MaxERHTTest: %d %f\n",MaxERHTest[i].instid,MaxERHTest[i].var);
      }
    }
 
    if(NMaxERZTest>0)
    {
      for(i=0; i<NMaxERZTest; i++)
      {
        logit("","MaxERZTest: %d %f\n",MaxERZTest[i].instid,MaxERZTest[i].var);
      }
    }
 
    if(NMinMagTest>0)
    {
      for(i=0; i<NMinMagTest; i++)
      {
        logit("","MinMagTest: %d %f\n",MinMagTest[i].instid,MinMagTest[i].var);
      }
    }

    if(NNcodaTest>0)
    {
      for(i=0; i<NNcodaTest; i++)
      {
        logit("","NcodaTest: %d %f %f\n",NcodaTest[i].instid,NcodaTest[i].var1,NcodaTest[i].var2);
      }
    }
    
    if(NMaxDistTest>0)
    {
      for(i=0; i<NMaxDistTest; i++)
      {
        logit("","NMaxDistTest: %d %f %f %f\n",MaxDistTest[i].instid,MaxDistTest[i].var1,MaxDistTest[i].var2,MaxDistTest[i].var3);
      }
    }
    if(MaxOriginAgeSecs>0)
    {
      logit("","MaxOriginAgeSecs: %d\n",MaxOriginAgeSecs);
    }
    

  }

  /* Create MsgQueue mutex */
  CreateMutex_ew();

  /* Allocate the message Queue
   ********************************/
  initqueue (&MsgQueue, QUEUE_SIZE, MAX_BYTES_PER_EQ);

  /* Start message stacking thread which will read 
   * messages from the InRing and put them into the Queue 
   *******************************************************/
  if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
  {
    logit( "e", "eqfilter: Error starting MessageStacker thread.  Exiting.\n");
    tport_detach (&InRegion);
    tport_detach (&OutRegion);
    return EW_FAILURE;
  }

  MessageStackerStatus = 0; /*assume the best*/

  /* Start processing thread which will read messages from
   * the Queue, process them and write them to the OutRing
   *******************************************************/
  if (StartThread (Processor, (unsigned) THREAD_STACK, &tidProcessor) == -1)
  {
    logit( "e", "eqfilter: Error starting Processor thread.  Exiting.\n");
    tport_detach (&InRegion);
    tport_detach (&OutRegion);
    return EW_FAILURE;
  }

  ProcessorStatus = 0; /*assume the best*/

/*--------------------- setup done; start main loop -------------------------*/

  /* We don't do much here - just beat our heart 
   * and check on our threads 
   **********************************************/
  while (tport_getflag (&InRegion) != TERMINATE  &&
         tport_getflag (&InRegion) !=  MyPid )
  {

    /* send eqfilter' heartbeat
    ***************************/
    if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
    {
      timeLastBeat = timeNow;
      eqfilter_status (TypeHeartBeat, 0, ""); 
    }

    /* Check on our threads */
    if (MessageStackerStatus < 0)
    {
      logit ("et", "eqfilter: MessageStacker thread died. Exiting\n");
      return EW_FAILURE;
    }

    if (ProcessorStatus < 0)
    {
      logit ("et", "eqfilter: Processor thread died. Exiting\n");
      return EW_FAILURE;
    }

    sleep_ew (1000);

  } /* wait until TERMINATE is raised  */  

  /* Termination has been requested
   ********************************/
  tport_detach (&InRegion);
  tport_detach (&OutRegion);
  logit ("t", "eqfilter: Termination requested; exiting!\n" );
  return EW_SUCCESS;

}

/******************************************************************************
 *  eqfilter_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int eqfilter_config (char *configfile)
{
  char  init[NUM_COMMANDS];   /* init flags, one byte for each required command */
  int  	nmiss;                /* number of required commands that were missed   */
  char 	*com;
  char 	*str;
  int  	nfiles;
  int  	success;
  int  	i, j, gotit, num_sides, tmpint;
  unsigned char	instid;

  /* Set to zero one init flag for each required command 
  *****************************************************/   
  for (i = 0; i < NUM_COMMANDS; i++)
    init[i] = 0;

  nLogo = 0;
  numInst = 0;

  NDepthTest=0;
  NnphTest=0;
  NnphtotTest=0;
  NGapTest=0;
  NDminTest=0;
  NRMSTest=0;
  NMaxE0Test=0;
  NMaxERHTest=0;
  NMaxERZTest=0;
  NMinMagTest=0;
  NNcodaTest=0;
  NMaxDistTest = 0;
  MaxOriginAgeSecs = 0;
  NAllocMaxDistTest = 10;
  MaxDistTest = malloc (sizeof(*MaxDistTest)*NAllocMaxDistTest);
  if ( MaxDistTest == NULL)
  {
    logit ("e", "eqfilter: error allocating MaxDistTest table; exiting!\n");
    return EW_FAILURE;
  }



  /* Open the main configuration file 
   **********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    logit("e", "eqfilter: Error opening command file <%s>; exiting!\n", configfile);
    return EW_FAILURE;
  }

  /* Process all command files
   ***************************/
  while (nfiles > 0)   /* While there are command files open */
  {
    while (k_rd ())        /* Read next line from active file  */
    {  
      com = k_str ();         /* Get the first token from line */

      /* Ignore blank lines & comments
       *******************************/
      if (!com)
        continue;

      if (com[0] == '#')
        continue;

      /* Open a nested configuration file 
       **********************************/
      if (com[0] == '@') 
      {
        success = nfiles + 1;
        nfiles  = k_open (&com[1]);
        if (nfiles != success) 
        {
          logit("e", "eqfilter: Error opening command file <%s>; exiting!\n",
                   &com[1]);
          return EW_FAILURE;
        }
        continue;
      }

      /* Process anything else as a command 
       ************************************/
/*0*/ if (k_its ("MyModuleId")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (MyModName, str);
          init[0] = 1;
        }
      }
/*1*/ else if (k_its ("InRing")) 
      {
      if ((str = k_str ()) != NULL)
        {
          strcpy (InRingName, str);
          init[1] = 1;
        }
      }
/*2*/ else if (k_its ("OutRing")) 
      {
        if ((str = k_str ()) != NULL)
        {
          strcpy (OutRingName, str);
          init[2] = 1;
        }
      }
/*3*/ else if (k_its ("HeartBeatInt")) 
      {
        HeartBeatInterval = k_long ();
        init[3] = 1;
      }
/*4*/ else if (k_its ("LogFile"))
      {
        LogSwitch = k_int();
        init[4] = 1;
      }

/*NR*/else if (k_its ("Debug"))
      {
        Debug = 1;
      }

/*NR*/else if (k_its ("AllowUndefInst"))
      {
        AllowUndefInst = 1;
      }
/*NR*/else if (k_its ("PolarityTest"))
      {
        minPolarityValues = k_int();
        maxPolarityGap = k_int();
        polarityPercent = k_val();
      }

      /* Enter installation & module types to get
       *******************************************/
/*5*/ else if (k_its ("GetEventsFrom")) 
      {
        if (nLogo >= MAXLOGO) 
        {
          logit("e", "eqfilter: Too many <GetMsgLogo> commands in <%s>; "
                   "; max=%d; exiting!\n", configfile, (int) MAXLOGO);
          return EW_FAILURE;
        }
        if ((str = k_str())) 
        {
          if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
          {
            logit("e", "eqfilter: Invalid installation name <%s> in "
                     "<GetEventsFrom> cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if ((str = k_str())) 
        {
          if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
          {
            logit("e", "eqfilter: Invalid module name <%s> in <GetEventsFrom> "
                     "cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }
        /* We'll always fetch arc messages */
        if (GetType ("TYPE_HYP2000ARC", &GetLogo[nLogo].type) != 0) 
        {
          logit("e", "eqfilter: Invalid msgtype <%s> in <GetEventsFrom> "
                   "cmd; exiting!\n", str);
          return EW_FAILURE;
        }
        nLogo++;
        init[5] = 1;
      }
/*NR*/else if (k_its ("InclRegion")) 
      {
        if ((str = k_str())) 
        {
          if (GetInst (str, &instid) != 0) 
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <InclRegion> "
                     "cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }

        /* see if already have this instid */
        i = -1;
        gotit = FALSE;
        while (gotit == FALSE)
        {
          if ((i = i + 1) >= numInst)
          {
            gotit = TRUE;
            i = -1;
          }
          else if (AuthReg[i].instid == instid)
          {
            gotit = TRUE;
          }

        }

        if (i >= 0)
        {
          tmpint = AuthReg[i].numIncReg;

          /* Read in the arrays */
          num_sides = k_int ();
          if ((num_sides <= 2) || (num_sides > MAX_SIDES))
          {
            logit("e", "eqfilter: Invalid NumSides (%d) in <InclRegion> cmd.\n", num_sides);
            return EW_FAILURE;
          }

          AuthReg[i].IncRegion[tmpint].num_sides = num_sides;

          for (j = 0; (j < num_sides + 1); j++)
          {
            AuthReg[i].IncRegion[tmpint].x[j] = (float) k_val ();
            AuthReg[i].IncRegion[tmpint].y[j] = (float) k_val ();
          }

          AuthReg[i].numIncReg = AuthReg[i].numIncReg + 1;

        }
        else
        {

          /* add this instid to the list */
          if ((numInst + 1) > MAX_INST)
          {
            logit("e", "%s: too many installations; exiting!\n", MyProgName);
            return EW_FAILURE;
          }

          AuthReg[numInst].instid = instid;

          /* Read in the arrays */
          num_sides = k_int ();
          if ((num_sides <= 2) || (num_sides > MAX_SIDES))
          {
            logit("e", "eqfilter: Invalid NumSides (%d) in <InclRegion> cmd.\n", num_sides);
            return EW_FAILURE;
          }
          
          AuthReg[numInst].IncRegion[0].num_sides = num_sides;


          for (j = 0; (j < num_sides + 1); j++)
          {
            AuthReg[numInst].IncRegion[0].x[j] = (float) k_val ();
            AuthReg[numInst].IncRegion[0].y[j] = (float) k_val ();
          }
          
          AuthReg[numInst].numIncReg = 1;
          numInst = numInst + 1;
          
        }
      }

/*NR*/else if (k_its ("ExclRegion")) 
      {

        if ((str = k_str())) 
        {
          if (GetInst (str, &instid) != 0) 
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <ExclRegion> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }

        /* see if already have this instid */
        i = -1;
        gotit = FALSE;
        while (gotit == FALSE)
        {
          if ((i = i + 1) >= numInst)
          {
            gotit = TRUE;
            i = -1;
          }
          else if (AuthReg[i].instid == instid)
          {
            gotit = TRUE;
          }
        }

        if (i >= 0)
        {
          tmpint = AuthReg[i].numExcReg;

          /* Read in the arrays */
	
          num_sides = k_int ();
          if ((num_sides <= 2) || (num_sides > MAX_SIDES))
          {
            logit("e", "eqfilter: Invalid NumSides (%d) in <ExclRegion> cmd.\n", num_sides);
            return EW_FAILURE;
          }

          AuthReg[i].ExcRegion[tmpint].num_sides = num_sides;

          for (j = 0; (j < num_sides + 1); j++)
          {
            AuthReg[i].ExcRegion[tmpint].x[j] = (float) k_val ();
            AuthReg[i].ExcRegion[tmpint].y[j] = (float) k_val ();
          }

          AuthReg[i].numExcReg = AuthReg[i].numExcReg + 1;

        }
        else
        {
          /* add this instid to the list */
          if ((numInst + 1) > MAX_INST)
          {
            logit("e", "%s: too many installations; exiting!\n", MyProgName);
            return EW_FAILURE;
          }
      
          AuthReg[numInst].instid = instid;
      
          /* Read in the arrays */
          num_sides = k_int ();
          if ((num_sides <= 2) || (num_sides > MAX_SIDES))
          {
            logit("e", "eqfilter: Invalid NumSides (%d) in <ExclRegion> cmd.\n", num_sides);
            return EW_FAILURE;
          }
        
          AuthReg[numInst].ExcRegion[0].num_sides = num_sides;
        
          for (j = 0; (j < num_sides + 1); j++)
          {
            AuthReg[numInst].ExcRegion[0].x[j] = (float) k_val ();
            AuthReg[numInst].ExcRegion[0].y[j] = (float) k_val ();
          }
          AuthReg[numInst].numExcReg = 1;
          numInst = numInst + 1;
        }
      }

      /* left Lucky's authreg stuff alone above, but for added tests,
       * we don't really care if we have duplicate entries or not for
       * a given instid.  We just do all tests for each instid.
       **************************************************************/
/*NR*/else if (k_its ("DepthTest")) 
      {
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <DepthTest> cmd;"                      " exiting!\n", str);
            return EW_FAILURE;
          }
        } 
        if( (NDepthTest+1) > MAX_INST)
        {
          logit("e", "eqfilter: too many installations for DepthTest; exiting!\n");
          return EW_FAILURE;
        }
        DepthTest[NDepthTest].var1 = (float) k_val();
        DepthTest[NDepthTest].var2 = (float) k_val();
        DepthTest[NDepthTest].instid = instid;
        NDepthTest++;
      }
/*NR*/else if (k_its ("nphTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <nphTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NnphTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for nphTest; exiting!\n");
          return EW_FAILURE;
        }
        nphTest[NnphTest].var = (float) k_val();
        nphTest[NnphTest].instid = instid;
        NnphTest++;
      }  
/*NR*/else if (k_its ("nphtotalTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <nphtotalTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NnphtotTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for nphtotalTest; exiting!\n");
          return EW_FAILURE;
        }
        nphtotTest[NnphtotTest].var = (float) k_val();
        nphtotTest[NnphtotTest].instid = instid;
        NnphtotTest++;
      }  
/*NR*/else if (k_its ("GapTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <GapTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        i=NGapTest;
        if((NGapTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for GapTest; exiting!\n");
          return EW_FAILURE;  
        }
        GapTest[NGapTest].var = (float) k_val();
        GapTest[NGapTest].instid = instid;
        NGapTest++;
      }
/*NR*/else if (k_its ("DminTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <DminTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        i=NDminTest;
        if((NDminTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for DminTest; exiting!\n");
          return EW_FAILURE;  
        }
        DminTest[NDminTest].var = (float) k_val();
        DminTest[NDminTest].instid = instid;
        NDminTest++;
      }
/*NR*/else if (k_its ("RMSTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <RMSTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        i=NRMSTest;
        if((NRMSTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for RMSTest; exiting!\n");
          return EW_FAILURE;  
        }
        RMSTest[NRMSTest].var = (float) k_val();
        RMSTest[NRMSTest].instid = instid;
        NRMSTest++;
      }
/*NR*/else if (k_its ("NcodaTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <NcodaTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NNcodaTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for NcodaTest; exiting!\n");
          return EW_FAILURE;
        }
        NcodaTest[NNcodaTest].var1 = (float) k_val();
        NcodaTest[NNcodaTest].var2 = (float) k_val();
        NcodaTest[NNcodaTest].instid = instid;
        NNcodaTest++;  
      }
/*NR*/else if (k_its ("MaxE0Test"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <MaxE0Test> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NMaxE0Test+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for MaxE0Test; exiting!\n");
          return EW_FAILURE;  
        }
        MaxE0Test[NMaxE0Test].var = (float) k_val();
        MaxE0Test[NMaxE0Test].instid = instid;
        NMaxE0Test++;
      }
/*NR*/else if (k_its ("MaxERHTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <MaxERHTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NMaxERHTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for MaxERHTest; exiting!\n");
          return EW_FAILURE;  
        }
        MaxERHTest[NMaxERHTest].var = (float) k_val();
        MaxERHTest[NMaxERHTest].instid = instid;
        NMaxERHTest++;
      }
/*NR*/else if (k_its ("MaxERZTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <MaxERZTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NMaxERZTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for MaxERZTest; exiting!\n");
          return EW_FAILURE;  
        }
        MaxERZTest[NMaxERZTest].var = (float) k_val();
        MaxERZTest[NMaxERZTest].instid = instid;
        NMaxERZTest++;
      }
/*NR*/else if (k_its ("MinMagTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <MinMagTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NMinMagTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for MinMagTest; exiting!\n");
          return EW_FAILURE;  
        }
        MinMagTest[NMinMagTest].var = (float) k_val();
        MinMagTest[NMinMagTest].instid = instid;
        NMinMagTest++;
      }
/*NR*/else if (k_its ("MaxDistTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <MaxDistTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NMaxDistTest+1)>NAllocMaxDistTest)
        {
          NAllocMaxDistTest += 10;
          MaxDistTest = realloc (MaxDistTest, sizeof(*MaxDistTest)*NAllocMaxDistTest);
          if ( MaxDistTest == NULL)
          {
            logit ("e", "eqfilter: error expanding MaxDistTest table; exiting!\n");
            return EW_FAILURE;
          }
        }
        MaxDistTest[NMaxDistTest].var1 = (float) k_val();
        MaxDistTest[NMaxDistTest].var2 = (float) k_val();
        MaxDistTest[NMaxDistTest].var3 = (float) k_val();
        MaxDistTest[NMaxDistTest].instid = instid;
        NMaxDistTest++;
      }

/*NR*/else if (k_its ("MaxOriginAgeSecs"))
      {  
        MaxOriginAgeSecs = k_int();
      }
      /* Unknown command
       *****************/ 
      else 
      {
        logit("e", "eqfilter: <%s> Unknown command in <%s>.\n", com, configfile);
        continue;
      }

      /* See if there were any errors processing the command 
       *****************************************************/
      if (k_err ()) 
      {
        logit("e", "eqfilter: Bad <%s> command in <%s>; exiting!\n", com, configfile);
        return EW_FAILURE;
      }

    } /** while k_rd() **/

    nfiles = k_close ();

  } /** while nfiles **/

  /* After all files are closed, check init flags for missed commands
   ******************************************************************/
  nmiss = 0;
  for (i = 0; i < NUM_COMMANDS; i++)  
  {
    if (!init[i]) 
    {
      nmiss++;
    }
  }

  if (nmiss) 
  {
    logit("e", "eqfilter: ERROR, no ");
    if (!init[0])  logit("e", "<MyModuleId> "    );
    if (!init[1])  logit("e", "<InRing> "        );
    if (!init[2])  logit("e", "<OutRing> "       );
    if (!init[3])  logit("e", "<HeartBeatInt> "  );
    if (!init[4])  logit("e", "<LogFile> "       );
    if (!init[5])  logit("e", "<GetEventsFrom> " );

    logit("e", "command(s) in <%s>; exiting!\n", configfile);
    return EW_FAILURE;
  }

  return EW_SUCCESS;
}

/******************************************************************************
 *  eqfilter_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int eqfilter_lookup( void )
{

  /* Look up keys to shared memory regions
  *************************************/
  if ((InKey = GetKey (InRingName)) == -1) 
  {
    logit( "e", "eqfilter:  Invalid ring name <%s>; exiting!\n", InRingName);
    return EW_FAILURE;
  }

  if ((OutKey = GetKey (OutRingName) ) == -1) 
  {
    logit( "e", "eqfilter:  Invalid ring name <%s>; exiting!\n", OutRingName);
    return EW_FAILURE;
  }

  /* Look up installations of interest
  *********************************/
  if (GetLocalInst (&InstId) != 0) 
  {
    logit( "e", "eqfilter: error getting local installation id; exiting!\n");
    return EW_FAILURE;
  }

  if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
  { 
    logit( "e", "eqfilter: error getting wildcard installation id; exiting!\n");
    return EW_FAILURE;
  }

  /* Look up modules of interest
  ******************************/
  if (GetModId (MyModName, &MyModId) != 0) 
  {
    logit( "e", "eqfilter: Invalid module name <%s>; exiting!\n", MyModName);
    return EW_FAILURE;
  }

  if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
  {
    logit( "e", "eqfilter: Invalid module name <MOD_WILDCARD>; exiting!\n");
    return EW_FAILURE;
  }

  /* Look up message types of interest
  *********************************/
  if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
  {
    logit( "e", "eqfilter: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &TypeError) != 0) 
  {
    logit( "e", "eqfilter: Invalid message type <TYPE_ERROR>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_HYP2000ARC", &TypeArc) != 0) 
  {
    logit( "e", "eqfilter: Invalid message type <TYPE_HYP2000ARC>; exiting!\n");
    return EW_FAILURE;
  }

  return EW_SUCCESS;

} 

/******************************************************************************
 * eqfilter_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void eqfilter_status( unsigned char type, short ierr, char *note )
{
  MSG_LOGO    logo;
  char        msg[256];
  long        size;
  time_t        t;
 
  /* Build the message 
  *******************/ 
  logo.instid = InstId;
  logo.mod    = MyModId;
  logo.type   = type;

  time (&t);

  if (type == TypeHeartBeat)
  {
    sprintf (msg, "%ld %d\n", (long) t, MyPid);
  }
  else if (type == TypeError)
  {
    sprintf (msg, "%ld %hd %s\n", (long) t, ierr, note);
    logit ("et", "%s(%s): %s\n", MyProgName, MyModName, note);
  }

  size = strlen (msg);   /* don't include the null byte in the message */     

  /* Write the message to shared memory
  ************************************/
  if (tport_putmsg (&OutRegion, &logo, size, msg) != PUT_OK)
  {
    if (type == TypeHeartBeat) 
    {
      logit ("et","%s(%s):  Error sending heartbeat.\n", MyProgName, MyModName);
    }
    else if (type == TypeError) 
    {
      logit ("et","%s(%s):  Error sending error:%d.\n", MyProgName, MyModName, ierr);
    }
  }

}


/********************** Message Stacking Thread *******************
 *           Move messages from transport to memory queue         *
 ******************************************************************/
thr_ret	MessageStacker (void *dummy)
{
  char         	*msg;           /* "raw" retrieved message */
  int          	res;
  long         	recsize;        /* size of retrieved message */
  MSG_LOGO     	reclogo;        /* logo of retrieved message */
  int          	ret;

  /* Allocate space for input/output messages
   *******************************************/
  if ((msg = (char *) malloc (MAX_BYTES_PER_EQ)) == (char *) NULL)
  {
    logit ("e", "eqfilter: error allocating msg; exiting!\n");
    goto error;
  }


  /* Tell the main thread we're ok
   ********************************/
  MessageStackerStatus = 0;

  /* Start service loop, picking up messages
   *****************************************/
  while (1)
  {
    /* Get a message from transport ring, carefully checking error codes
     *******************************************************************/
    res = tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
                        &recsize, msg, MAX_BYTES_PER_EQ-1);

    if (res == GET_NONE)  /* no messages for us now */
    {
      sleep_ew(100); 
      continue;
    } 

    if (res != GET_OK)    /* some kind of error code, speak */
    {
      if (res == GET_TOOBIG) /* msg too big for buffer */
      {
        sprintf (errText, "msg[%ld] i%d m%d t%d too long for target", recsize,
                           (int) reclogo.instid, (int) reclogo.mod, (int)reclogo.type);
        eqfilter_status (TypeError, ERR_TOOBIG, errText);
        continue;
      }
      else if (res == GET_MISS)  /* got msg, but may have missed some */
      {
        sprintf (errText, "missed msg(s) i%d m%d t%d in %s", (int) reclogo.instid,
                           (int) reclogo.mod, (int)reclogo.type, InRingName);
        eqfilter_status (TypeError, ERR_MISSMSG, errText);
      }
      else if (res == GET_NOTRACK) /* got msg, can't tell if any were missed */
      {
        sprintf (errText, "no tracking for logo i%d m%d t%d in %s", (int) reclogo.instid, 
                           (int) reclogo.mod, (int)reclogo.type, InRingName);
        eqfilter_status (TypeError, ERR_NOTRACK, errText);
      }
    }

 /* Queue retrieved msg (res==GET_OK,GET_MISS,GET_NOTRACK)
  ********************************************************/
    RequestMutex ();
    ret = enqueue (&MsgQueue, msg, recsize, reclogo); 
    ReleaseMutex_ew ();

    if (ret != 0)
    {
      if (ret == -2)  /* Serious: quit */
      {
        sprintf (errText, "internal queue error. Terminating.");
        eqfilter_status (TypeError, ERR_QUEUE, errText);
        goto error;
      }
      if (ret == -1)
      {
        sprintf (errText,"queue cannot allocate memory. Lost message.");
        eqfilter_status (TypeError, ERR_QUEUE, errText);
        continue;
      }
      if (ret == -3) 
      {
        sprintf (errText, "Queue full. Message lost.");
        eqfilter_status (TypeError, ERR_QUEUE, errText);
        continue;
      }
    } /* problem from enqueue */

  } /* while (1) */

  /* we're quitting
   *****************/
  error:
    MessageStackerStatus = -1; /* file a complaint to the main thread */
    KillSelfThread (); /* main thread will restart us */
    return(NULL);	/* should not reach here */
 
}



/********************** Message Processing Thread ****************
 *
 * This is where all the action is: grab a message from the 
 * queue, determine the originating installation ID. Find
 * the list of IncRegion and ExcRegion polygons that define
 * the authoritative region for the installation. Extract
 * the location of the event from the message, and determine
 * if the event is within the authoritative region. If it is,
 * write the hypoinverse arc message to OutRing.
 *
 ******************************************************************/
thr_ret	Processor (void *dummy)
{

  int ret, i, j, k;
  int gotMsg, gotit, gotit2;
  long msgSize;
  MSG_LOGO reclogo;           /* logo of retrieved message */
  float	lat, lon;
  struct Hsum Sum;            /* Hyp2000 summary data */
  struct Hpck Pick;          /* Hyp2000 pick structure */
  static char arcMsg[MAX_BYTES_PER_EQ];
  char     *in;             /* working pointer to archive message    */
  char      line[MAX_STR];  /* to store lines from msg               */ 
  char      shdw[MAX_STR];  /* to store shadow cards from msg        */ 
  int       msglen;         /* length of input archive message       */ 
  int       nline;          /* number of lines (not shadows) so far  */
  int  PASS;                /* number of tests that passed */
  float ncoda;
  int   num_polarity_up;    /* number FM up */
  int   num_polarity_down;  /* number FM down */
  int   total_polarity;     /* total FM measurements */
  double down_percent;      /* percent of FM down */
  double up_percent;	    /* percent of FM up */
  unsigned char INST_WILDCARD;

  GetInst("INST_WILDCARD",&INST_WILDCARD);
  while (1)
  {
    gotMsg = FALSE;
    while (gotMsg == FALSE)
      {
        /* Grab the next message 
        ************************/
        RequestMutex ();
        ret = dequeue (&MsgQueue, arcMsg, &msgSize, &reclogo);
        ReleaseMutex_ew ();

        if (ret < 0)  /* empty queue */
        {
          sleep_ew (500);
        }
        else  /* for a message from queue */
        {
          gotMsg = TRUE;
          PASS=TRUE;

          /*******************************************************
           * Read one data line and its shadow at a time from arcmsg; process them
           ***********************************************************************/
          /* Initialize some stuff
          ***********************/
          nline  = 0;
          msglen = strlen( arcMsg );
          in     = arcMsg;
          while( in < arcMsg+msglen )
          {
             if ( sscanf( in, "%[^\n]", line ) != 1 )  
             {
                logit( "et", "eqfilter: Error reading data from arcmsg\n" );
                ProcessorStatus = 1;
                KillSelfThread();
             }
             in += strlen( line ) + 1;
             if ( sscanf( in, "%[^\n]", shdw ) != 1 )  
             {
                logit( "et", "eqfilter: Error reading data shadow from arcmsg\n" );
                ProcessorStatus = 1;
                KillSelfThread();
             }
             in += strlen( shdw ) + 1;
             nline++;
             /* Process the hypocenter card (1st line of msg) & its shadow
             ************************************************************/
             if( nline == 1 ) {                /* hypocenter is 1st line in msg  */
                ncoda=0.0;

                num_polarity_up=0;
                num_polarity_down=0;
 
                if ( read_hyp( line, shdw, &Sum ) != 0)
                {
                  logit ("e", "eqfilter: Call to read_hyp failed.\n");
                  ProcessorStatus = 1;
                  KillSelfThread ();
                }
                if (Debug == 1)
                { 
                  logit( "e", "eqfilter got following EVENT MESSAGE:\n");
                  logit( "e", "%s\n", line );
                } 
                continue;
             } 
             if( strlen(line) < (size_t) 75 )  /* found the terminator line      */
             break;

             if ( read_phs( line, shdw, &Pick ) !=0)
             {
               logit ("e", "eqfilter: Call to read_phs failed.\n");
               ProcessorStatus = 1;
               KillSelfThread();
             }
             else if(Pick.codalen>0) 
             {
               ncoda = ncoda + 1.0;
             }
             if (Pick.Pfm == 'U') num_polarity_up++;
             if (Pick.Pfm == 'D') num_polarity_down++;
             if (Debug == 1)
             { 
               logit( "e", "%s\n", line );
             } 
             continue;
          } /*end while over reading message*/

          /********************************
            Begin Authoritative region test
           ********************************/

          i = -1;
          gotit = FALSE;
          while (gotit == FALSE)
          {
            if ((i = i + 1) >= numInst)
            {
              gotit = TRUE;
              i = -1;
            }
            else if (AuthReg[i].instid == reclogo.instid)
            {
              gotit = TRUE;
            }
 
          } /* while */
 
          if (i < 0)
          {

            /* Don't know about this installation 
            **************************************/

            if (AllowUndefInst == 1)
            {
              /* write message out anyway because AllowUndefInst is set 
              *********************************************************/
              if (Debug == 1)
                logit ("", "eqfilter: Don't have any regions for inst %d; "
                       "Allowing passing anyway because AllowUndefInst IS set\n",
                       reclogo.instid);

            }
            else
            {
              if (Debug == 1)
                logit ("", "eqfilter: Don't have any regions for inst %d; "
                       "IGNORING because AllowUndefInst is NOT set\n",
                       reclogo.instid);
             
              PASS=FALSE;
              goto FinishedTesting;
            }
          }
          else
          {
            /* Extract lat and lon of the event from the arc message 
             *******************************************************/
            lat = Sum.lat;
            lon = Sum.lon;

            if (Debug == 1)
              logit ("", "Inst %d, coords %0.2f, %0.2f ==> ", reclogo.instid, lat, lon);

            gotit = FALSE;
            for (j = 0; ((j < AuthReg[i].numIncReg) && (gotit == FALSE)); j++)
            {
              /* See if the lat,lon point is inside of this polygon
               ****************************************************/

              area (&ret, AuthReg[i].IncRegion[j].num_sides, AuthReg[i].IncRegion[j].x, 
                  AuthReg[i].IncRegion[j].y, &lat, &lon);

              if (ret == 1)
              {
                if (Debug == 1)
                  logit ("", "in InclRegion %d ==> ", j);

                /* Point was inside one of the IncRegions, now
                 * see if the point falls inside one of the
                 * regions which are to be excluded
                 **********************************************/

                gotit2 = FALSE;
                for (k = 0; ((k <  AuthReg[i].numExcReg) && (gotit2 == FALSE)); k++)
                {
                  area (&ret, AuthReg[i].ExcRegion[j].num_sides, AuthReg[i].ExcRegion[j].x, 
                        AuthReg[i].ExcRegion[j].y, &lat, &lon);

                  if (ret == 1)
                  {
                    /* This point should be excluded, based on ExclRegion */
                    if (Debug == 1)
                      logit ("", "in ExclRegion %d ==> IGNORING.\n", k);

                    PASS=FALSE;
                    gotit2 = TRUE;
                    goto FinishedTesting;
                  }

                }

                if (gotit2 == FALSE)
                {	
                  if (Debug == 1)
                    logit ("", "not in any ExclRegion ==> Okay to pass so far.\n");

                  gotit = TRUE;

                } /* if not found in ExclRegions */

              } /* if found in InclRegions */

              if (gotit == FALSE)
              {
                logit ("", "not in any InclRegion ==> IGNORING.\n");
                PASS=FALSE;
                goto FinishedTesting;
              }
  
            } /* if num inst > 0 */
          }  /* i<numInst */

          /* begin teleseism test */
          if (polarityPercent > 0.0) 
          {
              total_polarity = num_polarity_up + num_polarity_down;
              up_percent   = (double) num_polarity_up   / (double) total_polarity * 100.;
              down_percent = (double) num_polarity_down / (double) total_polarity * 100.;

              if ( total_polarity < minPolarityValues ) 
              {
                logit ("t", "polarityTest not possible, too few polarities at %d\n", total_polarity);
              }
              else if ( Sum.gap > maxPolarityGap )
              {
                logit ("t", "polarityTest not possible, max gap exceeded  at %d\n", Sum.gap);
              }
              else if ( up_percent >= polarityPercent )
              {
                logit ("t", "polarityTest event fails at %4.1f%%  with %d up   %d down\n", 
			up_percent, num_polarity_up, num_polarity_down);
                PASS=FALSE;
                goto FinishedTesting;
              }
              else if ( down_percent >= polarityPercent )
              {
                logit ("t", "polarityTest event fails at %4.1f%%  with %d up   %d down\n", 
			down_percent, num_polarity_up, num_polarity_down);
                PASS=FALSE;
                goto FinishedTesting;
              }
              else 
              {
                logit ("t", "polarityTest passes with %4.1f%% %d up  %4.1f%% %d down and gap=%d\n", 
			up_percent, num_polarity_up, down_percent, num_polarity_down, Sum.gap);
              }
          }
          /*******************************
            Begin Depth Test
           *******************************/
          if(NDepthTest>0)
          {
            gotit=FALSE;
            IsLimit1LTArgLTLimit2(&gotit, NDepthTest, DepthTest, 
                                  reclogo.instid, Sum.z , &PASS);

            /* if not found in the list, are wildcards enabled 
                  note that we really do need a second loop to
                  avoid clobbering defined instid's with wildcards.
             **************************************************/
            if(gotit==FALSE)
              IsLimit1LTArgLTLimit2(&gotit, NDepthTest, DepthTest, 
                                    INST_WILDCARD, Sum.z , &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed DepthTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin nphTest
           ***************/
          if(NnphTest>0)
          {
            gotit=FALSE;
            IsLimitLTArg(&gotit, NnphTest, nphTest, reclogo.instid, (float)Sum.nph, &PASS);

            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NnphTest, nphTest, INST_WILDCARD, (float)Sum.nph, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed nphTest\n");
              goto FinishedTesting;
            }
          }

          /*******************
            Begin nphtotalTest
           *******************/
          if(NnphtotTest>0)
          {
            gotit=FALSE;
            IsLimitLTArg(&gotit, NnphtotTest, nphtotTest, reclogo.instid, 
                         (float)Sum.nphtot, &PASS);

            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NnphtotTest, nphtotTest, INST_WILDCARD, 
                           (float)Sum.nphtot, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter:  event failed nphtotalTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin GapTest
           ***************/
          if(NGapTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NGapTest, GapTest, reclogo.instid, (float)Sum.gap, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NGapTest, GapTest, INST_WILDCARD, (float)Sum.gap, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed GapTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin DminTest
           ***************/
          if(NDminTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NDminTest, DminTest, reclogo.instid, (float)Sum.dmin, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NDminTest, DminTest, INST_WILDCARD, (float)Sum.dmin, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed DminTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin RMSTest
           ***************/
          if(NRMSTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NRMSTest, RMSTest, reclogo.instid, (float)Sum.rms, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NRMSTest, RMSTest, INST_WILDCARD, (float)Sum.rms, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed RMSTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin MaxE0Test
           ***************/
          if(NMaxE0Test>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NMaxE0Test, MaxE0Test, reclogo.instid, (float)Sum.e0, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxE0Test, MaxE0Test, INST_WILDCARD, (float)Sum.e0, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MaxE0Test\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin MaxERHTest
           ***************/
          if(NMaxERHTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NMaxERHTest, MaxERHTest, reclogo.instid, (float)Sum.erh, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxERHTest, MaxERHTest, INST_WILDCARD, (float)Sum.erh, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MaxERHTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin MaxERZTest
           ***************/
          if(NMaxERZTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NMaxERZTest, MaxERZTest, reclogo.instid, (float)Sum.erz, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxERZTest, MaxERZTest, INST_WILDCARD, (float)Sum.erz, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MaxERZTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin MinMagTest
           ***************/
          if(NMinMagTest>0)
          {
            gotit=FALSE;
            IsLimitLTArg(&gotit, NMinMagTest, MinMagTest, reclogo.instid, (float)Sum.Md, &PASS);
     
            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NMinMagTest, MinMagTest, INST_WILDCARD, (float)Sum.Md, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MinMagTest\n");
              goto FinishedTesting;
            }
          }

          /***********************************************************************
            Begin NcodaTest
               this ones a bit different so we don't get to use the Isxxx routines
           ***********************************************************************/
          if(NNcodaTest>0)
          {
/* note: this will perform the test successively on all configure Ncoda inst_id's
   so event must pass all or it will fail.  E.g. only the last configured inst_id
   really matters (unless it fails one prior to that).
*/
            gotit=FALSE;
            gotit2=FALSE;  /* this gotit is only used locally for this test */
            for(i=0; i<NNcodaTest; i++)
            {
              if(NcodaTest[i].instid==reclogo.instid)
                gotit2=gotit=TRUE; /* set the global gotit since we have at least one test */
              else
                gotit2=FALSE;

              if(gotit2==TRUE && ncoda < NcodaTest[i].var1 && Sum.Md > NcodaTest[i].var2)
              {
                PASS=FALSE;
                if(Debug==1)
                {  
                  logit("et","eqfilter: event failed NcodaTest\n");
                  logit("et","          ncoda = %f is less than %f\n",ncoda,NcodaTest[i].var1);
                  logit("et","          AND Md = %f is greater than %f\n",Sum.Md,NcodaTest[i].var2);
                }  
              }
              else if (gotit2 == TRUE && PASS==TRUE)
              {   
                if(Debug==1)
                  logit("et","eqfilter got %f codalen's with Md=%f\n",ncoda,Sum.Md);
              }  
            }  /* for i<NNcodatest */

            /* do the wild thing */
            /* use the global gotit here so that this is only performed
               if no ncoda id's were found */
            if(gotit==FALSE)
            {
              for(i=0; i<NNcodaTest; i++)
              {
                if(NcodaTest[i].instid==INST_WILDCARD)
                  gotit2=gotit=TRUE;
                else
                  gotit2=FALSE;
 
                if(gotit2==TRUE && ncoda < NcodaTest[i].var1 && Sum.Md > NcodaTest[i].var2)
                {  
                  PASS=FALSE;
                  if(Debug==1)
                  {  
                    logit("et","eqfilter: event failed NcodaTest\n");
                    logit("et","          ncoda = %f is less than %f\n",
                           ncoda,NcodaTest[i].var1);                                      
                    logit("et","          AND Md = %f is greater than %f\n",
                           Sum.Md,NcodaTest[i].var2);             
                  }  
                }   
                else if (gotit2==TRUE && PASS==TRUE)
                {   
                  if(Debug==1)
                    logit("et","eqfilter got %f codalen's with Md=%f\n",ncoda,Sum.Md);
                } 
              }  /* for i<NNcodatest */
            } /* if gotit2 == FALSE */

            /* did we get at least one match? */
            if(gotit==FALSE)
            {
              PASS=FALSE;
              if(Debug==1)
              {
                logit("et","eqfilter: event failed NcodaTest\n");
                logit("et","          instid=%s not in configured list\n",reclogo.instid);
              }
            }

          } /* if NNcodatTest > 0 */

          /***********************************************************************
            Begin NMaxDistTest
               this is also a bit different so we don't get to use the Isxxx routines
           ***********************************************************************/
          if(NMaxDistTest>0)
          {
            gotit=FALSE;
            for(i=0; i<NMaxDistTest && MaxDistTest[i].instid!=InstWild && MaxDistTest[i].instid!=reclogo.instid; i++);

            /* did we get at least one match? */
            if ( i == NMaxDistTest ) {
                PASS = FALSE;
                if  ( Debug == 1 ) {
                    logit( "et", "eqfilter: event failed MaxDistTest\n" );
                    logit( "et", "          instid=%d not in configured list\n", reclogo.instid);
                }
            } else {
                for(i=0; i<NMaxDistTest; i++) {
                    float dist;
                    if(MaxDistTest[i].instid!=InstWild && MaxDistTest[i].instid!=reclogo.instid)
                        continue;
                    dist = distance( MaxDistTest[i].var2, MaxDistTest[i].var3, Sum.lat, Sum.lon, 'K' );
                    if ( dist <= MaxDistTest[i].var1 ) {
                        gotit = TRUE;
                        break;
                    } else {
                        printf("Too far: %f vs. %f\n", dist, MaxDistTest[i].var1  );
                    }
                }
            
                if(gotit==FALSE)
                {
                  PASS=FALSE;
                  if(Debug==1)
                  {
                    logit("et","eqfilter: event failed MaxDistTest\n");
                    logit("et","          no distance within its limit\n");
                  }
                }
            }

          } /* if NMaxDistTest > 0 */

          /***********************************************************************
            Begin MaxOriginAgeSecs
               Simple test: is origin less than MaxOriginAgeSecs old?
           ***********************************************************************/
          if(MaxOriginAgeSecs>0)
          {
            time_t now, then;
            time(&now);
            then = Sum.ot-GSEC1970;
            if ( now - then > MaxOriginAgeSecs ) {
                PASS = FALSE;
                if(Debug==1)
                {
                    logit("et","eqfilter: event failed MaxOriginAgeSecs (%d vs %d)\n",  now-then, MaxOriginAgeSecs);
                }
            }
          } /* if MaxOriginAgeSecs > 0 */

       /**********************************************
        No more tests: pass the message if appropriate
       ***********************************************/
       FinishedTesting:

          if(PASS==TRUE)
          {
            if (tport_putmsg (&OutRegion, &reclogo, strlen (arcMsg), arcMsg) != PUT_OK)
            {
              logit ("et", "eqfilter: Error writing message %d-%d-%d.\n", 
                            reclogo.instid, reclogo.mod, reclogo.type);
              ProcessorStatus = 1;
              KillSelfThread ();
            }
            else
            {
              logit ("et","eqfilter passed message from inst:%d module:%d type:%d.\n",
                            reclogo.instid, reclogo.mod, reclogo.type);
            }
          }
          else
          {
            if (Debug==1)
            {
              logit ("et", "eqfilter: Rejected message from inst:%d module:%d type:%d.\n",
                            reclogo.instid, reclogo.mod, reclogo.type);
            }
          }
       } /* if ret > 0 */

     } /* while no messages are dequeued */

     ProcessorStatus = 0;
   }
   return(NULL);

}

/***************************************************************************************
 *  IsLimit1LTArgLTLimit2( int *gotit, int N, PARTEST2 *MyPar, unsigned char instid, 
 *                         float myarg, int *PASS)
 *
 *      given a set of tests containing N members, check for MyPar.instid == instid
 *        if it is, then check for Mypar.var1 < myarg < Mypar.var2
 *           if it is not, then set PASS=FALSE;
 *
 ***************************************************************************************/
void IsLimit1LTArgLTLimit2(int *gotit, int N, PARTEST2 *MyPar, 
                           unsigned char instid, float myarg, int *PASS)
{
  int i;
  int gotit2;

  if(N>=0)
  {  
    for(i=0; i<N; i++)
    {
      gotit2=FALSE;
      if(MyPar[i].instid == instid)
        *gotit=gotit2=TRUE;
 
      if (gotit2==TRUE)
      {  
        logit("et","gotit2==TRUE, instid=%d MyPar[i].instid=%d\n",instid,MyPar[i].instid);
        if (myarg > MyPar[i].var1 && myarg < MyPar[i].var2)
        {
          ;
        }
        else
        {
          *PASS=FALSE;
          if(Debug==1)
          {
            logit("et","eqfilter/IsLimit1LTArgLTLimit2: event failed\n");
            logit("et","          event value=%.2f is not between limits %.2f and %.2f\n",
                   myarg, MyPar[i].var1, MyPar[i].var2);
          }
        }  
      } /* if gotit==TRUE */
    }   /*  for i<N */
  } /* if N>=0 */
} /* end subroutine */

/***************************************************************************************
 *  IsLimitLTArg( int N, PARTEST1 *MyPar, unsigned char instid, float myarg, int *PASS)
 *
 *      given a set of tests containing N members, check for MyPar.instid == instid
 *        if it is, then check for Mypar.var1 < myarg 
 *           if it is not, then set PASS=FALSE;
 *
 ***************************************************************************************/
void IsLimitLTArg(int *gotit, int N, PARTEST1 *MyPar, unsigned char instid, 
                  float myarg, int *PASS)
{
  int i;
  int gotit2;

  if(N>=0)
  {
    for(i=0; i<N; i++)
    {
      gotit2=FALSE;
      if(MyPar[i].instid == instid)
        *gotit=gotit2=TRUE;

      if (gotit2==TRUE)
      {  
        if ( MyPar[i].var < myarg )
        {
          ;
        }
        else
        {
          *PASS=FALSE;
          if(Debug==1)
          {
            logit("et","eqfilter/IsLimitLTArg: event failed\n");
            logit("et","         event value=%.2f is less than or equal to limit=%.2f\n",
                   myarg, MyPar[i].var);
          }
        }
      } /* if gotit==TRUE */
    }   /*  for i<N */
  } /* if N>=0 */
} /* end subroutine */

/***************************************************************************************
 *  IsLimitGTArg( int N, PARTEST1 *MyPar, unsigned char instid, float myarg, int *PASS)
 *
 *      given a set of tests containing N members, check for MyPar.instid == instid
 *        if it is, then check for Mypar.var1 > myarg
 *           if it is not, then set PASS=FALSE;
 *
 ***************************************************************************************/
void IsLimitGTArg(int *gotit, int N, PARTEST1 *MyPar, unsigned char instid, 
                  float myarg, int *PASS)
{
  int i;
  int gotit2;
 
  if(N>=0)
  {
    for(i=0; i<N; i++)
    {
      gotit2=FALSE;
      if(MyPar[i].instid == instid)
        *gotit=gotit2=TRUE;
 
      if (gotit2==TRUE)
      {  
        if ( MyPar[i].var > myarg )
        {
          ;
        }
        else
        {
          *PASS=FALSE;
          if(Debug==1)
          {
            logit("et","eqfilter/IsLimitGTArg: event failed\n");
            logit("et","         event value=%.2f is greater than or equal to limit=%.2f\n",
                   myarg, MyPar[i].var);
          }
        }
      } /* if gotit==TRUE */
    }   /*  for i<N */
  } /* if N>=0 */
} /* end subroutine */

