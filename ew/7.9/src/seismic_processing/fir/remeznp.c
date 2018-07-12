
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: remeznp.c 6326 2015-05-01 02:42:48Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/* Code for designing an N-Pass optimal equiripple FIR filter using 
 * the Remez Exchange algorithm.
 *
 * remeznp:
 * Inputs:
 *        nbands: number of filter Stop and Pass bands 
 *    freq_edges: Array of frequency band edges, normalized to sampling freq
 *        levels: Array of levels at each band edge
 *          devs: Array of allowed ripple; one for each band
 *
 * Outputs:
 *  pCoeffs: pointer to array of filter coefficients
 *  pLength: pointer to number of filter coefficients
 *
 * Returns:
 *   EW_SUCCESS if all went well; otherwise EW_FAILURE
 *
 * Coagulated by: Pete Lombard; November, 1999
 */
#include <stdio.h>
#include <stdlib.h>   /* for malloc */
#include <math.h>
#include "earthworm.h"

#define MAX_FILTER_LENGTH 1000  /* Largest reasonable number of elements*/
/* #define DEBUG */

/* internal function prototypes */
int remlplen(double, double, double, double);
int remezgrid(int, int, double *, int, int, int, double **, int *);
int remezfrf(double *, int, double *, int, double *, double *, double **, 
             double **);
int remezf(int, int, int, int, double *, double *, double *, double **, 
           double *);
void remez(double, double *, double *, double *, double *, double *, double *,
            double *, int *, int, int, double *, double *, double *, double *,
           int);
double dd(int, int, int, double *);
double geed(int, int, double *, double *, double *, double, double *);

int remeznp( int nbands, double *freq_edges, double *levels, double *devs,
             double *weights, int *pLength, double **pCoeffArray, int verbose )
{
  double *grid, *des, *wt;
  double err, max_dev, min_dev;
  double deps = 1.0e-10;
  int nFreq;
  int nFilt, nGrid, lGrid, neg = 0l, nodd = 0l;
  int maxFilt = 0l;
  int trial = 0;
  int inc = 4;
  int i;
  
  grid = NULL;
  des = NULL;
  wt = NULL;
  
  if (levels[2 * nbands - 1] > 0.0)
  {
    nodd = 1;  /* High-pass filters must have odd length; (even order) */
  }
  
  /* Estimate filter length for each combination of low-and high-pass */
  if (nbands == 2)
  {
    if (levels[0] == 1.0)   /* Low-pass */
      nFilt = remlplen(freq_edges[1], freq_edges[2], devs[0], devs[1]);
    else    /* High-pass */
      nFilt = remlplen(freq_edges[2], freq_edges[1], devs[1], devs[0]);
  }
  else
  {
    for (i = 1; i < nbands - 1; i++)
    {
      nFilt = remlplen(freq_edges[(i-1)*2+1], freq_edges[i*2], devs[i-1], 
                       devs[i]);
      if (nFilt > maxFilt) maxFilt = nFilt;
      nFilt = remlplen(freq_edges[i*2+1], freq_edges[(i+1)*2],
                       devs[i], devs[i+1]);
      if (nFilt > maxFilt) maxFilt = nFilt;
    }
    nFilt = maxFilt;  /* Take the worst case */
  }
#ifdef DEBUG
  logit("", "raw nFilt: %d\n", nFilt);
#endif
  
  if (nFilt < 3)
  {
    logit("e", "remeznp: invalid filter length (=%d); must be >= 3\n", nFilt );
    return EW_FAILURE;
  }
  
  if (nFilt % 2 != nodd) nFilt ++;
  nFreq = 2 * nbands;

  /* Find dev limits for error test */
  max_dev = 0.0;
  min_dev = 1.0;
  for (i = 0; i < nbands; i++)
  {
    if (devs[i] > max_dev) max_dev = devs[i];
    if (devs[i] < min_dev) min_dev = devs[i];
  }
  
  if (nFilt > MAX_FILTER_LENGTH)
  {
    logit("e", "Maximum filter length exceeded. It is unlikely that a\n");
    logit("e", "\tsuitable filter can be designed with the requested Bands.\n");
    logit("e", "\tTry relaxing some of the Band criteria and run fir again\n");
    return EW_FAILURE;
  }
  
  /* Main loop to interate for acceptable filter */
  while ( trial < 5 )
  {
    /* Generate the Remez grid */
    lGrid = 16;
    if (remezgrid(nFilt, lGrid, freq_edges, nFreq, neg, nodd, &grid, &nGrid)
        != EW_SUCCESS)
      return EW_FAILURE;
    
    while ( nGrid < nFilt )
    {
      lGrid *= 4;
      if (remezgrid(nFilt, lGrid, freq_edges, nFreq, neg, nodd, &grid, &nGrid) 
          != EW_SUCCESS)
        return EW_FAILURE;
    }
    
    /* Generate the frequency response function on the grid */
    if (remezfrf(freq_edges, nbands, grid, nGrid, weights, levels, &des, &wt)
        != EW_SUCCESS)
      return EW_FAILURE;
    
#ifdef DEBUG
    for (i = 0; i < nGrid; i++)
      logit("", "%d, %e, %e, %e\n", i, grid[i], des[i], wt[i]);
#endif    
    
    /* Generate the filter coefficents (half of them since filter is symmetric */
    remezf( nFilt, neg, nbands, nGrid, grid, des, wt, pCoeffArray, &err );
    free( grid);
    grid = 0;
    free( des );
    des = 0;
    free( wt );
    wt = 0;
    
#ifdef DEBUG
    logit("", "nFilt: %d\n", nFilt);
    for (i = 0; i < (nFilt + 1)/2; i++)
      logit("", "%d, %lf\n", i, (*pCoeffArray)[i]);
 
    logit("", "err: %lf\n", err);
#endif
        
   
    /* Evaluate the filter: this is a pretty easy test. Anything more */
    /*  rigorous would take quite a bit of work.                      */
    
    if (err < 0.0) err = -err;
    if (verbose)
      logit("e", "trial %d: filter length: %d max deviation: %lf\n", trial,
            nFilt, err);
    if (err <= max_dev)
    {
      *pLength = nFilt;
      return EW_SUCCESS;
    } 
    else  /* not good enough; try a slightly longer filter */
    {
      free( *pCoeffArray );
      nFilt += inc;
      if (inc < 8) inc *= 2;
      trial++;
    }
    
  }
 
  logit("e", "remeznp: failed to generate adequate filter of size %d\n", 
        nFilt );
  return EW_FAILURE;
}

/* remlplen: FIR lowpass filter Length estimator */

/*   L = remlplen(freq1, freq2, dev1, dev2) */

/*   input: */
/*     freq1: passband cutoff freq (normalized to sampling freq) */
/*     freq2: stopband cutoff freq (normalized to sampling freq) */
/*     dev1:  passband ripple (DESIRED) */
/*     dev2:  stopband attenuation (not in dB) */

/*   output: */
/*     L = filter Length (# of samples) */
/*         NOT the order N, which is N = L-1 */

/*   NOTE: Will also work for highpass filters (i.e., f1 > f2) */
/*         Will not work well if transition zone is near f = 0, or */
/*         near f = fs/2 */

/*   References: */
/*     Proakis and Manolakis, Digital Signal Processing (1996) pp. 808-809 */
int remlplen(double freq1, double freq2, double delta1, double delta2)
{
  int ret_val;
  double D, f, d1, d2, df;

  d1 = log10(delta1);
  d2 = log10(delta2);
  D = (d1 * 0.005309 * d1 + d1 * 0.07114 - 0.4761) * d2 
    - (d1 * 0.00266 * d1 + d1 * 0.5941 + 0.4278);
  df = fabs(freq2 - freq1);
  f = (d1 - d2) * 0.51244 + 11.01217;
  ret_val = (int) (D / df - f * df + 1.5);
  return ret_val;
} /* remlplen */

/* remezgrid: generate frequency grid on which to perform Remez Exchange */
/*            Iteration. */
int remezgrid(int nfilt, int lgrid, double *ff, int nf, int neg, int nodd, 
               double **pGrid, int *ngrid)
{
  double *grid, delf;
  int j, l, nfcns, chunk;
  double gr, fup;

  /*  INPUTS: */
  /*     nfilt: number of filter elements */
  /*     lgrid: grid density */
  /*        ff: array of frequency band edges */
  /*        nf: number of frequency edges */
  /*       neg: == 1 ==> antisymmetric impulse response */
  /*            == 0 ==> symmetric impulse response */
  /*      nodd: == 1 ==> filter length is odd */
  /*            == 0 ==> filter length is even */
  /*  OUTPUTS: */
  /*      grid: grid (array) of frequencies */
  /*     ngrid: number of grid values */

  /* NOTE: Frequencies are normalized to sampling frequency. */
  /*     Thus, max(ff) = 0.5, not 1.0 */

  /* Grab a chunk for the frequency grid array */
  if (*pGrid != 0 )
    free (*pGrid);
  chunk = lgrid * (nfilt / 2 + 4) * sizeof(double);
  if ( (*pGrid = (double*) malloc(chunk)) == NULL )
  {
    logit("e", "remezgrid: error allocating memory\n");
    return EW_FAILURE;
  }
  grid = *pGrid;

  /* Function Body */
  nfcns = nfilt / 2;
  if (nodd == 1 && neg == 0) {
    ++nfcns;
  }
  grid[0] = ff[0];
  delf = 0.5 / (lgrid * nfcns);
  /* If value at frequency 0 is constrained, make sure first grid point */
  /* is not too small: */
  if (neg != 0 && grid[0] < delf)
    grid[0] = delf;

  j = 0;
  l = 0;
  /* Start main loop */
  while (l < nf)
  {
    fup = ff[l+1];
    gr = grid[j];
    /* Grid is closely spaced within each frequency bad */
    while ( (gr += delf) < fup + delf)
      grid[++j] = gr;

    /* The last interval must not be less than delf */
    grid[j] = fup;

    l += 2;
    if (l < nf)
      /* The grid jumps to the start of the next band */
      grid[++j] = ff[l];
      
  }

  *ngrid = j;

  /*  If value at frequency 1 is constrained, remove that grid point: */
  if (neg == nodd && grid[*ngrid] > 0.5 - delf) 
  {
    if (ff[nf - 2] < 0.5 - delf) 
    {
      --(*ngrid);
    } 
    else
    {
      grid[(*ngrid)-1] = ff[nf - 2];
    }
  }
  return EW_SUCCESS;
} /* remezgrid */

/* Generate frequency response function on the grid */
int remezfrf(double *f, int nb, double *gf, int ng, double *w, double *a, 
              double **pDes, double **pWt)
{
  double *gh, *gw, slope;
  double deps = 1.0e-10;  /* A small number, nearly zero */
  int j, l;

/*   INPUTS: */
/*      F: vector of band edges (frequencies) */
/*     NB: number of bands (length of W, A; half length of F) */
/*     GF: vector of interpolated grid frequencies */
/*     NG: number of grid frequencies */
/*      W: vector of weights, one per band */
/*      A: vector of amplitudes of desired frequency response at band edges F */
/*   OUTPUTS: */
/*     GH: vector of desired filter response (magnitude) */
/*     GW: vector of weights (positive) */

/* NOTE 1: GH(GF) and GW(GF) are specified as functions of frequency GF */
/* NOTE 2: Frequencies (F and GF) can be normalized to any reference as */
/*         long as the references are the same. */

  /* Grab some hunks for the frequency and weight arrays */
    if ( (*pDes = (double*) malloc(ng * sizeof(double))) == NULL )
  {
    logit("e", "remezfrf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( *pWt = (double*) malloc(ng * sizeof(double))) == NULL )
  {
    logit("e", "remezfrf: error allocating memory\n");
    return EW_FAILURE;
  }
  gh = *pDes;
  gw = *pWt;
  
  j = 0;
  for (l = 0; l < nb; l++)
  {
    if (j > ng) 
      return EW_SUCCESS;
    
    while (gf[j] < f[2 * l]) 
    {
      /* A transition band */
      gh[j] = 0.0;
      gw[j] = 0.0;
      ++j;
    }
    /* A specified-response band */
    if (f[2 * l + 1] - f[2 * l] > deps) 
    {
      slope = (a[2 * l + 1] - a[2 * l]) / (f[2 * l + 1] - f[2 * l]);
      while (gf[j] <= f[2 * l + 1] && j < ng) 
      {
        gh[j] = slope * (gf[j] - f[2 * l]) + a[2 * l];
        gw[j] = w[l];
        j++;
      }
    } 
    else 
    {
      /* zero bandwidth band */
      while (gf[j] <= f[2 * l + 1]) 
      {
        gh[j] = (a[2 * l] + a[2 * l + 1]) / 2;
        gw[j] = w[l];
        ++j;
      }
      
    }
  }
  return EW_SUCCESS;
} /* remezfrf */


/* ----------------------------------------------------------------------- */
/* FIR linear phase filter design subroutines */
/* Below functions were originally in Fortran; converted to C by Pete Lombard
 * using f2c and some manual cleanup. 11/13/99
 * Sorry, I didn't have the heart to remove all the goto's!
 */

/* Authors: James H. Mcclellan */
/*          Department of Electrical Engineering and Computer Science */
/*          Massachusetts Institute of Technology */
/*          Cambridge, Mass. 02139 */

/*          Thomas W. Parks */
/*          Department of Electrical Engineering */
/*          Rice University */
/*          Houston, Texas 77001 */

/*          Lawrence R. Rabiner */
/*          Bell Laboratories */
/*          Murray Hill, New Jersey 07974 */

/*     subroutine remezf ( nfilt, neg, nbands, ngrid, grid, */
/*    +   des, wt, ad, x, y, alpha, a, p, q, h, err, iext ) */
/* input: */
/*   nfilt-- filter length */
/*   neg  -- symmetry of filter */
/*           0 = even symmetry */
/*           1 = odd symmetry (anti- or negative symmetry) */
/*   nbands-- number of bands */
/*   ngrid-- length of frequency grid (grid, des, wt) */
/*   grid--  frequency grid of length ngrid */
/*   des--   desired function on frequency grid of length ngrid */
/*   wt--    weights for error on frequency grid of length ngrid */
/* output: */
/*   the following are inputs which are passed by reference and modified to */
/*   return the outputs */
/*   coef--     coefficients of basis vectors */
/*   err--   maximum ripple */

/* ----------------------------------------------------------------------- */

int remezf(int nfilt, int neg, int nbands, int ngrid, double *grid, 
           double *des, double *wt, double **pCoef, double *err)
{
  double *a, *ad, *alpha, *p, *q, *x, *y, *coef;
  double change, pi, pi2, temp, xt;
  int *iext, chunk;
  int jb, nf2j, nf3j, nm1, nodd, nz, nzmj, j, nfcns;
  
  /* Grab several hunks for coefficient array and internal storage */
  chunk = (nfilt / 2 + 4) * sizeof(double);
  if ( ( a = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( ad = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( alpha = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( p = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( q = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( x = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( y = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( *pCoef = (double*) malloc( chunk)) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  if ( ( iext = (int*) malloc((nfilt / 2 + 4) * sizeof(int))) == NULL )
  {
    logit("e", "remezf: error allocating memory\n");
    return EW_FAILURE;
  }
  coef = *pCoef;

  /* Parameter adjustments, so it will appear that arrays start indexing at 1 */
  --coef;
  --wt;
  --des;
  --grid;

  /* Function Body */
  pi = atan(1.0) * 4.0;
  pi2 = pi * 2.0;
  
  jb = nbands << 1;
  nodd = nfilt / 2;
  nodd = nfilt - (nodd << 1);
  nfcns = nfilt / 2;
  if (nodd == 1 && neg == 0) {
    ++nfcns;
  }

  /*  set up a new approximation problem which is equivalent */
  /*  to the original problem */

  if (neg <= 0) {
    goto L170;
  } else {
    goto L180;
  }
 L170:
  if (nodd == 1) {
    goto L200;
  }

  for (j = 1; j <= ngrid; ++j) {
    change = cos(pi * grid[j]);
    des[j] /= change;
    wt[j] *= change;
    /* L175: */
  }
  goto L200;
 L180:
  if (nodd == 1) {
    goto L190;
  }

  for (j = 1; j <= ngrid; ++j) {
    change = sin(pi * grid[j]);
    des[j] /= change;
    wt[j] *= change;
    /* L185: */
  }
  goto L200;
 L190:

  for (j = 1; j <= ngrid; ++j) {
    change = sin(pi2 * grid[j]);
    des[j] /= change;
    wt[j] *= change;
  }
  
  /*  initial guess for the extremal frequencies--equally */
  /*  spaced along the grid */

 L200:
  temp = (double) (ngrid - 1) / (double) nfcns;

  for (j = 1; j <= nfcns; ++j) {
    xt = (double) (j - 1);
    iext[j] = (int) (xt * temp + 1.0);
  }
  iext[nfcns + 1] = ngrid;
  nm1 = nfcns - 1;
  nz = nfcns + 1;
  
  /*  call the remez exchange algorithm to do the approximation problem */
  remez(pi2, ad, x, y, grid, des, wt, alpha, iext, nfcns, ngrid, a, p, q, err,
        chunk);
    
  /*  calculate the impulse response. */

  if (neg <= 0) {
    goto L300;
  } else {
    goto L320;
  }
 L300:
  if (nodd == 0) {
    goto L310;
  }

  for (j = 1; j <= nm1; ++j) {
    nzmj = nz - j;
    coef[j] = alpha[nzmj] * 0.5;

  }
  coef[nfcns] = alpha[1];
  goto L350;
 L310:
  coef[1] = alpha[nfcns] * 0.25;

  for (j = 2; j <= nm1; ++j) {
    nzmj = nz - j;
    nf2j = nfcns + 2 - j;
    coef[j] = (alpha[nzmj] + alpha[nf2j]) * 0.25;

  }
  coef[nfcns] = alpha[1] * 0.5 + alpha[2] * 0.25;
  goto L350;
 L320:
  if (nodd == 0) {
    goto L330;
  }
  coef[1] = alpha[nfcns] * 0.25;
  coef[2] = alpha[nm1] * 0.25;

  for (j = 3; j <= nm1; ++j) {
    nzmj = nz - j;
    nf3j = nfcns + 3 - j;

    coef[j] = (alpha[nzmj] - alpha[nf3j]) * 0.25;
  }
  coef[nfcns] = alpha[1] * 0.5 - alpha[3] * 0.25;
  coef[nz] = 0.0;
  goto L350;
 L330:
  coef[1] = alpha[nfcns] * 0.25;

  for (j = 2; j <= nm1; ++j) {
    nzmj = nz - j;
    nf2j = nfcns + 2 - j;
    coef[j] = (alpha[nzmj] - alpha[nf2j]) * 0.25;

  }
  coef[nfcns] = alpha[1] * 0.5 - alpha[2] * 0.25;

 L350:
  free(a);
  free(ad);
  free(alpha);
  free(p);
  free(q);
  free(x);
  free(y);
  free(iext);
  return EW_SUCCESS;
} /* remezf */


/* ----------------------------------------------------------------------- */
/* subroutine: remez */
/*   This subroutine implements the Remez Exchange Algorithm */
/*   for the weighted Chebyshev approximation of a continuous */
/*   function with a sum of cosines.  Inputs to the subroutine */
/*   are a dense grid which replaces the frequency axis, the */
/*   desired function on this grid, the weight function on the */
/*   grid, the number of cosines, and an initial guess of the */
/*   extremal frequencies.  The program minimizes the chebyshev */
/*   error by determining the best location of the extremal */
/*   frequencies (points of maximum error) and then calculates */
/*   the coefficients of the best approximation. */
/*   Frequencies are referenced to sampling frequency. */
/* ----------------------------------------------------------------------- */

void remez(double pi2, double *ad, double *x, double *y, double *grid, 
          double *des, double *wt, double *alpha, int *iext, int nfcns, 
          int ngrid, double *a, double *p, double *q, double *err, int chunk)
{
  double comp, dden, delf, devl, dev, dk, cn;
  double dnum, dtemp, gtemp, ft, xe, xt1, dak, fsh;
  double xt, y1, aa, bb, ynz;
  int itrmax, jm1, nm1, jp1, jchnge, nu, nz;
  int kkk, jet, kup, knz, jxt, nut;
  int klow, nzmj, j, k, l, kn, luck, niter, k1, nzzmj;
  int nzz, nf1j, nut1;

  /*  the program allows a maximum number of iterations of 25 */
  itrmax = 25;
  devl = -1.;
  nz = nfcns + 1;
  nzz = nfcns + 2;
  niter = 0;

 L100:
  iext[nzz] = ngrid + 1;
  ++niter;
  if (niter > itrmax) {
    goto L400;
  }

  for (j = 1; j <= nz; ++j) {
    jxt = iext[j];
    dtemp = grid[jxt];
    dtemp = cos(dtemp * pi2);
    x[j] = dtemp;
  }
  jet = (nfcns - 1) / 15 + 1;

  for (j = 1; j <= nz; ++j) {
    ad[j] = dd(j, nz, jet, x);
  }
  dnum = 0.;
  dden = 0.;
  k = 1;

  for (j = 1; j <= nz; ++j) {
    l = iext[j];
    dtemp = ad[j] * des[l];
    dnum += dtemp;
    dtemp = (double) k * ad[j] / wt[l];
    dden += dtemp;
    k = -k;
  }
  dev = dnum / dden;
  nu = 1;
  if (dev > 0.0) {
    nu = -1;
  }
  dev = -((double) nu) * dev;
  k = nu;

  for (j = 1; j <= nz; ++j) {
    l = iext[j];
    dtemp = (double) k * dev / wt[l];
    y[j] = des[l] + dtemp;
    k = -k;
  }
  if (dev > devl) {
    goto L150;
  }
  logit("e", "remez: failed to converge; error criteria not assured\n");
  goto L400;
 L150:
  devl = dev;
  jchnge = 0;
  k1 = iext[1];
  knz = iext[nz];
  klow = 0;
  nut = -nu;
  j = 1;

/*  search for the extremal frequencies of the best */
/*  approximation */

 L200:
  if (j == nzz) {
    ynz = comp;
  }
  if (j >= nzz) {
    goto L300;
  }
  kup = iext[j + 1];
  l = iext[j] + 1;
  nut = -nut;
  if (j == 2) {
    y1 = comp;
  }
  comp = dev;
  if (l >= kup) {
    goto L220;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp <= 0.0) {
    goto L220;
  }
  comp = (double) nut * *err;
 L210:
  ++l;
  if (l >= kup) {
    goto L215;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp <= 0.0) {
    goto L215;
  }
  comp = (double) nut * *err;
  goto L210;
 L215:
  iext[j] = l - 1;
  ++j;
  klow = l - 1;
  ++jchnge;
  goto L200;
 L220:
  --l;
 L225:
  --l;
  if (l <= klow) {
    goto L250;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp > 0.0) {
    goto L230;
  }
  if (jchnge <= 0) {
    goto L225;
  }
  goto L260;
 L230:
  comp = (double) nut * *err;
 L235:
  --l;
  if (l <= klow) {
    goto L240;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp <= 0.0) {
    goto L240;
  }
  comp = (double) nut * *err;
  goto L235;
 L240:
  klow = iext[j];
  iext[j] = l + 1;
  ++j;
  ++jchnge;
  goto L200;
 L250:
  l = iext[j] + 1;
  if (jchnge > 0) {
    goto L215;
  }
 L255:
  ++l;
  if (l >= kup) {
    goto L260;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp <= 0.0) {
    goto L255;
  }
  comp = (double) nut * *err;
  goto L210;
 L260:
  klow = iext[j];
  ++j;
  goto L200;
 L300:
  if (j > nzz) {
    goto L320;
  }
  if (k1 > iext[1]) {
    k1 = iext[1];
  }
  if (knz < iext[nz]) {
    knz = iext[nz];
  }
  nut1 = nut;
  nut = -nu;
  l = 0;
  kup = k1;
  comp = ynz * 1.00001;
  luck = 1;
 L310:
  ++l;
  if (l >= kup) {
    goto L315;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp <= 0.0) {
    goto L310;
  }
  comp = (double) nut * *err;
  j = nzz;
  goto L210;
 L315:
  luck = 6;
  goto L325;
 L320:
  if (luck > 9) {
    goto L350;
  }
  if (comp > y1) {
    y1 = comp;
  }
  k1 = iext[nzz];
 L325:
  l = ngrid + 1;
  klow = knz;
  nut = -nut1;
  comp = y1 * 1.00001;
 L330:
  --l;
  if (l <= klow) {
    goto L340;
  }
  *err = geed(l, nz, ad, x, y, pi2, grid);
  *err = (*err - des[l]) * wt[l];
  dtemp = (double) nut * *err - comp;
  if (dtemp <= 0.0) {
    goto L330;
  }
  j = nzz;
  comp = (double) nut * *err;
  luck += 10;
  goto L235;
 L340:
  if (luck == 6) {
    goto L370;
  }

  for (j = 1; j <= nfcns; ++j) {
    nzzmj = nzz - j;
    nzmj = nz - j;
    iext[nzzmj] = iext[nzmj];
  }
  iext[1] = k1;
  goto L100;
 L350:
  kn = iext[nzz];

  for (j = 1; j <= nfcns; ++j) {
    iext[j] = iext[j + 1];
  }
  iext[nz] = kn;
  goto L100;
 L370:
  if (jchnge > 0) {
    goto L100;
  }

  /*  calculation of the coefficients of the best approximation */
  /*  using the inverse discrete fourier transform */

 L400:
  nm1 = nfcns - 1;
  fsh = 1.0e-6;
  gtemp = grid[1];
  x[nzz] = -2.0;
  cn = (double) ((nfcns << 1) - 1);
  delf = 1.0 / cn;
  l = 1;
  kkk = 0;
  if (grid[1] < 0.01 && grid[ngrid] > 0.49) {
    kkk = 1;
  }
  if (nfcns <= 3) {
    kkk = 1;
  }
  if (kkk == 1) {
    goto L405;
  }
  dtemp = cos(pi2 * grid[1]);
  dnum = cos(pi2 * grid[ngrid]);
  aa = 2.0 / (dtemp - dnum);
  bb = -(dtemp + dnum) / (dtemp - dnum);
 L405:
  for (j = 1; j <= nfcns; ++j) {
    ft = (double) (j - 1);
    ft *= delf;
    xt = cos(pi2 * ft);
    if (kkk == 1) {
      goto L410;
    }
    xt = (xt - bb) / aa;
    xt1 = sqrt(1.0 - xt * xt);
    ft = atan2(xt1, xt) / pi2;
  L410:
    xe = x[l];
    if (xt > xe) {
      goto L420;
    }
    if (xe - xt < fsh) {
      goto L415;
    }
    ++l;
    if (l >= chunk - 1) {
      goto L415;
    }
    goto L410;
  L415:
    a[j] = y[l];
    goto L425;
  L420:
    if (xt - xe < fsh) {
      goto L415;
    }
    grid[1] = ft;
    a[j] = geed(1, nz, ad, x, y, pi2, grid);
  L425:
    if (l > 1) {
      --l;
    }
  }

  grid[1] = gtemp;
  dden = pi2 / cn;

  for (j = 1; j <= nfcns; ++j) {
    dtemp = 0.0;
    dnum = (double) (j - 1);
    dnum *= dden;
    if (nm1 < 1) {
      goto L505;
    }

    for (k = 1; k <= nm1; ++k) {
      dak = a[k + 1];
      dk = (double) k;
      dtemp += dak * cos(dnum * dk);
    }
  L505:
    dtemp = dtemp * 2.0 + a[1];
    alpha[j] = dtemp;
  }
  for (j = 2; j <= nfcns; ++j) {
    alpha[j] = alpha[j] * 2.0 / cn;
  }
  alpha[1] /= cn;
  if (kkk == 1) {
    goto L545;
  }
  p[1] = alpha[nfcns] * 2.0 * bb + alpha[nm1];
  p[2] = aa * 2.0 * alpha[nfcns];
  q[1] = alpha[nfcns - 2] - alpha[nfcns];

  for (j = 2; j <= nm1; ++j) {
    if (j < nm1) {
      goto L515;
    }
    aa *= 0.5;
    bb *= 0.5;
  L515:
    p[j + 1] = 0.0;

    for (k = 1; k <= j; ++k) {
      a[k] = p[k];
      p[k] = bb * 2.0 * a[k];
    }
    p[2] += a[1] * 2.0 * aa;
    jm1 = j - 1;
    for (k = 1; k <= jm1; ++k) {
      p[k] = p[k] + q[k] + aa * a[k + 1];
    }
    jp1 = j + 1;
    for (k = 3; k <= jp1; ++k) {
      p[k] += aa * a[k - 1];
    }
    if (j == nm1) {
      goto L540;
    }
    for (k = 1; k <= j; ++k) {
      q[k] = -a[k];
    }
    nf1j = nfcns - 1 - j;
    q[1] += alpha[nf1j];
  L540:
    ;
  }
  for (j = 1; j <= nfcns; ++j) {
    alpha[j] = p[j];
  }
 L545:
  if (nfcns > 3) {
    return;
  }
  alpha[nfcns + 1] = 0.0;
  alpha[nfcns + 2] = 0.0;
  return;
} /* remez */


/* ----------------------------------------------------------------------- */
/* function: dd */
/*   function to calculate the lagrange interpolation */
/*   coefficients for use in the function geed. */
/* ----------------------------------------------------------------------- */

double dd(int k, int n, int m, double *x)
{
  double q, ret_val;
  int j, l;
  
  ret_val = 1.0;
  q = x[k];

  for (l = 1; l <= m; ++l) {
    for (j = l; m < 0 ? j >= n : j <= n; j += m) {
      if (j - k != 0) {
        goto L1;
      } else {
        goto L2;
      }
    L1:
      ret_val = ret_val * 2.0 * (q - x[j]);
    L2:
      ;
    }
  }
  ret_val = 1.0 / ret_val;
  return ret_val;
} /* dd */


/* ----------------------------------------------------------------------- */
/* function: geed */
/*   function to evaluate the frequency response using the */
/*   lagrange interpolation formula in the barycentric form */
/* ----------------------------------------------------------------------- */

double geed(int k, int n, double *ad, double *x, double *y, double pi2, 
            double *grid)
{
  double ret_val, p, xf, c, d;
  int j;

  p = 0.0;
  xf = grid[k];
  xf = cos(pi2 * xf);
  d = 0.0;
  for (j = 1; j <= n; ++j) {
    c = xf - x[j];
    c = ad[j] / c;
    d += c;
    p += c * y[j];
  }
  ret_val = p / d;
  return ret_val;
} /* geed */


