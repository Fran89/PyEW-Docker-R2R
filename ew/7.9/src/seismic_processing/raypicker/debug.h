/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: debug.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: debug.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.5  2004/09/14 21:23:46  cjbryan
 *     *** empty log message ***
 *
 *     Revision 1.4  2004/07/16 19:27:22  cjbryan
 *     allowed continuation of a preliminary trigger
 *
 *     Revision 1.3  2004/04/23 17:27:31  cjbryan
 *      changed bool to int
 *
 *     Revision 1.2  2004/04/21 20:28:23  cjbryan
 *     *** empty log message ***
 *
 *
 *
 */
/*
 * Declarations of global variables and defines used in
 * writing debug files.  The files are written to the log directory,
 * and will accumulate between executions (so move them out of the
 * way between tests).
 * 
 * WARNING: Together, these debug files accumulate at the rate
 *          of over 1 MB / minute.  They should only be used
 *          for debugging of limited-length test data.  None
 *          of these files are suitable for ongoing debugging.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */
#ifndef DEBUG_H
#define DEBUG_H

/* system includes */
#include <stdio.h>                    /* debug file writing */

/* earthworm includes */
#include <global_msg.h>               /* TimeToDTString() */

extern FILE  *g_debugFile;            /* used for various debug outputs */
extern FILE  *dbg_symcalcfile;        /* for symmetry calculation debugging 
                                         (need if DBG_WRITE_SYMMCALC uncommented) */
extern FILE  *g_testFile;             /* used for testing output        */
extern FILE  *g_triggerFile;          /* for trigger debugging 
                                         (need if DBG_WRITE_TRIGGER uncommented) */
extern char   dbg_filename[240];
extern char   dbg_timestr[20];
extern char   dbg_arrstr[20];
extern double dbg_time;
extern int   dbg_write_header;       /* flag  */

/* #define DBG_WRITE_TEST     */            /* uncomment to write the multiple-associated test values      */
/* #define DBG_WRITE_ENVELOPE */            /* uncomment to write the envelope values                      */
/* #define DBG_WRITE_SYMMCALC */            /* uncomment to write symmetry calculation values              */
/* #define DBG_WRITE_FILTERED */            /* uncomment to write the filtered values                      */
/* #define DBG_WRITE_SYMMETRY */            /* uncomment to write the symmetry values                      */
/* #define DBG_WRITE_TRIGGER  */            /* uncomment to write the trigger state                        */

#endif /* DEBUG_H */
