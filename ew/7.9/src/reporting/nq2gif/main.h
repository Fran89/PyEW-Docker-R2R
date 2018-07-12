#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>

#include "earthworm.h"
#include "qlib2.h"
 
#include "msdatatypes.h"
/*
*/
#include <kom.h>
#include <transport.h>
#include <chron3.h>

#include "seedstrc.h"

#include "trace_buf.h"			/* earthworm TRACE message definition */

#include "externs.h"

#include "gd.h"
#include "gdfontt.h"   /*  6pt      */
#include "gdfonts.h"   /*  7pt      */
#include "gdfontmb.h"  /*  9pt Bold */
#include "gdfontl.h"   /* 13pt      */
#include "gdfontg.h"   /* 10pt Bold */


/* the defines below map into the q2ew.desc error file */
#define Q2EW_DEATH_SIG_TRAP   2
#define Q2EW_DEATH_EW_PUTMSG  3
#define Q2EW_DEATH_EW_TERM    4
#define Q2EW_DEATH_EW_CONFIG  5

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*****************************************************************************
 *  defines                                                                  *
 *****************************************************************************/

#define MAXCHANNELS     1600  /* Maximum number of channels                  */
#define MAXCHAN           20  /* Maximum number of channels per file         */
#define MAX_TARGETS       10  /* Maximum number of targets                   */
#define MAX_STADBS        20  /* Maximum number of Station database files    */
#define STALIST_SIZ      100  /* Size of string for station list file        */

#define MAXCOLORS         20  /* Number of colors defined                    */
#define GDIRSZ           132  /* Size of string for GIF target directory     */

/*****************************************************************************
 *  Define the structure for time records.                                   *
 *****************************************************************************/

typedef struct TStrct {   
    double  Time1600; /* Time (Sec since 1600/01/01 00:00:00.00)             */
    double  Time;     /* Time (Sec since 1970/01/01 00:00:00.00)             */
    int     Year;     /* Year                                                */
    int     Month;    /* Month                                               */
    int     Day;      /* Day                                                 */
    int     Hour;     /* Hour                                                */
    int     Min;      /* Minute                                              */
    double  Sec;      /* Second                                              */
} TStrct;

/*****************************************************************************
 *  Define the structure for Page location information.                      *
 *                                                                           *
 *    x1    x2                  x3     x4                                    *
 *  y1+-----+-------------------+------+                                     *
 *    |     |                   |      |                                     *
 *  y2+-----+-------------------+------+                                     *
 *    |     |                   |      |                                     *
 *  y3+-----+-------------------+------+                                     *
 *    |     |                   |      |                                     *
 *  y4+-----+-------------------+------+                                     *
 *                                                                           *
 *****************************************************************************/
typedef struct LocInfo {      /* A location information structure            */
    gdImagePtr    Gif;
    int     x1, x2, x3, x4;
    int     y1, y2, y3, y4, ycen;
} LocInfo;

/*****************************************************************************
 *  Define the structure for Station information.                            *
 *  This is an abbreviated structure.                                        *
 *****************************************************************************/

typedef struct Instance {      /* A channel information structure            */
    char    Site[6];           /* Site                                       */
    char    Net[5];            /* Net                                        */
    char    Comp[5];           /* Component                                  */
    char    Loc[5];            /* Loc                                        */

    char    SNCLnam[20];       /* S_N_C_L                                    */
    int     meta;              /* Index of entry in metadata list            */

    double  mintime;           /* Minimum time for this channel              */
    double  maxtime;           /* Maximum time for this channel              */
    int     minval;            /* Minimum value for this channel             */
    int     maxval;            /* Maximum value for this channel             */
    int     npts;              /* Total number of points for this channel    */
    int     mpts;              /* Total number of points for this block      */
    
    double  sigma;             /* Summation of all points                    */
    double  mean;              /* Mean of all points                         */
    double  samprate;          /* Sample rate for this channel               */
    
    LocInfo page;              /* The bounding params for this chan          */
    double  xsf, ysf;          /* x/y scaling of data                        */
    
    double  Inst_gain;         /* Gain of instrument (microv/count)          */
    double  Sens_gain;         /* Gain of sensor (volts/unit)                */
    int     Sens_unit;         /* Sensor units d=1; v=2; a=3                 */
    double  GainFudge;         /* Additional gain factor.                    */
    double  SiteCorr;          /* Site correction factor.                    */
    double  sensitivity;       /* Channel sensitivity  counts/units          */
    int		ShkQual;           /* Station (Chan) type                        */
    
    int		range;             /* Range of digital counts                    */
    double  stddev;            /* Std Dev of digital counts                  */
    double  ratio;             /* Ratio of the range/stddev                  */
} Instance;

/*****************************************************************************
 *  Define the structure for Station information.                            *
 *  This is an abbreviated structure.                                        *
 *****************************************************************************/

typedef struct StaInfo {       /* A channel information structure            */
    char    Site[6];           /* Site                                       */
    char    Net[5];            /* Net                                        */
    char    Loc[5];            /* Loc                                        */
    char    StaNet[15];        /* S_N                                        */
    char    SiteName[50];      /* Common Name of Site                        */
    int     Nchan;             /* Number of data streams for this site       */

    double  mintime;           /* Minimum time for all channels              */
    double  maxtime;           /* Maximum time for all channels              */
    int     minval;            /* Minimum value for all channels             */
    int     maxval;            /* Maximum value for all channels             */
    int		range;             /* Maximum Range of digital counts            */
    int		maxoff;            /* Maximum Offset of digital counts           */
    
    Instance SNCL[MAXCHAN];    /* Data stream data                           */
} StaInfo;

/*****************************************************************************
 *  Define the structure for Channel information.                            *
 *  This is metadata read from the xxx.db1 file.                             *
 *  This is an abbreviated structure.                                        *
 *****************************************************************************/

typedef struct ChanInfo {      /* A channel information structure            */
    char    Site[6];           /* Site                                       */
    char    Comp[5];           /* Component                                  */
    char    Net[5];            /* Net                                        */
    char    Loc[5];            /* Loc                                        */
    char    SCN[15];           /* SCN                                        */
    char    SCNtxt[17];        /* S C N                                      */
    char    SCNnam[17];        /* S_C_N                                      */
    char    StaNet[15];        /* S_N                                        */
    char    SiteName[50];      /* Common Name of Site                        */
    char    Descript[200];     /* Common Name of Site                        */
    double  Lat;               /* Latitude                                   */
    double  Lon;               /* Longitude                                  */
    double  Elev;              /* Elevation                                  */
    
    int     Inst_type;         /* Type of instrument                         */
    double  Inst_gain;         /* Gain of instrument (microv/count)          */
    int     Sens_type;         /* Type of sensor                             */
    double  Sens_gain;         /* Gain of sensor (volts/unit)                */
    int     Sens_unit;         /* Sensor units d=1; v=2; a=3                 */
    double  GainFudge;         /* Additional gain factor.                    */
    double  SiteCorr;          /* Site correction factor.                    */
    double  sensitivity;       /* Channel sensitivity  counts/units          */
    int		ShkQual;           /* Station (Chan) type                        */
    
    double  Scale;             /* Scale factor [Data] (in)                   */
    double  Scaler;            /* Scale factor [Data] (in)                   */
} ChanInfo;

/*****************************************************************************
 *  Define the structure for the Plotting parameters.                        *
 *****************************************************************************/
#define WHITE  0
#define BLACK  1
#define RED    2
#define BLUE   3
#define GREEN  4
#define GREY   5
#define YELLOW 6
#define TURQ   7
#define PURPLE 8

/*****************************************************************************
 *  Define the structure for the individual Global thread.                   *
 *  This is the private area the thread needs to keep track                  *
 *  of all those variables unique to itself.                                 *
 *****************************************************************************/

struct Global {
    char    mod[20];           /* Program name                               */
    int     Debug;             /*                                            */
    int     Clip;              /* number of divisions to clip trace;         *
				                * 0 for no clipping                          */
    
    int     NSCN;              /* Number of SCNLs whose metadata know about  */
    ChanInfo    Chan[MAXCHANNELS];
    
    int     Nsta;
    StaInfo zzSta[MAXCHANNELS]; /* Info about the current station             */
    StaInfo Sta;               /* Info about the current station             */

    int     nltargets;         /* Number of local target directories         */
    char    loctarget[MAX_TARGETS][100];/* Target in form-> /directory/      */
    
/* Variables used during the generation of each plot
 ***************************************************/
    char    GifDir[GDIRSZ];    /* Directory for .gif & .html on local machine */
    int     logo;              /* =1 if logo                                 */
    int     logox, logoy;      /* Dimensions of logo                         */
    char    logoname[GDIRSZ+50]; /* Name of the logo GIF                     */
    int     nStaDB;            /* number of station databases we know about  */
    char    stationList[MAX_STADBS][STALIST_SIZ];
    
    char    GifName[175];      /* Name of this gif file on host webserver    */
    char    TmpName[175];      /* Name of this gif file on host webserver    */
    char    LocalGif[175];     /* Name of this gif file on local machine     */
    long    gcolor[MAXCOLORS]; /* GIF colors                                 */
    gdImagePtr    GifImage;
    char    FileType[10];      /* TRG, USR, CAL, etc.                        */ 
    
    char    indir[GDIRSZ];     /* Directory for input arc files              */
    int     UseDST;            /* Daylight Savings Time used when needed     */
    int     xpix, ypix;        /* Size of data plot (pixels)                 */
    double  xsize, ysize;      /* Overall Size of plot (inches)              */
    int     mins;              /* # of minutes per display line              */
    double  axexmax;           /* max axe x position [Data] (in)             */
    double  axeymax;           /* max axe y position [Data] (in)             */
    int     TotMins;           /* Total minutes to plot                      */
    int     SecsPerPlot;       /* # of secondss per image                    */
    int     DeciFlag;          /* Data decimation flag                       */

    double  Scale;             /* Scale factor [Data] (in)                   */
    double  Scaler;            /* Scale factor [Data] (in)                   */
    
    double  T0, Tn;            /* Min/Max time for axes                      */
    TStrct  Stime;             /* Nominal start time for file name           */
    double  xsf, ysf;          /* x/y scaling of data                        */
    int     stype;             /* Type. 0-Uniform ysf;1-Chan-based ysf       */
    
    double  flo, fhi;          /* Filter corners                             */
    int     ftype;             /* Type. 0-none;1-lopass;2-hipass;3-bandpass  */

};
typedef struct Global Global;
