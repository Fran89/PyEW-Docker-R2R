/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_uw.h 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.1  2001/01/15 03:55:55  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/12/31 17:27:25  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_uw.h
 */

#ifndef LM_UW_H
#define LM_UW_H

#include "lm_config.h"
#include "lm_util.h"

#ifndef LAST_CHAR
#define LAST_CHAR(s) s[strlen(s)-1]
#endif

/* Some defineitions of strings found on and in UW Wood-Anderson *
 * pick and data files.                                          */
#define Z2P_PICK_LABEL "WA"
#define P2P_PLUS_LABEL "WAP"
#define P2P_MINUS_LABEL "WAN"
#define WA_FILE_LABEL ".wa."
#define DEFAULT_PICK_TAG 'a'
#define WA_SOURCE_CODE "WA"
#define LM_UW_COMMENT "Average_M_Lbs"
#define LM_UW_NOMAG "No local magnitude available"

int getUWpick(EVENT *, LMPARAMS *);
int getAmpDirectFromUW(EVENT *, LMPARAMS *);
int getAmpFromUW(EVENT *, LMPARAMS *, DATABUF *);
int getUWResp(char *, char *, char *, char *, ResponseStruct *, EVENT *);
int getUWStaLoc(char *, char *, double *, double *);
void endUWEvent(EVENT *, LMPARAMS *);
void initUWSave(EVENT *, LMPARAMS *);
void saveUWWATrace(DATABUF *, STA *, COMP3 *, LMPARAMS *);
void termUWSave(EVENT *, LMPARAMS *);
void writeUWEvent(EVENT *, LMPARAMS *);

#endif
