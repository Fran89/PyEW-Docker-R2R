
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: l1.c 1474 2004-05-14 23:35:37Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2004/05/14 23:35:37  dietz
 *     modified to work with TYPE_PICK_SCNL messages only
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>   /* for qsort */
#include <math.h>
#include "nrutil.h"
#define BIG 1.0e32
/*
 *> 94Jun29 : Added wmedian, weighted median routine.
 */

int l1( int m, int n, float **a, float *b, double eps, float *x )
{
        double sum;
        float min, max;
        float *e;
        float d;
        float pivot;
        int *s;
        int i, j, k, l;
        int m1, m2, n1, n2;
        int stage;
        int test;
        int kount;
        int kr, kl;
        int in, out;

/* Initialization */
        e = vector(1, m);
        s = ivector(1, m);
        m1 = m + 1;
        m2 = m + 2;
        n1 = n + 1;
        n2 = n + 2;
        for(j=1; j<=n; j++) {
                a[m2][j] = j;
                x[j] = (float)0.0;
        }
        for(i=1; i<=m; i++) {
                a[i][n1] = b[i];
                a[i][n2] = n + i;
                e[i] = (float)0.0;
                if(b[i] < (float)0.0)
                        for(j=1; j<=n2; j++)
                                a[i][j] = -a[i][j];
        }

/* Compute the marginal costs */
        for(j=1; j<=n1; j++) {
                a[m1][j] = (float)0.0;
                for(i=1; i<=m; i++)
                        a[m1][j] += a[i][j];
        }

/****************************** STAGE I ****************************************/
/* Determine the vector to enter the basis */
        stage = 1;
        kount = 0;
        kr = 1;
        kl = 1;
x70:
        max = (float)-1.0;
        for(j=kr; j<= n; j++) {
                if(fabs(a[m2][j]) <= n) {
                        d = (float)fabs(a[m1][j]);
                        if(d > max) {
                                max = d;
                                in = j;
                        }
            }
        }
        if(a[m1][in] < (float)0.0)
                for(i=1; i<=m2; i++)
                        a[i][in] = -a[i][in];

/* Determine the vector to leave the basis */
x100:
        k = 0;
        for(i=kl; i<= m; i++) {
                d = a[i][in];
                if(d > (float)eps) {
                        k++;
                        b[k] = a[i][n1] / d;
                        s[k] = i;
                        test = 1;
                }
        }
x120:
        if(k <= 0) {
                test = 0;
        } else {
                min = (float)BIG;
                for(i=1; i<=k; i++) {
                        if(b[i] < min) {
                                j = i;
                                min = b[i];
                                out = s[i];
                        }
                }
                b[j] = b[k];
                s[j] = s[k];
                k--;
        }

/* Check for linear dependence in stage 1 */
        if(!test && stage) {
                for(i=1; i<=m2; i++) {
                        d = a[i][kr];
                        a[i][kr] = a[i][in];
                        a[i][in] = d;
                }
                kr++;
                goto x260;
        }
        if(!test) {
                a[m2][n1] = (float)2.0;
                goto x350;
        }
        pivot = a[out][in];
        if(a[m1][in] - pivot - pivot > (float)eps) {
                for(j=kr; j<= n1; j++) {
                        d = a[out][j];
                        a[m1][j] -= 2.0*d;
                        a[out][j] = -d;
                }
                a[out][n2] = -a[out][n2];
                goto x120;
        }

/* Pivot on a[out][in] */
        for(j=kr; j<=n1; j++)
                if(j != in)
                        a[out][j] = a[out][j] / pivot;
        for(i=1; i<=m1; i++) {
                if(i != out) {
                        d = a[i][in];
                        for(j=kr; j<=n1; j++)
                                if(j != in)
                                        a[i][j] -= d*a[out][j];
                }
        }
        for(i=1; i<=m1; i++)
                if(i != out)
                        a[i][in] = -a[i][in] / pivot;
        a[out][in] = (float)1.0/pivot;
        d = a[out][n2];
        a[out][n2] = a[m2][in];
        a[m2][in] = d;
        kount++;
        if(!stage)
                goto x270;

/* Interchange rows in state 1 */
        kl++;
        for(j=kr; j<=n2; j++) {
                d = a[out][j];
                a[out][j] = a[kount][j];
                a[kount][j] = d;
        }
x260:
        if(kount + kr != n1)
                goto x70;

/*****************************STAGE 2 *************************************/
        stage = 0;

/* Determine the vector to enter basis */
x270:
        max = (float)(-BIG);
        for(j=kr; j<=n; j++) {
                d = a[m1][j];
            if(d > max) {
                        max = d;
                        in = j;
                        continue;
                }
                if(d < -max-2) {
                        max = -d - 2;
                        in = j;
                }
        }
        if(max > (float)eps) {
                if(a[m1][in] > (float)0.0)
                        goto x100;
                for(i=1; i<=m2; i++)
                        a[i][in] = -a[i][in];
                a[m1][in] -= 2.0;
                goto x100;
        }

/* Prepare output */
        l = kl - 1;
        for(i=1; i<=l; i++)
                if(a[i][n1] < (float)0.0)
                        for(j=kr; j<=n2; j++)
                                a[i][j] = -a[i][j];
        a[m2][n1] = (float)0.0;
        if(kr == 1) {
                for(j=1; j<=n; j++) {
                        d = (float)fabs(a[m1][j]);
                        if(d <= (float)eps || (float)2.0 - d < (float)eps)
                                goto x350;
                }
                a[m2][n1] = (float)1.0;
        }
x350:
        for(i=1; i<=m; i++) {
                k = (int)a[i][n2];
                d = a[i][n1];
                if(k <= 0) {
                        k = -k;
                        d = -d;
                }
                if(i < kl) {
                        x[k] = d;
                } else {
                        k -= n;
                        e[k] = d;
                }
        }
        a[m2][n2] = kount;
        a[m1][n2] = n1 - kr;
        sum = 0.0;
        for(i=kl; i<=m; i++)
                sum += a[i][n1];
        a[m1][n1] = (float)sum;

        free_vector(e, 1, m);
        free_ivector(s, 1, m);
        return((int)a[m1][n2]);
}

/* median : Calculate median of elements in an array.  Note, the array
        is sorted in ascending order upon return */
int medcmp( const void *x1, const void *x2 )
{
        if(*(double *)x1 < *(double *)x2)   return -1;
        if(*(double *)x1 > *(double *)x2)   return 1;
        return 0;
}

double median(int n, double *x)
{
        if(n < 1)
                return 0.0;
        qsort(x, n, sizeof(double), medcmp);
        return(n%2 ? x[n/2] : 0.5*(x[n/2]+x[n/2-1]));
}

#define MAXMED 500
static struct {
        double  x;
        double  w;
} wmed[MAXMED];
static int ix[MAXMED];


int wmedcmp(const void *i1, const void *i2)
{
        if(wmed[*(int *)i1].x < wmed[*(int *)i2].x)   return -1;
        if(wmed[*(int *)i1].x > wmed[*(int *)i2].x)   return 1;
        return 0;
}

double wmedian(int n, double *x, double *w)
{
        double wlast;
        double wsum;
        int i, j;
        int ixi, ixj;

        if(n < 1)
                return 0.0;
        if(n < 3)
                return x[0];
        if(n > MAXMED)
                n = MAXMED;
        for(i=0; i<n; i++) {
                ix[i] = i;
                wmed[i].x = x[i];
                wmed[i].w = w[i];
        }

        qsort(ix, n, sizeof(int), wmedcmp);

        wlast = 1.0e23;
        for(i=0; i<n; i++) {
                wsum = 0.0;
                ixi = ix[i];
                for(j=0; j<n; j++) {
                        ixj = ix[j];
                        if(j < i)
                                wsum += w[ixj] * (x[ixi] - x[ixj]);
                        if(j > i)
                                wsum += w[ixj] * (x[ixj] - x[ixi]);
                }
                if(wsum > wlast)
                        return x[ix[i-1]];
                wlast = wsum;
        }
        return x[ix[n/2]];
}


/* Test program */
/*
l1_test()
{
        int m, n;
        float **a;
        float *b;
        double eps;
        float *x;
        int i;

        printf("l1 test\n");
        m = 4;
        n = 2;
        a = matrix(1, m+2, 1, n+2);
        b = vector(1, m);
        x = vector(1, n);
        eps = 1.0e-3;

        for(i=1; i<= m; i++) {
                a[i][1] = 1;
                a[i][2] = i;
                b[i] = 2.0*i + 1.0;
        }
        b[4] = 100.0;
        l1(m, n, a, b, eps, x);
        printf("x: %f %f\n", x[1], x[2]);
        free_matrix(a, 1, m+2, 1, n+2);
        free_vector(b, 1, m);
        free_vector(x, 1, n);
        printf("Pau\n");
}
*/
