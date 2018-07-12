/*********************************************************************
     eqfilterII                        Mitch Withers, August 2004.
  
     cloned from eqfilter.

     no longer hardwired to hypo.  Accept:
   TYPE_HYP2000ARC  (as output by eqproc)
   TYPE_LOC_GLOBAL  (as output by glproc)
   TYPE_RAYLOC      (as output by rayloc_ew)

     For now, we only process the location struct which includes
     only lat, lon, dep, gap, dmin, rms, pick_count

*********************************************************************/

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
#include <global_loc_rw.h>
#include <read_arc.h>
#include <rayloc_message_rw.h>

#include "eqfilterII.h"

static  SHM_INFO  InRegion;      /* shared memory region to use for i/o    */
static  SHM_INFO  OutRegion;     /* shared memory region to use for i/o    */

#define BUFLEN MAX_BYTES_PER_EQ

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
static unsigned char InstWild;
static unsigned char ModWild;

static unsigned char TypeLocGlobal, TypeHyp2000Arc, TypeRayloc;

/* Error messages used by eqfilterII 
 *********************************/
#define  ERR_MISSMSG       0   /* message missed in transport ring       */
#define  ERR_TOOBIG        1   /* retreived msg too large for buffer     */
#define  ERR_NOTRACK       2   /* msg retreived; tracking limit exceeded */
#define  ERR_QUEUE         4   /* trouble with the MsgQueue operation */

static char  errText[256];    /* string for log/error messages          */

pid_t MyPid;	/** Hold our process ID to be sent with heartbeats **/

/* Functions in this source file 
 *******************************/
static	int  	eqfilterII_config (char *);
static	int  	eqfilterII_lookup (void);
static	void  	eqfilterII_status (unsigned char, short, char *);
thr_ret			MessageStacker (void *);
thr_ret			Processor (void *);
int 			area (int *, int, float *, float *, float *, float *);
int 			cntsct (int, float *, float *, float *, float *);
int 			isect (float, float, float, float, float *, float *);
void IsLimit1LTArgLTLimit2(int *, int, PARTEST2 *, unsigned char, float, int *);
void IsLimitLTArg(int *, int, PARTEST1 *, unsigned char, float, int *);
void IsLimitGTArg(int *, int, PARTEST1 *, unsigned char, float, int *);
int eqfiltmsg(char *, EQSUM *, unsigned char);
int hyp2eqfilt(char *, EQSUM *);
int glloc2eqfilt(char *, EQSUM *);
int rayloc2eqfilt(char *, EQSUM *);

main (int argc, char **argv)
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
    fprintf (stderr, "Usage: eqfilterII <configfile>\n");
    return EW_FAILURE;
  }

  /* To be used in logging functions
   *********************************/
  if (get_prog_name (argv[0], MyProgName) != EW_SUCCESS)
  {
    fprintf (stderr, "eqfilterII: Call to get_prog_name failed.\n");
    return EW_FAILURE;
  }

  /* Initialize name of log-file & open it 
   ***************************************/
  logit_init (argv[1], 0, 256, 1);

  /* Read the configuration file(s)
   ********************************/
  if (eqfilterII_config(argv[1]) != EW_SUCCESS)
  {
    logit( "e", "eqfilterII: Call to eqfilterII_config failed \n");
    return EW_FAILURE;
  }
  logit ("" , "%s(%s): Read command file <%s>\n", 
         MyProgName, MyModName, argv[1]);

  /* Look up important info from earthworm.h tables
   ************************************************/
  if (eqfilterII_lookup() != EW_SUCCESS)
  {
    logit( "e", "%s(%s): Call to eqfilterII_lookup failed \n",
             MyProgName, MyModName);
    return EW_FAILURE;
  }
  /* run through the logos and make sure only valid message types
     were configured.
   **************************************************************/
  for(i=0; i<nLogo; i++){
    if(GetLogo[i].type != TypeHyp2000Arc && 
           GetLogo[i].type != TypeLocGlobal && 
           GetLogo[i].type != TypeRayloc){
      logit( "e", "%s(%s): Unsupported message type (%d)\n",
               MyProgName, MyModName, GetLogo[i].type);
      return EW_FAILURE;
    }
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
    logit("e","FATAL ERROR: eqfilterII InRing=%s == OutRing = %s. Exiting\n",
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
  if ((flushmsg = (char *) malloc (BUFLEN)) ==  NULL)
  {
    logit ("e", "eqfilterII: can't allocate flushmsg; exiting.\n");
    return EW_FAILURE;
  }

  while (tport_getmsg (&InRegion, GetLogo, nLogo, &reclogo,
         &recsize, flushmsg, (BUFLEN - 1)) != GET_NONE);

  /* Show off our regions, if Debug is requested
   **********************************************/
  if (Debug == 1)
  {
    logit ("", "eqfilterII: Program starting - NumInsts = %d:\n", numInst);
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
        logit("","nphtotalTest: %d %f\n",nphtotTest[i].instid,nphtotTest[i].var)
;
      }
    }

    if(NnstaTest>0)
    {
      for(i=0; i<NnstaTest; i++)
      {
        logit("","nstaTest: %d %f\n",nstaTest[i].instid,nstaTest[i].var);
      }
    }

    if(NGapTest>0)
    {
      for(i=0; i<NGapTest; i++)
      {
        logit("","GapTest: %d %f\n",GapTest[i].instid,GapTest[i].var);
      }
    }
 
    if(NGDminTest>0)
    {
      for(i=0; i<NGDminTest; i++)
      {
        logit("","GDminTest: %d %f\n",GDminTest[i].instid,GDminTest[i].var);
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

    if(NMaxAVHTest>0)
    {
      for(i=0; i<NMaxAVHTest; i++)
      {
        logit("","MaxAVHTest: %d %f\n",MaxAVHTest[i].instid,MaxAVHTest[i].var);
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
 
  }

  /* Create MsgQueue mutex */
  CreateMutex_ew();

  /* Allocate the message Queue
   ********************************/
  initqueue (&MsgQueue, QUEUE_SIZE, BUFLEN);

  /* Start message stacking thread which will read 
   * messages from the InRing and put them into the Queue 
   *******************************************************/
  if (StartThread (MessageStacker, (unsigned) THREAD_STACK, &tidStacker) == -1)
  {
    logit( "e", "eqfilterII: Error starting MessageStacker thread.  Exiting.\n");
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
    logit( "e", "eqfilterII: Error starting Processor thread.  Exiting.\n");
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

    /* send eqfilterII' heartbeat
    ***************************/
    if (time (&timeNow) - timeLastBeat  >=  HeartBeatInterval) 
    {
      timeLastBeat = timeNow;
      eqfilterII_status (TypeHeartBeat, 0, ""); 
    }

    /* Check on our threads */
    if (MessageStackerStatus < 0)
    {
      logit ("et", "eqfilterII: MessageStacker thread died. Exiting\n");
      return EW_FAILURE;
    }

    if (ProcessorStatus < 0)
    {
      logit ("et", "eqfilterII: Processor thread died. Exiting\n");
      return EW_FAILURE;
    }

    sleep_ew (1000);

  } /* wait until TERMINATE is raised  */  

  /* Termination has been requested
   ********************************/
  tport_detach (&InRegion);
  tport_detach (&OutRegion);
  logit ("t", "eqfilterII: Termination requested; exiting!\n" );
  return EW_SUCCESS;

}

/******************************************************************************
 *  eqfilterII_config() processes command file(s) using kom.c functions;        *
 *                    exits if any errors are encountered.                    *
 ******************************************************************************/
static int eqfilterII_config (char *configfile)
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
  NnstaTest=0;
  NGapTest=0;
  NGDminTest=0;
  NDminTest=0;
  NRMSTest=0;
  NMaxE0Test=0;
  NMaxERHTest=0;
  NMaxERZTest=0;
  NMaxAVHTest=0;
  NMinMagTest=0;
  NNcodaTest=0;


  /* Open the main configuration file 
   **********************************/
  nfiles = k_open (configfile); 
  if (nfiles == 0) 
  {
    logit("e", "eqfilterII: Error opening command file <%s>; exiting!\n", configfile);
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
          logit("e", "eqfilterII: Error opening command file <%s>; exiting!\n",
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

      /* Enter installation & module types to get
       *******************************************/
/*5*/ else if (k_its ("GetEventsFrom")) 
      {
        if (nLogo >= MAXLOGO) 
        {
          logit("e", "eqfilterII: Too many <GetMsgLogo> commands in <%s>; "
                   "; max=%d; exiting!\n", configfile, (int) MAXLOGO);
          return EW_FAILURE;
        }
        if ((str = k_str())) 
        {
          if (GetInst (str, &GetLogo[nLogo].instid) != 0) 
          {
            logit("e", "eqfilterII: Invalid installation name <%s> in "
                     "<GetEventsFrom> cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if ((str = k_str())) 
        {
          if (GetModId (str, &GetLogo[nLogo].mod) != 0) 
          {
            logit("e", "eqfilterII: Invalid module name <%s> in <GetEventsFrom> "
                     "cmd; exiting!\n", str);
            return EW_FAILURE;
          }
        }
        /* We'll always fetch arc messages */
        if ((str = k_str())) 
          {
            if (GetType (str, &GetLogo[nLogo].type) != 0) 
            {
              logit("e", "eqfilterII: Invalid msgtype <%s> in <GetEventsFrom> "
                       "cmd; exiting!\n", str);
              return EW_FAILURE;
            }
            nLogo++;
            init[5] = 1;
         }
      }
/*NR*/else if (k_its ("InclRegion")) 
      {
        if ((str = k_str())) 
        {
          if (GetInst (str, &instid) != 0) 
          {
            logit("e", "eqfilterII: Invalid installation name <%s> in <InclRegion> "
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
            logit("e", "eqfilterII: Invalid NumSides (%d) in <InclRegion> cmd.\n", num_sides);
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
            logit("e", "eqfilterII: Invalid NumSides (%d) in <InclRegion> cmd.\n", num_sides);
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
            logit("e", "eqfilterII: Invalid installation name <%s> in <ExclRegion> cmd;"
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
            logit("e", "eqfilterII: Invalid NumSides (%d) in <ExclRegion> cmd.\n", num_sides);
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
            logit("e", "eqfilterII: Invalid NumSides (%d) in <ExclRegion> cmd.\n", num_sides);
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
            logit("e", "eqfilterII: Invalid installation name <%s> in <DepthTest> cmd;"                      " exiting!\n", str);
            return EW_FAILURE;
          }
        } 
        if( (NDepthTest+1) > MAX_INST)
        {
          logit("e", "eqfilterII: too many installations for DepthTest; exiting!\n");
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
            logit("e", "eqfilterII: Invalid installation name <%s> in <nphTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NnphTest+1)>MAX_INST)
        {
          logit("e", "eqfilterII: too many installations for nphTest; exiting!\n");
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
/*NR*/else if (k_its ("nstaTest"))
      {
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilterII: Invalid installation name <%s> in <nstaTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NnstaTest+1)>MAX_INST)
        {
          logit("e", "eqfilterII: too many installations for nstaTest; exiting!\n");
          return EW_FAILURE;
        }
        nstaTest[NnstaTest].var = (float) k_val();
        nstaTest[NnstaTest].instid = instid;
        NnstaTest++;
      }
/*NR*/else if (k_its ("GapTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilterII: Invalid installation name <%s> in <GapTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        i=NGapTest;
        if((NGapTest+1)>MAX_INST)
        {
          logit("e", "eqfilterII: too many installations for GapTest; exiting!\n");
          return EW_FAILURE;  
        }
        GapTest[NGapTest].var = (float) k_val();
        GapTest[NGapTest].instid = instid;
        NGapTest++;
      }
/*NR*/else if (k_its ("GDminTest"))
      {  
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilterII: Invalid installation name <%s> in <GDminTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        i=NGDminTest;
        if((NGDminTest+1)>MAX_INST)
        {
          logit("e", "eqfilterII: too many installations for GDminTest; exiting!\n");
          return EW_FAILURE;  
        }
        GDminTest[NGDminTest].var = (float) k_val();
        GDminTest[NGDminTest].instid = instid;
        NGDminTest++;
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
            logit("e", "eqfilterII: Invalid installation name <%s> in <RMSTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        i=NRMSTest;
        if((NRMSTest+1)>MAX_INST)
        {
          logit("e", "eqfilterII: too many installations for RMSTest; exiting!\n");
          return EW_FAILURE;  
        }
        RMSTest[NRMSTest].var = (float) k_val();
        RMSTest[NRMSTest].instid = instid;
        NRMSTest++;
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
/*NR*/else if (k_its ("MaxAVHTest"))
      {
        if ((str = k_str()))
        {
          if (GetInst (str, &instid) != 0)
          {
            logit("e", "eqfilter: Invalid installation name <%s> in <MaxAVHTest> cmd;"
                    " exiting!\n", str);
            return EW_FAILURE;
          }
        }
        if((NMaxAVHTest+1)>MAX_INST)
        {
          logit("e", "eqfilter: too many installations for MaxAVHTest; exiting!\n");
          return EW_FAILURE;
        }
        MaxAVHTest[NMaxAVHTest].var = (float) k_val();
        MaxAVHTest[NMaxAVHTest].instid = instid;
        NMaxAVHTest++;
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
      /* Unknown command
       *****************/ 
      else 
      {
        logit("e", "eqfilterII: <%s> Unknown command in <%s>.\n", com, configfile);
        continue;
      }

      /* See if there were any errors processing the command 
       *****************************************************/
      if (k_err ()) 
      {
        logit("e", "eqfilterII: Bad <%s> command in <%s>; exiting!\n", com, configfile);
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
    logit("e", "eqfilterII: ERROR, no ");
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
 *  eqfilterII_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
static int eqfilterII_lookup( void )
{

  /* Look up keys to shared memory regions
  *************************************/
  if ((InKey = GetKey (InRingName)) == -1) 
  {
    logit( "e", "eqfilterII:  Invalid ring name <%s>; exiting!\n", InRingName);
    return EW_FAILURE;
  }

  if ((OutKey = GetKey (OutRingName) ) == -1) 
  {
    logit( "e", "eqfilterII:  Invalid ring name <%s>; exiting!\n", OutRingName);
    return EW_FAILURE;
  }

  /* Look up installations of interest
  *********************************/
  if (GetLocalInst (&InstId) != 0) 
  {
    logit( "e", "eqfilterII: error getting local installation id; exiting!\n");
    return EW_FAILURE;
  }

  if (GetInst ("INST_WILDCARD", &InstWild ) != 0) 
  { 
    logit( "e", "eqfilterII: error getting wildcard installation id; exiting!\n");
    return EW_FAILURE;
  }

  /* Look up modules of interest
  ******************************/
  if (GetModId (MyModName, &MyModId) != 0) 
  {
    logit( "e", "eqfilterII: Invalid module name <%s>; exiting!\n", MyModName);
    return EW_FAILURE;
  }

  if (GetModId ("MOD_WILDCARD", &ModWild) != 0) 
  {
    logit( "e", "eqfilterII: Invalid module name <MOD_WILDCARD>; exiting!\n");
    return EW_FAILURE;
  }

  /* Look up message types of interest
  *********************************/
  if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat) != 0) 
  {
    logit( "e", "eqfilterII: Invalid message type <TYPE_HEARTBEAT>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_ERROR", &TypeError) != 0) 
  {
    logit( "e", "eqfilterII: Invalid message type <TYPE_ERROR>; exiting!\n");
    return EW_FAILURE;
  }

  if (GetType ("TYPE_LOC_GLOBAL", &TypeLocGlobal) != 0) 
  {
    logit( "e", "eqfilterII: Invalid message type <TYPE_LOC_GLOBAL>; exiting!\n");
    return EW_FAILURE;
  }
  if (GetType ("TYPE_HYP2000ARC", &TypeHyp2000Arc) != 0) 
  {
    logit( "e", "eqfilterII: Invalid message type <TYPE_HYP2000ARC>; exiting!\n");
    return EW_FAILURE;
  }
  if (GetType ("TYPE_RAYLOC", &TypeRayloc) != 0) 
  {
    logit( "e", "eqfilterII: Invalid message type <TYPE_RAYLOC>; exiting!\n");
    return EW_FAILURE;
  }

  return EW_SUCCESS;

} 

/******************************************************************************
 * eqfilterII_status() builds a heartbeat or error message & puts it into       *
 *                   shared memory.  Writes errors to log file & screen.      *
 ******************************************************************************/
static void eqfilterII_status( unsigned char type, short ierr, char *note )
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
    sprintf (msg, "%ld %ld\n\0", (long) t, (long) MyPid);
  }
  else if (type == TypeError)
  {
    sprintf (msg, "%ld %hd %s\n\0", (long) t, ierr, note);
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
  if ((msg = (char *) malloc (BUFLEN)) == (char *) NULL)
  {
    logit ("e", "eqfilterII: error allocating msg; exiting!\n");
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
                        &recsize, msg, BUFLEN-1);

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
        eqfilterII_status (TypeError, ERR_TOOBIG, errText);
        continue;
      }
      else if (res == GET_MISS)  /* got msg, but may have missed some */
      {
        sprintf (errText, "missed msg(s) i%d m%d t%d in %s", (int) reclogo.instid,
                           (int) reclogo.mod, (int)reclogo.type, InRingName);
        eqfilterII_status (TypeError, ERR_MISSMSG, errText);
      }
      else if (res == GET_NOTRACK) /* got msg, can't tell if any were missed */
      {
        sprintf (errText, "no tracking for logo i%d m%d t%d in %s", (int) reclogo.instid, 
                           (int) reclogo.mod, (int)reclogo.type, InRingName);
        eqfilterII_status (TypeError, ERR_NOTRACK, errText);
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
        eqfilterII_status (TypeError, ERR_QUEUE, errText);
        goto error;
      }
      if (ret == -1)
      {
        sprintf (errText,"queue cannot allocate memory. Lost message.");
        eqfilterII_status (TypeError, ERR_QUEUE, errText);
        continue;
      }
      if (ret == -3) 
      {
        sprintf (errText, "Queue full. Message lost.");
        eqfilterII_status (TypeError, ERR_QUEUE, errText);
        continue;
      }
    } /* problem from enqueue */

  } /* while (1) */

  /* we're quitting
   *****************/
  error:
    MessageStackerStatus = -1; /* file a complaint to the main thread */
    KillSelfThread (); /* main thread will restart us */
 
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
  static char record[BUFLEN]; /* dare we assume global locs are biggest? */
  static char record2[BUFLEN]; /* StringToLoc isolator */
  int  PASS;                /* number of tests that passed */
  unsigned char INST_WILDCARD;
  EQSUM locmsg;
  int retVal = -1;

  GetInst("INST_WILDCARD",&INST_WILDCARD);
  while (1)
  {
    gotMsg = FALSE;
    while (gotMsg == FALSE)
      {
        strcpy(record,"\0");
        /* Grab the next message 
        ************************/
        RequestMutex ();
        ret = dequeue (&MsgQueue, record, &msgSize, &reclogo);

        /* Warning: once you call StringToLoc then your string is no
           no longer suitable for another call to it, whether in this
           routine out uploaded to the shared memory and called by
           another module.  So, just make a sacred copy, record2, for
           sending to putmsg if all tests pass.
         ***********************************************************/
        strncpy(record2,record,msgSize);
        ReleaseMutex_ew ();

        if (ret < 0)  /* empty queue */
        {
          sleep_ew (500);
        }
        else  /* for a message from queue */
        {
          gotMsg = TRUE;
          PASS=TRUE;

          retVal = eqfiltmsg(record, &locmsg, reclogo.type);
          if(retVal != 0)
          {
#ifdef USE_LOGIT
               logit("pt", "eqfilterII: Failed to convert Input Message into str
ucture\n");
#endif
               PASS=FALSE;
               goto FinishedTesting;
          }

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
                logit ("", "eqfilterII: Don't have any regions for inst %d; "
                       "Allowing passing anyway because AllowUndefInst IS set\n",
                       reclogo.instid);

            }
            else
            {
              if (Debug == 1)
                logit ("", "eqfilterII: Don't have any regions for inst %d; "
                       "IGNORING because AllowUndefInst is NOT set\n",
                       reclogo.instid);
             
              PASS=FALSE;
              goto FinishedTesting;
            }
          } else if (locmsg.elat == (float)EQSUM_NULL || 
                   locmsg.elon == (float)EQSUM_NULL){
            logit("","eqfilterII: NULL epicenter values: lat = %f, lon = %f\n",
                      locmsg.elat,locmsg.elon);
          } else {
            /* Extract lat and lon of the event from the arc message 
             *******************************************************/

            lat = locmsg.elat;
            lon = locmsg.elon;

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

          /*******************************
            Begin Depth Test
           *******************************/
          if(locmsg.edepth == (float)EQSUM_NULL && NDepthTest>0){
            logit("","null edepth (%f), skipping test\n",locmsg.edepth);
          } else if(NDepthTest>0) {
            gotit=FALSE;
            IsLimit1LTArgLTLimit2(&gotit, NDepthTest, DepthTest, 
                                  reclogo.instid, locmsg.edepth , &PASS);

            /* if not found in the list, are wildcards enabled 
                  note that we really do need a second loop to
                  avoid clobbering defined instid's with wildcards.
             **************************************************/
            if(gotit==FALSE)
              IsLimit1LTArgLTLimit2(&gotit, NDepthTest, DepthTest, 
                                    INST_WILDCARD, locmsg.edepth , &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilterII: event failed DepthTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin nphTest
           ***************/
          if(locmsg.npha == (int)EQSUM_NULL && NnphTest>0){
            logit("","null npha (%d), skipping test\n",locmsg.npha);
          } else if(NnphTest>0) {
            gotit=FALSE;
            IsLimitLTArg(&gotit, NnphTest, nphTest, reclogo.instid, (float)locmsg.npha, &PASS);

            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NnphTest, nphTest, INST_WILDCARD, (float)locmsg.npha, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilterII: event failed nphTest\n");
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
                         (float)locmsg.nphtot, &PASS);

            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NnphtotTest, nphtotTest, INST_WILDCARD,
                           (float)locmsg.nphtot, &PASS);

            if(gotit==FALSE)
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter:  event failed nphtotalTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin nstaTest
           ***************/
          if(locmsg.nsta == (int)EQSUM_NULL && NnstaTest>0){
            logit("","null nsta (%d), skipping test\n",locmsg.nsta);
          } else if(NnstaTest>0) {
            gotit=FALSE;
            IsLimitLTArg(&gotit, NnstaTest, nstaTest, reclogo.instid, \
                         (float)locmsg.nsta, &PASS);

            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NnstaTest, nstaTest, INST_WILDCARD, \
                           (float)locmsg.nsta, &PASS);

            if(gotit==FALSE)
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilterII: event failed nstaTest\n");
              goto FinishedTesting;
            }
          }


          /***************
            Begin GapTest
           ***************/
          if(locmsg.gap == (float)EQSUM_NULL && NGapTest>0){
            logit("","null gap (%f), skipping test\n",locmsg.gap);
          } else if(NGapTest>0) {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NGapTest, GapTest, reclogo.instid, (float)locmsg.gap, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NGapTest, GapTest, INST_WILDCARD, (float)locmsg.gap, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilterII: event failed GapTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin GDminTest
           ***************/
          if(locmsg.gdmin == (float)EQSUM_NULL && NGDminTest>0){
            logit("","null gdmin (%f), skipping test\n",locmsg.gdmin);
          } else if(NGDminTest>0) {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NGDminTest, GDminTest, reclogo.instid, (float)locmsg.gdmin, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NGDminTest, GDminTest, INST_WILDCARD, (float)locmsg.gdmin, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilterII: event failed GDminTest\n");
              goto FinishedTesting;
            }
          }
          /***************
            Begin DminTest
           ***************/
          if(NDminTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NDminTest, DminTest, reclogo.instid, (float)locmsg.dmin, &PASS);

            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NDminTest, DminTest, INST_WILDCARD, (float)locmsg.dmin, &PASS);

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
          if(locmsg.rms == (float)EQSUM_NULL && NRMSTest>0){
            logit("","null rms (%f), skipping test\n",locmsg.rms);
          } else if(NRMSTest>0) {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NRMSTest, RMSTest, reclogo.instid, (float)locmsg.rms, &PASS);
     
            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NRMSTest, RMSTest, INST_WILDCARD, (float)locmsg.rms, &PASS);

            if(gotit==FALSE) 
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilterII: event failed RMSTest\n");
              goto FinishedTesting;
            }
          }
          /***************
            Begin MaxE0Test
           ***************/
          if(NMaxE0Test>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NMaxE0Test, MaxE0Test, reclogo.instid, (float)locmsg.e0, &PASS);

            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxE0Test, MaxE0Test, INST_WILDCARD, (float)locmsg.e0, &PASS);

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
            IsLimitGTArg(&gotit, NMaxERHTest, MaxERHTest, reclogo.instid, (float)locmsg.errh, &PASS);

            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxERHTest, MaxERHTest, INST_WILDCARD, (float)locmsg.errh, &PASS);

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
            IsLimitGTArg(&gotit, NMaxERZTest, MaxERZTest, reclogo.instid, (float)locmsg.errz, &PASS);

            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxERZTest, MaxERZTest, INST_WILDCARD, (float)locmsg.errz, &PASS);

            if(gotit==FALSE)
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MaxERZTest\n");
              goto FinishedTesting;
            }
          }

          /***************
            Begin MaxAVHTest
           ***************/
          if(NMaxAVHTest>0)
          {
            gotit=FALSE;
            IsLimitGTArg(&gotit, NMaxAVHTest, MaxAVHTest, reclogo.instid, (float
)locmsg.avh, &PASS);

            if(gotit==FALSE)
              IsLimitGTArg(&gotit, NMaxAVHTest, MaxAVHTest, INST_WILDCARD, (float)locmsg.avh, &PASS);

            if(gotit==FALSE)
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MaxAVHTest\n");
              goto FinishedTesting;
            }
          }


          /***************
            Begin MinMagTest
           ***************/
          if(NMinMagTest>0)
          {
            gotit=FALSE;
            IsLimitLTArg(&gotit, NMinMagTest, MinMagTest, reclogo.instid, (float)locmsg.Md, &PASS);

            if(gotit==FALSE)
              IsLimitLTArg(&gotit, NMinMagTest, MinMagTest, INST_WILDCARD, (float)locmsg.Md, &PASS);

            if(gotit==FALSE)
              PASS=FALSE;

            if(PASS==FALSE) {
              if(Debug==1) logit("t","eqfilter: event failed MinMagTest\n");
              goto FinishedTesting;
            }
          }
          /*********************************************************************
            Begin NcodaTest
            this ones a bit different so we don't get to use the Isxxx routines
           ********************************************************************/
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

              if(gotit2==TRUE && locmsg.ncoda < NcodaTest[i].var1 && locmsg.Md > NcodaTest[i].var2)
              {
                PASS=FALSE;
                if(Debug==1)
                { 
                  logit("et","eqfilter: event failed NcodaTest\n");
                  logit("et","          locmsg.ncoda = %f is less than %f\n",locmsg.ncoda,NcodaTest[i].var1);
                  logit("et","          AND Md = %f is greater than %f\n",locmsg.Md,NcodaTest[i].var2);
                } 
              }
              else if (gotit2 == TRUE && PASS==TRUE)
              {  
                if(Debug==1)
                  logit("et","eqfilter got %f codalen's with Md=%f\n",locmsg.ncoda,locmsg.Md);
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

                if(gotit2==TRUE && locmsg.ncoda < NcodaTest[i].var1 && locmsg.Md > NcodaTest[i].var2)
                { 
                  PASS=FALSE;
                  if(Debug==1)
                  {
                    logit("et","eqfilter: event failed NcodaTest\n");
                    logit("et","          ncoda = %f is less than %f\n",
                           locmsg.ncoda,NcodaTest[i].var1);

                    logit("et","          AND Md = %f is greater than %f\n",
                           locmsg.Md,NcodaTest[i].var2);
                  }
                } 
                else if (gotit2==TRUE && PASS==TRUE)
                { 
                  if(Debug==1)
                    logit("et","eqfilter got %f codalen's with Md=%f\n",locmsg.ncoda,locmsg.Md);
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


       /**********************************************
        No more tests: pass the message if appropriate
       ***********************************************/
       FinishedTesting:

          if(PASS==TRUE)
          {
            /* eqfilter used strlen(record) but should probably use
               whatever size dequeue told us it was*/
            if (tport_putmsg (&OutRegion, &reclogo, msgSize, record2) != PUT_OK)
            {
              logit ("et", "eqfilterII: Error writing message %d-%d-%d.\n", 
                            reclogo.instid, reclogo.mod, reclogo.type);
              ProcessorStatus = 1;
              KillSelfThread ();
            }
            else
            {
              logit ("et","eqfilterII passed message from inst:%d module:%d type:%d.\n",
                           reclogo.instid, reclogo.mod, reclogo.type);
              if (Debug==1){
                logit("et","Time: %s lat: %5.2f lon: %6.2f Dep: %6.2f \n",
                      locmsg.origin_time_char,locmsg.elat, locmsg.elon,
                      locmsg.edepth);
              }
            }
          }
          else
          {
            if (Debug==1)
            {
              logit ("et", "eqfilterII: Rejected message from inst:%d module:%d type:%d.\n",
                            reclogo.instid, reclogo.mod, reclogo.type);
              if (Debug==1){
                logit("et","lat: %5.2f lon: %6.2f Dep: %6.2f \n", 
                      locmsg.elat, locmsg.elon, locmsg.edepth);
              }
            }
          }
       } /* if ret > 0 */

     } /* while no messages are dequeued */

     ProcessorStatus = 0;
   }

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
            logit("et","eqfilterII/IsLimit1LTArgLTLimit2: event failed\n");
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
            logit("et","eqfilterII/IsLimitLTArg: event failed\n");
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
            logit("et","eqfilterII/IsLimitGTArg: event failed\n");
            logit("et","         event value=%.2f is greater than or equal to limit=%.2f\n",
                   myarg, MyPar[i].var);
          }
        }
      } /* if gotit==TRUE */
    }   /*  for i<N */
  } /* if N>=0 */
} /* end subroutine */



/* eqfiltmsg. Put all the ifs here and add additional subroutines as
   message types are added
   *****************************************************************/
int eqfiltmsg(char *in_msg, EQSUM *out_msg, unsigned char msg_type)
{
  int ret;

  ret = -1;

  strcpy(out_msg->origin_time_char,"-12345");
  out_msg->tOrigin = (double)EQSUM_NULL;
  out_msg->elat = (float)EQSUM_NULL;
  out_msg->elon = (float)EQSUM_NULL;
  out_msg->edepth = (float)EQSUM_NULL;
  out_msg->gap = (float)EQSUM_NULL;
  out_msg->nsta = (int)EQSUM_NULL;
  out_msg->npha = (int)EQSUM_NULL;
  out_msg->nphtot = (int)EQSUM_NULL;
  out_msg->gdmin = (float)EQSUM_NULL;
  out_msg->dmin = (float)EQSUM_NULL;
  out_msg->rms = (float)EQSUM_NULL;
  out_msg->e0 = (float)EQSUM_NULL;
  out_msg->errh = (float)EQSUM_NULL;
  out_msg->errz = (float)EQSUM_NULL;
  out_msg->avh = (float)EQSUM_NULL;
  out_msg->Md = (float)EQSUM_NULL;

  if (msg_type == TypeHyp2000Arc)
      ret = hyp2eqfilt(in_msg, out_msg);
  else if (msg_type == TypeLocGlobal)
      ret = glloc2eqfilt(in_msg, out_msg);
  else if (msg_type == TypeRayloc)
      ret = rayloc2eqfilt(in_msg, out_msg);
  else
    logit ("e", "eqfilterII: Uknown message type %d\n",msg_type);
    
  return(ret);
}

/* hyp2eqfilt  from hyp2000arc to eqfiltsum */
int hyp2eqfilt(char *in_msg, EQSUM *out_msg)
{
  /* don't really need all these vars but I (withers) am lazy
     and did a straight copy and paste from eqfilter */

  struct Hsum Sum;            /* Hyp2000 summary data */
  struct Hpck Pick;          /* Hyp2000 pick structure */
  char     *in;             /* working pointer to archive message    */
  char      line[MAX_STR];  /* to store lines from msg               */
  char      shdw[MAX_STR];  /* to store shadow cards from msg        */
  int       msglen;         /* length of input archive message       */
  int       nline;          /* number of lines (not shadows) so far  */
  float ncoda;
  int ret;

  /*******************************************************
   * Read one data line and its shadow at a time from arcmsg; process them
   ***********************************************************************/

  /* Initialize some stuff
   ***********************/
  nline  = 0;
  msglen = strlen( in_msg );
  in     = in_msg;
  while( in < in_msg+msglen ) {
    if ( sscanf( in, "%[^\n]", line ) != 1 ) {
      logit( "et", "eqfilterII: Error reading data from arcmsg\n" );
      return(-1);
    }
    in += strlen( line ) + 1;
    if ( sscanf( in, "%[^\n]", shdw ) != 1 ) {
      logit( "et", "eqfilterII: Error reading data shadow from arcmsg\n");
      return(-1);
    }
    in += strlen( shdw ) + 1;
    nline++;

    /* Process the hypocenter card (1st line of msg) & its shadow
     ************************************************************/
    if( nline == 1 ) {                /* hypocenter is 1st line in msg */
      ncoda=0.0;
      if ( read_hyp( line, shdw, &Sum ) != 0) {
        logit ("e", "eqfilterII: Call to read_hyp failed.\n");
        return(-1);
      }
/*
      if (Debug == 1) {
        logit( "e", "eqfilterII got following EVENT MESSAGE:\n");
        logit( "e", "%s\n", line );
      }
*/
      continue;
    }
    if( strlen(line) < (size_t) 75 )  /* found the terminator line */
      break;

    if ( read_phs( line, shdw, &Pick ) !=0) {
      logit ("e", "eqfilterII: Call to read_phs failed.\n");
      return(-1);
    }
    else if(Pick.codalen>0) {
      ncoda = ncoda + 1.0;
    }
/*
    if (Debug == 1) {
      logit( "e", "%s\n", line );
    }
*/
    continue;
  } /*end while over reading message*/

  /* Sum call doesn't make sense */
  strcpy(out_msg->origin_time_char,(&Sum)->cdate);
  if(ret = epochsec17(&out_msg->tOrigin,out_msg->origin_time_char)!=0){
    logit("pt", ": Failed to convert Input Message into structure\n");
  }
  out_msg->elat = (double)Sum.lat;
  out_msg->elon = (double)Sum.lon;
  out_msg->edepth = (double)Sum.z;
  out_msg->gap = (double)Sum.gap;
  /* no nsta in Hsum */
  out_msg->npha = (int)Sum.nph;
  out_msg->nphtot = (int)Sum.nphtot;
  /* no gdmin in Hsum */
  out_msg->dmin = (double)Sum.dmin;
  out_msg->rms = (double)Sum.rms;
  out_msg->e0 = (double)Sum.e0;
  out_msg->errh = (double)Sum.erh;
  out_msg->errz = (double)Sum.erz;
  /* no avh in Hsum */
  out_msg->Md = (double)Sum.Md;
  out_msg->ncoda = (int)ncoda;

  return(ret);

}

/* glloc2eqfilter  from loc_global to eqfiltsum */
int glloc2eqfilt(char *in_msg, EQSUM *out_msg)
{
  GLOBAL_LOC_STRUCT p_loc;
  int ret;

  ret = StringToLoc(&p_loc, in_msg);
  if(ret != GLOBAL_MSG_SUCCESS) {
    logit("pt", "lib_global_loc: Failed to convert Input Message into structure\n");
    return(ret);
  }

  /* something is wrong here */
  /* v6.2 used a time string, we now use epoch seconds
  strcpy(out_msg->origin_time_char,(&p_loc)->origin_time); */

  out_msg->tOrigin = p_loc.tOrigin;
  /* convert to a date time string */
  ret = TimeToDTString( out_msg->tOrigin, out_msg->origin_time_char );
  if(ret != GLOBAL_MSG_SUCCESS )
    logit("pt", ": Failed to convert Input origin time to date string\n");

  out_msg->elat = (double)p_loc.lat;
  out_msg->elon = (double)p_loc.lon;
  out_msg->edepth = (double)p_loc.depth;
  out_msg->gap = (double)p_loc.gap;
  /* no nsta in global_loc */
  /* no npha in global_loc */
  out_msg->nphtot = (int)p_loc.pick_count;
  /* no dmin in global_loc (we define dmin in km, gdmin in degrees) */
  out_msg->gdmin = (double)p_loc.dmin;
  out_msg->rms = (double)p_loc.rms;
  /* no e0 in global_loc */
  /* no errh in global_loc */
  /* no errz in global_loc */
  /* no avh in global_loc */
  /* no minmag in global_loc */
  /* no ncoda in global_loc */

  return(ret);

}

/* rayloc2eqfilt  from rayloc to eqfiltsum */
rayloc2eqfilt(char *in_msg, EQSUM *out_msg)
{
  RAYLOC_MESSAGE_HEADER_STRUCT *p_struct = NULL;
  int ret,i;

  ret = rayloc_MessageToRaylocHeader(&p_struct, in_msg);
  
  if(ret != RAYLOC_MSG_SUCCESS) {
    logit("pt", "lib_rayloc: Failed to convert Input Message into structure\n");
    return(ret);
  }else{
    logit("", "lib_rayloc: Succesfully converted Input Message into structure\n");
  }

  /* temp kludge to create rayloc msg files */
/*
  RaylocHeaderToUniqueFile( p_struct );
*/

  strcpy(out_msg->origin_time_char,p_struct->origin_time_char);
  out_msg->tOrigin = (double)p_struct->origin_time;
  out_msg->elat = (double)p_struct->elat;
  out_msg->elon = (double)p_struct->elon;
  out_msg->edepth = (double)p_struct->edepth;
  out_msg->gap = (double)p_struct->gap;
  out_msg->nsta = (int)p_struct->nsta;
  /* nphtot is really npha in rayloc */
  out_msg->nphtot = (int)p_struct->npha;
  out_msg->gdmin = (double)p_struct->dmin;
  out_msg->rms = (double)p_struct->se;
  out_msg->e0 = (double)p_struct->axis[0];
  for (i=1; i<3; i++){
    if((double)p_struct->axis[i] > out_msg->e0)
      out_msg->e0 = (double)p_struct->axis[i];
  }
  out_msg->errh = (double)p_struct->errh;
  out_msg->errz = (double)p_struct->errz;
  out_msg->avh = (double)p_struct->avh;
  /* no minmag in rayloc */
  /* no ncoda in global_loc */


  /* the rayloc lib allocates the struct space so make
     sure we tidy up.
   ***************************************************/

  rayloc_FreeRaylocHeader( &p_struct );

  return(ret);

}

/**************************************************************
   temporary routine used in rayloc2eqfilt to create cache of
   rayloc message files
 *************************************************************/
int RaylocHeaderToUniqueFile( RAYLOC_MESSAGE_HEADER_STRUCT *p_struct )
{
  FILE *fid;
  char MsgDir[]="/export/home/ew/RaylocMsgs";
  char MsgFile[2048];
  int i;
  char _author[10];

  logit("","construcing filename %s, %lf, %ld\n",MsgDir,p_struct->origin_time,p_struct->event_id);
  sprintf(MsgFile,"%s/%ld.msg\0",MsgDir,p_struct->event_id);
  logit("","filename constructed: %s\n",MsgFile);
  if ( (fid=fopen(MsgFile,"w")) == NULL){
     logit("","eqfilterII: could not open %s for write\n",MsgFile);
  } else {
    /* the fprintf statements were stolen from rayloc_message_rw lib */
    EncodeAuthor( p_struct->logo, _author );
    fprintf(fid,
            "RLC %s %d %ld %s %.4f %.4f %.3f %d %d %d %d %.3f %.3f %c %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %c %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
             _author,
             p_struct->version,
             p_struct->event_id ,
             p_struct->origin_time_char ,
             p_struct->elat ,
             p_struct->elon ,
             p_struct->edepth ,
             p_struct->nsta ,
             p_struct->npha ,
             p_struct->suse ,
             p_struct->puse ,
             p_struct->gap ,
             p_struct->dmin ,
             p_struct->depth_flag ,
             p_struct->oterr ,
             p_struct->laterr ,
             p_struct->lonerr ,
             p_struct->deperr ,
             p_struct->se ,
             p_struct->errh ,
             p_struct->errz ,
             p_struct->avh ,
             p_struct->q ,
             p_struct->axis[0] ,
             p_struct->az[0] ,
             p_struct->dp[0] ,
             p_struct->axis[1] ,
             p_struct->az[1] ,
             p_struct->dp[1] ,
             p_struct->axis[2] ,
             p_struct->az[2] ,
             p_struct->dp[2]);
    for (i = 0; i < p_struct->numPicks; i++){
      EncodeAuthor( (p_struct->picks[i])->logo, _author );
      fprintf(fid, "PCK %s %d %ld %s %s %s %s %s %.3f %.3f %.3f %c\n",
                    _author,
                    (p_struct->picks[i])->version ,
                    (p_struct->picks[i])->pick_id ,
                    (p_struct->picks[i])->station ,
                    (p_struct->picks[i])->channel ,
                    (p_struct->picks[i])->network ,
                    (p_struct->picks[i])->location ,
                    (p_struct->picks[i])->phase_name ,
                    (p_struct->picks[i])->residual ,
                    (p_struct->picks[i])->dist ,
                    (p_struct->picks[i])->az ,
                    (p_struct->picks[i])->weight_flag);
    }
    fprintf(fid,"\n");

    fclose(fid);
  }
}
