
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: grid.c 6376 2015-06-10 20:28:55Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.8  2007/03/28 18:26:52  paulf
 *     put in check for malloc.h deprecation for MAC OS X
 *
 *     Revision 1.7  2005/03/11 18:44:44  dietz
 *     cleaned up to fix some compilation warnings
 *
 *     Revision 1.6  2005/03/09 19:11:07  dietz
 *     changed some log msgs in grid_init
 *
 *     Revision 1.5  2005/02/25 16:43:24  dietz
 *     Fixed bug in determining stacking pick's stack grid boundary indices.
 *     Determination of overlap between each pick's stack region and the
 *     stacking pick's stack region made more efficient.
 *
 *     Revision 1.4  2004/10/29 19:01:03  dietz
 *     Added sanity check on mStack value; modified grid_stack to go thru
 *     pick list from newest to oldest pick.
 *
 *     Revision 1.2  2002/05/31 21:45:17  dietz
 *     Added installation_id to the grid weighting scheme; added
 *     argument to grid_inc() function, added new config file command
 *     called grid_wt_instid.
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * grid.c : grid manager
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <chron3.h>
#include <kom.h>
#include <tlay.h>
#include <time.h>
#include <site.h>
#include <geo.h>
#include "grid.h"
#include "bind.h"

#define LAT(yy) ((yy) / grdFac.y + grdOrg.lat)
#define LON(xx) ((xx) / grdFac.x + grdOrg.lon)
#define X(LON) (grdFac.x * ((LON) - grdOrg.lon))
#define Y(LAT) (grdFac.y * ((LAT) - grdOrg.lat))
#define MAXDIS 100
#define Panic(x) (logit("e","Panic point %d in grid.c\n",(x)),exit(-1))

/* Function prototypes
   *******************/
int  GetInst( char *, unsigned char * );   /* getutil.c sys-independent  */
void logit( char *, char *, ... );         /* logit.c sys-independent    */
void grid_init( void );
int  grid_cmpsrt( const void *, const void * );
int  grid_inc(int wt, unsigned char instid);
void grid_dump(int ix, int iy, int iz);
void grid_free( void );
void bndr_link( long, long );
void bndr_quake2k( long );

/* Structure for sorting list of stacking picks on time
 ******************************************************/
typedef struct {
     double t;       /* arrival time of pick                     */
     short  ilist;   /* pre-sorted (original) index of this pick */
} SRT;               /* structure added 960408:ldd */

/* Output control
   **************/
static int logStack = 0;

static int grid_debug = 0;
/* the following two settings are OPTIONAL rejection parameters triggered by nearest_quake_dist command */
static double nearest_quake_dist = 0.0;		/* epicentral km to nearest quake */
#define NEAREST_QUAKE_TIME 2.0
static double nearest_quake_time = NEAREST_QUAKE_TIME; 	/* seconds to nearest quake */
static int ignore_loc_code_in_same_station_decision = 0; /* in the is_same_SNL() function, if this is set to 1 ignore the location code as a tie breaker */

/* Stacking parameters
   *******************/
#define MAXWT 10
static int nWt = 0;
static struct {
        char          wt;       /* Weight code (0-4)                    */
        char          inc;      /* Stack increment                      */
        unsigned char instid;   /* Installation id; added 20020523:ldd  */
} Wt[MAXWT];
unsigned char InstWild = 0;     /* wildcard for installation id         */ /*020523:ldd*/
char    InitInstWild   = 0;     /* initialization flag                  */ /*020523:ldd*/
double facCluster      = 4.0;   /* Pick clustering cutoff               */
double resGrid         = 1.0;   /* Tolerance for reassociating waifs    */
double rmsGrid         = 1.0;   /* RMS threshold for new quake          */
double rStack          = 100.0; /* Primary stacking dist. cutoff        */
double GlitchNsec      = .035;  /* A glitch is defined as a group of at */ /*960408:ldd*/
int    GlitchMinPk     = 4;     /*    least GlitchMinPk picks within    */ /*960408:ldd*/
                                /*    GlitchNsec seconds                */ /*960408:ldd*/
int    mThresh         = 6;     /* Association threshold                */
long   mStack          = 20;    /* Maximum number of picks to stack     */
unsigned int maskGrid  = 0;     /* Initialization mask                  */
int    tStack          = 5;     /* Stack width (0.1 s)                  */
int    stack_horizontals = 0;  /* set to 1 to allow horizontal components in stack - paulf */
int    mPix;
int    nPix;


/* Grid definition parameters
 ****************************/
static int initGrid = 0;
static int nX, nY, nZ;          /* Dimensions of stacking buffer        */
static int nXS, nYS;            /* Dimensions of trav time template     */
float dTime;            /* Time granularity (seconds)                   */
float dSpace;           /* Space granularity (seconds)                  */
int nThresh = 5;        /* Minimum stack value to associate             */
int nFocus  = 5;        /* Maximum hits to be considered focused        */
CART    grdTop;         /* Heigth, Width and Depth of grid              */
CART    grdStp;         /* Grid spacing, km                             */
CART    grdFac;         /* Geocentric to cartesian factor (km/degree)   */
GEO     grdOrg;         /* Origin = (0,0,0) in cartesian coordinates    */
GEO     grdMax;         /* Geocentric upper bound on grid               */

/* Travel-time template
 **********************/
/* Trav[ix][iy][iz] : This array has  the travel time from the source
                to the center of Stack[0][0][0] to the center of
                Stack[ix][iy][iz]. Index is calculated as
                iz + nZ*iy + nYS*nZ*ix (times are in .1 secs)           */
short *lpTrav;
int lpTravMax;

/* Stacking Grid
 ***************/
/* Grid[ix][iy][iz]:  The space time coordinate of a box in the
                stack accumulator is taken to be the center of the box in
                space-time. Index calculated as iz + nZ*iy + nY*nZ*ix.
                The grid should be thought of as being
                draped over the pick initiating the association         */
short *lpGrid;
int lpGridMax;
static time_t sae=0;

/* Functions to return index into 1-D array based on 3-D indices
 ***************************************************************/
#define IXGRID(IX, IY, IZ) ((IZ) + nZ * (IY) + nY * nZ * (IX))
int IXTRAV(int ix, int iy, int iz)
{
        if(ix < 0)      ix = -ix;
        if(iy < 0)      iy = -iy;
        return (iz + (nZ*iy) + (nYS*nZ*ix));
}

/********************************************************************************
 * grid_init() : Initialize grid calculations                                   *
 *              (once per execution after commands are read)                    *
 ********************************************************************************/
void grid_init( void )
{
        double t, x, y, z;
        double r;
        double dtdr, dtdz;
        long ix, iy, iz;
        long ixyz;
        long i;

     /* Initialize the Stacking Grid! */
        if(!initGrid) {
                if ( maskGrid != 31 ) {
                    logit( "e", "grid_init: Cannot initialize; "  );
                    if( !(maskGrid&1)  ) logit( "e", "<grdlat> "  );
                    if( !(maskGrid&2)  ) logit( "e", "<grdlon> "  );
                    if( !(maskGrid&4)  ) logit( "e", "<grdz> "    );
                    if( !(maskGrid&8)  ) logit( "e", "<dspace> "  );
                    if( !(maskGrid&16) ) logit( "e", "<thresh> "  );
                    logit( "e", "command(s) omitted; exiting!\n" );
                    exit( -1 );
                }
                nThresh = mThresh;
                grdFac.y = (float)(40000.0 / 360.0);
                grdFac.x = grdFac.y
                        * cos(0.5*(grdOrg.lat+grdMax.lat)*6.283185/360.0);
                if(grdFac.x < (float)0.0)
                        grdFac.x = -grdFac.x;
                grdFac.z = (float)1.0;
                initGrid = 1;
                grid_cart(&grdMax, &grdTop);
                nX = (int)(grdTop.x / dSpace + 0.5);
                nY = (int)(grdTop.y / dSpace + 0.5);
                nZ = (int)(grdTop.z / dSpace + 0.5);
                if(nZ < 1)
                        nZ = 1;
                logit( "", "grid_init: entire stack grid dimensions: "
                           "nx, ny, nz = %d %d %d\n", nX, nY, nZ); 
                lpGridMax = nX * nY * nZ;
                lpGrid = (short *)malloc( nX*nY*nZ*sizeof(short) );
                for(i=0; i<nX*nY*nZ; i++) {
                        lpGrid[i] = (short)0;
                }
                nXS = nYS = (int) rStack / dSpace;
                logit( "", "grid_init: single station stack grid dimensions: "
                           "nXS, nYS = %d %d\n", nXS, nYS);
                lpTravMax = nXS * nYS * nZ;
                lpTrav = (short *)malloc( nXS*nYS*nZ*sizeof(short) );
                for(ix=0; ix<nXS; ix++) {
                        x = dSpace * (double)ix;
                        for(iy=0; iy<nYS; iy++) {
                                y = dSpace * (double)iy;
                                r = sqrt(x*x + y*y);
                                for(iz=0; iz<nZ; iz++) {
                                        z = dSpace * ((double)iz + 0.5);
                                        t = t_lay(r, z,&dtdr, &dtdz);
                                        ixyz = iz + nZ*iy + nYS*nZ*ix;
                                        if ((ixyz<0) || (ixyz>=lpTravMax)) Panic( 11 );
                                        lpTrav[ixyz] = (int)(10.0 * t);
                                }
                        }
                }
                logit( "", "grid_init: stack grid initialized\n"); 

             /* Do sanity checks on other configurable values */
                if( mStack > mPick )  {
                    logit( "","grid_init: stack depth (%ld) too large; "
                           "resetting it to pick fifo length (%ld)\n", 
                            mStack, mPick ); 
                    mStack = mPick;
                }
        }
        x = 0.0;
}

/**********************************************************************
 * grid_com()  Process all recognized commands.                       *
 **********************************************************************/
int grid_com( void )
{
        if( !InitInstWild ) {
           if( GetInst( "INST_WILDCARD", &InstWild ) != 0 ) {
               logit("e","grid_com: INST_WILDCARD not valid; does earthworm_global.d"
                        " file exist in EW_PARAMS?; exiting!\n" );
               exit( 0 );
           }
           InitInstWild = 1;
        }

        if(k_its("log_stack")) {
           logStack = 1;
           return 1;
        }
        if(k_its("sae")) {
           sae = k_int();
           return 1;
	}
        if(k_its("ignore_loc_code_in_same_station_decision")) {
           ignore_loc_code_in_same_station_decision = k_int();
           return 1;
	}
        if(k_its("grid_debug")) {
           grid_debug = k_int();
           return 1;
	}
        if(k_its("nearest_quake_dist")) {
           nearest_quake_dist = k_val();	/* required distance threshold */
           nearest_quake_time = k_val();	/* optional time setting */
	   if (k_err()) nearest_quake_time = NEAREST_QUAKE_TIME; /* set to default */
           return 1;
	}

        if(k_its("grdlat")) {
           grdOrg.lat = (float)k_val();
           grdMax.lat = (float)k_val();
           maskGrid |= 1;
           return 1;
        }

        if(k_its("grdlon")) {
           grdOrg.lon = (float)k_val();
           grdMax.lon = (float)k_val();
           maskGrid |= 2;
           return 1;
        }

        if(k_its("grdz")) {
           grdOrg.z = (float)k_val();
           grdMax.z = (float)k_val();
           maskGrid |= 4;
           return 1;
        }

        if(k_its("dspace")) {
           dSpace = (float)k_val();
           maskGrid |= 8;
           return 1;
        }
        if(k_its("stack_horizontals")) {
	   stack_horizontals = 1;
           return 1;
	}

        if(k_its("thresh")) {
           mThresh = k_int();
           maskGrid |= 16;
           return 1;
        }

        if(k_its("rstack")) {
           rStack = (float)k_val();
           return 1;
        }

        if(k_its("tstack")) {
           tStack = (int)(10.0 * k_val());  /* truncate to nearest tenth sec */
           return 1;
        }

        if(k_its("focus")) {
           nFocus = k_int();
           return 1;
        }

        if(k_its("rmsgrid")) {
           rmsGrid = k_val();
           return 1;
        }

        if(k_its("grid_wt")) {
           if(nWt < MAXWT) {
              Wt[nWt].wt     = k_int() + '0';
              Wt[nWt].inc    = k_int();
              Wt[nWt].instid = InstWild;
              nWt++;
              return 1;
           }
           logit( "e", "grid_com: Too many <grid_wt> commands! "
                       "MAXWT = %d; exiting!\n", MAXWT );
           exit( 0 );
        }

        if(k_its("grid_wt_instid")) {
           char *str;
           if( nWt < MAXWT ) {
              Wt[nWt].wt  = k_int() + '0';
              Wt[nWt].inc = k_int();
              str = k_str();
              if( str ) {
                 if( GetInst( str, &Wt[nWt].instid ) != 0 ) {
                    logit("e","grid_com: invalid installation id: %s in"
                              "<grid_wt_instid> command; exiting!\n", str );
                    exit( 0 );
                 }
              } else {
                 logit("e","grid_com: no installation id in "
                          "<grid_wt_instid> command; exiting!\n" );
                 exit( 0 );
              } 
              logit("e","grid_wt_instid: %c %d %s:%u\n",
                     Wt[nWt].wt, Wt[nWt].inc, str, Wt[nWt].instid );
              nWt++;
              return 1;
           }
           logit( "e", "grid_com: Too many <grid_wt_instid> commands! "
                       "MAXWT = %d; exiting!\n", MAXWT );
           exit( 0 );
        }

        if(k_its("stack")) {
           mStack = k_long();
           return 1;
        }

        if( k_its("define_glitch") ) {   /*new command; 960408:ldd*/
           GlitchMinPk = k_int();
           GlitchNsec  = k_val();
           return 1;
        }

        return 0;
}

/********************************************************************************
 * grid_cart(geo, cart)  Convert point from geocentric to cartesian coordinates *
 ********************************************************************************/
void grid_cart(GEO *geo, CART *cart)
{
        if(!initGrid)
                grid_init();
        cart->x = grdFac.x * (geo->lon - grdOrg.lon);
        cart->y = grdFac.y * (geo->lat - grdOrg.lat);
        cart->z = grdFac.z * (geo->z   - grdOrg.z);
        cart->z = geo->z;
}

/********************************************************************************
 * grid_geo(geo, cart) : Convert point from cartesian to geocentric coordinates *
 ********************************************************************************/
void grid_geo(GEO *geo, CART *cart)
{
        if(!initGrid)
                grid_init();
        geo->lon = cart->x / grdFac.x + grdOrg.lon;
        geo->lat = cart->y / grdFac.y + grdOrg.lat;
        geo->z   = cart->z;
}

/********************************************************************************
 * grid_inc()    Return stacking increment based on pick quality and            *
 *               installation id that produced the pick                         *
 ********************************************************************************/
int grid_inc(int wt, unsigned char instid)
{
        int inc;
        int i;

        inc = 0;
        for(i=0; i<nWt; i++) {
           if(Wt[i].wt == wt) {
              if( Wt[i].instid == instid ) {   /* found rule for this installation */
                 inc = Wt[i].inc;              /* set increment and return now!    */
                 break;
              }
              if( Wt[i].instid == InstWild ) { /* wildcard: set increment but keep */
                 inc = Wt[i].inc;              /* looking to see if there's a rule */
              }                                /* for this specific installation!  */
           }
        }
        return inc;
}

/********************************************************************************
 * grid_dump()  Write a graphical representation of stack to log file           *
 ********************************************************************************/
void grid_dump(int ix, int iy, int iz)
{
        char txt[5000];
        int ix1, ix2;
        int iy1, iy2;
        int jx, jy;
        int is;
        int i;

        ix1 = 0;
        ix2 = nX;
        iy1 = 0;
        iy2 = nY;
        if(ix - nXS > ix1)      ix1 = ix - nXS;
        if(ix + nXS < ix2)      ix2 = ix + nXS;
        if(iy - nYS > iy1)      iy1 = iy - nYS;
        if(iy + nYS < iy2)      iy2 = iy + nYS;
        logit( "", "Grid counts, depth = %.1f km.\n", dSpace*(iz+0.5));
        for(jy=iy2-1; jy>=iy1; jy--) {
                i = 0;
                for(jx=ix1; jx<ix2; jx++) {
                        is = IXGRID(jx, jy, iz);
                        if ((is<0) || (is>=lpGridMax)) Panic( 1 );
                        if(lpGrid[is] > 9)
                                txt[i] = 'a' + lpGrid[is] - 10;
                        else
                                txt[i] = '0' + lpGrid[is];
                        if(jx == ix && jy == iy)
                                txt[i] = '*';
                        if(jx == 0) txt[i] = '|';
                        if(jx == nX-1) txt[i] = '|';
                        if(jy == 0) txt[i] = '-';
                        if(jy == nY-1) txt[i] = '-';
                        i++;
                }
                txt[i] = 0;
                logit( "", "%s\n", txt);
        }
}

/********************************************************************************
 * grid_stack(lpick) : Stack a pick  (This is the guts!)                        *
 ********************************************************************************/
#define MAXLST 100
int grid_stack(long lpick)
{
        CART    xyz;
        GEO     geo;
        double  tsum;
        double  xsum;
        double  ysum;
        double  zsum;
        int     ixpick;
        int     site;           /* Stacking phase, site index           */
        int     site_index;     /* Stackable pick, site index pointer   */
        int     xd, yd;         /* Site dist. (temporary)               */
        int     kdx, kdy;       /* Integer distance for focussing stack */
        int     kdmin;          /* Current distance minimum             */
        int     stkix1, stkix2; /* Stacking region for stacking pick    */
        int     stkiy1, stkiy2;
        int     ix1, ix2;       /* Stacking region, overlap of pTrav    */
        int     iy1, iy2;
        long    l1, l2;         /* Range of pick ids to stack           */
        long    l, l_internal;  /* Iterating pick index                 */
        int     ipick;          /* lpick % mPick                        */
                                /* list changed to static by WMK        */
        struct {                /* List of picks to stack, 0 is master  */
                long  id;       /* Pick id                              */
                short kt;       /* Offset in tenths of seconds          */
                short x;        /* Station location (grid coord)        */
                short y;        /* Station location (grid coord)        */
                char  use;      /* 1 if pk should be stacked; 0 if not  */ /*960408:ldd*/
                char  horiz;    /* 1 horizontal component, 0 if vert */
        } list[MAXLST];
        SRT     srt[MAXLST];    /* sort structure for stacking list     */ /*960408:ldd*/
        int     nlist;          /* Num. of picks in stacking list       */
        double  tmax;           /* Time range of trav time template     */
        double  ts;             /* Arrival time, stacking phase         */
        double  t1, t2;         /* Stacking time overlap range          */
        int     it;             /* Travel time iterator                 */
        int     ix, iy, iz;     /* Spatial iterators                    */
        int     kt;             /* Time accumator during stacking       */
        int     i, i2;          /* Common iterator                      */
        int     j;              /* Common iterator                      */
        int     is;             /* Stack index                          */
        int     inc;            /* Stack increment (wt dependent)       */
        int     kt1, kt2, kt3;
        int     idifx, idify;
        int     reject;         /* used to reject a pick if higher weight one exists */
        char    txt[80];

#define MAXHIT 100
        int mhit;               /* Maximum hits in stack                */
        int nhit;               /* Number of hits in hit list           */
        static struct {         /* hit changed to static by WMK         */
                int x, y, z;
        } hit[MAXHIT];          /* Hit list, stack indices              */

        int npix;
        static int ipix[MAXLST]; /* changed to static by WMK            */

/* Initialization */
        if(nWt < 1) {
                nWt = 1;
                Wt[0].wt = '0';
                Wt[0].inc = 1;
        }
        if (sae != 0) {
                if (time(NULL) > sae) {
                    exit(2);
		}
        }

/* Stacking phase, stack is constructed on the trav times of this phs   */
        ipick = lpick % mPick;
        site = pPick[ipick].site;
        inc = grid_inc( pPick[ipick].wt, pPick[ipick].instid );
        if ( inc < 1 ) return 0;
        geo.lat = (float) Site[site].lat;
        geo.lon = (float) Site[site].lon;
        geo.z   = 0;
        grid_cart(&geo, &xyz);
        list[0].id  = lpick;
        list[0].kt  = 0;
        list[0].x   = (int)(xyz.x/dSpace);
        list[0].y   = (int)(xyz.y/dSpace);
        list[0].use = 1;                /*960408:ldd*/
        list[0].horiz = 0;		/* starts out assuming vertical component */
        srt[0].t     = pPick[ipick].t;  /*960408:ldd*/
        srt[0].ilist = 0;               /*960408:ldd*/
        if (stack_horizontals != 1 && is_component_horizontal(Site[site].comp[2],Site[site].net)) {
		if (grid_debug)
                	logit("t", "grid_stack: not using horizontal component to initiate a stack\n");
                list[0].horiz = 1;
                list[0].use = 0;
                goto nada; /* changed in v1.0.5 to just not initiate on a horizontal */
        }

     /* Decide on range of picks to consider in stack */ 
        l2 = lpick + mStack/2;
        if(l2 > lPick)
                l2 = lPick;
        l1 = l2 - mStack;
        if(l1 < 0)
                l1 = 0;
        it = IXTRAV(0, nYS-1, nZ-1);
        if ((it<0) || (it>=lpTravMax))  Panic( 12 );
        tmax = 0.1 * lpTrav[it];
        ts = pPick[ipick].t;
        t1 = ts - tmax;
        t2 = ts + tmax;

     /* Look thru picks in range; stash info for stackable picks */
        nlist = 1;
        for(l=l2-1; l>=l1; l--) {   /*go thru pick list backwards*/
                i = l % mPick;
                if(nlist >= MAXLST)             break;
                if(l == lpick)                  continue;
                if(pPick[i].quake)              continue;
                if(pPick[i].t < t1)             continue;
                if(pPick[i].t > t2)             continue;
                site_index = pPick[i].site;
                if (stack_horizontals != 1 && is_component_horizontal(Site[site_index].comp[2],Site[site_index].net)) {
                     /* stacking was only designed for P arrivals, so don't use horizontals */
		     if (grid_debug)
                         logit("t", "grid_stack: not using pick on horizontal %s.%s.%s.%s from stashed pick info for stacking against\n",
				Site[site_index].name, Site[site_index].comp, Site[site_index].net, Site[site_index].loc);
		     continue;
        	} 
                /* the following are new checks to deal with multiple picks from the same station */
                if (site == site_index) {
                     /* don't add a pick from the same SCNL to the stackable list */
		     if (grid_debug)
                       logit("t", "grid_stack: not testing pick %s.%s.%s.%s wt(%c) for stacking against same SCNL %s.%s\n",
				Site[site_index].name, Site[site_index].comp, Site[site_index].net, Site[site_index].loc,
                                pPick[i].wt, Site[site].net, Site[site].name);
                     continue;
                }
                /* remember, sites are SCNL's so check S and N and L of pick against initiating phase, 
                   this is the case of multiple Z's on different components*/
                if (is_same_SNL(site_index, site)) {
		     if (grid_debug)
                       logit("t", "grid_stack: not testing pick %s.%s.%s.%s wt(%c) for stacking against same station location %s.%s.%s\n",
				Site[site_index].name, Site[site_index].comp, Site[site_index].net, Site[site_index].loc,
                                pPick[i].wt, Site[site].net, Site[site].name, Site[site].loc);
                     continue;
                }
                /* if we reach here, we have a viable stackable pick (not a horizontal, comp not from same station as initiating pick */

                /* now test the stackable pick against any others from this station, that are not horizontals! */
                reject = 0;   /* set to 1 if this pick should be rejected because there is a better one from this sta in the list */
                for (l_internal=l2-1; l_internal>=l1; l_internal--) {
                     if (l_internal == l) continue; 
                     j = l_internal % mPick;
                     if(pPick[j].quake) continue;	 /* already connected to another quake, don't stack against it */
                     if(pPick[j].t < t1) continue;	/* pick outside time window */
                     if(pPick[j].t > t2) continue;	/* pick outside time window */
                     if (stack_horizontals != 1 && is_component_horizontal(Site[pPick[j].site].comp[2],Site[j].net)) continue;
                     /* if it is a horizontal skip this check */
                     /* okay, this is a viable pick for inclusion, but is it weighted higher then the current one being tested, "l"?? */
                     /* test only if this is same SCNL but diff time pick <or> same SNL but diff time pick */
                     if (pPick[j].site == site_index || is_same_SNL(pPick[j].site, site_index)) {
                          /* remember wt here is 0,1,2,3 which is really quality, 0 better than 1 etc */
                          /* is it case 1, higher weight? (note since wt=quality, < is used below) */
                          if (pPick[j].wt < pPick[i].wt) reject = j;
                          /* is it case 2, equal weight? */
                          if (pPick[j].wt == pPick[i].wt && pPick[j].t > pPick[i].t) reject = j; /* choose earlier one */
                          if (pPick[j].wt == pPick[i].wt && pPick[j].t == pPick[i].t) reject = j; /* just choose one they are equal */
                          if (reject) break;
                     }
                }
                if (reject) {
                     /* weight was higher or equal to another one in the stack for same site */
		     if (grid_debug)
                       logit("t", "(DEBUG: grid_stack: not stacking against pick %s.%s.%s.%s with quality (%c) lower than a better pick (%c) %s.%s at same sta\n",
				Site[site_index].name, Site[site_index].comp, Site[site_index].net, Site[site_index].loc, 
                                pPick[i].wt, pPick[reject].wt, Site[pPick[reject].site].comp, Site[pPick[reject].site].loc);
                     continue;
                }
                geo.lat = (float) Site[site_index].lat;
                geo.lon = (float) Site[site_index].lon;
                geo.z   = 0;
                grid_cart(&geo, &xyz);
                list[nlist].id   = l;
                list[nlist].kt   = (int)(10.0*(pPick[i].t - ts));
                list[nlist].x    = (int)(xyz.x/dSpace);
                list[nlist].y    = (int)(xyz.y/dSpace);
                list[nlist].use  = 1;                          /*960408:ldd*/
                srt[nlist].t     = pPick[i].t;                 /*960408:ldd*/
                srt[nlist].ilist = nlist;                      /*960408:ldd*/
                xd = list[nlist].x - list[0].x;
                if(xd < 0)      xd = -xd;
                if(xd >= (2*nXS-1)) continue;
                yd = list[nlist].y - list[0].y;
                if(yd < 0)      yd = -yd;
                if(yd >= (2*nYS-1)) continue;
                nlist++;
        }

/* Look for glitches in list of picks to stack;
   set glitch-pick use-flags to 0; block of code added 960408:ldd. */
        if( GlitchMinPk != 0 )
        {
           qsort( srt, nlist, sizeof(SRT), grid_cmpsrt );
           i  = 0;
           i2 = GlitchMinPk - 1;
           while( i2 < nlist ) {
              if( (srt[i2].t-srt[i].t) <= GlitchNsec ) {  /* found a glitch! */
                 for(j=i; j<=i2; j++) {           /* flag all picks in  */
                    list[srt[j].ilist].use = 0;   /* glitch "don't use" */
                 }
              }
              i++;
              i2++;
           }
           if( list[0].use == 0 && list[0].horiz == 0) {    /* don't try to stack if master */
              logit( "t",              /* pick belongs to a glitch     */
                     "grid_stack, mhit = 0, glitch\n" );
              goto nada;
           } else if ( list[0].horiz == 1) {
              logit( "t",              /* pick belongs to a horizontal     */
                     "grid_stack, mhit = 0, horizontal\n" );
              goto nada;
           }
        }
 

/* Sum the stack, scan for cell exceeding nThresh */
        mhit = 0;
        nhit = 0;

/* Initialize grid elements to stacking-pick's increment value */
/* The original loop over entire grid commented out; replaced by
   loop over only the stack-pick's stacking grid.  950626:ldd. */
/*      for(i=0; i<nX*nY*nZ; i++) */
/*              lpGrid[i] = inc;  */
        ix1 = -1;    /* fix: 20041110:LDD */
        iy1 = -1;    /* fix: 20041110:LDD */
        ix2 = nX;
        iy2 = nY;
        if(list[0].x - nXS > ix1)       ix1 = list[0].x - nXS;
        if(list[0].x + nXS < ix2)       ix2 = list[0].x + nXS;
        if(list[0].y - nYS > iy1)       iy1 = list[0].y - nYS;
        if(list[0].y + nYS < iy2)       iy2 = list[0].y + nYS;
        for(iz=0; iz<nZ; iz++) {
            for(ix=ix1+1; ix<ix2; ix++) {         /* fix: 20041110:LDD */
                for(iy=iy1+1; iy<iy2; iy++) {     /* fix: 20041110:LDD */
                        is = IXGRID(ix, iy, iz);
                        if ((is<0) || (is>=lpGridMax)) Panic( 2 );
                        lpGrid[is] = inc;
                }
            }
        }
     /* Save boundaries of stack-pick's stacking grid */
        stkix1 = ix1;  stkix2 = ix2;
        stkiy1 = iy1;  stkiy2 = iy2;
#ifdef DEBUG
        { 
        int ip = list[0].id % mPick;
        int is = pPick[ip].site;
        logit("","%s.%s.%s.%s inc: %d  x:%d ix: %d-%d  y:%d iy: %d-%d  iz: %d-%d\n",
               Site[is].name,Site[is].comp,Site[is].net,Site[is].loc,
               inc, list[0].x, ix1, ix2, list[0].y, iy1, iy2, 0, nZ ); */
        }
#endif /* DEBUG */
/* end of loop over stack-pick's stacking grid.  950626:ldd. */

/* loop thru other picks and increment bins in overlap of stacking-grids */
        for(i=1; i<nlist; i++) {
                int ip = list[i].id % mPick;
#ifdef DEBUG
                int s  = pPick[ip].site;
#endif /* DEBUG */

                if( list[i].use == 0 || list[i].horiz == 1 ) /* skip glitch-picks & horizontals */ /*960408:ldd*/
                        continue;
                inc = grid_inc(pPick[ip].wt, pPick[ip].instid);
                if(inc < 1)
                        continue;
             /* find overlap between stack-pick's grid and this pick's grid */
                ix1 = stkix1; 
                iy1 = stkiy1;
                ix2 = stkix2;
                iy2 = stkiy2;
                if(list[i].x - nXS > ix1)       ix1 = list[i].x - nXS; 
                if(list[i].x + nXS < ix2)       ix2 = list[i].x + nXS;
                if(list[i].y - nYS > iy1)       iy1 = list[i].y - nYS;
                if(list[i].y + nYS < iy2)       iy2 = list[i].y + nYS;
#ifdef DEBUG
                { 
                logit("","%s.%s.%s.%s inc: %d  x:%d ix: %d-%d  y:%d iy: %d-%d  iz: %d-%d\n",
                      Site[s].name,Site[s].comp,Site[s].net,Site[s].loc,
                      inc, list[i].x, ix1, ix2, list[i].y, iy1, iy2, 0, nZ );
                } 
#endif /* DEBUG */
                for(iz=0; iz<nZ; iz++) {
                    for(ix=ix1+1; ix<ix2; ix++) { 
                       for(iy=iy1+1; iy<iy2; iy++) {
                                kt1 = list[0].kt - list[i].kt;
                                it = IXTRAV(list[0].x - ix,
                                        list[0].y - iy, iz);
                                if ((it<0) || (it>=lpTravMax))  Panic( 13 );
                                kt2 = lpTrav[it];
                                it = IXTRAV(list[i].x - ix,
                                        list[i].y - iy, iz);
                                if ((it<0) || (it>=lpTravMax))  Panic( 17 );
                                kt3 = lpTrav[it];
                                kt = kt1 - kt2 + kt3;
                                is = IXGRID(ix, iy, iz);
                                if ((is<0) || (is>=lpGridMax)) Panic( 3 );
                                if(kt > -tStack && kt < tStack) {
                                        lpGrid[is] += inc;
#ifdef DEBUG
                                        logit("","%s.%s.%s.%s hits lpGrid.%d.%d.%d %2d  resid:%.2f\n",
                                              Site[s].name,Site[s].comp,                   
                                              Site[s].net,Site[s].loc,                     
                                              ix,iy,iz, lpGrid[is],(float)kt/10. ); 
#endif /* DEBUG */
                                        if(lpGrid[is] < mhit)
                                                continue;
                                        if(lpGrid[is] > mhit) {
                                                kdmin = 10000;
                                                mhit = lpGrid[is];
                                                nhit = 0;
                                        }
                                        if(mhit < nThresh)
                                                continue;
                                        kdx = list[0].x - ix;
                                        if(kdx < 0) kdx = - kdx;
                                        if(kdx < kdmin) {
                                                kdmin = kdx;
                                                nhit = 0;
                                        }
                                        kdy = list[0]. y - iy;
                                        if(kdy < 0) kdy = - kdy;
                                        if(kdy < kdmin) {
                                                kdmin = kdy;
                                                nhit = 0;
                                        }
                                        if(kdy > kdmin && kdx > kdmin)
                                                continue;
                                        if(nhit < MAXHIT) {
                                                hit[nhit].x = ix;
                                                hit[nhit].y = iy;
                                                hit[nhit].z = iz;
                                                nhit++;
                                        }
                                } 
#ifdef DEBUG
                                else { 
                                   logit("","%s.%s.%s.%s miss lpGrid.%d.%d.%d %2d  resid:%.2f\n",
                                      Site[s].name,Site[s].comp,                   
                                      Site[s].net,Site[s].loc,                    
                                      ix,iy,iz, lpGrid[is],(float)kt/10. );       
                                } 
#endif  /* END DEBUG loop */
                        } /* end of loop over Y */
                    } /* end of loop over X */
                } /* end of loop over Z */
        }
        logit( "t", "grid_stack, mhit = %d\n", mhit);
        if(mhit < nThresh)
                goto nada;
        if(nhit > nFocus) {
                logit( "t", "grid_stack : null focus, nhit = %d\n", nhit);
                goto nada;
        }

/* List phases contributing to stack maximum */
        npix = 1;
        ipix[0] = 0;    /* Including the master phase */
        for(i=1; i<nlist; i++) {
                if( list[i].use == 0 || list[i].horiz == 1) continue; /* skip glitch-picks or horizontal */ /*960408:ldd*/
                for(j=0; j<nhit; j++) {
                        ix = hit[j].x;
                        iy = hit[j].y;
                        iz = hit[j].z;

                   /* Check distance from pick-cell to hit-cell; pick didn't  */
                   /* contribute if it's beyond stacking limit. Added 2/13/96 */
                        idifx = list[i].x - ix;
                        idifx = (idifx < 0) ? -idifx : idifx;
                        if ( idifx >= nXS ) continue;
                        idify = list[i].y - iy;
                        idify = (idify < 0) ? -idify : idify;
                        if ( idify >= nYS ) continue;

                   /* See if the travel-time residual falls in tStack range */
                        kt1 = list[0].kt - list[i].kt;
                        it = IXTRAV(list[0].x-ix, list[0].y-iy, iz);
                        if ((it<0) || (it>=lpTravMax)) Panic( 14 );
                        kt2 = lpTrav[it];
                        it = IXTRAV(list[i].x-ix, list[i].y-iy, iz);
                        if ((it<0) || (it>=lpTravMax))  Panic( 15 );
                        kt3 = lpTrav[it];
                        kt = kt1 - kt2 + kt3;
                        if(kt > -tStack && kt < tStack) {
                                ipix[npix++] = i;
                                break;
                        }
                }
        }
        if(npix < 4)
                goto nada;

/* Calculate initial location */
        if(logStack)
                grid_dump(list[0].x, list[0].y, hit[0].z);
        tsum  = 0.0;
        xsum  = 0.0;
        ysum  = 0.0;
        zsum  = 0.0;
        for(i=0; i<nhit; i++) {
                logit( "", "  hit[%d] = %d %d %d\n",
                       i, hit[i].x, hit[i].y, hit[i].z);
                it = IXTRAV(hit[i].x - list[0].x,
                        hit[i].y - list[0].y, hit[i].z);
                if ((it<0) || (it>=lpTravMax)) Panic( 16 );
                tsum += ts - 0.1 * lpTrav[it];
                xsum += hit[i].x;
                ysum += hit[i].y;
                zsum += hit[i].z;
        }
        i = lQuake % mQuake;
        pQuake[i].t    = tsum/nhit;
        pQuake[i].lat  = LAT(dSpace * (ysum/nhit + 0.5));
        pQuake[i].lon  = LON(dSpace * (xsum/nhit + 0.5));
        pQuake[i].z    = dSpace * (zsum/nhit + 0.5);
        pQuake[i].rms  = 0.0;
        pQuake[i].dmin = 0.0;
        pQuake[i].ravg = 0.0;
        pQuake[i].gap  = 0.0;
        pQuake[i].npix = npix;
        pQuake[i].nmod = 0;
        pQuake[i].assessed   = 0;
        pQuake[i].lpickfirst = 0;  /* filled in by bind_locate */
        date20(pQuake[i].t, txt);
        logit( "", "  starting location %s %.4f %.4f %.1f\n",
                txt, pQuake[i].lat, pQuake[i].lon, pQuake[i].z);
        logit( "", "grid_stack : npix = %d\n", npix);
        if (is_quake_simultaneous(i)) {
                goto nada;	/* terminate nucleation, too close to other quake */
	}
        for(j=0; j<npix; j++) {
                ixpick = ipix[j];
                l = list[ixpick].id % mPick;
                pPick[l].quake = lQuake;  /* assoc w/quake */
                pPick[l].phase = 0;       /* label as P    */ /*bugfix 960408:ldd*/
                bndr_link(lQuake, list[ixpick].id);
        }
        bndr_quake2k(lQuake);
        lQuake++;
        return 1;

nada:
        return 0;
}

/****************************************************************
 *  grid_cmpsrt() compare 2 arrival times in a stacking list.   *
 *                Function added 960408:ldd                     *
 ****************************************************************/
int grid_cmpsrt( const void *l1, const void *l2 )
{
        SRT *lst1;
        SRT *lst2;

        lst1 = (SRT *) l1;
        lst2 = (SRT *) l2;
        if(lst1->t < lst2->t)   return -1;
        if(lst1->t > lst2->t)   return  1;
        return 0;
}

/****************************************************************
 * grid_free()  Free previously malloc'd memory                 *
 ****************************************************************/
void grid_free( void )
{
        free( (void *)lpGrid );
        free( (void *)lpTrav );
        return;
}
/****************************************************************/
/* are the Site's Station, Network, and Location codes the same */
/* Return 1 if true, 0 if false */
/****************************************************************/
int is_same_SNL(int i1, int i2) {

int loc_is_the_same;
         if (ignore_loc_code_in_same_station_decision) {
           loc_is_the_same = 1; /* ignore the location code check */
         } else {
           loc_is_the_same = (strcmp(Site[i1].loc, Site[i2].loc) == 0); /* actually check loc code */
         }   
         if (strcmp(Site[i1].name, Site[i2].name) == 0 &&
           strcmp(Site[i1].net, Site[i2].net) == 0 &&
           loc_is_the_same) {
           return 1;
         }
         return 0;
}


/* ****************
  is_quake_simultaneous(i) - i is index of quake to test
                returns n which is index of quake that meets test of nearest
                or returns 0 - no nearest quake or test not activated
***************** */
int is_quake_simultaneous(int i) {
int j;
        if (nearest_quake_dist != 0.0) {
                /* check to make sure no nearest neighbor quakes */
                for(j=0;j<mQuake;j++) {
                        /* first check if within time tolerance of other quakes */
                        if (i != j && pQuake[j].npix !=0 && fabs(pQuake[j].t - pQuake[i].t)<nearest_quake_time) {
                                /* then compute distance */
                                double dist, azm;
                                geo_to_km(pQuake[i].lat, pQuake[i].lon, pQuake[j].lat, pQuake[j].lon, &dist, &azm);
                                /* debug */
                                logit("t", "Debug: simultaneous event, time-diff %5.2fs and dist %6.2f km\n",
                                                fabs(pQuake[j].t - pQuake[i].t), dist );
                                if (dist < nearest_quake_dist) {
                                        logit("t", "killing event, too close in time %5.2f < tolerance: %5.2fs and space to another by %6.2f km < tolerance %6.2f km\n",
                                                fabs(pQuake[j].t - pQuake[i].t),nearest_quake_time, dist, nearest_quake_dist);
                                         return(j);
                                }
                        }
                }
        }
        return(0);
}
