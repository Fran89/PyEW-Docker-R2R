  /**********************************************************************
   *                              complex.c                             *
   *                                                                    *
   * These functions perform complex arithmetic (real/imaginary stuff). *
   * They were taken directly from "Numerical Recipes in 'C'".  The     *
   * type fcomplex must be defined along with the functions somewhere.  *
   *                                                                    *
   **********************************************************************/
#ifndef COMPLEX_C
#define COMPLEX_C
   
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "earlybirdlib.h"

fcomplex Cadd( fcomplex a, fcomplex b )
{
   fcomplex c;

   c.r = a.r + b.r;
   c.i = a.i + b.i;
   return c;
}

fcomplex Csub( fcomplex a, fcomplex b )
{
   fcomplex c;

   c.r = a.r - b.r;
   c.i = a.i - b.i;
   return c;
}

fcomplex Cmul( fcomplex a, fcomplex b )
{
   fcomplex c;

   c.r = a.r*b.r - a.i*b.i;
   c.i = a.i*b.r + a.r*b.i;
   return c;
}

fcomplex Complex( double re, double im )
{
   fcomplex c;

   c.r = (float) re;
   c.i = (float) im;
   return c;
}

fcomplex Conjg( fcomplex z )
{
   fcomplex c;

   c.r = z.r;
   c.i = -z.i;
   return c;
}

fcomplex Cdiv( fcomplex a, fcomplex b )
{
   fcomplex c;
   float  r, den;

   if ( fabs (b.r) >= fabs (b.i) ) 
   {
      r = b.i / b.r;
      den = b.r + r*b.i;
      c.r = (a.r + r*a.i) / den;
      c.i = (a.i - r*a.r) / den;
   } 
   else 
   {
      r = b.r / b.i;
      den = b.i + r*b.r;
      c.r = (a.r*r + a.i) / den;
      c.i = (a.i*r - a.r) / den;
   }
return c;
}

float Cabs( fcomplex z )
{
   float x, y, ans, temp;

   x = (float) fabs (z.r);
   y = (float) fabs (z.i);
   if ( x == 0.0 ) ans=y;
   else if ( y == 0.0 ) ans=x;
   else if ( x > y ) 
   {
      temp = y / x;
      ans = x * (float) sqrt (1.0 + temp*temp);
   } 
   else 
   {
      temp = x / y;
      ans = y * (float) sqrt (1.0 + temp*temp);
   }
   return ans;
}

fcomplex Csqrt( fcomplex z )
{
   fcomplex c;
   float    x, y, w, r;

   if ( (z.r == 0.0) && (z.i == 0.0) ) 
   {
      c.r = 0.0;
      c.i = 0.0;
      return c;
   } 
   else 
   {
      x = (float) fabs (z.r);
      y = (float) fabs (z.i);
      if ( x >= y ) 
      {
         r = y / x;
         w = (float) (sqrt (x) * sqrt (0.5 * (1.0 + sqrt (1.0 + r*r))));
      } 
      else 
      {
         r = x / y;
         w = (float) (sqrt (y) * sqrt (0.5 * (r + sqrt (1.0 + r*r))));
      }
      if ( z.r >= 0.0 ) 
      {
         c.r = w;
         c.i = (float) (z.i / (2.0*w));
      } 
      else 
      {
         c.i = (z.i >= 0) ? w : -w;
         c.r = (float) (z.i / (2.0*c.i));
      }
      return c;
   }
}

fcomplex RCmul( double x, fcomplex a )
{
   fcomplex c;

   c.r = (float) x * a.r;
   c.i = (float) x * a.i;
   return c;
}

#endif
