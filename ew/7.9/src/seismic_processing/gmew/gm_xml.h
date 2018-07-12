/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gm_xml.h 6292 2015-04-03 20:53:45Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2001/03/30 19:14:25  lombard
 *     Initial revision
 *
 *
 *
 */
/*
 * gm_xml.h: write shakemap strong-motion XML file.
 */

#ifndef GM_XML_H
#define GM_XML_H

#include <trace_buf.h>

/**********************************************
   Structures to make up the Agency, Station,
   and Instrument Type long name tables. 
********************************************/
/* If you change these lengths, make sure you change the *
 * sscanf format statements in initMappings()!!!         */
#define AGENCY_LEN 40
#define LONG_STA_LEN 20
#define INST_TYPE_LEN 50

typedef struct {
  char net[TRACE_NET_LEN+1];
  char agency[AGENCY_LEN+1];
} AGENCY;

typedef struct {
  char sta[TRACE_STA_LEN+1];
  char net[TRACE_NET_LEN+1];
  char longName[LONG_STA_LEN+1];
} LONGSTANAME;

typedef struct {
  char sta[TRACE_STA_LEN+1];
  char comp[TRACE_CHAN_LEN+1];
  char net[TRACE_NET_LEN+1];
  char instType[INST_TYPE_LEN+1];
} INSTTYPE;

/* Function Prototypes */
int initMappings( char *mapFile );
int Start_XML_file(  GMPARAMS *, EVENT * );
int Send_XML_activate(  EVENT *, GMPARAMS * );
void next_XML_SCN(GMPARAMS *, STA *, SM_INFO *);
void next_XML_SCNL(GMPARAMS *glmParam, STA *pSta, SM_INFO *pSM);

int Close_XML_file( void );

#endif
