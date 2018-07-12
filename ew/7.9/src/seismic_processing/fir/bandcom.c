
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: bandcom.c 7 2000-02-14 17:27:23Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 17:27:23  lucky
 *     Initial revision
 *
 *
 */

/*
 * bandcom.c: Process Band commands from the config file
 *              1) Allocate members of the BAND structures.
 *              2) Build an ordered, linked list of Band structures
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: BandCom                                               */
/*                                                                      */
/*      Inputs:         Pointer to the Fir World structure              */
/*                      Partially parsed command file (in Kom buffers   */
/*                                                                      */
/*      Outputs:        Linked list of BAND strucutres                  */
/*                      Error messages to stderr                        */
/*                                                                      */
/*      Returns:        1 on successful parsing of Band command         */
/*                      0 if not a Band command                         */
/*                                                                      */
/*      Exits:          On error after logging to stderr                */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>
#include <kom.h>        /* k_close, k_err, k_int, k_its, k_open, k_rd,  */
                        /*   k_str, k_val                               */
/*******                                                        *********/
/*      Fir Includes                                            */
/*******                                                        *********/
#include "fir.h"

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: BandCom                                               */
int BandCom (WORLD* pDcm)
{
  PBAND this, new;
  
  if (k_its("Band") )
  {
    new = (PBAND) malloc( sizeof(BAND) );
    if (new == (PBAND) NULL)
    {
      fprintf(stderr, "fir: error allocating BAND structure\n");
      exit( -1 );
    }
    
    new->f_low = k_val();
    if (new->f_low < 0.0 || new->f_low >= 1.0)
    {
      fprintf(stderr, "fir: invalid f_low; must be between 0.0 and 1.0\n");
      exit( -1 );
    }
    
    new->f_high = k_val();
    if (new->f_high <= 0.0 || new->f_high > 1.0)
    {
      fprintf(stderr, "fir: invalid f_high; must be between 0.0 and 1.0\n");
      exit( -1 );
    }
    if (new->f_high < new->f_low)
    {
      fprintf(stderr, "fir: invalid f_high; must not be less than f_low\n");
      exit( -1 );
    }
    
    new->level = k_int();
    if (new->level != 0 && new->level != 1)
    {
      fprintf(stderr, "fir: invalid level: must be 0 or 1\n");
      exit( -1 );
    }
    
    new->dev = k_val();
    if (new->dev < MIN_DEV || new->dev > 0.5)
    {
      fprintf(stderr, "fir: invalid deviation;"
              "must be small positive number greater than %lf\n", MIN_DEV);
      exit( -1 );
    }
    
    /* Put the new BAND in the right place in the list                */
    /*   ordered by increasing f_low                                  */
    if (pDcm->pBand == (PBAND) NULL)
    { 
      pDcm->pBand = new;
      new->next = (PBAND) NULL;
    }
    else if (new->f_low < pDcm->pBand->f_low)
    {
      if (pDcm->pBand->next != (PBAND) NULL) new->next = pDcm->pBand->next;
      pDcm->pBand = new;
    } 
    else
    {
      this = pDcm->pBand;
      while( this->next != NULL && this->next->f_low < new->f_low)
        this = this->next;
        
      if (this->next != (PBAND) NULL) 
        new->next = this->next;
      else
        new->next = (PBAND) NULL;
      this->next = new;
    }
    /* Success! */
    return 1;
  }
  
  /* Not a "Band" command */
  return 0;
}
