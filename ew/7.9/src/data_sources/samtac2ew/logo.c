/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: logo.c 3536 2009-01-15 22:09:51Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

#include <stdio.h>
#include  "earthworm.h"
#include "glbvars.h"

static unsigned char InstId = 255;

void
setuplogo(MSG_LOGO *logo) 
{
   /* only get the InstId once */
   if (InstId == 255  && GetLocalInst(&InstId) != 0) {
      logit("ET", "samtac2ew: Invalid Installation code; exiting!\n");
      exit(-1);
   }
   logo->mod = gcfg_module_idnum;
   logo->instid = InstId;
}
