/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: debug.c 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: debug.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/07/16 19:27:21  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.2  2004/04/23 17:25:42  cjbryan
 *     changed bool to int
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Global variables used in detailed debugging of the raypicker.
 * See debug.h for comments.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

#include "earthworm_defs.h"
#include "debug.h"

FILE  *g_debugFile = NULL;       /* used for various debug outputs */
FILE  *dbg_symcalcfile = NULL;   /* for symmetry calculation debugging 
                                    (need if DBG_WRITE_SYMMCALC uncommented) */
FILE  *g_testFile = NULL;        /* used for testing output */
FILE  *g_triggerFile = NULL;     /* used for testing trigger output 
                                    (need if DBG_WRITE_TEST uncommented) */
char   dbg_filename[240] = { "" };
char   dbg_timestr[20];
char   dbg_arrstr[20];
double dbg_time;
int   dbg_write_header = TRUE;
