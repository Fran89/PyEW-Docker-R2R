
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sgram.h 4851 2012-06-11 20:33:02Z luetgert $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2005/01/28 20:48:33  luetgert
 *     Now location code compatible.
 *     Remote-copy eliminated.
 *     Uses binary wsclient routines.
 *     .
 *
 *     Revision 1.1  2000/02/14 19:17:10  lucky
 *     Initial revision
 *
 *
 */

/* Include file for sgram */
/*****************************************************************************
 *  defines                                                                  *
 *****************************************************************************/

#define MAXCHANNELS     2400  /* Maximum number of channels                  */
#define MAXSAMPRATE      550  /* Maximum # of samples/sec.                   */
#define MAXMINUTES         5  /* Maximum # of minutes of trace.              */
#define MAXTRACELTH MAXMINUTES*60*MAXSAMPRATE /* Max. data trace length      */
#define MAXTRACEBUF MAXTRACELTH*10  /* This should work for 24-bit data      */

#define WSTIMEOUT          5  /* Number of seconds 'til waveserver times out */

#define MAXCOLORS        256  /* Number of colors defined                    */
#define PALCOLORS        200  /* Number of colors in pallette                */
#define MAXXPIX         1024  /* Max width of plot in pixels                 */

#define MAXPLOTS         100  /* Maximum number of SCNs to plot              */
#define MAX_STADBS        20  /* Maximum number of Station database files    */
#define MAX_WAVESERVERS   60  /* Maximum number of Waveservers               */
#define MAX_ADRLEN        20  /* Size of waveserver address arrays           */
#define MAX_TARGETS       10  /* Maximum number of targets                   */

#define PRFXSZ            20  /* Size of string for GIF file prefix          */
#define GDIRSZ           132  /* Size of string for GIF target directory     */
#define STALIST_SIZ      100  /* Size of string for station list file        */
#define MAXLOGO            2  /* Maximum number of Logos                     */

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
 *  Define the structure for specifying gaps in the data.                    *
 * Note: if a gap would be declared at end of data, the data must be         *
 * truncated instead of adding another GAP structure. A gap may be           *
 * declared at the start of the data, however.                               *
 *****************************************************************************/

typedef struct _GAP *PGAP;
typedef struct _GAP {
  double starttime;  /* time of first sample in the gap                      */
  double gapLen;     /* time from first gap sample to first sample after gap */
  long firstSamp;    /* index of first gap sample in data buffer             */
  long lastSamp;     /* index of last gap sample in data buffer              */
  PGAP next;         /* The next gap structure in the list                   */
} GAP;

/*****************************************************************************
 *  Define the structure for keeping track of buffer of trace data.          *
 *****************************************************************************/

typedef struct _DATABUF {
  double rawData[MAXTRACELTH*5];   /* The raw trace data; native byte order  */
  double delta;      /* The nominal time between sample points               */
  double starttime;  /* time of first sample in raw data buffer              */
  double endtime;    /* time of last sample in raw data buffer               */
  long nRaw;         /* number of samples in raw data buffer, including gaps */
  long lenRaw;       /* length to the rawData array                          */
  GAP *gapList;      /* linked list of gaps in raw data                      */
  int nGaps;         /* number of gaps found in raw data                     */
} DATABUF;

/*****************************************************************************
 *  Define the structure for Channel information.                            *
 *  This is an abbreviated structure.                                        *
 *****************************************************************************/

typedef struct ChanInfo {      /* A channel information structure            */
    char    Site[6];           /* Site                                       */
    char    Comp[5];           /* Component                                  */
    char    Net[5];            /* Net                                        */
    char    Loc[5];            /* Loc                                        */
    char    SCN[20];           /* SCN                                        */
    char    SCNtxt[20];        /* S C N                                      */
    char    SCNnam[20];        /* S_C_N                                      */
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
} ChanInfo;

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
    int     y1, y2, y3, y4;
} LocInfo;

/*****************************************************************************
 *  Define the structure for the Plotting parameters.                        *
 *****************************************************************************/
#define WHITE  240
#define BLACK  241
#define RED    242
#define BLUE   243
#define GREEN  244
#define GREY   245
#define YELLOW 246
#define TURQ   247
#define PURPLE 248

struct PltPar {
    char    Site[6];           /*  0- 3 Site                                 */
    char    Comp[5];           /* Component                                  */
    char    Net[5];            /* Net                                        */
    char    Loc[5];            /* Location                                   */
    char    SCN[20];           /* SCN                                        */
    char    SCNtxt[20];        /* S C N                                      */
    char    SCNnam[20];        /* S_C_N                                      */
    char    Comment[50];       /* Description (for web page)                 */
    char    Descrip[200];      /* Description (for web page)                 */
    int     OldData;           /* Old data plot flag                         */
    char    LocalTimeID[4];    /* Local standard time ID e.g. PST            */
    int     LocalTime;         /* Offset of local stand. time from GMT hr    */
    int     LocalTimeOffset;   /* Offset of local stand. time (secs)         */
    int     UseDST;            /* Daylight Savings Time used when needed     */
    
    int     DCremoved;         /* Attempt to make data zero mean             */
    int     DClabeled;         /* Attempt to make data zero mean             */
    double  DCcorr;            /* Current DC correction                      */
    double  LastTime;          /* Last time plotted                          */
    double  CurrentTime;       /* start time (UTC/local) for this image      */
    int     CurrentHour;       /* start hour (UTC/local) for this image      */
    int     CurrentDay;        /* start day  (UTC/local) for this image      */
    char    Today[12];         /* Day of current plot. DDMMYYYY              */

    double  Scale;             /* Scale factor for wiggle-line trace         */
    double  Scaler;            /* Scale factor [Data] (in)                   */
    
    int     Inst_type;         /* Type of instrument                         */
    double  Inst_gain;         /* Gain of instrument (microv/count)          */
    int     Sens_type;         /* Type of sensor                             */
    double  Sens_gain;         /* Gain of sensor (volts/unit)                */
    int     Sens_unit;         /* Sensor units d=1; v=2; a=3                 */
    double  GainFudge;         /* Additional gain factor.                    */
    double  SiteCorr;          /* Site correction factor.                    */
    double  sensitivity;       /* Channel sensitivity  counts/units          */
    int		ShkQual;           /* Station (Chan) type                        */

    LocInfo  Work;             /* Working version of the image               */
    LocInfo  Final;            /* Export version of the image                */
    
    int     UseLocal;          /* Reference frames to local time             */
    int     ShowUTC;           /* Show UTC on right margin                   */
    int     xpix, ypix;        /* Size of data plot (pixels)                 */
    int     mins;              /* # of minutes per display line              */
    int     Pallette;          /* Pallette flag                              */

    int     LinesPerHour;      /* # of Lines per hour                        */
    int     HoursPerPlot;      /* # of hours per image                       */
    int     LocalSecs;         /* Offset of local time in seconds            */
    int     PixPerLine;        /* # of Pixels per display line               */
    int     secsPerStep;       /* # of seconds per processing step           */
    int     secsPerGulp;       /* # of seconds per acquisition               */
    
    double  fmax;              /* Maximum frequency (Hz)                     */
    double  fmute;             /* Maximum frequency (Hz)                     */
    double  amax;              /* Maximum value of FFT for scaling           */
    double  dmin;              /* Minimum value of FFT for this line         */
    double  dmax;              /* Maximum value of FFT for this line         */
    double  dminhr;            /* Maximum value of FFT for last hour         */
    double  dmaxhr;            /* Maximum value of FFT for last hour         */
    double  PixPerMin;         /* # of Pixels per minute line                */
    int     nbw;               /* Plot scaling flag                          */
    double  oplot[MAXXPIX];    /* Spectral line to be plotted                */
};
typedef struct PltPar PltPar;


/*****************************************************************************
 *  Define the Global structure.                                             *
 *  This is the private area the module needs to keep track                  *
 *  of all those variables.                                                  *
 *****************************************************************************/

struct Global {
    int     Debug;             /*                                            */
    int     WSDebug;           /*                                            */
    int     UpdateInt;         /* Interval (secs) between updates            */
    int     nPlots;            /* number of plots actually needed            */
    int     Current_Plot;      /* number of plot being worked on             */
    
    int     SaveDrifts;
    int     BuildOnRestart;    /* Build totally new images on restart        */
    int     Make_HTML;         /* Set to 1 to construct and ship index.html file */
    int     Days2Save;
    int     DaysAgo;           /* Number of days ago to plot                 */
    int     OneDayOnly;        /* Plot only one day and exit                 */
    char    TraceBuf[MAXTRACEBUF]; /* This should work for 24-bit digitizers */
    char    mod[20];
    
    int     nltargets;         /* Number of local target directories         */
    char    loctarget[MAX_TARGETS][100];/* Target in form-> /directory/      */

    char    GifDir[GDIRSZ];    /* Dir for .gif & .html on local machine      */
    PltPar  plt[MAXPLOTS];     /* plotting parameters                        */
    char    Prefix[PRFXSZ];    /* Prefix for .gif & .html files on target    */
    int     logo;              /* =1 if logo                                 */
    int     logox, logoy;      /* Dimensions of logo                         */
    char    logoname[GDIRSZ+50]; /* Name of the logo GIF                     */
    int     nStaDB;            /* number of station databases we know about  */
    char    stationList[MAX_STADBS][STALIST_SIZ];
    int     stationListType[MAX_STADBS];
    pid_t   pid;
    int     status;
    WS_MENU_QUEUE_REC menu_queue[MAX_WAVESERVERS];
    
    int     NSCN;              /* Number of SCNs we know about               */
    ChanInfo    Chan[MAXCHANNELS];
/* Globals to set from configuration file
 ****************************************/
    long    wsTimeout;         /* seconds to wait for reply from ws          */
    int     nServer;           /* number of wave servers we know about       */
    long    RetryCount;        /* Retry count for waveserver errors.         */
                        /* list of available waveServers, from config. file  */
    int     inmenu[MAX_WAVESERVERS];
    int     wsLoc[MAX_WAVESERVERS];
    char    wsIp[MAX_WAVESERVERS][MAX_ADRLEN];
    char    wsPort[MAX_WAVESERVERS][MAX_ADRLEN];
    char    wsComment[MAX_WAVESERVERS][50];

    int     NoMenuCount;       /* Number of consecutive Nomenu errors        */
    int     NoGIFCount;        /* Number of consecutive GIF write errors     */

/* Variables used during the generation of each plot
 ***************************************************/
    long    samp_sec;          /* samples/sec                                */
    long    Npts;              /* Number of points in trace                  */
    double  Mean;              /* Mean value of the last data gulp           */
    char    GifName[175];      /* Name of this gif file on host webserver    */
    char    TmpName[175];      /* Name of this gif file on host webserver    */
    char    LocalGif[175];     /* Name of this gif file on local machine     */
    long    gcolor[MAXCOLORS]; /* GIF colors                                 */
    gdImagePtr    GifImage;

    int     nentries;          /* Number of menu entries for this SCN        */
    double  TStime[MAX_WAVESERVERS*2]; /* Tank start for this entry          */
    double  TEtime[MAX_WAVESERVERS*2]; /* Tank end for this entry            */
    int     index[MAX_WAVESERVERS*2];  /* WaveServer for this entry          */

    int     UseDST;            /* Daylight Savings Time used when needed     */
    int     ShowUTC;           /* Show UTC on right margin                   */
    int     UseLocal;          /* Reference frames to local time             */
    int     xpix, ypix;        /* Size of data plot (pixels)                 */
    double  xsize, ysize;      /* Overall Size of plot (inches)              */
    int     mins;              /* # of minutes per display line              */
    double  axexmax;           /* max axe x position [Data] (in)             */
    double  axeymax;           /* max axe y position [Data] (in)             */
    int     LinesPerHour;      /* # of Lines per hour                        */
    int     HoursPerPlot;      /* # of hours per image                       */
    
    int     LocalSecs;         /* Offset of local time in seconds            */
    int     PixPerLine;        /* # of Pixels per display line               */
    
    int     secsPerStep;       /* # of seconds per processing step           */
    int     secsPerGulp;       /* # of seconds per acquisition               */
};
typedef struct Global Global;

