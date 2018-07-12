
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hyp.c 5967 2013-09-23 19:22:53Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/05/14 23:35:37  dietz
 *     modified to work with TYPE_PICK_SCNL messages only
 *
 *     Revision 1.2  2002/05/31 21:45:17  dietz
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

/*
 * hyp.c : Location routines
 */
/*********************C O P Y R I G H T   N O T I C E ***********************/
/* Copyright 1991 by Carl Johnson.  All rights are reserved. Permission     */
/* is hereby granted for the use of this product for nonprofit, commercial, */
/* or noncommercial publications that contain appropriate acknowledgement   */
/* of the author. Modification of this code is permitted as long as this    */
/* notice is included in each resulting source module.                      */
/****************************************************************************/
#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>    /* for calloc */
#include <kom.h>
#include <chron3.h>    /* for date18 */
#include <site.h>
#include <tlay.h>
#include "nrutil.h"
#include "hyp.h"

#define DEGRAD 0.01745329
#define ABS(X) (((X) >= 0) ? (X) : -(X))
#define X(lon) (facLon * ((lon) - orgLon))
#define Y(lat) (facLat * ((lat) - orgLat))
#define LAT(y) (orgLat + (y) / facLat)
#define LON(y) (orgLon + (y) / facLon)
#define Panic(x) (logit("e","Panic point %d in hyp.c\n",(x)),exit(-1))
#define hypot(x,y) (sqrt((x)*(x) + (y)*(y)))

/* Function prototypes
 *********************/
void logit( char *, char *, ... );
double median( int, double * );
double wmedian( int, double *, double * );
int l1( int, int, float **, float *, double, float * );

#define HYP_OK          0
#define HYP_INCREASE    1
#define HYP_SHALLOW     2
#define HYP_DEEP        3

#define MAXPIX 250
HYP tstHyp;

/* 941207:bfh. Hypocentral XY motion constrained to externally read in value (km.) */
double maxXYstep = 10.0;
double maxZstep  =  2.0;

/* 941216:bfh. Maximum allowable XYZ motion for hypoc to be considered "stable".  */
double minXYZstep = 0.1;

/* 941207:bfh. Read in Maximum allowable number of iterations in from binder.d     */
static int maxIter = 4;

/* 941207:bfh. Read in Maximum allowable change in rms for "convergence" to hold.  */
double maxDeltaRms =  1.01;

/* 950205:bfh. Read in Maximum allowable dmin for free soln, h to fix at otherwise.    */
double dmin_free   = 50.0;
double depth_fixed =  8.0;

double delZ = 0.01;     /* 941217:bfh. */
double zMin = 2.0;
double zMax = 20.0;
static int maxPix = MAXPIX;
static int initMem = 0;
static unsigned int Code; /* Shared by l1, solve, and check               */
static int Iter;          /*   ..                                         */
static int nPix;          /*   ..                                         */
static float **a;         /* Coefficient matrix                           */
static float **c;         /* Coefficient matrix for fixed depth solutions */
static float *b;          /* Vector of weighted residuals                 */
static float *x;          /* Solution vector                              */

/* Coordinate conversion factors
 *******************************/
static double orgLat;           /* Network origin                       */
static double orgLon;
static double facLat;           /* Number of km per degree of lat       */
static double facLon;           /* Number of km per degree of lon       */
typedef struct {
        double x;
        double y;
        double z;
} HYPSITE;
HYPSITE *hypSite;

/*************************************************************************
 * hyp_com()  Process all recognized commands                            *
 *************************************************************************/
int hyp_com( void )
{
        if(k_its("zrange")) {
                zMin = k_val();
                zMax = k_val();
                return 1;
        }

/*941207:bfh.----------------------------------------------------------------------*/
        if(k_its("maxstep") || k_its("MaxStep")) {   /* changed command name 950501:ldd.*/
                maxXYstep = k_val();
                maxZstep  = k_val();
                return 1;
        }
        if(k_its("maxiter") || k_its("MaxIter")) {
                maxIter = k_int();
                return 1;
        }
        if(k_its("maxdeltarms") || k_its("MaxDeltaRms")) {
                maxDeltaRms = k_val();
                return 1;
        }
/*---------------------------------------------------------------------------------*/

/*941216:bfh.----------------------------------------------------------------------*/
        if(k_its("minxyzstep") || k_its("MinXYZstep")) {
                minXYZstep = k_val();
                return 1;
        }
/*---------------------------------------------------------------------------------*/

/*950205:bfh.----------------------------------------------------------------------*/
        if(k_its("fixdepth") || k_its("FixDepth")) {
                dmin_free = k_val();
                depth_fixed  = k_val();
                return 1;
        }
/*---------------------------------------------------------------------------------*/

        if(k_its("maxpix")) {
                maxPix = k_int();
                if(maxPix > MAXPIX)
                        maxPix = MAXPIX;
                return 1;
        }

        return 0;
}

/*************************************************************************
 *  hyp_wt()  Return floating-pt pick-weight to use in location process  *
 *************************************************************************/
double hyp_wt(PIX *pix)
{
        double w;

        w = 0.0001;
        if(pix->wt100 > 0)
                w = 0.01 * pix->wt100;
        return w;
}

/*
 *  hyp_cmp()  Compare values of two variables
 */
int hyp_cmp(const void *x1, const void *x2)
{
        if(*(double *)x1 < *(double *)x2)   return -1;
        if(*(double *)x1 > *(double *)x2)   return 1;
        return 0;
}

/*************************************************************************
 * hyp_init()  Initialize site table, network origin and scaling factors *
 *************************************************************************/
void hyp_init( void )
{
        int i;

        if(initMem == 0) {
                initMem = 1;
                a = matrix(1, maxPix+2, 1, 6);
                c = matrix(1, maxPix+2, 1, 5);
                b = vector(1, maxPix);
                x = vector(1, 4);
                hypSite = (HYPSITE *)calloc(nSite, sizeof(HYPSITE));
                orgLat = 0.0;
                orgLon = 0.0;
                for(i=0; i<nSite; i++)
                {
                   orgLat += Site[i].lat;
                   orgLon += Site[i].lon;
                }

        /* Calculate network origin and scaling factors only once */
                orgLat /= nSite;
                orgLon /= nSite;
                facLat = (double)(40000.0 / 360.0);
                facLon = facLat * cos(6.283185 * orgLat / 360.0);
                for(i=0; i<nSite; i++)
                {
                   hypSite[i].x = X(Site[i].lon);
                   hypSite[i].y = Y(Site[i].lat);
                   hypSite[i].z = Site[i].elev;
                }
        }
        return;
}

/*************************************************************************
 *  hyp_check()  Set up location matrix                                  *
 *************************************************************************/
int hyp_check(HYP *hyp, PIX *pix, char *txt)
{
        int ipix;
        int is;
        int ph;
        int kres;
        double r;
        double z;
        double dtdr;
        double dtdz;
        double w;
        double az;
        double res;
        double rms;
        double xmed;
        static double srt[MAXPIX];  /* Changed to static by WMK 2/12/96 */
        static double wt[MAXPIX];
        double sum;
        double wsum;
        char chr;

        hyp->dmin = 10000.0;
        for(ipix=0; ipix<nPix; ipix++) {
                is = pix[ipix].site;
                r = hypot(hypSite[is].x - hyp->lon, hypSite[is].y - hyp->lat);
                if(r < hyp->dmin)
                        hyp->dmin = r;
                w = hyp_wt(&pix[ipix]);
                z = hyp->z;
                ph = pix[ipix].ph;
                res = pix[ipix].t - hyp->t - t_phase(ph, r, z, &dtdr, &dtdz);
                srt[ipix] = res;
                wt[ipix] = w;
                az = atan2(hypSite[is].x-hyp->lon, hypSite[is].y-hyp->lat);
                a[ipix+1][1] =   w;
                a[ipix+1][2] = - w * dtdr * sin(az);
                a[ipix+1][3] = - w * dtdr * cos(az);
                a[ipix+1][4] =   w * dtdz;
        }
        xmed = wmedian(nPix, srt, wt);
        sum = 0.0;
        wsum = 0.0;
        hyp->t += xmed;
        for(ipix=0; ipix<nPix; ipix++) {
                w = wt[ipix];
                res = srt[ipix] - xmed;
                b[ipix+1] = w * res;
                pix[ipix].res = res;
                sum += w * ABS(res);
                wsum += w;
        }
        if(wsum < 0.001)
                wsum = 0.001;
        rms = sum / wsum;
        kres = HYP_OK;
/*      if(rms > hyp->rms)                           941206:bfh.        */
        if(rms > maxDeltaRms * hyp->rms)        /*   941206:bfh.        */
                kres = HYP_INCREASE;
        if(hyp->z > zMax)
                kres = HYP_DEEP;
        if(hyp->z < zMin)
                kres = HYP_SHALLOW;
        chr = ' ';
        if(kres != HYP_OK)
                chr = '*';
        if(Code & 1)
                logit( "",
                       "  iter : %2d %c%-12s %5.2f%9.4f%10.4f%7.2f\n",
                       Iter, chr, txt, rms,
                       LAT(hyp->lat), LON(hyp->lon), hyp->z);
        hyp->rms = rms;
        return kres;
}

/*************************************************************************
 *  hyp_solve()  Locate earthquake                                       *
 *************************************************************************/
int hyp_solve(int fix, HYP *hyp)
{
        int    ipix;
        double eps;
        double delXY;                                   /* 950201:bfh. */
        double delXYZ;                                  /* 950201:bfh. */
        float  damp;                                    /* 950501:ldd. */
        float  dampz;                                   /* 950501:ldd. */
        int    i;                                       /* 950501:ldd. */

        eps = 1.0e-3;
        if(fix) {
                for(ipix=0; ipix<nPix; ipix++) {
                        c[ipix+1][1] = a[ipix+1][1];
                        c[ipix+1][2] = a[ipix+1][2];
                        c[ipix+1][3] = a[ipix+1][3];
                }
                l1(nPix, 3, c, b, eps, x);
                x[4] = 0.0;
        } else {
                l1(nPix, 4, a, b, eps, x);
        }

/*950201:bfh.--------------------------------------------------------------------------*/
        delXY  = sqrt( (x[2]*x[2]) + (x[3]*x[3]) );
        delXYZ = sqrt( (x[2]*x[2]) + (x[3]*x[3]) + (x[4]*x[4]) );

/*  TERMINATE IF HYPOCENTER IS STABLE IN XYZ SPACE:                                   */
        if ( delXYZ < minXYZstep ) {
           hyp->t   += x[1];
           hyp->lon += x[2];
           hyp->lat += x[3];
           hyp->z   += x[4];
           return 2;
        }

/*  CONSTRAIN XYZ MOTION TO AN EXTERNALLY CONTROLLABLE AMOUNT:             950201:bfh.*/
/*      if ( delXY > maxXYstep ) {                      */    /*commented  950501:ldd.*/
/*              x[2] = (maxXYstep/delXY) * x[2];        */    /*see changes below     */
/*              x[3] = (maxXYstep/delXY) * x[3];        */
/*      }                                               */
/*      if (ABS(x[4]) > maxZstep ) {                    */
/*              if  (x[4] > 0.0) x[4] =   maxZstep;     */
/*              else             x[4] = - maxZstep;     */
/*      }                                               */

/*  Changed XYZ constraint to preserve direction of 4-D adjustment vector. 950501:ldd.*/
        damp = 1.0;
        if ( delXY > maxXYstep )
                damp  = (float) ( maxXYstep/delXY );

        if ( ABS(x[4]) > maxZstep ) {
                dampz = (float) ( maxZstep/ABS(x[4]) );
                if ( dampz < damp )  damp  = dampz;
        }
        if ( damp < 1.0 ) {
                for (i=1; i<5; i++)  x[i] *= damp;
        }
/*-------------------------------------------------------------------------------------*/

        hyp->t   += x[1];
        hyp->lon += x[2];
        hyp->lat += x[3];
        hyp->z   += x[4];
        return 1;
}

/*************************************************************************
 *  hyp_pau()  Finished locating; calculate things like ravg and dmin    *
 *************************************************************************/
int hyp_pau(HYP *hyp, PIX *pix)
{
        int ipix;
        int is;
        int ph;
        int navg;
        int i;
        double r;
        double z;
        double dtdr;
        double dtdz;
        double w;
        double res;
        double rms;
        double xmed;
        static double srt[MAXPIX+1];    /* Changed to static by WMK 2/12/96 */
        static double wt[MAXPIX];
        double sum;
        double wsum;
        double dmin;
        double ravg;
        double gap;

        navg = 0;
        ravg = 0.0;
        dmin = 10000.0;
        gap = 0.0;

        for(ipix=0; ipix<nPix; ipix++) {
                wt[ipix] = hyp_wt(&pix[ipix]);
                is = pix[ipix].site;
                r  = hypot(hypSite[is].x - hyp->lon, hypSite[is].y - hyp->lat);

                if(wt[ipix] > 0.0001) {        /* 950501:ldd.  Enclosed 2 lines in */
                        ravg += r;             /* if{} to exclude non-weighted     */
                        navg++;                /* picks from ravg calculations.    */
                }                              /* As per Carl, corrects oversight. */
                if(r < dmin)  dmin = r;
                z   = hyp->z;
                ph  = pix[ipix].ph;
                res = pix[ipix].t - hyp->t - t_phase(ph, r, z, &dtdr, &dtdz);
                srt[ipix] = res;
        }
        xmed = wmedian(nPix, srt, wt);
        sum = 0.0;
        wsum = 0.0;
        hyp->t += xmed;
        for(ipix=0; ipix<nPix; ipix++) {
                is = pix[ipix].site;
                w = wt[ipix];
                pix[ipix].res = srt[ipix] - xmed;
                sum += w * ABS(pix[ipix].res);
                wsum += w;
                srt[ipix] = atan2(hypSite[is].x - hyp->lon,
                        hypSite[is].y - hyp->lat) / DEGRAD;
        }
        if(wsum < 0.001)
                wsum = 0.001;
        rms = sum / wsum;
        if(navg > 0) ravg /= navg;           /* 960425:ldd. Added if-else to avoid */
        else         ravg  = dmin;           /* divide-by-0 on eqs far outside net.*/
        qsort(srt, nPix, sizeof(srt[0]), hyp_cmp);
        srt[nPix] = srt[0] + 360.0;
        for(i=0; i<nPix; i++) {
                if(srt[i+1] - srt[i] > gap)
                        gap = srt[i+1] - srt[i];
        }
        hyp->rms = rms;
        hyp->dmin = dmin;
        hyp->ravg = ravg;
        hyp->gap = gap;
        hyp->nph = nPix;

        return 0;
}

/*************************************************************************
 * hyp_l1()  Supervise event location                                    *
 *************************************************************************/
int hyp_l1( int code, HYP *hyp, int npix, PIX *pix )
{
        HYP hyp1, hyp2;
        int ires;
        int lres;                                         /* 941216:bfh */
        int fixed_flag = 0;                               /* 950209:bfh */
        int nhalf;

        hyp_init();
        Code = (unsigned)code;
        hyp1 = *hyp;
        hyp1.rms = 10000.0;
        hyp1.lat = Y(hyp1.lat);
        hyp1.lon = X(hyp1.lon);
        if(npix > MAXPIX)
                npix = MAXPIX;
        nPix = npix;
        Iter = 0;

        hyp_check(&hyp1, pix, "Start");

        if( hyp1.dmin > dmin_free ){                            /* 950209:bfh */
                fixed_flag = 1;                                 /* 950209:bfh */
                if( hyp1.z != depth_fixed ) {                   /* 950428:ldd */
                        hyp1.z = depth_fixed;                   /* 950209:bfh */
                        hyp_check(&hyp1, pix, "Default Z");     /* 950428:ldd */
                }                                               /* 950428:ldd */
        }                                                       /* 950209:bfh */

/*CARLS CODE: REPLACED WITH SUBSEQUENT BLOCK OF CHANGES DATED 950201:bfh.--------------*/
/*iter:                                                                                */
/*      Iter++;                                                                        */
/*      hyp2 = hyp1;                                                                   */
/*      hyp_solve(0, &hyp2);                                                           */
/*      ires = hyp_check(&hyp2, pix, "Free");                                          */
/*      switch(ires) {                                                                 */
/*      case HYP_OK:                                                                   */
/*              goto update;                                                           */
/*      case HYP_INCREASE:                                                             */
/*              goto half;                                                             */
/*      case HYP_SHALLOW:                                                              */
/*              s = 0.9 * (hyp1.z - zMin) / (hyp1.z - hyp2.z);                         */
/*              break;                                                                 */
/*      case HYP_DEEP:                                                                 */
/*              s = 0.9 * (zMax - hyp1.z) / (hyp2.z - hyp1.z);                         */
/*              break;                                                                 */
/*      }                                                                              */
/*      hyp2.t = hyp1.t + s * (hyp2.t - hyp1.t);                                       */
/*      hyp2.lon = hyp1.lon + s * (hyp2.lon - hyp1.lon);                               */
/*      hyp2.lat = hyp1.lat + s * (hyp2.lat - hyp1.lat);                               */
/*      hyp2.z = hyp1.z + s * (hyp2.z - hyp1.z);                                       */
/*      hyp2.rms = hyp1.rms;                                                           */
/*      ires = hyp_check(&hyp2, pix, "Damped");                                        */
/*      if(ires == HYP_INCREASE)                                                       */
/*              goto half;                                                             */
/*                                                                                     */
/* Fixed depth step from damped solution                                               */
/*      hyp1 = hyp2;                                                                   */
/*      hyp_solve(1, &hyp2);                                                           */
/*      ires = hyp_check(&hyp2, pix, "Fixed Z");                                       */
/*      if(ires == HYP_INCREASE)                                                       */
/*              goto half;                                                             */
/*      goto update;                                                                   */
/*-------------------------------------------------------------------------------------*/

/*950201:bfh.-------------------------------------------BEGINNING of NEW BLOCK:--------*/
/*950427:ldd.------------------------------------------ variation on the bfh block-----*/
iter:
        Iter++;
        hyp2 = hyp1;
        if(fixed_flag)   goto fixed;

        lres = hyp_solve(0, &hyp2);
/*      if (lres == 2)   goto update; */          /*commented 950427:ldd.*/

        ires = hyp_check(&hyp2, pix, "Free");
        switch(ires) {
        case HYP_OK:
                goto update;
        case HYP_INCREASE:
/*              break; */                        /*commented 950427:ldd.*/
                goto half;                       /*added     950427:ldd.*/
        case HYP_SHALLOW:
        case HYP_DEEP:
                hyp_check(&hyp1, pix, "ReStart");         /* 950427:ldd.*/
                hyp2 = hyp1;                              /* 950427:ldd.*/
                break;
        }

fixed:
        lres = hyp_solve(1, &hyp2);
/*      if (lres == 2)   goto update; */         /*commented 950427:ldd.*/

        ires = hyp_check(&hyp2, pix, "Fixed Z");
        if(ires == HYP_INCREASE)
                goto half;
        goto update;

/*950201:bfh.-------------------------------------------END of NEW BLOCK.--------------*/

half:
        nhalf = 3;
half2:
        hyp2.rms = hyp1.rms;
        hyp2.t   = 0.5 * (hyp1.t + hyp2.t);
        hyp2.lat = 0.5 * (hyp1.lat + hyp2.lat);
        hyp2.lon = 0.5 * (hyp1.lon + hyp2.lon);
        hyp2.z   = 0.5 * (hyp1.z + hyp2.z);
        ires = hyp_check(&hyp2, pix, "Half");
        if(ires == HYP_OK)
                goto update;
        nhalf--;
        if(nhalf > 0)
                goto half2;

update:
        hyp1 = hyp2;    /* save new location as start for next Iter */

        if(lres == 2)                         /* added  941216:bfh. */
                goto done;                    /* added  941216:bfh. */

/*      if(Iter < 4)         */                      /* 941207:bfh. */
        if(Iter < maxIter)                           /* 941207:bfh. */
                goto iter;

done:
        if(Iter) {
                hyp_pau(&hyp1, pix);
                hyp1.lat = LAT(hyp1.lat);
                hyp1.lon = LON(hyp1.lon);
                *hyp = hyp1;
        }
        return (Iter);
}

/*************************************************************************
 *  hyp_out()  Print some stuff to the log file                          *
 *************************************************************************/
double hyp_out( int code, int serial, HYP *hyp, int npix, PIX *pix)
{
        double dtdr, dtdz;
        double hyp_x;
        double hyp_y;
        int ipix;
        int is;
        double r;
        double z;
        double res;
        double sum, rms;
        double wsum;
        static char cdate[24];    /* changed to static by WMK 2/12/96 */
        int mins;
        double tsec;
        int iph;
        int m;
        double w;
        unsigned int COde;

        COde = (unsigned)code;
        hyp_x = X(hyp->lon);
        hyp_y = Y(hyp->lat);
        date20(hyp->t, cdate);
        if(COde & 1) {
                logit( "t", "  hyp : %4d %18s %8.4f %9.4f %6.2f%4d\n",
                        serial, cdate, hyp->lat, hyp->lon, hyp->z,
                        npix);
                logit( "", "  par : dmin = %.1f, ravg = %.1f, gap = %.1f\n",
                        hyp->dmin, hyp->ravg, hyp->gap);
        }

        sum = 0.0;
        wsum = 0.0;
        m = npix;
        if(m > maxPix)
                m = maxPix;
        for(ipix=0; ipix<npix; ipix++) {
                is = pix[ipix].site;
                r = hypot(hypSite[is].x - hyp_x, hypSite[is].y - hyp_y);
                w = hyp_wt(&pix[ipix]);
                z = hyp->z;
                mins = pix[ipix].t / 60.0;
                tsec = pix[ipix].t - 60.0 * mins;
                iph = pix[ipix].ph;
                res = pix[ipix].t - hyp->t - t_phase(iph, r, z, &dtdr, &dtdz);
                if(COde & 2) {
                    logit( "",
                           "  pck : %-5s %-3s %-2s %-2s %-2s%c%c%c%6.2f %6.2f%6.2f <%4.2f> %s\n",
                           Site[is].name, Site[is].comp, 
                           Site[is].net, Site[is].loc, 
                           Phs[(int)pix[ipix].ph], 
			   pix[ipix].ie, pix[ipix].wt, pix[ipix].fm,
                           r, tsec, res, w, pix[ipix].dup?"dup":" ");
                }
                if(ipix < m) {
                        wsum += w;
                        sum += w * ABS(res);
                }
        }
        if(wsum < 0.001)
                wsum = 0.001;
        rms = sum / wsum;
        if(COde & 4) {
                logit( "", "  rms : %6.2f, dmin = %6.2f\n", rms, hyp->dmin);
        }
        return (rms);
}

/*************************************************************************
 * hyp_res()  Calculate the travel-time residual for a single pick       *
 *************************************************************************/
double hyp_res(HYP *qk, PIX *pk)
{
        int is;
        double r, z;
        double res;
        double dtdr, dtdz;

        hyp_init();
        is = pk->site;
        r = hypot(hypSite[is].x - X(qk->lon),
                hypSite[is].y - Y(qk->lat));
        z = qk->z;
        res = pk->t - qk->t - t_phase(pk->ph, r, z, &dtdr, &dtdz);
        return res;
}

/*************************************************************************
 * hyp_dis()  Calculate the epicentral distance of a single pick         *
 *************************************************************************/
double hyp_dis(HYP *qk, PIX *pk)
{
        int is;
        double r;

        hyp_init();
        is = pk->site;
        r = hypot(hypSite[is].x - X(qk->lon),
                hypSite[is].y - Y(qk->lat));
        return r;
}

/******************************************************
 * hyp_free()  free previously calloc'd memory        *
 ******************************************************/
void hyp_free( void )
{
        free( (void *)hypSite );
        return;
}
