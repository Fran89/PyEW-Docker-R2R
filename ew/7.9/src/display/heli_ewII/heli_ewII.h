
/* include file for heli_ewII.
  
	2000/05/13 19:01:20  lombard
 *     Fixed tracebuf length. This bug prevented aquisition of 200 SPS data
 *     at update intervals of MAXMINUTES.
 */


/**************************************************************************
 *  defines                                                               *
 **************************************************************************/

#define MAXSAMPRATE_DEFAULT      500  /* Maximum # of samples/sec.                */
#define MAXMINUTES         5  /* Maximum # of minutes of trace.           */
//#define MAXTRACELTH MAXMINUTES*60*MAXSAMPRATE_DEFAULT /* Max. data trace length   */
//#define MAXTRACEBUF MAXTRACELTH*13  /* This should work for 32-bit data   */
//     /* `12' should be enough (above) but doesn't leave anything to spare */
#define MAXCOLORS         20  /* Number of colors defined                 */
#define MAXPLOTS         200  /* Maximum number of SCNs to plot           */
#define MAX_WAVESERVERS   20  /* Maximum number of Waveservers 
								 Changed from 20. Alex 11/20/00           */
#define MAX_ADRLEN        20  /* Size of waveserver address arrays        */
#define MAX_TARGETS        5  /* largest number of targets                */
#define NAMELEN           50  /* Length of some comment strings           */
#undef MAX_PATH
#define MAX_PATH         256  /* Size of full directory paths             */
#define GDIRSZ           132  /* Size of string for GIF target directory  */
#define MAXLOGO            2  /* Maximum number of Logos                  */
#define SLEEP_SEC          5  /* Seconds at a time that we sleep		  */

/**************************************************************************
 *  Define the structure for time records.                                *
 **************************************************************************/

typedef struct TStrct {   
    double  Time1600; /* Time (Sec since 1600/01/01 00:00:00.00)          */
    double  Time;     /* Time (Sec since 1970/01/01 00:00:00.00)          */
    int     Year;     /* Year                                             */
    int     Month;    /* Month                                            */
    int     Day;      /* Day                                              */
    int     Hour;     /* Hour                                             */
    int     Min;      /* Minute                                           */
    double  Sec;      /* Second                                           */
} TStrct;

/**************************************************************************
 *  Define the structure for the Plotting parameters.                     *
 **************************************************************************/
#define WHITE  0
#define BLACK  1
#define RED    2
#define BLUE   3
#define GREEN  4
#define GREY   5
#define YELLOW 6
#define TURQ   7
#define PURPLE 8

struct PltPar {
    char    Site[6];           /* Site                                    */
    char    Comp[4];           /* Component                               */
    char    Net[4];            /* Net                                     */
    char    Loc[3];            /* Location Code                           */
    char    SCNL[17];           /* SCNL                                   */
    char    SCNLtxt[20];        /* S C N L                                */
    char    SCNLnam[20];        /* S_C_N_L                                */
    char    Comment[NAMELEN];  /* Description (for web page)              */
    double  actDuration;       /* Actual Seconds acquired                 */
    double  LastTime;          /* Last time plotted                       */
    double  actStime;          /* Actual start time                       */
    float   samp_sec;          /* samples/sec                             */
    double  xsize, ysize;      /* Overall Size of plot (inches)           */
    int     xpix, ypix;        /* Size of data plot (pixels)              */
    int     xgpix, ygpix;      /* Overall Size of plot (pixels)           */
    double  axexmax;           /* max axe x position [Data] (in)          */
    double  axeymax;           /* max axe y position [Data] (in)          */
    int     mins;              /* # of minutes per display line           */
    int     LinesPerHour;      /* # of Lines per hour                     */
    int     HoursPerPlot;      /* # of hours per image                    */
    int     CurrentHour;       /* start hour (UTC/local) for this image   */
    int     CurrentDay;        /* start day  (UTC/local) for this image   */
    int     LocalTime;         /* Offset of local time from GMT e.g. -7 = PST */
    int     LocalSecs;         /* Offset of local time in seconds         */
    char    LocalTimeID[4];    /* Local time ID e.g. PST                  */
    char    Today[12];         /* Day of current plot. DDMMYYYY           */
    int     UseLocal;          /* Reference frames to local time          */
    int     ShowUTC;           /* Show UTC on right margin                */
    int     Npts;              /* Number of points in trace               */
    int     OldData;           /* Old data plot flag                      */
    int     DCremoved;         /* Attempt to make data zero mean          */
    double  Mean;              /* Mean value of the last data gulp        */
    double  DCcorr;            /* Current DC correction                   */
    int     Pallette;          /* Pallette flag                           */
    int     PixPerLine;        /* # of Pixels per display line            */
    int     secsPerStep;       /* # of seconds per processing step        */
    int     secsPerGulp;       /* # of seconds per acquisition            */
    int     first_gulp;        /* flag for first acquisition              */
    
    double  Gain;              /* Gain factor [Data]                      */
    double  Scale;             /* Scale factor [Data] (in)                */

    int     nentries;          /* Number of menu entries for this SCN     */
    double  TStime[MAX_WAVESERVERS*2]; /* Tank start for this entry       */
    double  TEtime[MAX_WAVESERVERS*2]; /* Tank end for this entry         */
    int     index[MAX_WAVESERVERS*2];  /* WaveServer for this entry       */
    
    long    gcolor[MAXCOLORS]; /* GIF colors                              */
    char    GifName[75];       /* Name of this gif file on host webserver */
    char    LocalGif[MAX_PATH];  /* Name of this gif file on local machine  */
    gdImagePtr    GifImage;
};
typedef struct PltPar PltPar;


/**************************************************************************
 *  Define the structure for the individual Butler thread.                *
 *  This is the private area the thread needs to keep track               *
 *  of all those variables unique to itself.                              *
 **************************************************************************/

struct Butler {
    int     Debug;             /*                                           */
    int     WSDebug;           /*                                           */
    int     UpdateInt;         /* Interval (secs) between updates           */
    PltPar  plt[MAXPLOTS];     /* plotting parameters                       */
    int     nPlots;            /* number of plots actually needed           */
    int     Current_Plot;      /* number of plot being worked on            */
    int     got_a_menu;
    int     Clip;              /* number of divisions to clip trace; 0 for
				* no clipping */
    size_t  maxSampRate;
    int     Days2Save;
    int     SaveDrifts;
    int     BuildOnRestart;    /* Build totally new images on restart       */
    int     Make_HTML;         /* Set to 1 to construct and ship index.html file */
    char    *TraceBuf; /* This should work for 24-bit digitizers should be MAXTRACELTH*10 size */
    char    mod[20];
    
    int     ntargets;          /* Number of target directories              */
    char    target[MAX_TARGETS][3*MAX_PATH];  /* Target in form-> UserId@IPname:/directory/    */
    char    UserID[MAX_TARGETS][MAX_PATH];    /* Target in form-> UserId         */
    char    Host[MAX_TARGETS][MAX_PATH];      /* Target in form-> IPname         */
    char    Directory[MAX_TARGETS][MAX_PATH]; /* Target in form-> /directory/    */
    char    GifDir[GDIRSZ];    /* Directory for storage of .gif & .html on local machine */
    char    IndexFile[NAMELEN];     /* Name of the index HTML file             */
    int     logo;              /* =1 if logo                                */
    int     logox, logoy;      /* Dimensions of logo                        */
    char    logoname[GDIRSZ+NAMELEN]; /* Name of the logo GIF                    */
    pid_t   pid;
    int     status;
    WS_MENU_QUEUE_REC menu_queue[MAX_WAVESERVERS];
/* Globals to set from configuration file
 ****************************************/
    long    wsTimeout;       /* seconds to wait for reply from ws           */
    int     nServer;         /* number of wave servers we know about        */
    long    RetryCount;      /* Retry count for waveserver errors.          */
                             /* list of available waveServers, from config. file  */
    int     inmenu[MAX_WAVESERVERS];
    char    wsIp[MAX_WAVESERVERS][MAX_ADRLEN];
    char    wsPort[MAX_WAVESERVERS][MAX_ADRLEN];
    char    wsComment[MAX_WAVESERVERS][NAMELEN];

    int     NoMenuCount;    /* Number of consecutive Nomenu errors         */
    int     NoGIFCount;     /* Number of consecutive GIF write errors      */
};
typedef struct Butler Butler;

