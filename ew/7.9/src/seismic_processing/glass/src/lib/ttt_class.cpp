#include <stdlib.h>
#include <string.h>
#include <ttt.hpp>

CTTT::CTTT()
{
  pTable = new(TTTableStruct);
  memset(pTable, 0, sizeof(TTTableStruct));
  memset(szTTTFileName, 0, sizeof(szTTTFileName));
  iNumRows = iNumDCols =iNumTCols = 0;
}

CTTT::CTTT(char * szFileName)
{
  pTable = new(TTTableStruct);
  memset(pTable, 0, sizeof(TTTableStruct));
  memset(szTTTFileName, 0, sizeof(szTTTFileName));
  Load(szFileName);
  iNumRows = iNumDCols =iNumTCols = 0;
}

CTTT::~CTTT()
{
  delete(pTable);
}


bool CTTT::Load(char * szFileName)
{
  int rc;

  strncpy(szTTTFileName, szFileName, sizeof(szTTTFileName)-1);
  rc=ReadTTTFromFile(pTable, szTTTFileName);
  if(rc)
  {
    /* the 0.1 is to compensate for any potential floating point rounding error */
    iNumDCols = (int)(((pTable->dDMax - pTable->dDMin)/pTable->dDDelta) + 0.1) + 1;
    iNumTCols = (int)(((pTable->dTMax - pTable->dTMin)/pTable->dTDelta) + 0.1) + 1;
    iNumRows = (int)(((pTable->dZMax - pTable->dZMin)/pTable->dZDelta) + 0.1) + 1;
    if(iNumRows > 0 && iNumDCols > 0 && iNumTCols > 0)
      return(true);
    else
    {
      reportTTError(TT_ERROR_FATAL, "Insufficient RowsD/RowsT/Cols (%d/%d/%d) for file <%s>\n",
                    iNumRows, iNumDCols, iNumTCols, szFileName);
      return(false);
    }
  }
  return(false);
}

static int iDistCol;
static int iTimeCol;
static int iDepthRow;
static TTEntry  *p00, *p01, *p10, *p11;  // pointers to the four entries to be interpolated 
static double q00, q01, q10, q11;        // contribution strength of the four entries 
static TTEntry * pStrongest;
static double    qStrongest;
static double dStrX0,dStrY0;
static TTEntry  e00, e01, e10, e11;  // temp entries, used when actual not avaialable.
static double    dDeltaZ, dDeltaD, dDeltaT;
static TTEntry  *pNot;
static double    dBaseTime, dBaseDepth;

TTEntry * CTTT::T(double dZkm, double dDdeg, TTEntry * pTTE)
{
  double dAccuracy;
  return(T(dZkm, dDdeg, pTTE, &dAccuracy));
}

TTEntry * CTTT::T(double dZkm, double dDdeg, TTEntry * pTTE, double * pdAccuracy)
{

  // Step 1:  T() is a mirror of the D() function.
  //          T() looks up the time of a phase from a table
  //          based on it's Depth and Angular Distance;
  //          D() looks up the Angular Distance of a phase from a table
  //          based on it's Depth and Time.
  //          Please DO NOT CHANGE ONE without CHANGING THE OTHER (unless you REALLY 
  //           REALLY know what you are doing!
  //          DK 2004/10/19

	if(dDdeg < pTable->dDMin || dDdeg > pTable->dDMax)
    return(NULL);
	if(dZkm < pTable->dZMin || dZkm > pTable->dZMax)
    return(NULL);

  iDistCol  = (int)((dDdeg - pTable->dDMin)/pTable->dDDelta);
	iDepthRow = (int)((dZkm  - pTable->dZMin)/pTable->dZDelta);

  // eliminating this statement to improve performance
	//iTableIndex = iDepthRow*iNumDCols+iDistCol;

  /* set the pointers to the 4 relevant entries to be interpolated */
  p00 = &pTable->peDZTable[iDepthRow*iNumDCols+iDistCol];  // get the entry from iTableIndex
  pNot = p00;

  if(p00->dTPhase < 0.0)
  { 
    /************
    reportTTError(TT_ERROR_WARNING, 
                  "CTTT:T() Lookup for (z=%.2f d=%.2f) Valid table index(%d/%d),"
                  " but no valid entry at dist (%.2f).\n",
                  dZkm, dDdeg, iDepthRow, iDistCol, 
                  pTable->dDMin + iDistCol * pTable->dDDelta);
     ************/
    p00 = NULL;
  }

  if(dZkm > (pTable->dZMin + pTable->dZDelta * iDepthRow))  // if our depth is between rows
  {
    if(iDepthRow >= (iNumRows-1))  // if we're already in the last row
    {
      // we're supposed to use data from the next row in our interpolated
      // calculation, but there is no "next row".  This is a logic error.
      // make some noise, and then quit and go home.
      reportTTError(TT_ERROR_WARNING, "CTTT:T() Lookup for (z=%.2f d=%.2f) Zmax = %.2f, but no row after z=%.2f(%d).\n",
                    dZkm, dDdeg, pTable->dZMax, 
                    pTable->dZMin + pTable->dZDelta * iDepthRow, iDepthRow);
      return(NULL);
    }
    p10 = pNot + iNumDCols;  // p10 is the matching entry in the next row (depth)
    if(p10->dTPhase < 0.0)
    { 
      p10 = NULL;
    }
  }
  else
  {
    p10 = p00;  // the depth of the first entry is right-on!
  }

  if(dDdeg > (pTable->dDMin + pTable->dDDelta * iDistCol)) // if our distnace is between cols
  {
    if(iDistCol >= (iNumDCols-1))   // if we're already in the last col
    {
      // we're supposed to use data from the next col in our interpolated
      // calculation, but there is no "next col".  This is a logic error.
      // make some noise, and then quit and go home.
      reportTTError(TT_ERROR_WARNING, "CTTT:T() Lookup for (z=%.2f d=%.2f) Dmax = %.2f, but no row after d=%.2f(%d).\n",
                    dZkm, dDdeg, pTable->dZMax, 
                    pTable->dDMin + pTable->dDDelta * iDistCol, iDistCol);
      return(NULL);
    }
    p01 = pNot + 1;          // p10 is the matching entry in the next col (dist)
    if(p01->dTPhase < 0.0)  // if the new entry is not a valid data point 
      p01 = NULL;            //   go back to the old.
  }
  else
  {
    p01 = p00;  // the dist of the first entry is right-on!
  }

  p11 = pNot;  // start it at pNot, and then increment it by 1 col + 1 row, unless are requested value is not between cols/rows
  if(dZkm > (pTable->dZMin + pTable->dZDelta * iDepthRow))  // if our depth is between rows
    p11+= iNumDCols;
  if(dDdeg > (pTable->dDMin + pTable->dDDelta * iDistCol)) // if our distnace is between cols
    p11+= 1;

  if(p11->dTPhase < 0.0)    // if the new entry is not a valid data point 
     p11 = NULL;             //   mark it as invalid

  /* if none of the entries is valid.  quit and return null. */
  if(!(p00 || p01 || p10 || p11))
  {
    /* none of the entries is valid.  No data available. */
    return(NULL);
  }


  /* compute the strength of each entry in determining the interpolated final */
  //combined strength of entries from the 1st row
  dStrY0 = 1.0 - (dZkm -  (pTable->dZMin + (iDepthRow*pTable->dZDelta) ) ) / pTable->dZDelta; 
  //combined strength of entries from the 1st col
  dStrX0 = 1.0 - (dDdeg - (pTable->dDMin + (iDistCol*pTable->dDDelta ) ) ) / pTable->dDDelta;

  /* calculate the strength of the 4 contributing entries */
  q00 = dStrY0 * dStrX0;
  q01 = dStrY0 * (1.0 - dStrX0);
  q10 = (1.0 - dStrY0) * dStrX0;
  q11 = (1.0 - dStrY0) * (1.0 - dStrX0);

  // DK CLEANUP  - make sure the combined strengths add to 1
  double dQTotal = q00 + q01 + q10 + q11;
  if((dQTotal < 0.99) || (dQTotal > 1.01))
  {
    reportTTError(TT_ERROR_WARNING, "CTTT:T() Lookup for (z=%.2f d=%.2f) Bad probabilities (%.2f %.2f %.2f %.2f = %.2f)\n",
                  dZkm, dDdeg, q00, q01, q10, q11, dQTotal);
    return(NULL);
  }

  // we may not interpolate all of the TTEntry values.  Save a pointer to 
  // strongest contributive entry.
  pStrongest = p00;
  if(p00)
    qStrongest = q00;
  else
    qStrongest = 0.0;

  if(q01 >= qStrongest && p01)
  {
    pStrongest = p01;
    qStrongest = q01;
  }
  if(q10 >= qStrongest && p10)
  {
    pStrongest = p10;
    qStrongest = q10;
  }
  if(q11 >= qStrongest && p11)
  {
    pStrongest = p11;
    qStrongest = q11;
  }

  /* initialize the custom entry with the values of the strongest */
  memcpy(pTTE,pStrongest, sizeof(TTEntry));
 
  *pdAccuracy = 0.0;

  if(!p00)
  {
    p00 = &e00;
    memcpy(p00,pStrongest, sizeof(TTEntry));
    if(pStrongest == p01)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 - pTable->dZDelta;
    
    if(pStrongest == p10)
      dDeltaD = 0.0;
    else 
      dDeltaD = 0.0 - pTable->dDDelta;
    p00->dDPhase = (float)(pStrongest->dDPhase + dDeltaD);

    p00->dTPhase = (float)(pStrongest->dTPhase + (dDeltaD * (pStrongest->dtdx)) - (dDeltaZ  * KM2DEG * (pStrongest->dtdz)));
  }
  else
  {
    *pdAccuracy+= q00;
  }

  if(!p01)
  {
    p01 = &e01;
    memcpy(p01,pStrongest, sizeof(TTEntry));
    if(pStrongest == p00)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 - pTable->dZDelta;
    
    if(pStrongest == p11)
      dDeltaD = 0.0;
    else 
      dDeltaD = 0.0 + pTable->dDDelta;
    p01->dDPhase = (float)(pStrongest->dDPhase + dDeltaD);

    p01->dTPhase = (float)(pStrongest->dTPhase + (dDeltaD * (pStrongest->dtdx)) - (dDeltaZ  * KM2DEG * (pStrongest->dtdz)));
  }
  else
  {
    *pdAccuracy+= q01;
  }

  if(!p10)
  {
    p10 = &e10;
    memcpy(p10,pStrongest, sizeof(TTEntry));
    if(pStrongest == p11)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 + pTable->dZDelta;
    
    if(pStrongest == p00)
      dDeltaD = 0.0;
    else 
      dDeltaD = 0.0 - pTable->dDDelta;
    p10->dDPhase = (float)(pStrongest->dDPhase + dDeltaD);

    p10->dTPhase = (float)(pStrongest->dTPhase + (dDeltaD * (pStrongest->dtdx)) - (dDeltaZ * KM2DEG * (pStrongest->dtdz)));
  }
  else
  {
    *pdAccuracy+= q10;
  }

  if(!p11)
  {
    p11 = &e11;
    memcpy(p11,pStrongest, sizeof(TTEntry));
    if(pStrongest == p10)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 + pTable->dZDelta;
    
    if(pStrongest == p01)
      dDeltaD = 0.0;
    else 
      dDeltaD = 0.0 + pTable->dDDelta;
    p11->dDPhase = (float)(pStrongest->dDPhase + dDeltaD);

    p11->dTPhase = (float)(pStrongest->dTPhase + (dDeltaD * (pStrongest->dtdx)) - (dDeltaZ  * KM2DEG * (pStrongest->dtdz)));
  }
  else
  {
    *pdAccuracy+= q11;
  }

  /* get the phase name from the table */
  pTTE->szPhase = Phases[pTable->ID.iNum].szName;

  /* interpolate the distance */
  pTTE->dDPhase = (float)(
                  (p00->dDPhase * q00) + 
                  (p01->dDPhase * q01) +
                  (p10->dDPhase * q10) +
                  (p11->dDPhase * q11));

  /* interpolate the time */
  pTTE->dTPhase = (float)(
                  (p00->dTPhase * q00) + 
                  (p01->dTPhase * q01) +
                  (p10->dTPhase * q10) +
                  (p11->dTPhase * q11));

  /* interpolate the take-off angle */
  pTTE->dTOA    = (float)(
                  (p00->dTOA * q00) + 
                  (p01->dTOA * q01) +
                  (p10->dTOA * q10) +
                  (p11->dTOA * q11));

  /* interpolate the vertical slowness */
  pTTE->dtdz    = (float)(
                  (p00->dtdz * q00) + 
                  (p01->dtdz * q01) +
                  (p10->dtdz * q10) +
                  (p11->dtdz * q11));
    
  /* interpolate the horizontal slowness(ray-parameter) */
  pTTE->dtdx    = (float)(
                  (p00->dtdx * q00) + 
                  (p01->dtdx * q01) +
                  (p10->dtdx * q10) +
                  (p11->dtdx * q11));

  /* we could interpolate the other TTEntry properties, but
     that is overkill for now (ESPECIALLY since they are not yet used) */

  /* now ensure that the delivered time is close to the requested time. */
  if((dDdeg - pTTE->dDPhase) > (0.025 * pTable->dDDelta))
  {
    /* THIS CODE SHOULD NO LONGER BE EXECUTED
       I fixed the bug that was causing it to execute, and tested it
       with DebugBreak() for some time, which never triggered.  I pulled
       the debug breaks and left the code in just in case. 
       DK 2005/10/13 */
    // DebugBreak;
    /* adjust dTPhase based on the ray param */
    /* this is for special cases near the end of the 
       table, where the results are affected by the 
       granularity of the table.
       This is not accurate, but should be close enough,
       and a better solution than having to generate especially
       detailed tables near the end of various traveltime curves
       (such as where P becomes P-Diff)
     ***********************************************/
    pTTE->dTPhase += (float)((dDdeg - pTTE->dDPhase)/*deg*/ * pTTE->dtdx/*sec/deg*/);

  }
  
  /* now set pTTE->dDPhase to the requested value */
  pTTE->dDPhase = (float)(dDdeg);

  return(pTTE);
}  // end T()


TTEntry * CTTT::D(double dZkm, double dTsec, TTEntry * pTTE)
{
  double dAccuracy;
  return(D(dZkm, dTsec, pTTE, &dAccuracy));
}


TTEntry * CTTT::D(double dZkm, double dTsec, TTEntry * pTTE, double * pdAccuracy)
{
  // Step 1:  T() is a mirror of the D() function.
  //          T() looks up the time of a phase from a table
  //          based on it's Depth and Angular Distance;
  //          D() looks up the Angular Distance of a phase from a table
  //          based on it's Depth and Time.
  //          Please DO NOT CHANGE ONE without CHANGING THE OTHER (unless you REALLY 
  //           REALLY know what you are doing!
  //          DK 2004/10/19
  
  // DK CLEANUP:  UH... We broke the above rule.  we modified T without modifying D.
  //  We need to clean this up, or atleast revisit the modifications and make an
  //  appropriate comment.  10/10/2005 DK
	if(dTsec < pTable->dTMin || dTsec > pTable->dTMax)
    return(NULL);
	if(dZkm < pTable->dZMin || dZkm > pTable->dZMax)
    return(NULL);

  iTimeCol  = (int)((dTsec - pTable->dTMin)/pTable->dTDelta);
	iDepthRow = (int)((dZkm  - pTable->dZMin)/pTable->dZDelta);

  // eliminating this statement to improve performance
	//iTableIndex = iDepthRow*iNumDCols+iTimeCol;

  /* set the pointers to the 4 relevant entries to be interpolated */
  p00 = &pTable->peTZTable[iDepthRow*iNumTCols+iTimeCol];  // get the entry from iTableIndex
  pNot = p00;

  if(p00->dTPhase < 0.0)
  {
    /************
    reportTTError(TT_ERROR_WARNING, 
                  "CTTT:T() Lookup for (z=%.2f t=%.2f) Valid table index(%d/%d),"
                  " but no valid entry at time (%.2f).\n",
                  dZkm, dTsec, iDepthRow, iTimeCol, 
                  pTable->dTMin + iTimeCol * pTable->dTDelta);
     ************/
    p00 = NULL;
  }
  if(dZkm > (pTable->dZMin + pTable->dZDelta * iDepthRow))  // if our depth is between rows
  {
    if(iDepthRow >= (iNumRows-1))  // if we're already in the last row
    {
      // we're supposed to use data from the next row in our interpolated
      // calculation, but there is no "next row".  This is a logic error.
      // make some noise, and then quit and go home.
      reportTTError(TT_ERROR_WARNING, "CTTT:D() Lookup for (z=%.2f t=%.2f) Zmax = %.2f, but no row after z=%.2f(%d).\n",
                    dZkm, dTsec, pTable->dZMax, 
                    pTable->dZMin + pTable->dZDelta * iDepthRow, iDepthRow);
      return(NULL);
    }
    p10 = pNot + iNumTCols;  // p10 is the matching entry in the next row (depth)
    if(p10->dDPhase < 0.0)  // if the new entry is not a valid data point 
      p10 = NULL;            //   go back to the old.
  }
  else
  {
    p10 = p00;  // the depth of the first entry is right-on!
  }

  if(dTsec > (pTable->dTMin + pTable->dTDelta * iTimeCol)) // if our time is between cols
  {
    if(iTimeCol >= (iNumTCols-1))   // if we're already in the last col
    {
      // we're supposed to use data from the next col in our interpolated
      // calculation, but there is no "next col".  This is a logic error.
      // make some noise, and then quit and go home.
      reportTTError(TT_ERROR_WARNING, "CTTT:D() Lookup for (z=%.2f t=%.2f) Dmax = %.2f, but no col after d=%.2f(%d).\n",
                    dZkm, dTsec, pTable->dZMax, 
                    pTable->dTMin + pTable->dTDelta * iTimeCol, iTimeCol);
      return(NULL);
    }
    p01 = pNot + 1;          // p10 is the matching entry in the next col (time)
    if(p01->dDPhase < 0.0)  // if the new entry is not a valid data point 
      p01 = NULL;            //   go back to the old.
  }
  else
  {
    p01 = p00;  // the time of the first entry is right-on!
  }

  p11 = pNot;  // start it at pNot, and then increment it by 1 col + 1 row, unless are requested value is not between cols/rows
  if(dZkm > (pTable->dZMin + pTable->dZDelta * iDepthRow))  // if our depth is between rows
    p11+= iNumTCols;
  if(dTsec > (pTable->dTMin + pTable->dTDelta * iTimeCol)) // if our distnace is between cols
    p11+= 1;

  if(p11->dDPhase < 0.0)    // if the new entry is not a valid data point 
     p11 = NULL;             //   go back to the old.

  /* if none of the entries is valid.  quit and return null. */
  if(!(p00 || p01 || p10 || p11))
  {
    /* none of the entries is valid.  No data available. */
    return(NULL);
  }

  /* compute the strength of each entry in determining the interpolated final */
  //combined strength of entries from the 1st row
  dStrY0 = 1.0 - (dZkm -  (pTable->dZMin + (iDepthRow*pTable->dZDelta) ) ) / pTable->dZDelta; 
  //combined strength of entries from the 1st col
  dStrX0 = 1.0 - (dTsec - (pTable->dTMin + (iTimeCol*pTable->dTDelta ) ) ) / pTable->dTDelta;

  /* calculate the strength of the 4 contributing entries */
  q00 = dStrY0 * dStrX0;
  q01 = dStrY0 * (1.0 - dStrX0);
  q10 = (1.0 - dStrY0) * dStrX0;
  q11 = (1.0 - dStrY0) * (1.0 - dStrX0);

  // DK CLEANUP  - make sure the combined strengths add to 1
  double dQTotal = q00 + q01 + q10 + q11;
  if((dQTotal < 0.99) || (dQTotal > 1.01))
  {
    reportTTError(TT_ERROR_WARNING, "CTTT:D() Lookup for (z=%.2f t=%.2f) Bad probabilities (%.2f %.2f %.2f %.2f = %.2f)\n",
                  dZkm, dTsec, q00, q01, q10, q11, dQTotal);
    return(NULL);
  }

  // we may not interpolate all of the TTEntry values.  Save a pointer to 
  // strongest contributive entry.
  pStrongest = p00;
  if(p00)
    qStrongest = q00;
  else
    qStrongest = 0.0;

  if(q01 >= qStrongest && p01)
  {
    pStrongest = p01;
    qStrongest = q01;
  }
  if(q10 >= qStrongest && p10)
  {
    pStrongest = p10;
    qStrongest = q10;
  }
  if(q11 >= qStrongest && p11)
  {
    pStrongest = p11;
    qStrongest = q11;
  }


  /***************************************************
   * THIS IS A SPECIAL STEP FOR D() that is not in T.
   * Because the Dist values cannot be directly 
   * derived from Time and Depth, but must instead be
   * generated by an oversampling of Dist and Depth
   * to derive enough time values to get approximately
   * approximately the desired time values, the entries
   * aren't exact.  Meaning that if you ask for the
   * distance for a Pn phase at 15 seconds and 5km,
   * then you are likely to get a phase record that 
   * is 14.95 seconds or 15.03, instead of one that is
   * exactly at 15.  To circumvent this problem without
   * having to do something more complex than a linear
   * interpolation, we will "calibrate" each table entry
   * by adjusting it's time from the actual to the theoretical
   * and appropriately adjusting the distance via dtdx
   ******************************************************/

  /* calculate shortcut values for the 00 Time and Depth */
  dBaseTime = pTable->dTMin + iTimeCol*pTable->dTDelta;
  dBaseDepth =  pTable->dZMin + iDepthRow * pTable->dZDelta;

  if(p00)
  {
    dDeltaT = (dBaseTime) - p00->dTPhase ;
    dDeltaD = dDeltaT/*sec*/ / p00->dtdx/*sec/deg*/;
    p00->dTPhase+=(float)dDeltaT;
    p00->dDPhase+=(float)dDeltaD;
  }
  if(p01)
  {
    dDeltaT = (dBaseTime + pTable->dTDelta) - p01->dTPhase ;
    dDeltaD = dDeltaT/*sec*/ / p01->dtdx/*sec/deg*/;
    p01->dTPhase+=(float)dDeltaT;
    p01->dDPhase+=(float)dDeltaD;
  }
  if(p10)
  {
    dDeltaT = (dBaseTime) - p10->dTPhase ;
    dDeltaD = dDeltaT/*sec*/ / p10->dtdx/*sec/deg*/;
    p10->dTPhase+=(float)dDeltaT;
    p10->dDPhase+=(float)dDeltaD;
  }
  if(p11)
  {
    dDeltaT = (dBaseTime + pTable->dTDelta) - p11->dTPhase ;
    dDeltaD = dDeltaT/*sec*/ / p11->dtdx/*sec/deg*/;
    p11->dTPhase+=(float)dDeltaT;
    p11->dDPhase+=(float)dDeltaD;
  }

  /* initialize the custom entry with the values of the strongest */
  memcpy(pTTE,pStrongest, sizeof(TTEntry));
 
  *pdAccuracy = 0.0;


  if(!p00)
  {
    p00 = &e00;
    memcpy(p00,pStrongest, sizeof(TTEntry));
    if(pStrongest == p01)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 - pTable->dZDelta;
    
    if(pStrongest == p10)
      dDeltaT = 0.0;
    else 
      dDeltaT = 0.0 - pTable->dTDelta;

    p00->dTPhase = (float)(pStrongest->dTPhase + dDeltaT);

    p00->dDPhase = (float)(pStrongest->dDPhase + (dDeltaT + (dDeltaZ  * KM2DEG * pStrongest->dtdz)) / pStrongest->dtdx);
  }
  else
  {
    *pdAccuracy+= q00;
  }

  if(!p01)
  {
    p01 = &e01;
    memcpy(p01,pStrongest, sizeof(TTEntry));
    if(pStrongest == p00)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 - pTable->dZDelta;
    
    if(pStrongest == p11)
      dDeltaT = 0.0;
    else 
      dDeltaT = 0.0 + pTable->dTDelta;
    p01->dTPhase = (float)(pStrongest->dTPhase + dDeltaT);

    p01->dDPhase = (float)(pStrongest->dDPhase + (dDeltaT + (dDeltaZ  * KM2DEG * pStrongest->dtdz)) / pStrongest->dtdx);
  }
  else
  {
    *pdAccuracy+= q01;
  }

  if(!p10)
  {
    p10 = &e10;
    memcpy(p10,pStrongest, sizeof(TTEntry));
    if(pStrongest == p11)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 + pTable->dZDelta;
    
    if(pStrongest == p00)
      dDeltaT = 0.0;
    else 
      dDeltaT = 0.0 - pTable->dTDelta;
    p10->dTPhase = (float)(pStrongest->dTPhase + dDeltaT);

    p10->dDPhase = (float)(pStrongest->dDPhase + (dDeltaT + (dDeltaZ  * KM2DEG * pStrongest->dtdz)) / pStrongest->dtdx);
  }
  else
  {
    *pdAccuracy+= q10;
  }

  if(!p11)
  {
    p11 = &e11;
    memcpy(p11,pStrongest, sizeof(TTEntry));
    if(pStrongest == p10)
      dDeltaZ = 0.0;
    else 
      dDeltaZ = 0.0 + pTable->dZDelta;
    
    if(pStrongest == p01)
      dDeltaT = 0.0;
    else 
      dDeltaT = 0.0 + pTable->dTDelta;
    p11->dTPhase = (float)(pStrongest->dTPhase + dDeltaT);

    p11->dDPhase = (float)(pStrongest->dDPhase + (dDeltaT + (dDeltaZ  * KM2DEG * pStrongest->dtdz)) / pStrongest->dtdx);
  }
  else
  {
    *pdAccuracy+= q11;
  }




  /* get the phase name from the table */
  pTTE->szPhase = Phases[pTable->ID.iNum].szName;

  /* interpolate the time */
  pTTE->dTPhase = (float)(
                  (p00->dTPhase * q00) + 
                  (p01->dTPhase * q01) +
                  (p10->dTPhase * q10) +
                  (p11->dTPhase * q11));

  /* interpolate the dist */
  pTTE->dDPhase = (float)(
                  (p00->dDPhase * q00) + 
                  (p01->dDPhase * q01) +
                  (p10->dDPhase * q10) +
                  (p11->dDPhase * q11));

  /* interpolate the take-off angle */
  pTTE->dTOA    = (float)(
                  (p00->dTOA * q00) + 
                  (p01->dTOA * q01) +
                  (p10->dTOA * q10) +
                  (p11->dTOA * q11));

  /* interpolate the vertical slowness */
  pTTE->dtdz    = (float)(
                  (p00->dtdz * q00) + 
                  (p01->dtdz * q01) +
                  (p10->dtdz * q10) +
                  (p11->dtdz * q11));
    
  /* interpolate the horizontal slowness(ray-parameter) */
  pTTE->dtdx    = (float)(
                  (p00->dtdx * q00) + 
                  (p01->dtdx * q01) +
                  (p10->dtdx * q10) +
                  (p11->dtdx * q11));

  /* we could interpolate the other TTEntry properties, but
     that is overkill for now (ESPECIALLY since they are not yet used) */

  /* now ensure that the delivered time is close to the requested time. */
  if((dTsec - pTTE->dTPhase) > (0.05 * pTable->dTDelta))
  {
    /* THIS CODE SHOULD NO LONGER BE EXECUTED
       I fixed the bug that was causing it to execute, and tested it
       with DebugBreak() for some time, which never triggered.  I pulled
       the debug breaks and left the code in just in case. 
       DK 2005/10/13 */
    // DebugBreak;
    /* adjust dTPhase based on the ray param */
    /* this is for special cases near the end of the 
       table, where the results are affected by the 
       granularity of the table.
       This is not accurate, but should be close enough,
       and a better solution than having to generate especially
       detailed tables near the end of various traveltime curves
       (such as where P becomes P-Diff)
     ***********************************************/
    pTTE->dDPhase += (float)((dTsec - pTTE->dTPhase)/*sec*/ / pTTE->dtdx/*sec/deg*/);

  }

  /* now set the distance = the requested distance */
  pTTE->dTPhase = (float)dTsec;

  return(pTTE);
}  // D()

