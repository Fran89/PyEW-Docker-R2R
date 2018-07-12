
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: area.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:08:44  lucky
 *     Initial revision
 *
 *
 */

/* foo.f -- translated by f2c (version 19950223).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int 	area (int *, int, float *, float *, float *, float *);
int 	cntsct (int, float *, float *, float *, float *);
int 	isect (float, float, float, float, float *, float *);

/* Subroutine */ 
int area (int *iout, int n, float *x, float *y, float *u, float *v)
{
    static int i;

/* ** area **  algorithm from ron sheen - 1972 */

/* this function returns a value of 1 if the point (u,v) */
/* lies inside the polygon defined by arrays x and y, 0 */
/* is returned otherwise */

/* inputs: */

/* 	n - number of sides to the polygon */
/* 	    ( must be less or equal to 20) */
/* 	x - array of dimension 21, contains n+1 x coordinates of polygon */
/* 	y - array of dimension 21, contains n+1 y coordinates of polygon */
/* 	  *** note *** the first point of the polygon is duplicated */
/*         in location n+1 of the x and y arrays. */
/* 	u - x coordinate of point to check */
/* 	v - y coordinate of point to check */

/* outputs: */

/* 	iout = 1 if the point is inside or on the polygon */
/* 		0 otherwise. */

/* calls: */
/*       iand,cntsct */


    /* Function Body */
    i = cntsct(n, x, y, u, v);


    *iout = 2 * ((i + 1) / 2) - i;

    return 0;

} /* area_ */



int cntsct (int n, float *x, float *y, float *u, float *v)
{
    /* Local variables */
    static int isec, i;
    static int itimes;


/* ** cntsct **  algorithm by ron sheen - 1972 */

/* this function returns the count of the number of times a ray */
/* projected from point (u,v) intersects the polygon defined in the */
/* arrays x and y. */

/* inputs: */

/* 	n = the number of sides to polygon */
/* 	x = n+1 x coordinates of polygon */
/* 	y = n+1 y coordinates of polygon */
/* 	   *** note *** element n+1 of the x and y arrays are */
/* 	   duplicates of element 1 of both arrays. */
/* 	u = x coordinate */
/* 	v = y coordinate */

/* outputs: */

/* 	cntsct = count of intersections */

/* calls: */

/* 	isect */


    /* Function Body */
    itimes = 0;

    for (i = 0; i < n; i++) 
	{
		isec = isect(x[i], y[i], x[i + 1], y[i + 1], u, v);

		if (isec == 2) 
		{
		    return (1);
		}
		itimes = itimes + isec;
    }

    return (itimes);


} /* cntsct_ */



int isect (float x1, float y1, float x2, float y2, float *u, float *v)
{

    static float eps = (float) 0.00001;
    int ret_val;
    static float yr, xt;


/* ** isect **  algorithm by ron sheen - 1972 */

/* this function determines whether a ray projected from (u,v) */
/* intersects or lies on the line through (x1,y1), (x2,y2). */

/* inputs: */
/* 	x1 = x-coordinate of first point */
/* 	y1 = y-coordinate of first point */
/* 	x2 = x-coordinate of second point */
/* 	y2 = y-coordinate of second point */
/* 	u  = x-coordinate of test point */
/* 	v  = y-coordinate of test point */

/* outputs: */
/* 	isect = 0; ray does not intersect line */
/* 	isect = 1; ray does intersect line */
/* 	isect = 2; ray lies on line */

/* calls: */
/* 	abs */


	yr = *v;
	if (((fabs (yr - y1) < eps)  && (fabs (y1 - y2) < eps)) &&
		(((*u < (x1 + eps)) && (*u > (x2 + eps))) ||
		 ((*u < (x2 + eps)) && (*u > (x1 - eps)))))
	{
    	ret_val = 2;
	}

	else if (((fabs (*u - x1) < eps) && (fabs (x1 - x2) < eps)) && 
			(((yr < (y1 + eps)) && (yr > (y2 - eps))) || 
			((yr < (y2 + eps)) && (yr > (y1 - eps)))))
	{
    	ret_val = 2;
	}

	else if ((fabs (yr - y1) < eps) || (fabs (yr - y2) < eps)) 
	{
		yr = (float) (yr + (eps * 10.0));
    }


    if (((yr < (y2 + eps)) && (yr > (y1 - eps))) || 
			((yr < (y1 + eps)) && (yr > (y2 - eps)))) 
	{
		if ((*u < (x1 + eps)) || (yr < (x2 + eps)))
		{
    		if ((*u > (x1 - eps)) || (*u > (x2 - eps))) 
			{
    			yr = *v;
			    xt = x1 + (((x2 - x1) * (yr - y1)) / (y2 - y1));
			    if (*u <= (xt + (eps * 0.001))) 
				{
			    	if (fabs (*u - xt) < eps) 
					{
						ret_val = 2;	
					}
					else
					{
						ret_val = 1;
					}
				}
				else
				{
					ret_val = 0;
				}

			}
			else
			{
				ret_val = 1;
			}
		}
		else
		{
			ret_val = 0;
		}
    }
	else
	{
		ret_val = 0;
	}
	
    return ret_val;
} /* isect_ */

