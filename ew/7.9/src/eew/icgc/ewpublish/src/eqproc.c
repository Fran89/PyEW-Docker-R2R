
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqproc.c,v 1.4 2002/05/16 15:34:50 patton Exp $
 *
 *    Revision history:
 *     $Log: eqproc.c,v $
 *     Revision 1.4  2002/05/16 15:34:50  patton
 *     Made logit changes.
 *
 *     Revision 1.3  2001/05/09 18:39:43  dietz
 *     Changed to shut down gracefully if the transport flag is
 *     set to TERMINATE or MyPID.
 *
 *     Revision 1.2  2000/07/24 20:38:44  lucky
 *     Implemented global limits to module, installation, ring, and message type strings.
 *
 *     Revision 1.1  2000/02/14 17:12:03  lucky
 *     Initial revision
 *
 * // /D_USE_32BIT_TIME_T   
 */

/*
 * eqproc.c : Determine event termination and report results.
 *              This is a notifier module prototype
 */
#define ABS(x) (((x) >= 0.0) ? (x) : -(x) )
#define X(lon) (facLon * ((lon) - orgLon))
#define Y(lat) (facLat * ((lat) - orgLat))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b)) 
#define MAX(a, b)  (((a) > (b)) ? (a) : (b)) 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <chron3.h>
#include <earthworm.h>
#include <kom.h>
#include <site.h>
#include <tlay.h>
#include <transport.h>

#define hypot(x,y) (sqrt((x)*(x) + (y)*(y)))



static SHM_INFO  Region;          /* shared memory region to use for input  */
static SHM_INFO  TestRegion;      /* shared memory region to use for test output */

#define   MAXLOGO   5
MSG_LOGO  GetLogo[MAXLOGO];       /* array for requesting module,type,instid */
short     nLogo;

/* Things to read from configuration file
 ****************************************/
static char RingName[MAX_RING_STR];        /* transport ring to read from            */
static char TestRingName[MAX_RING_STR];    /* transport test ring to write to        */
static char MyModName[MAX_MOD_STR];        /* speak as this module name/id           */
static int  LogSwitch;            /* 0 if no logging should be done to disk */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of ring for input         */
static long          TestRingKey=0; /* key of ring for test output   */  
static unsigned char InstId;        /* local installation id         */
static unsigned char MyModId;       /* Module Id for this program    */
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeQuake2K;
static unsigned char TypeLink;
static unsigned char TypePick2K;
static unsigned char TypeCoda2K;
static unsigned char TypeEvent2K;
static unsigned char TypeKill;
static unsigned char TypeProxies;
static unsigned char TypeHyp2000Arc;

static char  EqMsg[MAX_BYTES_PER_EQ];    /* char string to hold event2k message */

/* Error messages used by ewpublish
 *******************************/
#define  ERR_MISSMSG            0
#define  ERR_TOOBIG             1
#define  ERR_NOTRACK            2
#define  ERR_PICKREAD           3
#define  ERR_CODAREAD           4
#define  ERR_QUAKEREAD          5
#define  ERR_LINKREAD           6
#define  ERR_UNKNOWNSTA         7
#define  ERR_TIMEDECODE         8
#define  ERR_PROXIESREAD        9

#define MAXSTRLEN             256
static char  Text[MAXSTRLEN];         /* string for log/error messages       */

#define DIST_SPTIMEDIF          8.	 /* relation between distance from earthquake to station 
									  and the difference of time between S and P arrivals at the station */

#define MAXHYP 100
typedef struct {
        double  trpt;
        double  t;
        double  lat;
        double  lon;
        double  z;
        long    id;
        float   rms;
        float   dmin;
        float   ravg;
        float   gap;
        int     nph;
        short   flag;    /* 0=virgin, 1=modified, 2=reported, -1=killed  */
        double  mag;	 // new: estimated magnitude 
		char    magtype; // new: magnitude type 
		int     nummags; // new: number of channel magnitudes 

} RPT_HYP;
RPT_HYP Hyp[MAXHYP];

#define MAXPCK 1000
#define FLAG_PICK		1
#define FLAG_CODA		2
#define FLAG_PROXIES	4
long nPck = 0;
typedef struct {
        double          t;
        long            quake;
        short           id;
        unsigned char   src;
        unsigned char   instid;
        short           site;
        char            phase;
        char            ie;
        char            fm;
        char            wt;
        long            pamp[3];
        long            caav[6];	// ho elimino... 
        short           clen;		// ho elimino...
        char            codawt;		// ho elimino... 
        time_t          timeout;
		int             tau0;		// new
		double          tauc;		// new
		double          pd;			// new
		double          tauc3s;		// new
		double          pd3s;		// new
		double          magpd;		// new
		double          magtauc;	// new
		double          mag;		// new
		double          r;			// new: depi
		double          R;			// new: dhypo
		double          tres;		// new: residual
		int             flag;		// new: FLAG_PICK: pick; FLAG_CODA:coda; FLAG_PROXIES:proxies
} RPT_PCK;
RPT_PCK Pck[MAXPCK];

int nSrt;
typedef struct {
        double  t;
        int     ip;
} SRT;
SRT Srt[MAXPCK];


/* Parameters
 ************/
double HeartBeatInt = 30.0;         /* Latency before quake report          */
double orgLat;
double orgLon;
double facLat;
double facLon;
pid_t  MyPID=0;
//
// Valors per defecte
double dDepiMax = 300;          /* Magnitude estimation: maximum depi */
double dPdMin = 1e-5;           /* Magnitude estimation: minimum Pd value */
double a0 = 1.5;                /* Magnitude estimation: Pd|200 calculation */
double a1 = 1.0;                /* Magnitude estimation: 1st coefficient for MagPd */
double a2 = 8.4;                /* Magnitude estimation: 2nd coefficient for MagPd */
//
double b1 = 0.25;               /* Magnitude estimation: 1st coefficient for MagTauC */
double b2 = 1.;                 /* Magnitude estimation: 2nd coefficient for MagTauC */
//
double c1 = 0.5;                /* Magnitude estimation: 1st coefficient for the relationship between final magnitude and MagPd,MagTauC  */
double c2 = 0.5;                /* Magnitude estimation: 2nd coefficient for the relationship between final magnitude and MagPd,MagTauC  */
char   cMagType = 'W';          /* Magnitude estimation: magnitude type */


/* Functions in this source file
 *******************************/
void  eqp_config ( char * );
void  eqp_lookup ( void   );
int   eqp_pick2k ( char * );
int   eqp_coda2k ( char * );
int   eqp_link   ( char * );
int   eqp_quake2k( char * );
int   eqp_compare( const void *, const void * );
void  eqp_check  ( double );
void  eqp_status ( unsigned char, short, char * );
int   eqp_proxies ( char * );
//
void eqc_bldhyp( char *, RPT_HYP * );
void eqc_bldphs( char *, RPT_PCK * );
void eqc_bldterm( char *, RPT_HYP * );
void eqp_mag( int iq );
int CompareDoubles( const void *s1, const void *s2 );

int main( int argc, char **argv )
{
   double     t;
   double     tcheck;
   char       rec[MAXSTRLEN];   /* actual retrieved message  */
   long       recsize;          /* size of retrieved message */
   MSG_LOGO   reclogo;          /* logo of retrieved message */
   int        res;
   int        iq;
   int        is;


/* Check command line arguments
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: ewpublish <configfile>\n" );
        exit( 0 );
   }

/* Initialization
 ****************/
   tcheck = 0.0;
   nPck   = 0;                  /* no picks have been processed    */
   for(iq=0; iq<MAXHYP; iq++)   /* set all hypocenter id's to zero */
   {
        Hyp[iq].id  = 0;
   }

/* Initialize name of log-file & open it
 ***************************************/
   logit_init( argv[1], 0, MAXSTRLEN*2, 1 );

/* Read the configuration file(s)
 ********************************/
   eqp_config( argv[1] );
   logit( "" , "ewpublish: Read command file <%s>\n", argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   eqp_lookup();

/* Store my own processid
 ************************/
   MyPID = getpid();

/* Reinitialize logit to desired logging level
 *********************************************/
   logit_init( argv[1], 0, MAXSTRLEN*2, LogSwitch );
   

/* Attach to PICK shared memory ring & flush out all old messages
 ****************************************************************/
   tport_attach( &Region, RingKey );
   logit( "", "ewpublish: Attached to public memory region %s: %d\n",
          RingName, RingKey );
   while( tport_getmsg( &Region, GetLogo, nLogo,
                        &reclogo, &recsize, rec, MAXSTRLEN ) != GET_NONE );

   if(TestRingKey)
   {
	   tport_attach( &TestRegion, TestRingKey );
	   logit( "", "ewpublish: Attached to public memory region %s: %d\n",
			  TestRingName, TestRingKey );
   }

/* Calculate network origin
 **************************/
   logit( "", "ewpublish: nSite = %d\n", nSite );

   orgLat = 0.0;
   orgLon = 0.0;
   for(is=0; is<nSite; is++) {
        orgLat += Site[is].lat;
        orgLon += Site[is].lon;
   }
   orgLat /= nSite;
   orgLon /= nSite;
   facLat = (double)(40000.0 / 360.0);
   facLon = facLat * cos(6.283185 * orgLat / 360.0);

/*------------------- setup done; start main loop -------------------------*/

   while(1)
   {
        /* See if it's time to check all the hypocenters for reporting
         *************************************************************/
        t = tnow();
        if(t < tcheck) {
           sleep_ew(1000);
        }
        else {
           tcheck = t + 0.3 * HeartBeatInt;
           /*eqp_check(t);*/

        /* Send heartbeat
         ****************/
           eqp_status( TypeHeartBeat, 0, "" );
        }

/* Process all new hypocenter, pick-time, pick-coda, & link messages
 *******************************************************************/
        do
        {
        /* see if a termination has been requested
         *****************************************/
           if ( tport_getflag( &Region ) == TERMINATE  ||
                tport_getflag( &Region ) == MyPID )
           {
           /* detach from shared memory */
                tport_detach( &Region );
                tport_detach( &TestRegion ); 
           /* write a few more things to log file and close it */
                logit( "t", "ewpublish: Termination requested; exiting!\n" );
                fflush( stdout );
                return 0;
           }

        /* Get & process the next message from shared memory
         ***************************************************/
           res = tport_getmsg( &Region, GetLogo, nLogo,
                               &reclogo, &recsize, rec, MAXSTRLEN-1 );
           switch(res)
           {
           case GET_MISS:
                sprintf( Text,
                        "Missed msg(s)  i%u m%u t%u  region:%ld.",
                         reclogo.instid, reclogo.mod, reclogo.type, Region.key);
                eqp_status( TypeError, ERR_MISSMSG, Text );

           case GET_NOTRACK:
                if(res == GET_NOTRACK)
                {
                    sprintf( Text,
                            "Msg received (i%u m%u t%u); transport.h NTRACK_GET exceeded",
                             reclogo.instid, reclogo.mod, reclogo.type );
                    eqp_status( TypeError, ERR_NOTRACK, Text );
                }
           case GET_OK:
                rec[recsize] = '\0';              /*null terminate the message*/
                /*logit( "", "%s\n", rec );*/  /*debug*/
                
                if( reclogo.type == TypePick2K )
                {
                    eqp_pick2k( rec );
                }
                else if( reclogo.type == TypeProxies ) 
                {
                    eqp_proxies( rec ); 
                }
                else if( reclogo.type == TypeQuake2K )
                {
                    eqp_quake2k( rec );
                }
                else if( reclogo.type == TypeLink )
                {
                    eqp_link( rec );
                }
                break;

           case GET_TOOBIG:
                sprintf(Text,
                        "Retrieved msg[%ld] (i%u m%u t%u) too big for rec[%d]",
                        recsize, reclogo.instid, reclogo.mod, reclogo.type,
                        MAXSTRLEN-1 );
                eqp_status( TypeError, ERR_TOOBIG, Text );

           case GET_NONE:
                break;

           }
           fflush( stdout );

        } while( res != GET_NONE );  /*end of message-processing-loop */

   }
/*-----------------------------end of main loop-------------------------------*/
}

/******************************************************************************/
/*      eqp_config() processes command file(s) using kom.c functions          */
/*                   exits if any errors are encountered                      */
/******************************************************************************/
void eqp_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   char     processor[15];
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command
 *****************************************************/
   ncommand = 5;  
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file
 **********************************/
   nfiles = k_open( configfile );
   if ( nfiles == 0 ) {
        fprintf( stderr,
                "ewpublish: Error opening command file <%s>; exiting!\n",
                 configfile );
        exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  fprintf( stderr,
                          "ewpublish: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command
         ************************************/
            strcpy( processor, "eqp_config" );

         /* Numbered commands are required
          ********************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
            else if( k_its("TestRing") ) {  
				if( ( str=k_str() ) ) {
					if(str){
						strcpy( TestRingName, str );
						if( ( TestRingKey = GetKey(TestRingName) ) == -1 ) {
						  fprintf( stderr,
								   "ewpublish:  Invalid test ring name <%s>; exiting!\n", str);
						  exit( -1 );
						}
					}else
					{
						fprintf( stderr,
							   "ewpublish: Invalid module name <%s>", str );
						fprintf( stderr, " in <GetPicksFrom> cmd; exiting!\n" );
						exit( -1 );
					}
				}
			}

         /* Enter installation & module to get picks & codas from
          *******************************************************/
  /*3*/     else if( k_its("GetPicksFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    fprintf( stderr,
                            "ewpublish: Too many <Get*> commands in <%s>",
                             configfile );
                    fprintf( stderr, "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       fprintf( stderr,
                               "ewpublish: Invalid installation name <%s>", str );
                       fprintf( stderr, " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       fprintf( stderr,
                               "ewpublish: Invalid module name <%s>", str );
                       fprintf( stderr, " in <GetPicksFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_PICK2K", &GetLogo[nLogo].type ) != 0 ) {
                    fprintf( stderr,
                               "ewpublish: Invalid message type <TYPE_PICK2K>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_CODA2K", &GetLogo[nLogo+1].type ) != 0 ) {
                    fprintf( stderr,
                               "ewpublish: Invalid message type <TYPE_CODA2K>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_PROXIES", &GetLogo[nLogo+2].type ) != 0 ) {
                    fprintf( stderr,
                               "ewpublish: Invalid message type <TYPE_PROXIES>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }

                nLogo  += 3; 
                init[3] = 1;
            }
         /* Enter installation & module to get associations from
          ******************************************************/
  /*4*/     else if( k_its("GetAssocFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    fprintf( stderr,
                            "ewpublish: Too many <Get*From> commands in <%s>",
                             configfile );
                    fprintf( stderr, "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       fprintf( stderr,
                               "ewpublish: Invalid installation name <%s>", str );
                       fprintf( stderr, " in <GetAssocFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       fprintf( stderr,
                               "ewpublish: Invalid module name <%s>", str );
                       fprintf( stderr, " in <GetAssocFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_QUAKE2K", &GetLogo[nLogo].type ) != 0 ) {
                    fprintf( stderr,
                            "ewpublish: Invalid message type <TYPE_QUAKE2K>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_LINK", &GetLogo[nLogo+1].type ) != 0 ) {
                    fprintf( stderr,
                            "ewpublish: Invalid message type <TYPE_LINK>" );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
                }
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo, (int) GetLogo[nLogo].instid,
                               (int) GetLogo[nLogo].mod,
                               (int) GetLogo[nLogo].type ); */  /*DEBUG*/
                /*printf("GetLogo[%d] inst:%d module:%d type:%d\n",
                        nLogo+1, (int) GetLogo[nLogo+1].instid,
                               (int) GetLogo[nLogo+1].mod,
                               (int) GetLogo[nLogo+1].type ); */  /*DEBUG*/
                nLogo  += 2;
                init[4] = 1;
            }

         /* These commands change default values; so are not required
          ***********************************************************/
            else if( k_its("HeartBeatInt") )
                HeartBeatInt = k_val();

            else if( k_its("DepiMax") )	
                dDepiMax = k_val();

            else if( k_its("PdMin") )
                dPdMin = k_val();

			else if( k_its("MagType") ){
                str = k_str();
                if( !str || strlen(str)>1 ){
                    fprintf( stderr,
                            "ewpublish: Invalid MagType parameter " );
                    fprintf( stderr, "; exiting!\n" );
                    exit( -1 );
				}
                cMagType = str[0];
			}

			else if( k_its("MagPd") ) {
				a0 = k_val();
				a1 = k_val();
				a2 = k_val();
				if( a1==0 ){
					fprintf( stderr,
						"ewpublish: Invalid MagPd parameter (a1)" );
					fprintf( stderr, "; exiting!\n" );
					exit( -1 );
				}
			}

			else if( k_its("MagTauC") ) {
				b1 = k_val();
				b2 = k_val();
				if( b1==0 ){
					fprintf( stderr,
						"ewpublish: Invalid MagTauC parameter (b1)" );
					fprintf( stderr, "; exiting!\n" );
					exit( -1 );
				}			
			}

			else if( k_its("Mag") ) {
				c1 = k_val();
				c2 = k_val();
				if( (c1==0 && c2==0) || (c1 + c2 >1.01) || (c1 + c2 <0.99) ){
					fprintf( stderr,
						"ewpublish: Inappropiated parameters (c1/c2)" );
					fprintf( stderr, "; exiting!\n" );
					exit( -1 );
				}			
			}

		  /* Some commands may be processed by other functions
          ***************************************************/
            else if( t_com()    )  strcpy( processor, "t_com"    );
            else if( site_com() )  strcpy( processor, "site_com" );
            else {
                fprintf( stderr, "ewpublish: <%s> Unknown command in <%s>.\n",
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command
         *****************************************************/
            if( k_err() ) {
               fprintf( stderr,
                       "ewpublish: Bad <%s> command for %s() in <%s>; exiting!\n",
                        com, processor, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       fprintf( stderr, "ewpublish: ERROR, no " );
       if ( !init[0] )  fprintf( stderr, "<LogFile> "      );
       if ( !init[1] )  fprintf( stderr, "<MyModuleId> "   );
       if ( !init[2] )  fprintf( stderr, "<RingName> "     );
       if ( !init[3] )  fprintf( stderr, "<GetPicksFrom> " );
       if ( !init[4] )  fprintf( stderr, "<GetAssocFrom> " );
       fprintf( stderr, "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/****************************************************************************/
/*  eqp_lookup( ) Look up important info from earthworm.h tables            */
/****************************************************************************/
void eqp_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        logit( "e" , "ewpublish:  Invalid ring name <%s>; exiting!\n", RingName);
        fprintf( stderr,
                "ewpublish:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      logit( "e" , "ewpublish: error getting local installation id; exiting!\n" );
      fprintf( stderr,
              "ewpublish: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      logit( "e" , "ewpublish: Invalid module name <%s>; exiting!\n", MyModName );
      fprintf( stderr,
              "ewpublish: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_ERROR>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_QUAKE2K", &TypeQuake2K ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_QUAKE2K>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_QUAKE2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_LINK", &TypeLink ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_LINK>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_LINK>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PICK2K", &TypePick2K ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_PICK2K>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_PICK2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_CODA2K", &TypeCoda2K ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_CODA2K>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_CODA2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_EVENT2K", &TypeEvent2K ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_EVENT2K>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_EVENT2K>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_KILL", &TypeKill ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_KILL>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_KILL>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_PROXIES", &TypeProxies ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_PROXIES>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_PROXIES>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHyp2000Arc ) != 0 ) {
      logit( "e" , "ewpublish: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      fprintf( stderr,
              "ewpublish: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   return;
}

/****************************************************************************/
/*  eqp_quake2k() processes a TYPE_QUAKE2K message from binder              */
/****************************************************************************/
int eqp_quake2k( char *msg )
{
        /*char       cdate[25];
        char       corg[25];*/
        char       timestr[20];
        double     t;
        double     lat, lon, z;
        float      rms, dmin, ravg, gap;
        long       qkseq;
        int        nph;
        int        iq;
        int        narg;
        struct Greg  g;
        int        tsec, thun;


/***  Read info from ascii message  ***/
        narg =  sscanf( msg,
                       "%*d %*d %ld %s %lf %lf %lf %f %f %f %f %d",
                        &qkseq, timestr, &lat, &lon, &z,
                        &rms, &dmin, &ravg, &gap, &nph );

        if ( narg < 10 )
        {
                sprintf( Text, "eqp_quake2k: Error reading ascii quake msg: %s", msg );
                eqp_status( TypeError, ERR_QUAKEREAD, Text );
                return( 0 );
        }

        t = julsec17( timestr );
        if ( t == 0. )
        {
                sprintf( Text, "eqp_quake2k: Error decoding quake time: %s", timestr );
                eqp_status( TypeError, ERR_TIMEDECODE, Text );
                return( 0 );
        }


/***  Store all hypo info in ewpublish's RPT_HYP structure  ***/
        iq = qkseq % MAXHYP;

        if( qkseq != Hyp[iq].id )                /* initialize flags the 1st  */
        {                                        /* time a new quake id#      */
                Hyp[iq].flag  = 0;
                Hyp[iq].id    = qkseq;
        }
        Hyp[iq].t    = t;
        Hyp[iq].trpt = tnow() + HeartBeatInt;
        Hyp[iq].lat  = lat;
        Hyp[iq].lon  = lon;
        Hyp[iq].z    = z;
        Hyp[iq].rms  = rms;
        Hyp[iq].dmin = dmin;
        Hyp[iq].ravg = ravg;
        Hyp[iq].gap  = gap;
        Hyp[iq].nph  = nph;

        Hyp[iq].mag      = -99;		// new: related to estimated magnitude 
        Hyp[iq].magtype  = cMagType;// new: related to estimated magnitude 
        Hyp[iq].nummags  = 0;		// new: related to estimated magnitude  

        if (Hyp[iq].flag != 2)                   /* don't reflag if the event */
                Hyp[iq].flag =  1;               /* has already been reported */
        if (Hyp[iq].nph  == 0)
                Hyp[iq].flag = -1;               /* flag an event as killed   */


/***  Write out the time-stamped hypocenter to the log file  ***/
        /*t = tnow();
        date20( t, cdate );
        date20( Hyp[iq].t, corg );*/
        /* Convert julian seconds to date and time
        ***************************************/
        datime( Hyp[iq].t, &g );
        tsec = (int)floor( (double) g.second );
        thun = (int)((100.*(g.second - tsec)) + 0.5);
        if ( thun == 100 )
           tsec++, thun = 0;


        logit("t", "\n");
        logit("t", "Processing eventid: %8ld\n", Hyp[iq].id );
        logit("t",
               "%4d%02d%02d%02d%02d%02d.%02d lat: %9.4f lon: %10.4f z: %6.2f rms: %5.2f dmin: %5.1f ravg: %5.1f gap: %3.0f nph: %3d\n",
               g.year,g.month, g.day, g.hour, g.minute, tsec, thun,
               Hyp[iq].lat, Hyp[iq].lon, Hyp[iq].z,
               Hyp[iq].rms, Hyp[iq].dmin, Hyp[iq].ravg, Hyp[iq].gap, Hyp[iq].nph);

 /***  Magnitude estimation and building of HYP2000ARC message ***/
        eqp_mag(iq);

        return( 1 );
}

/****************************************************************************/
/*  eqp_link() processes a MSG_LINK                                         */
/****************************************************************************/
int eqp_link( char *msg )
{
        long          lp1;
        long          lp2;
        long          lp;
        int           ip, narg;
        long          qkseq;
        short         pkseq;
        unsigned char pksrc;
        unsigned char pkinstid;
        int           isrc, iinstid, iphs;

        narg  = sscanf( msg, "%ld %d %d %hd %d",
                        &qkseq, &iinstid, &isrc, &pkseq, &iphs );
        if ( narg < 5 )
        {
                sprintf( Text, "eqp_link: Error reading ascii link msg: %s", msg );
                eqp_status( TypeError, ERR_LINKREAD, Text );
                return( 0 );
        }
		logit( "t","Read from link:  %ld %d %d %hd %d\n",
			   qkseq, iinstid, isrc, pkseq, iphs );   /*debug*/

        pksrc    = (unsigned char) isrc;
        pkinstid = (unsigned char) iinstid;

        lp2 = nPck;
        lp1 = lp2 - MAXPCK;
        if(lp1 < 0)
                lp1 = 0;
        for( lp=lp1; lp<lp2; lp++ )   /* loop from oldest to most recent */
        {
                ip = lp % MAXPCK;
                if( pkseq    != Pck[ip].id )        continue;
                if( pksrc    != Pck[ip].src )       continue;
                if( pkinstid != Pck[ip].instid )    continue;
                Pck[ip].quake = qkseq;
                Pck[ip].phase = (char) iphs;
                return( 1 );
        }
        return( 0 );
}

/****************************************************************************/
/*    eqp_pick2k() decodes a TYPE_PICK2K message from ascii to binary       */
/****************************************************************************/
int eqp_pick2k( char *msg )
{
        char         str[80];
        time_t       t_in;
        double       tpick;
        int          isite;
        int          i;
        long         pamp[3];
        char         timestr[20];
        char         site[6], net[3], comp[4];
        char         fm, wt;
        int          type, modid, instid;
        short        seq;
        int          lp, lp1, lp2, ip;

/*-------------------------------------------------------------------------
Here's a sample TYPE_PICK2K message (72 chars long):
 10  4  3 2133 CMN  NCVHZ U1  19950831183134.90     953    1113     968\n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 12
---------------------------------------------------------------------------*/

/* Make note of current system time
 **********************************/
        time( &t_in );

/* Check the pick's length
 *************************/
        if( strlen(msg) < 72 )
        {
           sprintf( Text, "eqp_pick2k: Will not decode short TYPE_PICK2K: %s", msg );
           eqp_status( TypeError, ERR_PICKREAD, Text );
           return( 0 );
        }

/* Copy the incoming string to a local buffer so alter it.
 *********************************************************/
        strncpy (str, msg, 72);

/* Read PICK2K string (column oriented).
   Start at the end and work backwards.
 ***************************************/
        str[71] = '\0';   pamp[2] = atol(str+63);
        str[63] = '\0';   pamp[1] = atol(str+55);
        str[55] = '\0';   pamp[0] = atol(str+47);
        str[47] = '\0';   strcpy(timestr, str+30);
        str[30] = '\0';   wt      = str[27];
                          fm      = str[26];
        str[25] = '\0';   strcpy(comp, str+22);
        str[22] = '\0';   strcpy(net,  str+20);
        str[20] = '\0';   strcpy(site, str+15);
        str[14] = '\0';   seq     = (short) atoi(str+10);
        str[9]  = '\0';   instid  = atoi(str+6);
        str[6]  = '\0';   modid   = atoi(str+3);
        str[3]  = '\0';   type    = atoi(str+0);

      logit( "t","Read from pick2k:  %3d %3d %3d %4hd %5s%2s%3s %1c %1c %s\n",
               type,modid,instid,seq,site,net,comp,fm,wt,timestr );   /*debug*/

/* Check for valid values of instid, modid, type
 ***********************************************/
        if( instid<0 || instid>255 ||
            modid<0  || modid>255  ||
            type<0   || type>255      )
        {
           sprintf( Text, "eqp_pick2k: Invalid msgtype, modid, or instid in: %s", msg );
           eqp_status( TypeError, ERR_PICKREAD, Text );
           return( 0 );
        }

/* Calculate pick time (tpick) from character string
 ***************************************************/
        for(i=0; i<18; i++) if(timestr[i] == ' ') timestr[i] = '0';
        tpick = julsec17( timestr );
        if ( tpick == 0. )
        {
           sprintf( Text, "eqp_pick2k: Error decoding time: %s", timestr );
           eqp_status( TypeError, ERR_TIMEDECODE, Text );
           return( 0 );
        }

/* Get site index (isite)
 ************************/
        isite = site_index( site, net, comp );
        if (isite < 0)
        {
           sprintf( Text, "eqp_pick2k: %s%s%s - Not in station list.",
                    site, net, comp );
           eqp_status( TypeError, ERR_UNKNOWNSTA, Text );
           return( 0 );
        }

/* Try to find coda part in existing pick list
 *********************************************/
        lp2 = nPck;
        lp1 = lp2 - MAXPCK;
        if(lp1 < 0)
                lp1 = 0;
        for( lp=lp2-1; lp>=lp1; lp-- )  /* loop from most recent to oldest */
        {
                ip = lp % MAXPCK;
                if( instid != Pck[ip].instid )  continue;
                if( modid  != Pck[ip].src )     continue;
                if( seq    != Pck[ip].id )      continue;
                Pck[ip].t       = tpick;
                Pck[ip].site    = isite;
                Pck[ip].phase   = 0;
                Pck[ip].fm      = fm;
                Pck[ip].ie      = ' ';
                Pck[ip].wt      = wt;
                Pck[ip].timeout = 0;			/***/ //?
                for (i=0; i<3; i++) {
                        Pck[ip].pamp[i] = pamp[i];
                }
				Pck[ip].flag |= FLAG_PICK;  
                return( 1 );
        }

/* Coda was not in list; load pick info into list
 ************************************************/
        ip  = nPck % MAXPCK;
        Pck[ip].instid  = instid;
        Pck[ip].src     = modid;
        Pck[ip].id      = seq;
        Pck[ip].t       = tpick;
        Pck[ip].site    = isite;
        Pck[ip].phase   = 0;
        Pck[ip].fm      = fm;
        Pck[ip].ie      = ' ';
        Pck[ip].wt      = wt;
        for (i=0; i<3; i++) {
                Pck[ip].pamp[i] = pamp[i];
        }
        Pck[ip].quake   = 0;
        Pck[ip].timeout = t_in + 150;		/***/ //?
        Pck[ip].flag    = FLAG_PICK;  
        nPck++;

/* Coda was not in list; zero-out all coda info;
   it will NOT be filled 
 ***********************************************/
        for (i=0; i<6; i++) {
                Pck[ip].caav[i] = 0;
        }
        Pck[ip].clen   = 0;
        Pck[ip].codawt = ' ';

/* Proxies were not in list; zero-out all proxies info;
   it will be filled by TYPE_PROXIES later.
 ***********************************************/		
        Pck[ip].tau0    = 0;
        Pck[ip].tauc    = .0;
        Pck[ip].pd      = .0;
        Pck[ip].tauc3s  = .0;
        Pck[ip].pd3s    = .0;
        Pck[ip].magpd   = .0;
        Pck[ip].magtauc = .0;
        Pck[ip].mag     = .0;
		Pck[ip].r       = .0;
		Pck[ip].R       = .0;
        Pck[ip].tres    = .0;

        return ( 1 );
}

/****************************************************************************/
/*  eqp_coda2k() processes a TYPE_CODA2K message                            */
/****************************************************************************/

int eqp_coda2k( char *msg )
{
        char       str[90];
        long       lp1;
        long       lp2;
        long       lp;
        int        i, ip, n;
        short      coda;
        short      seq;
        long       caav[6];
        char       codawt;
        int        type, modid, instid;

/*----------------------------------------------------------------------------
Here's a sample TYPE_CODA2K message (79 characters long):
 11  6  3 7893 NMW  NCVHZ      48     106     211     182     148     133  15 \n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
------------------------------------------------------------------------------*/
/* Check the coda's length
 *************************/
        if( strlen(msg) < 79 )
        {
           sprintf( Text,
                   "eqp_coda2k: Will not decode short TYPE_CODA2K: %s",
                    msg );
           eqp_status( TypeError, ERR_CODAREAD, Text );
           return( 0 );
        }

/* Copy incoming string to a local buffer so we can alter it.
 ************************************************************/
        strncpy (str, msg, 79);

/* Read CODA2K string (column oriented).
   Start at the end and work backwards.
 ***************************************/
        str[78] = '\0';   codawt  = str[77];
        str[77] = '\0';   coda    = (short) atoi(str+73);
        for (i=5, n=73 ; i>=0 ; i--, n-=8)
        {
           str[n] = '\0'; caav[i] = atol(str+n-8);
        }
     /* str[25] = '\0';   strcpy(comp, str+22); */
     /* str[22] = '\0';   strcpy(net,  str+20); */
     /* str[20] = '\0';   strcpy(site, str+15); */
        str[14] = '\0';   seq     = (short) atoi(str+10);
        str[9]  = '\0';   instid  = atoi(str+6);
        str[6]  = '\0';   modid   = atoi(str+3);
        str[3]  = '\0';   type    = atoi(str+0);

/* Check for valid values of instid, modid, type
 ***********************************************/
        if( instid<0 || instid>255 ||
            modid<0  || modid>255  ||
            type<0   || type>255      )
        {
           sprintf( Text,
                   "eqp_coda2k: Invalid msgtype, modid, or instid in: %s",
                    msg );
           eqp_status( TypeError, ERR_CODAREAD, Text );
           return( 0 );
        }

/* Try to find pick part in existing pick list
 *********************************************/
        lp2 = nPck;
        lp1 = lp2 - MAXPCK;
        if(lp1 < 0)
                lp1 = 0;
        for( lp=lp2-1; lp>=lp1; lp-- )  /* loop from most recent to oldest */
        {
                ip = lp % MAXPCK;
                if( instid != Pck[ip].instid )  continue;
                if( modid  != Pck[ip].src )     continue;
                if( seq    != Pck[ip].id )      continue;
                for (i=0; i<6; i++) {
                   Pck[ip].caav[i] = caav[i];
                }
                Pck[ip].clen     = coda;
                Pck[ip].codawt   = codawt;
                Pck[ip].timeout  = 0;
                Pck[ip].flag    |= FLAG_CODA;
                return( 1 );
        }

/* Pick was not in list; load coda info into list
 ************************************************/
        ip  = nPck % MAXPCK;
        Pck[ip].instid = instid;
        Pck[ip].src    = modid;
        Pck[ip].id     = seq;
        for (i=0; i<6; i++) {
           Pck[ip].caav[i] = caav[i];
        }
        Pck[ip].clen   = coda;
        Pck[ip].codawt = codawt;
        Pck[ip].quake  = 0;
        Pck[ip].flag   = FLAG_CODA; 
        nPck++;

/* Pick was not in list; zero-out all pick info; will be filled by TYPE_PICK
 ***************************************************************************/
        Pck[ip].t      = 0.0;
        Pck[ip].site   = 0;
        Pck[ip].phase  = 0;
        Pck[ip].fm     = ' ';
        Pck[ip].ie     = ' ';
        Pck[ip].wt     = '9';
        for (i=0; i<3; i++) {
           Pck[ip].pamp[i] = (long) 0.;
        }
        Pck[ip].timeout  = 0;			/***/ //?
		
/* Proxies were not in list; zero-out all pick info; will be filled by TYPE_PROXIES
 ***************************************************************************/
		Pck[ip].tau0    = 0;
		Pck[ip].tauc    = .0;
		Pck[ip].pd      = .0;
        Pck[ip].tauc3s  = .0;
        Pck[ip].pd3s    = .0;
		Pck[ip].magpd   = .0;
		Pck[ip].magtauc = .0;
		Pck[ip].mag     = .0;
		Pck[ip].r       = .0;
		Pck[ip].R       = .0;
		Pck[ip].tres    = .0;

        return( 1 );
}


/****************************************************************************/
/*  eqp_proxies() processes a TYPE_PROXIES message                          */
/****************************************************************************/

int eqp_proxies( char *msg )
{
        char       str[60];
        long       lp1;
        long       lp2;
        long       lp;
        int        i, ip;
        int        tau0;
        double     tauc, pd;
        int        type, modid, instid;
		short      seq;
        char       site[6], net[3], comp[4];

/*----------------------------------------------------------------------------
Here's a sample TYPE_PROXIES message (55 characters long):
logo      id   scn        T0   Tc       Pd
112 78  0 6811 PFVI PMHHZ   3  2.313123 1.961118E-005 \n
0123456789 123456789 123456789 123456789 123456789 1234
0         10        20        30        40        50
------------------------------------------------------------------------------*/
/* Check the coda's length
 *************************/
        if( strlen(msg) < 55 ) 
        {
           sprintf( Text,
                   "eqp_proxies: Will not decode short TYPE_PROXIES: %s",
                    msg );
           eqp_status( TypeError, ERR_PROXIESREAD, Text );
           return( 0 );
        }

/* Copy incoming string to a local buffer so we can alter it.
 ************************************************************/
        strncpy (str, msg, 55);

/* Read PROXIES string (column oriented).
   Start at the end and work backwards.
 ***************************************/
        str[53] = '\0';   pd      = atof(str+40);
        str[39] = '\0';   tauc    = atof(str+29);
        str[29] = '\0';   tau0    = atoi(str+26);

        str[25] = '\0';   strcpy(comp, str+22);
        str[22] = '\0';   strcpy(net,  str+20);
        str[20] = '\0';   strcpy(site, str+15);

        str[14] = '\0';   seq     = (short) atoi(str+10);
        str[9]  = '\0';   instid  = atoi(str+6);
        str[6]  = '\0';   modid   = atoi(str+3);
        str[3]  = '\0';   type    = atoi(str+0);

        logit( "t","Read from proxies: %3d %3d %3d %4hd %5s%2s%3s %3d %9.6f %.6e\n",
            type,modid,instid,seq,site,net,comp,tau0,tauc,pd );   /*debug*/

/* Check for valid values of instid, modid, type
 ***********************************************/
        if( instid<0 || instid>255 ||
            modid<0  || modid>255  ||
            type<0   || type>255      )
        {
           sprintf( Text,
                   "eqp_proxies: Invalid msgtype, modid, or instid in: %s",
                    msg );
           eqp_status( TypeError, ERR_PROXIESREAD, Text );
           return( 0 );
        }

/* Try to find pick/coda part in existing pick list
 *********************************************/
        lp2 = nPck;
        lp1 = lp2 - MAXPCK;
        if(lp1 < 0)
                lp1 = 0;
        for( lp=lp2-1; lp>=lp1; lp-- )  /* loop from most recent to oldest */
        {
                ip = lp % MAXPCK;
                if( instid != Pck[ip].instid )  continue;
                if( modid  != Pck[ip].src )     continue;
                if( seq    != Pck[ip].id )      continue;


/* Try to check if tau0 includes S phase
Pick must be listed and linked in a location previouly processed 
 ***********************************************/
				if(		Pck[ip].quake!=0				// pick linked in a location
					&&	Pck[ip].R>0.					// hypocentral distance already calculated 
					&&	tau0 > (Pck[ip].R)/DIST_SPTIMEDIF	)		// estimated S phase arrives before the end of tau0
				{			
					logit( "t","Est S arrives %.2fs after detected P, proxies just received with t0 %ds will be rejected\n",
									(Pck[ip].R)/DIST_SPTIMEDIF,tau0);
					return (1);
				}

/* Proxies accepted, load the proxies info into the list
 ***********************************************/
				Pck[ip].tau0     = tau0;
				Pck[ip].tauc     = tauc;
				Pck[ip].pd       = pd;
				if(tau0 == 3){
					Pck[ip].tauc3s   = tauc;
					Pck[ip].pd3s     = pd;
				}
                Pck[ip].timeout  = 0; 
                Pck[ip].flag    |= FLAG_PROXIES; 
                return( 1 );
        }

/* Pick/coda was not in list; load coda info into list
 ************************************************/
        ip  = nPck % MAXPCK;
        Pck[ip].instid   = instid;
        Pck[ip].src      = modid;
        Pck[ip].id       = seq;
        Pck[ip].tau0     = tau0;
        Pck[ip].tauc     = tauc;
        Pck[ip].pd       = pd;
		if(tau0 == 3){
			Pck[ip].tauc3s   = tauc;
			Pck[ip].pd3s     = pd;
		}
        Pck[ip].magtauc  = .0;		
        Pck[ip].magpd    = .0;		
        Pck[ip].mag	     = .0;		
        Pck[ip].r        = .0;
        Pck[ip].R        = .0;
        Pck[ip].tres     = .0;
        Pck[ip].quake    = 0;
        Pck[ip].flag     = FLAG_PROXIES; 
        Pck[ip].timeout  = 0;				/***/ 
        nPck++;

/* Pick was not in list; zero-out all pick info; will be filled by TYPE_PICK
 ***************************************************************************/
        Pck[ip].t      = 0.0;
        Pck[ip].site   = 0;
        Pck[ip].phase  = 0;
        Pck[ip].fm     = ' ';
        Pck[ip].ie     = ' ';
        Pck[ip].wt     = '9';
        for (i=0; i<3; i++) {
           Pck[ip].pamp[i] = (long) 0.;
        }
/* Coda was not in list; zero-out all coda info;
   it will NOT be filled 
 ***********************************************/
        for (i=0; i<6; i++) {
                Pck[ip].caav[i] = 0;
        }
        Pck[ip].clen   = 0;
        Pck[ip].codawt = ' ';

        return( 1 );
}



/*********************************************************************************/
/*  eqp_compare() compare 2 times                                                */
/*********************************************************************************/
int eqp_compare( const void *p1, const void *p2 )
{
        SRT *srt1;
        SRT *srt2;

        srt1 = (SRT *) p1;
        srt2 = (SRT *) p2;
        if(srt1->t < srt2->t)   return -1;
        if(srt1->t > srt2->t)   return  1;
        return 0;
}


/******************************************************************************/
/* eqp_status() builds a heartbeat or error msg & puts it into shared memory  */
/******************************************************************************/
void eqp_status( unsigned char type, short ierr, char *note )
{
   char         msg[256];
   time_t       t;
   int          size;
   MSG_LOGO     logo;

/* Build the message
 *******************/
   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%I64d %d\n", t, MyPID );			// %ld
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%I64d %d %s\n", t, ierr, note);	// %ld
        logit( "et", "%s\n", note );
   }

	logo.type   = type;
	logo.mod    = MyModId;
	logo.instid = InstId;

	size = strlen( msg );   /* don't include the null byte in the message */
	if ( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
		logit( "et", "ewpublish: Error sending msg to output ring.\n" );

   return;
}

/******************************************************************************/
/* eqp_mag() estimates magnitude from TauC and Pd                             */
/******************************************************************************/
void eqp_mag( int iq )
{
	char msgHypArc[MAX_BYTES_PER_EQ];
	char *bufHypArc;
	MSG_LOGO     logo;
	int    size;
	//TPHASE treg[10];
	double xs, ys;
	double xq, yq, zq;
	double dtdr, dtdz;
	long lp, lp1, lp2;
	int  is, iph, ip, im, tau0;
	double pd200,pd,tauc;
	double magPdArray[1000];
	double magTauCArray[1000];
	double sumChanMagPd,magPdAve,magPdMed,
		   sumChanMagTauC,magTauCAve,magTauCMed,
		   stdev;
	int    nNumMagChans;
	char       cdate[25];
	struct Greg  g;
	int          tsec, thun;

	sumChanMagPd	= .0;
	magPdAve		= .0;
	magPdMed		= .0;
	sumChanMagTauC	= .0;
	magTauCAve		= .0;
	magTauCMed		= .0;
	stdev			= .0;
	nNumMagChans	= 0;
	bufHypArc		= msgHypArc;

	if( (iq<0 || iq>=MAXHYP) && Hyp[iq].flag != 1){
		logit( "et", "eqp_mag:  Error indexing an hypocenter.\n");
		return ;
	}

	if(		Hyp[iq].flag == 1 
		&&	/*Hyp[iq].rms > .0 && Hyp[iq].dmin > .0 && */ Hyp[iq].ravg > .0 && Hyp[iq].gap > 0.0 
		)
		logit( "t", "<Selected solution>\n" );

    xq = X(Hyp[iq].lon);
    yq = Y(Hyp[iq].lat);
    zq = Hyp[iq].z;
		
	/* proxies selection from picks really associated
	*******************************/
    lp2   = nPck;
    lp1   = lp2 - MAXPCK;
    if (lp1 < 0)
            lp1 = 0;
    for(lp=lp1; lp<lp2; lp++) {

		ip = lp % MAXPCK;
		if ( Pck[ip].quake  != Hyp[iq].id ) continue;


		/* Convert julian seconds to date and time
		***************************************/	
		datime( Pck[ip].t, &g );
		tsec = (int)floor( (double) g.second );
		thun = (int)((100.*(g.second - tsec)) + 0.5);
		if ( thun == 100 )
			tsec++, thun = 0;

		is = Pck[ip].site;
		iph = Pck[ip].phase;
		date17(Pck[ip].t, cdate);
		xs				= X(Site[is].lon);
		ys				= Y(Site[is].lat);
		Pck[ip].r		= hypot(xs - xq, ys - yq);
		Pck[ip].R		= sqrt( zq*zq + Pck[ip].r*Pck[ip].r );
		Pck[ip].tres	= Pck[ip].t - Hyp[iq].t - t_phase(iph, Pck[ip].r, zq, &dtdr, &dtdz);
		logit( "t", "Processing pick %4hd %5s%2s%3s %1c %1c %4d%02d%02d%02d%02d%02d.%02d r: %6.2f tres: %5.2f phase: %-2s\n",  
					Pck[ip].id,Site[is].name,Site[is].net,Site[is].comp,Pck[ip].fm,Pck[ip].wt,
					g.year,g.month, g.day, g.hour, g.minute, tsec, thun, Pck[ip].r, Pck[ip].tres, Phs[iph]    );	

		if ( Pck[ip].flag&FLAG_PROXIES ){

			/* Channel magnitudes estimation from Pd,TauC
			*******************************/

			// Conditions: minPd, depiOK,..
			
			// static char *Phs[] = {"P", "S", "Pn", "Sn", "Pg", "Sg"};
			if(iph==1 || iph==3 || iph==5 ){	/***/
				logit( "t", "Proxies rejected because pick is a %s-phase\n",Phs[iph]);
			}
			else if(Pck[ip].r>dDepiMax){
				logit( "t", "Proxies rejected because depi(%.1f) is too large (limit at %.1f)\n",Pck[ip].r,dDepiMax);
			}
			else{

				if(				   3 > (Pck[ip].R)/DIST_SPTIMEDIF
					&&	Pck[ip].tau0 > (Pck[ip].R)/DIST_SPTIMEDIF  ){
					logit( "t", "Est S arrives %.2fs after detected P, proxies with t0 3s and %ds will be rejected for this location\n",
							(Pck[ip].R)/DIST_SPTIMEDIF,Pck[ip].tau0);
				}
				else{

					if( Pck[ip].tau0 > (Pck[ip].R)/DIST_SPTIMEDIF  ){


						logit( "t", "Est S arrives %.2fs after detected P, proxies with t0 3s will be used instead of t0 %ds\n",
							(Pck[ip].R)/DIST_SPTIMEDIF,Pck[ip].tau0);
						tau0	= 3;
						pd		= Pck[ip].pd3s;
						tauc	= Pck[ip].tauc3s;
					}else{
						tau0	= Pck[ip].tau0;
						pd		= Pck[ip].pd;
						tauc	= Pck[ip].tauc;
					}

					if(pd<dPdMin){
						logit( "t", "Proxies rejected because Pd(%g) is too small (limit at %g)\n",pd,dPdMin);
					}
					else if( pd<=0 || tauc<=0 ){
						logit( "t", "Inappropiated values from Pd and/or tc\n");
					}
					else{
						/* Channel magnitudes estimation from Pd,TauC
							Pd|200			= Pd*10^(-1.5*log10(200/dhypo))=> a0 = 1.5
							log10 Pd|200	= 1.0  Mw_Pd - 8.4             => a1,a2 = 1.0,8.4  
							log10 TauC		= 0.25 Mw_Tc - 1.2             => b1,b2 = 0.25,1.2 
							//
							MagChan			= 0.5 Mw_Pd + 0.5 Mw_Tc		   => c1,c2 = 0.5,0.5  
						*******************************/

						pd200            = pd * pow(10,-a0*log10(200/Pck[ip].R));
						Pck[ip].magpd    = (a2+log10(pd200) )/a1;
						Pck[ip].magtauc  = (b2+log10(tauc) )/b1;
						// 
						Pck[ip].mag      = c1*Pck[ip].magpd + c2*Pck[ip].magtauc;
						//
						// 
						sumChanMagPd				+= Pck[ip].magpd;
						magPdArray[nNumMagChans]	= Pck[ip].magpd;
						sumChanMagTauC				+= Pck[ip].magtauc;
						magTauCArray[nNumMagChans]	= Pck[ip].magtauc;
						//
						nNumMagChans++;
						//
						logit( "t", "t0: %3d Pd: %.6e dhypo: %.1f Pd|200: %.6e tc: %9.6f Mag(Pd): %5.2f Mag(tc): %5.2f Mag: %5.2f \n",
									tau0,pd,Pck[ip].R,pd200,tauc,Pck[ip].magpd,Pck[ip].magtauc,Pck[ip].mag);
					}
				}
			}
		}else
			logit( "t", "Channel without proxies \n");

	}


/* Final magnitude and statistics: average and standard deviation
 *******************************/
	if(nNumMagChans>1){

		// Mag Pd 
		//
		magPdAve              = sumChanMagPd/nNumMagChans;
		//
		qsort(magPdArray, nNumMagChans, sizeof(double), CompareDoubles);
		if(nNumMagChans%2==0) // even 
			magPdMed = 0.5*(magPdArray[(nNumMagChans/2)-1]+magPdArray[nNumMagChans/2]);
		else // odd 
			magPdMed = magPdArray[nNumMagChans/2];
		//
		sumChanMagPd=0;
		for (im = 0; im < nNumMagChans; im++)
			sumChanMagPd += (magPdArray[im] - magPdAve) * (magPdArray[im] - magPdAve);  // està malament, he de tornar a triar! 
		stdev = sqrt(sumChanMagPd / (nNumMagChans - 1));
		//
		logit( "t", "Mag(Pd): %5.2f type:%1c (num: %3d avg: %5.2f stdev: %4.2f)\n",
					magPdMed,cMagType,nNumMagChans,magPdAve,stdev);

		// Mag TauC 
		//
		magTauCAve              = sumChanMagTauC/nNumMagChans;
		//
		qsort(magTauCArray, nNumMagChans, sizeof(double), CompareDoubles);
		if(nNumMagChans%2==0) // even
			magTauCMed = 0.5*(magTauCArray[(nNumMagChans/2)-1]+magTauCArray[nNumMagChans/2]);
		else // odd
			magTauCMed = magTauCArray[nNumMagChans/2];
		//
		sumChanMagTauC=0;
		for (im = 0; im < nNumMagChans; im++)
			sumChanMagTauC += (magTauCArray[im] - magTauCAve) * (magTauCArray[im] - magTauCAve);  // està malament, he de tornar a triar! 
		stdev = sqrt(sumChanMagTauC / (nNumMagChans - 1));
		//
		logit( "t", "Mag(tc): %5.2f type:%1c (num: %3d avg: %5.2f stdev: %4.2f)\n",
					magTauCMed,cMagType,nNumMagChans,magTauCAve,stdev);


		// Final Magnitude 
		//		Mag 	= 0.5 Mw_Pd + 0.5 Mw_Tc		   => c1,c2 = 0.5,0.5  
		//
		Hyp[iq].mag     = c1*magPdMed + c2*magTauCMed;
		Hyp[iq].magtype	= cMagType;   
		Hyp[iq].nummags = nNumMagChans;		
		//
		logit( "t", "Mag: %5.2f type:%1c\n\n",Hyp[iq].mag,Hyp[iq].magtype);


	}else if(nNumMagChans==1){

		// Mag Pd
		magPdAve 		= sumChanMagPd;
		magPdMed 		= sumChanMagPd;
		nNumMagChans	= 1;
		stdev			= 0;
		//
		logit( "t", "Mag(Pd): %5.2f type:%1c (num: %3d)\n",
					magPdMed,cMagType,nNumMagChans);

		// Mag TauC
		magTauCAve 		= sumChanMagTauC;
		magTauCMed 		= sumChanMagTauC;
		nNumMagChans	= 1;
		stdev			= 0;
		//
		logit( "t", "Mag(tc): %5.2f type:%1c (num: %3d)\n",
					magTauCMed,cMagType,nNumMagChans);

		// Final Magnitude 
		//		Mag 	= 0.5 Mw_Pd + 0.5 Mw_Tc		   => c1,c2 = 0.5,0.5  
		//
		Hyp[iq].mag     = c1*magPdMed + c2*magTauCMed;
		Hyp[iq].magtype	= cMagType;   
		Hyp[iq].nummags = nNumMagChans;		
		//
		logit( "t", "Mag: %5.2f type:%1c\n\n",Hyp[iq].mag,Hyp[iq].magtype);

	}else{

		// Mag Pd
		magPdAve 		= -99;
		magPdMed 		= -99;
		nNumMagChans	= 0;
		stdev			= 0;

		// Mag TauC
		magTauCAve 		= -99;
		magTauCMed 		= -99;
		nNumMagChans	= 0;
		stdev			= 0;

		// Final Magnitude 
		Hyp[iq].mag     = -99;
		Hyp[iq].magtype	= cMagType;   
		Hyp[iq].nummags = nNumMagChans;		
		//
		logit( "t", "No magnitude");
	}


	/* Messages building
	*******************************/
	eqc_bldhyp( bufHypArc, &Hyp[iq] );		
	bufHypArc = msgHypArc+strlen(msgHypArc);			
	//
    lp2   = nPck;
    lp1   = lp2 - MAXPCK;
    if (lp1 < 0) lp1 = 0;
    for(lp=lp1; lp<lp2; lp++) {
		ip = lp % MAXPCK;
		if ( Pck[ip].quake  != Hyp[iq].id ) continue;

		/* Hyp2000Arc: builds a hyp2000 (hypoinverse) archive phase card & its shadow */	
		eqc_bldphs( bufHypArc, &Pck[ip] );					
		bufHypArc = msgHypArc+strlen(msgHypArc);			
	}
/* Hyp2000Arc: builds a hypoinverse event terminator card & its shadow */
	eqc_bldterm( bufHypArc, &Hyp[iq] );
	bufHypArc = msgHypArc+strlen(msgHypArc);


/* Hyp2000Arc: Enviem el msg */
	logo.type   = TypeHyp2000Arc;
	logo.instid = InstId;
	logo.mod    = MyModId;
	if(		Hyp[iq].flag == 1 
		&&	/*Hyp[iq].rms > .0 && Hyp[iq].dmin > .0 && */ Hyp[iq].ravg > .0 && Hyp[iq].gap > 0.0 
		){
		Hyp[iq].flag = 2;

		size = strlen( msgHypArc );   /* don't include the null byte in the message */
		if ( tport_putmsg( &Region, &logo, size, msgHypArc ) != PUT_OK )
			logit( "et", "ewpublish: Error sending msg to output ring.\n" );

	}

	if( TestRingKey>0 )
	{
		size = strlen( msgHypArc );   /* don't include the null byte in the message */
		if ( tport_putmsg( &TestRegion, &logo, size, msgHypArc ) != PUT_OK )
			logit( "et", "ewpublish: Error sending msg to test ring.\n" );
	}


   return;
}



/***************************************************************************/
/* eqc_bldhyp() builds a hyp2000 (hypoinverse) summary card & its shadow   */
/***************************************************************************/
void eqc_bldhyp( char *hypcard, RPT_HYP *Hyp )
{
		char line[170];   /* temporary working place */
		char shdw[170];   /* temporary shadow card   */
		char cdate[25];
		char ns,ew;
		int  tmp;
		int  i;
		int  dlat,dlon;
		double mlat,mlon;


/* Put all blanks into line and shadow card
 ******************************************/
		for ( i=0; i<170; i++ )  line[i] = shdw[i] = ' ';
		ns=' ';
		ew='E';

/*----------------------------------------------------------------------------------
Sample HYP2000 (HYPOINVERSE) hypocenter and shadow card.  Binder's eventid is stored 
in cols 136-145.  Many fields will be blank due to lack of information from binder.
(summary is 165 chars, including newline; shadow is 95 chars, including newline):
199806262007418537 3557118 4836  624 00 26 94  2   633776  5119810  33400MOR  15    0  32  50 99
   0 999  0 11MAM WW D189XL426 80         51057145L426  80Z464  102 \n
$1                                                                                0399   0   0\n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 12345
6789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
-----------------------------------------------------------------------------------*/

/***  Convert date to character string  ***/
		date17( Hyp->t, cdate );

/***  Convert latitude into degrees and minutes, set ns flag properly ***/
        mlat = Hyp->lat;
        if ( mlat < 0. ) {
                ns   = 'S';
                mlat = -mlat;
        }
        dlat = (int) mlat;
        mlat = (mlat - (double) dlat) * 60.0;

/***  Convert longitude into degrees and minutes, set ew flag properly ***/
        mlon = Hyp->lon;
        if ( mlon < 0. ) {
                ew   = ' ';
                mlon = -mlon;
        }
        dlon = (int) mlon;
        mlon = (mlon - (double) dlon) * 60.0;

/* Write out hypoinverse summary card
 ************************************/
        strncpy( line,     cdate,     14 );
        strncpy( line+14,  cdate+15,  2  );
        sprintf( line+16,  "%2d%c", dlat, ns );
        tmp = (int) (mlat*100.0);
        sprintf( line+19,  "%4d",   (int) MIN( tmp, 9999 ) );
        sprintf( line+23,  "%3d%c", dlon, ew );
        tmp = (int) (mlon*100.0);
        sprintf( line+27,  "%4d", (int) MIN( tmp, 9999 ) );
		tmp = (int) (Hyp->z*100.0);
        sprintf( line+31,  "%5d", (int) MIN( tmp, 99999 ) );   line[36] = ' ';
		sprintf( line+39,  "%3d", (int) MIN( Hyp->nph, 999 ) );
		sprintf( line+42,  "%3d", (int) MIN( Hyp->gap, 999 ) );
		sprintf( line+45,  "%3d", (int) MIN( Hyp->dmin, 999 ) );
		tmp = (int) (Hyp->rms*100.);
		sprintf( line+48,  "%4d", (int) MIN( tmp, 9999 ) );    line[52]  = ' ';

		// EXTERNAL MAGNITUDE = PREFERRED MAGNITUDE
		if(Hyp->mag>-99){
			// external
			tmp = (int) (Hyp->mag*100.);
			sprintf( line+122,  "%1c%3d", Hyp->magtype, (int) MIN( tmp, 999 ) );    line[126]  = ' ';
			tmp = (int) (Hyp->nummags*10.);
			sprintf( line+126,  "%3d", (int) MIN( tmp, 999 ) );    line[129] = ' ';
		}

		sprintf( line+136, "%10ld", Hyp->id );                 line[146] = ' ';

		// EXTERNAL MAGNITUDE = PREFERRED MAGNITUDE
		if(Hyp->mag>-99){
			// preferred magnitude
			tmp = (int) (Hyp->mag*100.);
			sprintf( line+146,  "%1c%3d", Hyp->magtype, (int) MIN( tmp, 999 ) );    line[150]  = ' ';
			tmp = (int) (Hyp->nummags*10.);
			sprintf( line+150,  "%4d", (int) MIN( tmp, 9999 ) );    line[154] = ' ';
		}

		/*if(LabelVersion) line[162] = Hyp.version;		else*/ /***/
		line[162] = ' ';
		sprintf( line+164, "\n\0" );

/* Write out summary shadow card
 *******************************/
        sprintf( shdw,     "$1"   );
        shdw[2]= ' ';
        sprintf( shdw+94,  "\n\0" );

/* Copy both to the target address
 *********************************/
		/*logit( "", "%s", line );        /*DEBUG*/
        strcpy( hypcard, line );
        strcat( hypcard, shdw );
        return;
}


/*******************************************************************************/
/* eqc_bldphs() builds a hyp2000 (hypoinverse) archive phase card & its shadow */
/*******************************************************************************/
void eqc_bldphs( char *phscard, RPT_PCK *Pck )
{
   char line[120];     /* temporary phase card    */
   char shdw[120];     /* temporary shadow card   */
   //float qfix, qfree;
   //float afix, afree;
   int   i;
   char   cdate[25];
   int    is, iph;
   char   datasrc;
   int    tmp;

/* Put all blanks into line and shadow card
 ******************************************/
   for ( i=0; i<120; i++ )  line[i] = shdw[i] = ' ';

/*-----------------------------------------------------------------------------
Sample Hyp2000 (hypoinverse) station archive cards (for a P-arrival and 
an S-arrival) and a sample shadow card. Many fields are blank due to lack 
of information from binder. Station phase card is 112 chars, including newline; 
shadow is 104 chars, including newline:
MTC  NC  VLZ  PD0199806262007 4507                                                0      79                 W  \n
MTC  NC  VLZ    4199806262007 9999        5523 S 1                                4      79                 W  \n
$   6 5.27 1.80 4.56 1.42 0.06 PSN0   79 PHP2 1197 39 198 47 150 55 137 63 100 71  89 79  48           \n
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 1
-------------------------------------------------------------------------------*/


        is  = Pck->site;
        iph = Pck->phase;
		date17( Pck->t, cdate );
/***  Get a character to denote the data source ***/
        if   ( Pck->instid == InstId ) datasrc = 'W'; /* local worm */
        else                           datasrc = 'I'; /* imported   */

/* Build station archive card for a P-phase
static char *Phs[] = {"P", "S", "Pn", "Sn", "Pg", "Sg"};
 ******************************************/
   if( Pck->phase%2 == 0 )     /* It's a P-phase... */
   {
        strncpy( line,    Site[is].name, 5 );
        strncpy( line+5,  Site[is].net,  2 );   line[7]  = ' ';
        strncpy( line+9,  Site[is].comp, 3 );   line[12] = ' ';
        /*if( LabelAsBinder ) strncpy( line+13, Phs[iph], 2 );		/***/
        /*else*/
		strncpy( line+13, " P",      2 );
        line[15] = Pck->fm;
        line[16] = Pck->wt;
        strncpy( line+17, cdate,   12 );  line[29] = ' '; /* yyyymmddhhmm    */
        strncpy( line+30, cdate+12, 2 );                  /* whole secs      */
        strncpy( line+32, cdate+15, 2 );  line[34] = ' '; /* fractional secs */
		tmp = (int) (Pck->tres*100.0);
		if(Pck->tres>0)	sprintf( line+34, "%4d", (int) MIN( tmp, 9999 ) );         
		else			sprintf( line+34, "%4d", (int) MAX( tmp, -999 ) );         line[38] = ' ';
		tmp = (int) (Pck->r*10.0);
		sprintf( line+74, "%4d", (int) MIN( tmp, 9999 )    );         line[78] = ' ';
        //if( Calc.clenx > 9999 ) Calc.cwtx = 4;      /* weight out duration which */
        //                                            /* overflows format          */
        //sprintf( line+82, "%1d", (int) MIN( Calc.cwtx,  9 )    );   line[83] = ' ';
        //sprintf( line+87, "%4d", (int) MIN( Calc.clenx, 9999 ) );   line[91] = ' ';
        line[108] = datasrc;
        sprintf( line+111, "\n\0" );
   }

/* Build station archive card for a S-phase
 ******************************************/
   else                            /* It's an S-phase... */
   {
   /* Force the coda-length to be weighted-out since it was measured */
   /* from the S-arrival time instead of the P-arrival time          */
        /*Calc.cwtx = 4;*/
   /* Build the phase card */
        strncpy( line,    Site[is].name, 5 );
        strncpy( line+5,  Site[is].net,  2 );   line[7]  = ' ';
        strncpy( line+9,  Site[is].comp, 3 );   line[12] = ' ';
        line[16] = '4';  /* weight out P-arrival; we're loading a dummy time */
        strncpy( line+17, cdate, 12 );    /* real year,mon,day,hr,min    */
        sprintf( line+29,  " 9999"      );    /* dummy seconds for P-arrival */
        line[34] = ' ';
        strncpy( line+42, cdate+12, 2 );  /* actual whole secs S-arrival */
        strncpy( line+44, cdate+15, 2 );  /* actual fractional secs S    */
        /*if( LabelAsBinder ) strncpy( line+46, Phs[iph],  2 ); */
        /*else*/
		strncpy( line+46, " S",      2 );                             line[48] = ' ';
		line[49] = Pck->wt;
		tmp = (int) (Pck->tres*100.0);
		if(Pck->tres>0)	sprintf( line+50, "%4d", (int) MIN( tmp, 9999 ) );         
		else			sprintf( line+50, "%4d", (int) MAX( tmp, -999 ) );         line[54] = ' ';
		tmp = (int) (Pck->r*10.0);
		sprintf( line+74, "%4d", (int) MIN( tmp, 9999 )    );         line[78] = ' ';
        //if( Calc.clenx > 9999 ) Calc.cwtx = 4;  /* weight out duration which  */
        //                                       /* overflows format           */
        //sprintf( line+82, "%1d", (int) MIN( Calc.cwtx,  9 )    );   line[83] = ' ';
        //sprintf( line+87, "%4d", (int) MIN( Calc.clenx, 9999 ) );   line[91] = ' ';
        line[108] = datasrc;
        sprintf( line+111, "\n\0" );
   }

/* Build the shadow card
 ***********************/
   ///* To follow CUSP convention, the slope of a "normal" coda decay    */
   ///* should be positive; so print the inverse of the calculated slope */
   //qfix  = (float) (-1.0 * Calc.qfix);
   //qfree = (float) (-1.0 * Calc.qfree);

   ///* Protect against print-format over-flow from negative numbers */
   //qfix  = (float) MAX( qfix,       -9.99 );
   //qfree = (float) MAX( qfree,      -9.99 );
   //afix  = (float) MAX( Calc.afix,  -9.99 );
   //afree = (float) MAX( Calc.afree, -9.99 );
   //if( Calc.rms < 0. ) Calc.rms *= -1.;

   /* Now really build the shadow card */
   sprintf( shdw,    "$");   //sprintf( shdw,    "$ %3d",  (int) Calc.naav    );
   //sprintf( shdw+5,  "%5.2f",  MIN( afix,  99.99 )        );
   //sprintf( shdw+10, "%5.2f",  MIN( qfix,  99.99 )        );
   //sprintf( shdw+15, "%5.2f",  MIN( afree, 99.99 )        );
   //sprintf( shdw+20, "%5.2f",  MIN( qfree, 99.99 )        );
   //sprintf( shdw+25, "%5.2f ", MIN( Calc.rms, 99.99 )     );
   //strncpy( shdw+31,  Calc.cdesc, 3 );
   //sprintf( shdw+34, "%1d",    (int) MIN( Calc.cwtx, 9 )    );


   /*if(Pck->clen>0 && Pck->clen<99999)
		sprintf( shdw+35, "%5d ",   Pck->clen                   );
   sprintf( shdw+41, "PHP");*/



   //sprintf( shdw+41, "PHP%1d", MIN( Calc.pampwt, 9 )        );
   //sprintf( shdw+45, "%5ld",   MIN( Calc.avgpamp, 99999 ) );

   //offset = 50; 
   //nblank = 0; 
   //for ( i=5; i>=0; i-- ) {  /* add coda aav's in increasing time order */
   //   //if( Pck.ccntr[i] == 0. ) {
   //   //    nblank++;
   //   //    continue;
   //   //}
   //   if(Pck->caav[i]>0)
		 // sprintf( shdw+offset, "   %4d", 
			//	  /*(int) Pck.ccntr[i],*/ (int) MIN( Pck->caav[i], 9999 ) );
   //   offset += 7;
   //}

   //if( nblank ) {      /* put unused (blank) coda aav's at end of line */
   //   for( i=0; i<nblank; i++ ) {
   //      sprintf( shdw+offset, "%3d%4d", 
   //              (int) Pck.ccntr[5-i], (int) MIN( Pck.caav[5-i], 9999 ) );
   //      offset += 7;
   //   }
   //}shdw[92]=' ';
   shdw[1]=' '; // elimino el \0
   sprintf( shdw+103, "\n\0" );

	/*logit( "", "%s", line );        /*DEBUG*/


/* Copy both to the target address
 *********************************/
   strcpy( phscard, line );
   strcat( phscard, shdw );

   return;
}



/***************************************************************************/
/* eqc_bldterm() builds a hypoinverse event terminator card & its shadow   */
/***************************************************************************/
void eqc_bldterm( char *termcard, RPT_HYP *Hyp )
{
    char line[100];   /* temporary working place */
    char shdw[100];   /* temporary shadow card   */
    int  i;

/* Put all blanks into line and shadow card
 ******************************************/
    for ( i=0; i<100; i++ )  line[i] = shdw[i] = ' ';

/* Build terminator
 ******************/
    sprintf( line+62, "%10ld\n\0",  Hyp->id );

/* Build shadow card
 *******************/
    sprintf( shdw, "$" );
    shdw[1] = ' ';
    sprintf( shdw+62, "%10ld\n\0",  Hyp->id );

/* Copy both to the target address
 *********************************/
    strcpy( termcard, line );
    strcat( termcard, shdw );

    return;
}

/*************************************************************
 *                   CompareDoubles()                        *
 *                                                           *
 *  This function can be passed to qsort() and bsearch().    *
 *  Compare two doubles for ordering in increasing order     *
 *************************************************************/
int CompareDoubles( const void *s1, const void *s2 )
{
  double *d1 = (double *)s1;
  double *d2 = (double *)s2;

  if (*d1 < *d2)
     return -1;
  else if (*d1 > *d2)
    return 1;
  
  return 0;
}




