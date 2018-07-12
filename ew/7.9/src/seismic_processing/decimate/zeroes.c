
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: zeroes.c 6 2000-02-14 17:02:31Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:56:25  lucky
 *     Initial revision
 *
 *
 */

/*
 * zeroes.c: Calculate zeroes of a filter
 *              1) Sets up companion matrix of the filter polynomial 
 *              2) Finds eigenvalues of matrix with QR algorithm
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: Zeroes                                                */
/*                                                                      */
/*      Inputs:       Pointer to half of symmetric filter coefficients  */
/*                    Pointers to arrays for real and imaginary parts   */
/*                      of the zeroes                                   */
/*                    Order of the filter                               */
/*                                                                      */
/*      Outputs:      Filter zeroes in real and imaginary arrays        */
/*                                                                      */
/*      Returns:        EW_SUCCESS on success, else EW_FAILURE          */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit, threads                               */

/*******                                                        *********/
/*      Decimate Includes                                               */
/*******                                                        *********/
#include "decimate.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: Zeroes                                                */
int Zeroes( double *coeffs, double **wr, double **wi, int order )
{
  double *a;
  int half_l;      /* Half the length of the (symmetric) filter array   */
  int i, j;
  
  half_l = (order + 1) / 2;
  if (order % 2 == 0) half_l++;
  
  if ((a = (double*)malloc(order * order * sizeof(double))) == NULL )
  {
    logit("e", "decimate: error allocating matrix\n");
    return EW_FAILURE;
  }
  if ((*wr = (double*)malloc(order * sizeof(double))) == NULL)
  {
    logit("e", "decimate: error allocating wr\n");
    return EW_FAILURE;
  }
  if ((*wi = (double*)malloc(order * sizeof(double))) == NULL)
  {
    logit("e", "decimate: error allocating wi\n");
    return EW_FAILURE;
  }

  /* Fill in the companion matrix in Upper Hessenberg form */
  for (i = 0; i < order * order; i++) a[i] = 0.0;
  a[order * (order - 1)] = -1.0;
  for (i = 1; i < half_l; i++)
  {
    /* First row elements */
    a[order * (i - 1)] = -coeffs[i] / coeffs[0];
    a[order * (order - i - 1)] = a[order * (i - 1)];

     /* Sub-diagonal elements */
    a[order * (i - 1) + i] = 1.0;
    j = order - i;
    a[order * (j - 1) + j] = 1.0;
  }
  hqr(a, order, order, *wr, *wi);
  free( a );
  
  return EW_SUCCESS;
}

  
  
    
