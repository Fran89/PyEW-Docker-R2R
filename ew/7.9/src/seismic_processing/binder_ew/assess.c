
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: assess.c 5967 2013-09-23 19:22:53Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/14 23:35:37  dietz
 *     modified to work with TYPE_PICK_SCNL messages only
 *
 *     Revision 1.2  2002/05/31 21:41:32  dietz
 *     changed to use logit instead of fprintf when processing config file
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

/*  assess.c         September 1996

    C functions to eliminate any outliers from a given set of picks.
    Converted to C by Lynn Dietz from a series of Splus functions
    written by Bill Ellsworth which in turn was based on Rex Allen's
    associator code.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <site.h>
#include "bind.h"
#include "grid.h"
#include "hyp.h"
#include "sample.h"

/* Function prototypes
 *********************/
void logit( char *, char *, ... );          /* logit.c sys-independent */
int assess_com( void );
int assess_pcks( HYP *, int, PIX * );
int assess_tcmp( const void *, const void * );
int ncombo( int, int );
int ingelada( int, double *, double *, double *, float, double * );
void   logit( char *, char *, ... );  /* logit.c  sys-independent  */
double median( int, double * );
double tt_resid( int, int, float );

#define ABS(X) (((X) >= 0) ? (X) : -(X))

#define MAXSTA      25
#define MAXTRIAL   500
#define SAMPLESIZE   4

/* Globals for assessing picks; all have default values!
 *******************************************************/
extern double GlitchNsec;           /* threshold difference between arrival*/
                                    /* times used to define glitch.        */
			            /* default = 0.03s; declared in grid.c */
extern int    GlitchMinPk;          /* min # of arrivals within interval   */
			            /* GlitchNsec used to remove a glitch. */
        		            /* default = 4;     declared in grid.c */
extern double zMin;                 /* min allowed hypcentral depth (hyp.c)*/
extern double zMax;                 /* max allowed hypcentral depth (hyp.c)*/
static int    LogAccept  = 0;       /* if 1, accepted picks are logged     */
static char   MaxWt      = '3';     /* max pick quality to use in resamplng*/
static int    MaxSta     = MAXSTA;  /* max # stations to use in resampling */
static int    MaxTrial   = MAXTRIAL;/* max # resampling trials to run      */
static float  V_hlfspace = 5.0;     /* halfspace P-wave velocity (km/s)    */
/* static double R_hlfspace = 100.0; */   /* max dist that V_hlfspace is valid   */
static float  MedianMax  = 3.0;     /* test value for median tt residual   */
static float  MADmax     = 10.0;    /* test value for median abs deviation */
			            /* from the median tt residual         */

/* Structure for storing arrival information
 *******************************************/
static struct  {
	double  x;        /* x coords w/respect to hypocenter    */
	double  y;        /* y coords w/respect to hypocenter    */
 	double  t;        /* arrival time                        */
 	int     index;    /* index of this pick in PIX structure */
        char    use;      /* intermediate flag                   */
} arrival[MAXSTA];

/* Hypocenters calculated from each combination of SAMPLESIZE picks
 ******************************************************************/
static double  eq_t[MAXTRIAL];   /* origin time */
static double  eq_x[MAXTRIAL];   /* x coords    */
static double  eq_y[MAXTRIAL];   /* y coords    */
static double  eq_z[MAXTRIAL];   /* depth       */


/********************************************************************
 * assess_com: Process assess commands from binder's config file    *
 ********************************************************************/
int assess_com( void )
{
   char *str;

   if(k_its("maxwt")) {
        str = k_str();
        if(!str) return 1;
        MaxWt =  str[0];
        if( MaxWt<'0' || MaxWt>'4' )
        {
            logit( "e","assess_com: Invalid max_wt argument (%c); exiting!\n",
                    MaxWt );
            exit( -1 );
        }
        return 1;
   }

   if(k_its("maxtrial")) {
        MaxTrial = k_int();
        if( MaxTrial > MAXTRIAL ) MaxTrial = MAXTRIAL;
        if( MaxTrial < 0        ) MaxTrial = 0;
        return 1;
   }

   if(k_its("v_halfspace")) {
        V_hlfspace = (float) k_val();
        return 1;
   }

   /*if(k_its("halfspace")) {
        V_hlfspace = (float) k_val();
        R_hlfspace = k_val();
        return 1;
   }*/

   if(k_its("residual_cut")) {
        MedianMax = (float) k_val();
        MADmax    = (float) k_val();
        return 1;
   }

   if(k_its("log_accepted")) {
        LogAccept = 1;
        return 1;
   }
   return 0;
}

/********************************************************************
 *  assess_pcks   Function to perform WLE's acceptance tests on a   *
 *                group of picks				    *
 *                                                                  *
 *  Returns the total number of picks that were rejected ('G','F')  *
 *          or -1*(#rejected picks)-1 if there were too few         *
 *          qualifying picks to complete the assessment.            *
 *                                                                  *
 *  If return >=0 a trial hypocenter is returned in HYP argument.   *
 *             <0 there were too few qualifying picks to complete   *
 *                the assessment; HYP argument remains unchanged.   *
 *                                                                  *
 *  Regardless of the return value, the PIX.flag of each arrival    *
 *  is set to one of these values:                                  *
 *     'T' if this arrival is associated with this event            *
 *     'F' if this arrival is NOT associated                        *
 *     'G' if the arrival is a member of a glitch                   *
 *     '?' if assess_pcks didn't make a ruling on this pick         *
 *                                                                  *
 ********************************************************************/
int assess_pcks( HYP *hyp, int npix, PIX *pix )
{
   MTX      combos;          /* matrix of samples (indices of arrivals)*/
   CART     eq, xyz;         /* Cartesian coords (x,y,z)               */
   GEO      geo;             /* geocentric coords (lat,lon,z)          */
   double   x[SAMPLESIZE];   /* x array to ship to locator, ingelada   */
   double   y[SAMPLESIZE];   /* y array to ship to locator, ingelada   */
   double   t[SAMPLESIZE];   /* t array to ship to locator, ingelada   */
   double   e[SAMPLESIZE];   /* array (hypocenter) returned by ingelada*/
   double   res[MAXTRIAL];   /* tt residual w/respect to each location */
   double   medres;          /* median residual for a station          */
   double   mad;             /* median absolute deviation from medres  */
   long     ntrial;          /* how many combinations of 4 will be used*/
   int      fullset;         /* 1 = do all possible combinations;      */
                             /* 0 = do MaxTrial random resamples       */
   int      nobs;            /* total # phases to use in resampling    */
   int      nkeep;           /* # phases that pass medres & mad tests  */
   int      nreject=0;       /* total # picks rejected (glitch, medres)*/
   int      pass=1;          /* =1 on first pass; =2 on second pass    */
   int      site;            /* index of station in site table         */
   int      i, i2, j;        /* miscellaneous indices                  */
   int      iarr, ip;        /* miscellaneous indices                  */

/* Sort picks in increasing arrival time
 ***************************************/
   qsort( pix, npix, sizeof(PIX), assess_tcmp );

/* Flag glitch picks
 *******************/
   if( GlitchMinPk != 0 )
   {
       i  = 0;
       i2 = GlitchMinPk - 1;
       while( i2 < npix ) {
          if( (pix[i2].t-pix[i].t) <= GlitchNsec )    /* found a glitch! */
          {
             for(j=i; j<=i2; j++)  pix[j].flag = 'G'; /* flag its picks  */
          }
          i++;  i2++;
       }
   }

/* Put the hypocenter into cartesian coordinates
 ***********************************************/
   geo.lat = (float)hyp->lat;
   geo.lon = (float)hyp->lon;
   geo.z   = (float)hyp->z;
   grid_cart( &geo, &eq );

/* Get the cartesian coordinates of the first MaxSta picks
   that meet all the testing criteria; flag all other picks
   with a '?' to show that they weren't used in resampling
 ***********************************************************/
   nobs = 0;
   for( i=0; i<npix; i++ )
   {
       site = pix[i].site;
       if( pix[i].flag == 'G' )           /* skip glitch picks      */
       {
           logit( "", "  assess P%d : %-5s %-3s %-2s %-2s  %5.2lf - Rejected (%c)\n",
                 pass, Site[site].name, Site[site].comp, 
                 Site[site].net, Site[site].loc, 
                 fmod(pix[i].t, 60.0), pix[i].flag );
           nreject++;
           continue;
       }
       if( pix[i].wt>MaxWt ||             /* skip low-quality picks  */
           pix[i].ph%2     ||             /* skip S-phases           */
           nobs >= MaxSta    )            /* already have max number */
       {
           pix[i].flag = '?';
           continue;
       }
       geo.lat = Site[site].lat;
       geo.lon = Site[site].lon;
       geo.z   = 0;
       grid_cart(&geo, &xyz);
       /*r = hypot( (double)(xyz.x-eq.x), (double)(xyz.y-eq.y) );*/
       /*if( r > R_hlfspace )  */
       /*{                     */            /* too distant for simple  */
       /*    pix[i].flag = '?';*/            /* velocity model to work  */
       /*    continue;         */
       /*}                     */
       arrival[nobs].x     = (double) xyz.x;
       arrival[nobs].y     = (double) xyz.y;
       arrival[nobs].t     = pix[i].t;
       arrival[nobs].index = i;
       arrival[nobs].use   = 1;
       nobs++;
   }

/**********************************************************************
   Now run the resampling associator of WLE:
	This is a two-pass procedure that uses the exact, 4-station
	hypocenter solution method of Ingelada and resampling to
	determine the station set that is consistent with an earthquake
	hypocenter.  The first pass uses all data and is used to
	winnow the data to arrivals with median residuals < MedianMax
	The second pass uses resampling of the winnowed data to
	determine the trial hypocenter.  The routine returns the trial
	hypocenter and a flag (in the PIX structure) indicating if the
	arrival is to be associated with it.
***********************************************************************/
resample:

   if( nobs < SAMPLESIZE ) return( -nreject-1 );

/* How many trials will we do?
 *****************************/
   fullset = 1;
   ntrial  = ncombo( nobs, SAMPLESIZE );
   if( ntrial > MaxTrial ) {
      ntrial  = MaxTrial;
      fullset = 0;
   }

/* Get all combinations of picks to use
 **************************************/
   combos.nr = combos.nc =0;
   combos.nmax = ntrial*SAMPLESIZE;
   combos.m    = (int *) malloc( combos.nmax*sizeof(int) );
   if( combos.m == (int *) NULL )
   {
      logit( "", "  assess_pcks : malloc for combos.m failed\n" );
      return( -nreject-1 );
   }
   if( fullset ) n_draw_p( nobs, SAMPLESIZE, &combos );
   else            sample( nobs, SAMPLESIZE, ntrial, &combos );
   if( combos.nr!=SAMPLESIZE || combos.nc!=ntrial )
   {
      if(fullset) logit( "", "  assess_pcks : error in n_draw_p\n" );
      else        logit( "", "  assess_pcks : error in sample\n"   );
      return( -nreject-1 );
   }

/* Calculate event hypocenter from each combination
 **************************************************/
   for( j=0; j<ntrial; j++ )
   {
      for( i=0; i<SAMPLESIZE; i++ ) {
         iarr = combos.m[IXMTX(i,j,ntrial)];
         x[i] = arrival[iarr].x;
         y[i] = arrival[iarr].y;
         t[i] = arrival[iarr].t;
      }
      if( ingelada( SAMPLESIZE, x, y, t, V_hlfspace, e ) < 0 )
          return( -nreject-1 );
      eq_x[j] = e[0];
      eq_y[j] = e[1];
      eq_t[j] = e[2];
      eq_z[j] = e[3];
   }
   free( (void *)combos.m );

/* For each arrival, calculate the traveltime residual for
   all locations, then find the median residual (medres) &
   the median absolute deviation from the median residual,
   and test each against the maximum allowed values.
 **********************************************************/
   nkeep  = 0;
   medres = 0.;
   mad    = 0.;

   for( i=0; i<nobs; i++ ) {                    /* For each pick, */
      ip   = arrival[i].index;
      site = pix[ip].site;
      for( j=0; j<ntrial; j++ ) {
         res[j] = tt_resid( i, j, V_hlfspace );    /* calculate all tt-residuals,*/
      }
      medres = median( ntrial, res );           /* find the median residual, */
      if( ABS(medres) >= (double)MedianMax ) { /* test it, */
          arrival[i].use = 0;
          pix[ip].flag  = 'F';
          nreject++;
          logit( "",
             "  assess P%d : %-5s %-3s %-2s %-2s  %5.2lf  nobs: %d  med: %.2lf - Rejected (%c)\n",
                pass, Site[site].name, Site[site].comp, 
                Site[site].net, Site[site].loc,  
                fmod(pix[ip].t, 60.0), nobs, medres, pix[ip].flag );
          continue;
      }
      if( pass==2 ) {                           /* and only on the 2nd pass */
         for( j=0; j<ntrial; j++ ) {
            res[j] = ABS(res[j]-medres);
         }
         mad = median( ntrial, res );           /* find the "mad" */
         if( mad >= (double)MADmax ) {          /* and test it too */
             arrival[i].use = 0;
             pix[ip].flag  = 'F';
             nreject++;
             logit( "",
               "  assess P%d : %-5s %-3s %-2s %-2s  %5.2lf  nobs: %d  med: %.2lf  mad: %.2lf - Rejected (%c)\n",
                  pass, Site[site].name, Site[site].comp,
                  Site[site].net, Site[site].loc,  
                  fmod(pix[ip].t, 60.0), nobs, medres, mad, pix[ip].flag );
             continue;
         }
      }

   /* Log arrivals that are accepted
    ********************************/
      if( LogAccept ) {
        logit( "",
         "  assess P%d : %-5s %-3s %-2s %-2s  %5.2lf  nobs: %d  med: %.2lf",
            pass, Site[site].name, Site[site].comp,
            Site[site].net, Site[site].loc,  
            fmod(pix[ip].t, 60.0), nobs, medres );
        if( pass==2 )  logit( "","  mad: %.2lf\n", mad );
        else           logit( "","\n" );
      }
      nkeep++;
   }

/* Decide if we need to do another pass or not
 *********************************************/
   if( nkeep < 4 )          /* not enough left for another pass, */
      return( -nreject-1 ); /* return now!                       */

   if( nkeep != nobs &&     /* not all picks were accepted AND */
       pass  == 1       )   /* we only finished the 1st pass;  */
   {                        /* so get rid of picks that failed */
      nkeep = 0;	    /* and do a 2nd pass               */
      for( i=0; i<nobs; i++ ) {
         if( arrival[i].use == 0 ) continue;
         arrival[nkeep] = arrival[i];
         nkeep++;
      }
      nobs = nkeep;
      pass = 2;
      goto resample;
   }

/* Return median hypocenter in HYP argument
 ******************************************/
   eq.x = (float) median( ntrial, eq_x );
   eq.y = (float) median( ntrial, eq_y );
   eq.z =  0.;
   grid_geo( &geo, &eq );
   hyp->lat = geo.lat;
   hyp->lon = geo.lon;
   hyp->t   = median( ntrial, eq_t );
   hyp->z   = median( ntrial, eq_z );
   if( hyp->z < zMin ) hyp->z = zMin;
   if( hyp->z > zMax ) hyp->z = zMax;

   return( nreject );

} /*end of assess_pcks*/

/****************************************************************
 *  assess_tcmp() compare 2 arrival times in a  pick list.      *
 ****************************************************************/
int assess_tcmp( const void *p1, const void *p2 )
{
        PIX *pck1;
        PIX *pck2;

        pck1 = (PIX *) p1;
        pck2 = (PIX *) p2;
        if(pck1->t < pck2->t)   return -1;
        if(pck1->t > pck2->t)   return  1;
        return 0;
}

/****************************************************************
 *  tt_resid()  function to compute traveltime residual to      *
 *              a station for homogeneous half space            *
 ****************************************************************/
double tt_resid( int iarr, int ieq, float v )
{
    double dx, dy, zsq;

    dx = arrival[iarr].x - eq_x[ieq];
    dy = arrival[iarr].y - eq_y[ieq];
    zsq = dx*dx + dy*dy + eq_z[ieq]*eq_z[ieq];

    return( arrival[iarr].t - sqrt(zsq)/(double)v - eq_t[ieq] );
}
