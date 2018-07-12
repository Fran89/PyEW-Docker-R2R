
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: compare.c 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: compare.c,v $
 *     Revision 1.2  2009/02/13 20:36:40  mark
 *     Renamed SCNL to RaypickerSCNL to avoid naming collision
 *
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <string.h>
#include "compare.h"
#include "raypicker.h"


/*************************************************************
 *                      CompareSCNLs()                       *
 *                                                           *
 *  This function is passed to qsort() and bsearch().        *
 *  We use qsort() to sort the station list by SCNL numbers, *
 *  and we use bsearch to look up an SCNL in the liSt.       *
 *************************************************************/
int CompareSCNLs(const void *s1, const void *s2)
{
    int rc;
    RaypickerSCNL *t1 = (RaypickerSCNL *)s1;
    RaypickerSCNL *t2 = (RaypickerSCNL *)s2;

    rc = strcmp(t1->sta, t2->sta);
    if (rc != 0) 
      return rc;
    rc = strcmp(t1->chan, t2->chan);
    if (rc != 0) 
      return rc;
    rc = strcmp(t1->net, t2->net);
    if (rc != 0) 
      return rc;
    rc = strcmp(t1->loc, t2->loc);
    return rc;
}
