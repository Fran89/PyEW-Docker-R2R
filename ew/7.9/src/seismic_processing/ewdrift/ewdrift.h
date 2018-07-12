
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewintegrate.h,v 1.6 2004/05/11 18:53:40 dietz Exp $
 *
 *    Revision history:
 *
 *
 
 ewintegrate: Sample xfrm module's header file
     
 */

/*
 * ewintegrate.h: Definitions for the Your Earthworm Module.
 */

/*******                                                        *********/
/*      Redefinition Exclusion                                          */
/*******                                                        *********/
#ifndef __EWDRIFT_H__
#define __EWDRIFT_H__

#include <xfrm.h> 

/*******                                                        *********/
/*      Constant Definitions                                            */
/*******                                                        *********/

/*******                                                        *********/
/*      Structure Definitions                                           */
/*******                                                        *********/

  /*    Debias Parameter Structure (read from *.d files)                */
typedef struct _INTPARAM
{
#include <xfrm_param.h>              /* Common fields -- MUST BE FIRST  */
  /* Fields specific to debias */
} INTPARAM;

  /*    Information Retrieved from Earthworm*.h                         */
typedef struct _INTEWH
{
#include <xfrm_ewh.h>                /* Common fields -- MUST BE FIRST  */
  /* Fields specific to debias */
} INTEWH;

  /*    Decimate World structure                                        */
typedef struct _INTWORLD
{
  /* FIRST 3 LINES MUST BE THESE 3                                      */
  INTEWH      *xfrmEWH;         /* Structure for Earthworm parameters.  */
  INTPARAM    *xfrmParam;       /* Network parameters.                  */
#include <xfrm_world.h>         /* Other Common fields                  */
  /* Fields specific to debias */
  mutex_t      completionMutex; /* To wait for after last packet        */
} INTWORLD;

  /*    SCNL structure specific to this filter                        */
typedef struct _INTSCNL
{
  /* FIRST LINE MUST BE THIS                                      */
#include <xfrm_scnl.h>         /* Other Common fields                  */
  /* Fields specific to ewintegrate */
} INTSCNL;


#endif  /*  __EWDRIFT_H__                                                */
