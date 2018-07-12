/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tttlist_class.cpp 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.5  2006/01/25 20:20:29  davidk
 *     Added code that automatically returns null in TBestByAll() when there are no
 *     traveltime tables.
 *
 *     Revision 1.4  2006/01/24 21:17:43  davidk
 *     Fixed declarations of template iterators, because the vc05 compiler
 *     complained heavily.
 *
 *     Revision 1.3  2005/10/21 23:10:17  davidk
 *     Fixed a bug introduced during recent Accuracy code addition, where a phase that
 *     wasn't completely accurate could be overriden by a phase that wasn't even
 *     close to the requested time, because it had a higher accuracy.
 *     This was killing glass in the 95 - 100 degree range where PP was overriding
 *     P/Pdif.  Added a sanity check based on requestedtime - residual window width.
 *
 *     Revision 1.2  2005/10/13 21:37:26  davidk
 *     Added code to take advantage of the new Accuracy parameter in the T() function.
 *     The code now selects the most accurate time available from all the tables,
 *     and if there is more than one, will then select the time closest to that which
 *     was requested.  Fixed a problem with cohesivness between separate tables
 *     where times would get off kilter at an area where two tables come together,
 *     such as P and Pdif around 100 degrees.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.6  2005/03/09 22:34:36  mark
 *     Attempt at cleanup
 *
 *
 */

#include <tttlist.hpp>

#define DELIMITERS " \t\n"

CTTTList::CTTTList()
{
}


CTTTList::~CTTTList()
{
	// Call the destructors for anything in our table list.  (I think.)
	vTableList.clear();
}

int  CTTTList::Load(char * szFileName)
{
  FILE * fpIn;
  char   szBuffer[256];
  char * szToken;
  int    iNumTables = 0;
  int    iLineNum = 0;
  CTTT * pTTT;
  int    rc;

  memset(szBuffer, 0, sizeof(szBuffer));

  /* load the tt tables */
  fpIn = fopen(szFileName, "r");
  if(!fpIn)
  {
    reportTTError(TT_ERROR_FATAL,
                  "ERROR: CTTTList::Load(): Could not open traveltime table list"
                  " input file (%s)\n", 
                  szFileName);
    return(-1);
  }

  while(fgets(szBuffer, sizeof(szBuffer)-1, fpIn))
  {
    iLineNum++;
    if(szBuffer[0] == '#' || szBuffer[0] == 0x00)
    {
      /* ignore blank lines and comments */
      continue;
    }
    szToken = strtok(szBuffer, DELIMITERS);

    pTTT = new CTTT();
  
    if(!(rc = pTTT->Load(szToken)))
    {
      reportTTError(TT_ERROR_FATAL,
                    "ERROR: CTTTList::Load(): load traveltime table (%s) from line %d of input file (%s)\n", 
                    szFileName);
      return(-1);
    }
    Add(pTTT);
    iNumTables++;
  }  /* while there are more lines in the input file */

  if(!iNumTables)
  {
      reportTTError(TT_ERROR_FATAL,
                    "ERROR: CTTTList::Load(): No traveltime tables loaded from input file (%s)\n", 
                    szFileName);
      return(iNumTables);
  }

  return(iNumTables);
}  /* end CTTTList::Load() */


bool CTTTList::Add(CTTT * pNewTable)
{
  if(!pNewTable)
    return(false);

  vTableList.push_back(pNewTable);
  return(true);
}

TTEntry * CTTTList::TBestByAll(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE, PhaseClass iClass, PhaseType iType)
{
  TTEntry Entry;
  TTEntry * pTempEntry;
  double t1,t2;
  bool bEntrySet = false;
  vector <CTTT *>::iterator pCurrTable;
  vector <CTTT *>::iterator pLastTable;
  double dAccuracy, dLastAccuracy;


  if (!vTableList.size())
	  return(NULL);
  pCurrTable = (vTableList.begin());
  pLastTable = pCurrTable + vTableList.size() - 1;
  /* iterate through each table in the list  - the old fashioned way */
  for(; pCurrTable <= pLastTable; pCurrTable++)
  {
    if(iClass != PHASECLASS_Unknown && iClass != (*pCurrTable)->pTable->ID.iClass)
      continue;  /* not the right class */

    if(iType != PHASE_Unknown && iType != (*pCurrTable)->pTable->ID.iNum)
      continue;  /* not the right type */

    /* get the time for this table based on depth/dist */
    pTempEntry = (*pCurrTable)->T(dZkm, dDdeg, pTTE, &dAccuracy);
    if(!pTempEntry)
      continue;

    if(!bEntrySet)
    {
      bEntrySet = true;
      memcpy(&Entry, pTempEntry, sizeof(Entry));
      dLastAccuracy = dAccuracy;
    }
    else
    {
      /* if the current entry is closer than the old one, then use
         the current one instead.   In the future, this code should
         be more complex(complicated) and perform a more advanced comparison,
         based on some combination of phase probability, and residual window width.
       ****************************************************************************/
      t1 = Entry.dTPhase - dTsec;
      if(t1 < 0.0)
        t1 = 0.0 - t1;
      t2 = pTempEntry->dTPhase - dTsec;
      if(t2 < 0.0)
        t2 = 0.0 - t2;

      if(t1 < Entry.dResidWidth && t2 > pTempEntry->dResidWidth)
      {
        /* do nothing, the original entry stays */
      }
      else if(t1 > Entry.dResidWidth && t2 < pTempEntry->dResidWidth)
      {
        memcpy(&Entry, pTempEntry, sizeof(Entry));
        dLastAccuracy = dAccuracy;
      }
      else
      {
        if(dAccuracy > (1.05 * dLastAccuracy))  // allow a 5% slop 
        {
          memcpy(&Entry, pTempEntry, sizeof(Entry));
          dLastAccuracy = dAccuracy;
        }
        else if(dAccuracy > (0.95 * dLastAccuracy) && t2 < t1)
        {
          memcpy(&Entry, pTempEntry, sizeof(Entry));
          dLastAccuracy = dAccuracy;
        }
      }
    }
  }   /* end for each table */

  if(bEntrySet)
  {
    memcpy(pTTE, &Entry, sizeof(Entry));
    return(pTTE);
  }
  else
  {
    return(NULL);
  }
}  /* end TBestByAll() */


TTEntry * CTTTList::DBestByAll(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE, PhaseClass iClass, PhaseType iType)
{
  TTEntry Entry;
  TTEntry * pTempEntry;
  int i,iSize;
  double d1,d2;
  bool bEntrySet = false;


  iSize = vTableList.size();
  for(i=0; i < iSize; i++)
  {
    if(iClass != PHASECLASS_Unknown && iClass != vTableList[i]->pTable->ID.iClass)
      continue;  /* not the right class */

    if(iType != PHASE_Unknown && iType != vTableList[i]->pTable->ID.iNum)
      continue;  /* not the right type */

    /* get the distance for this table based on depth/time */
    pTempEntry = vTableList[i]->D(dZkm, dTsec, pTTE);
    if(!pTempEntry)
      continue;

    if(!bEntrySet)
    {
      bEntrySet = true;
      memcpy(&Entry, pTempEntry, sizeof(Entry));
    }
    else
    {
      /* if the current entry is closer than the old one, then use
         the current one instead.   In the future, this code should
         be more complex(complicated) and perform a more advanced comparison,
         based on some combination of phase probability, and residual window width.
       ****************************************************************************/
      d1 = Entry.dDPhase - dDdeg;
      if(d1 < 0.0)
        d1 = 0.0 - d1;
      d2 = pTempEntry->dDPhase - dDdeg;
      if(d2 < 0.0)
        d2 = 0.0 - d2;
      if(d2 < d1)
        memcpy(&Entry, pTempEntry, sizeof(Entry));
    }
  }   /* end for each table */

  if(bEntrySet)
  {
    memcpy(pTTE, &Entry, sizeof(Entry));
    return(pTTE);
  }
  else
  {
    return(NULL);
  }
}  /* end DBestByAll() */

TTEntry * CTTTList::TBest(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE)
{
  return(TBestByAll(dZkm, dDdeg, dTsec, pTTE, PHASECLASS_Unknown, PHASE_Unknown));
}

TTEntry * CTTTList::TBestByClass(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE, PhaseClass iClass)
{
  return(TBestByAll(dZkm, dDdeg, dTsec, pTTE, iClass, PHASE_Unknown));
}

TTEntry * CTTTList::TBestByPhase(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE, PhaseType iPhase)
{
  return(TBestByAll(dZkm, dDdeg, dTsec, pTTE, PHASECLASS_Unknown, iPhase));
}

TTEntry * CTTTList::DBest(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE)
{
  return(DBestByAll(dZkm, dTsec, dDdeg, pTTE, PHASECLASS_Unknown, PHASE_Unknown));
}

TTEntry * CTTTList::DBestByClass(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE, PhaseClass iClass)
{
  return(DBestByAll(dZkm, dTsec, dDdeg, pTTE, iClass, PHASE_Unknown));
}

TTEntry * CTTTList::DBestByPhase(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE, PhaseType iPhase)
{
  return(DBestByAll(dZkm, dTsec, dDdeg, pTTE, PHASECLASS_Unknown, iPhase));
}
