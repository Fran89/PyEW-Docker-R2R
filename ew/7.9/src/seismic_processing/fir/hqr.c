
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hqr.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/* HQR: finds all eigenvalues of an N by N upper Hessenberg matrix A that is */
/* stored in an NP by NP array. On output, A is destroyed. */
/* The real and imaginary parts of the eigenvalues are returned in WR */
/* and WI respectively. */

#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#ifndef MIN
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
#endif
double d_sign(double a, double b);

void hqr(double *a, int n, int np, double *wr, double *wi)
{
  int a_dim1, a_offset, i__2, i__3, i__4;
  double d__1;
  int i__, j, k, l, m;
  double p, q, r__, s, t;
  double u, v;
  double w, x, y, z__, anorm;
  int nn, its;

/* Compute matric norm for possible use in locating single small */
/* subdiagonal element. */
    /* Parameter adjustments */
  --wi;
  --wr;
  a_dim1 = np;
  a_offset = 1 + a_dim1 * 1;
  a -= a_offset;

  anorm = fabs( a[a_dim1 + 1]);
  for (i__ = 2; i__ <= n; ++i__) 
  {
    for (j = i__ - 1; j <= n; ++j)
      anorm += fabs( a[i__ + j * a_dim1]);
  }
  nn = n;
  t = 0.;
  /* Begin search for next eigenvalue */
  /* Gets changed only by an exceptional shift. */
 L1:
  if (nn >= 1) 
  {
    its = 0;
    /* VBegin iteration: look for single small subdiagonal element. */
  L2:
    for (l = nn; l >= 2; --l) 
    {
      s = fabs(a[l - 1 + (l - 1) * a_dim1]) 
        + fabs (a[l + l * a_dim1]);
      if (s == 0.0)
        s = anorm;
      if (fabs( a[l + (l - 1) * a_dim1]) + s == s)
        goto L3;
    }
    l = 1;
  L3:
    x = a[nn + nn * a_dim1];
    if (l == nn) 
    {
      /* One root found */
      wr[nn] = x + t;
      wi[nn] = 0.;
      --nn;
    } 
    else
    {
      y = a[nn - 1 + (nn - 1) * a_dim1];
      w = a[nn + (nn - 1) * a_dim1] * a[nn - 1 + nn * a_dim1];
      if (l == nn - 1) 
      {
        /* Two roots found... */
        p = (y - x) * .5;
        /* Computing 2nd power */
        d__1 = p;
        q = d__1 * d__1 + w;
        z__ = sqrt((fabs(q)));
        x += t;
        if (q >= 0.) 
        {
          /* ...a real pair */
          z__ = p + d_sign(z__, p);
          wr[nn] = x + z__;
          wr[nn - 1] = wr[nn];
          if (z__ != 0.) {
            wr[nn] = x - w / z__;
          }
          wi[nn] = 0.;
          wi[nn - 1] = 0.;
        } 
        else
        {
          /* ...a complex pair */
          wr[nn] = x + p;
          wr[nn - 1] = wr[nn];
          wi[nn] = z__;
          wi[nn - 1] = -z__;
        }
        nn += -2;
      } 
      else
      {
        /* No roots found. Continue iteration. */
        if (its == 30) 
        {
          fprintf(stderr, "hqr: too many iterations; exitting");
          exit(1);
        }
        if (its == 10 || its == 20) 
        {
          /* Form exceptional s */
          t += x;
          for (i__ = 1; i__ <= n; ++i__) 
          {
            a[i__ + i__ * a_dim1] -= x;
            /* L14: */
          }
          s = fabs( a[nn + (nn - 1) * a_dim1]) 
            + fabs( a[nn - 1 + (nn - 2) * a_dim1]);
          x = s * 0.75;
          y = x;
          /* Computing 2nd power */
          d__1 = s;
          w = d__1 * d__1 * -0.4375;
        }
        ++its;
        /* Form shift and then look for 2 consecutive small subdiagonal elements. */
        for (m = nn - 2; m >= l; --m)
        {
          z__ = a[m + m * a_dim1];
          r__ = x - z__;
          s = y - z__;
          p = (r__ * s - w) / a[m + 1 + m * a_dim1] 
            + a[m + (m + 1) * a_dim1];
          q = a[m + 1 + (m + 1) * a_dim1] - z__ - r__ - s;
          r__ = a[m + 2 + (m + 1) * a_dim1];
          /* Scale to prevent overflow or underflow */
          s = fabs(p) + fabs(q) + fabs(r__);
          p /= s;
          q /= s;
          r__ /= s;
          if (m == l) 
            goto L4;
          u = fabs( a[m + (m - 1) * a_dim1]) * (fabs(q) + fabs(r__));
          v = fabs(p) * (fabs( a[m - 1 + (m - 1) * a_dim1])
            + fabs(z__) + fabs( a[m + 1 + (m + 1) * a_dim1]));
          if (u + v == v)
            goto L4;
        }
      L4:
        for (i__ = m + 2; i__ <= nn; ++i__)
        {
          a[i__ + (i__ - 2) * a_dim1] = 0.;
          if (i__ != m + 2) 
            a[i__ + (i__ - 3) * a_dim1] = 0.;
        }
        /* Double QR step on rows L to NN and columns M to NN. */
        for (k = m; k <= nn - 1; ++k) 
        {
          if (k != m) 
          {
            p = a[k + (k - 1) * a_dim1];
            /* Begin setup of Householder vector */
            q = a[k + 1 + (k - 1) * a_dim1];
            r__ = 0.0;
            if (k != nn - 1)
              r__ = a[k + 2 + (k - 1) * a_dim1];
            x = fabs(p) + fabs(q) + fabs(r__);
            if (x != 0.) 
            {
              /* Scale to prevent overflow or underflow */
              p /= x;
              q /= x;
              r__ /= x;
            }
          }
          d__1 = sqrt(p * p + q * q + r__ * r__);
          s = d_sign(d__1, p);
          if (s != 0.) 
          {
            if (k == m) 
            {
              if (l != m) 
                a[k + (k - 1) * a_dim1] = -a[k + (k - 1) * a_dim1];
            } 
            else 
              a[k + (k - 1) * a_dim1] = -s * x;
            p += s;
            x = p / s;
            y = q / s;
            z__ = r__ / s;
            q /= p;
            r__ /= p;
            for (j = k; j <= nn; ++j) 
            {
              /* Row modification. */
              p = a[k + j * a_dim1] + q * a[k + 1 + j * a_dim1];
              if (k != nn - 1) 
              {
                p += r__ * a[k + 2 + j * a_dim1];
                a[k + 2 + j * a_dim1] -= p * z__;
              }
              a[k + 1 + j * a_dim1] -= p * y;
              a[k + j * a_dim1] -= p * x;
              /* L17: */
            }
            /* Computing MIN */
            i__3 = nn, i__4 = k + 3;
            i__2 = MIN(i__3,i__4);
            for (i__ = l; i__ <= i__2; ++i__) 
            {
              /* Column modification. */
              p = x * a[i__ + k * a_dim1] 
                + y * a[i__ + (k + 1) * a_dim1];
              if (k != nn - 1) 
              {
                p += z__ * a[i__ + (k + 2) * a_dim1];
                a[i__ + (k + 2) * a_dim1] -= p * r__;
              }
              a[i__ + (k + 1) * a_dim1] -= p * q;
              a[i__ + k * a_dim1] -= p;
            }
          }
        }
        goto L2;
        /* for next iteration or current eigenvalue */
      }
    }
    goto L1;
    /* for next iteration. */
  }
  return;
} /* hqr */

double d_sign(double a, double b)
{
  double x;
  x = (a >= 0 ? a : - a);
  return( b >= 0 ? x : -x);
}
