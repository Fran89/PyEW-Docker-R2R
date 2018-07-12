/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ttt.h 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:37  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2004/12/02 17:09:44  davidk
 *     Modified TTTableStruct:
 *        Increased size of szTauPPhaseName field to 20 to include support for
 *        any really wierd phase types (PKIKPPKIKP broke the previous 9 char limit)
 *
 *        Added default table values for ResWindowWidth, AssocStrength, and
 *        LocWeight, so that the ttt_gen program could set these attributes on a
 *        per phase basis in addition to a per ttentry basis.
 *
 *     Revision 1.3  2004/11/30 02:50:54  davidk
 *     Added to constants for aproximate conversion of  km to/from degrees on the earth's surface.
 *
 *     Revision 1.2  2004/10/23 07:18:29  davidk
 *     removed iPhaseNum from TTTableStruct, as it is already provided in ID.iNum.
 *
 *     Revision 1.1  2004/10/23 00:33:59  davidk
 *     no message
 *
 *
 *
 */


#ifndef TTT_H
# define TTT_H

#include <phase.h>

#ifndef false
#  define false 0
#  define true 1
#endif  /* false */


typedef struct _TTEntry
{
  float dTPhase;
  float dDPhase;
  float dtdz;
  float dtdx;
  float dTOA;
  float dAssocStrength;  /* binding strength to an origin */
  float dLocWeight;      /* strength in affecting origin location */
  float dMagThresh;      /* minimum Mx magnitude threshhold (not seen on smaller events) */
  float dResidWidth;     /* width of residual std-dev */
  char  * szPhase;
} TTEntry;

typedef struct _TTTableStruct
{
  Phase  ID;
  char   szTauPPhaseName[20];
  double dDMin;
  double dDMax;
  double dDDelta;
  double dZMin;
  double dZMax;
  double dZDelta;
  double dTMin;
  double dTMax;
  double dTDelta;
  double dPMin;  /* criteria used in building tt.  ignore */
  double dPMax;  /* criteria used in building tt.  ignore */
  double dDefaultResWidth;
  double dDefaultAssocStrength;
  double dDefaultLocWeight;
  TTEntry *peDZTable;
  TTEntry *peTZTable;
  char   szModelType[10];
  char   szNote[256];
} TTTableStruct;


int ReadTTTFromFile(TTTableStruct * pTable, char * szFileName);
int WriteTTTToFile(TTTableStruct * pTable, char * szFileName);

/* these are loosely derived from watchdog_client.h */
#define TT_ERROR_DEBUG    1
#define TT_ERROR_WARNING -1
#define TT_ERROR_FATAL   -2

/* rough km_2_degree conversion based on an earth mean radius of 6371.3 */
#define KM2DEG  0.008994
#define DEG2KM  111.19

#define TTT_FILE_FORMAT_VERSION_NUM  1003

int  reportTTError(int severityLevel, 
                   const char *messageFormat, 
                   ... );  

#endif /* TTT_H */

