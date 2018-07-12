/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ttt.c 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/10/23 07:28:40  davidk
 *     no message
 *
 *     Revision 1.2  2004/10/22 19:51:51  davidk
 *     update do include working version of new traveltime library code.
 *
 *     Revision 1.1  2004/10/20 18:25:36  davidk
 *     no message
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ttt.h>
#include <phase.h>

#define PHASE_BUFFER_FILESIZE 16

int ReadTTTFromFile(TTTableStruct * pTable, char * szFileName)
{
	FILE *fttt;
	int nD, nZ, nT;
	int magic;
  int rc;
  char szPhaseBuffer[PHASE_BUFFER_FILESIZE];

  if(!(fttt = fopen(szFileName, "rb")))
  {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  Could not open travel-time table file (%s)\n",
                  szFileName);
		return(false);
  }
	fread(&magic, sizeof(magic), 1, fttt);
	if(magic != TTT_FILE_FORMAT_VERSION_NUM) {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  Magic sequence at"
                                " beginning of file(%s) not found.  "
                                " Expecting (%d) found(%d)\n",
                szFileName, TTT_FILE_FORMAT_VERSION_NUM, magic);
		return(false);
	}

  /* read the phase name */
	fread(szPhaseBuffer, sizeof(szPhaseBuffer), 1, fttt);
  pTable->ID.iNum = GetPhaseType(szPhaseBuffer);
  if(pTable->ID.iNum == PHASE_Unknown)
  {
    reportTTError(TT_ERROR_WARNING,
                  "ReadTTTFromFile(): ERROR!  Unrecognized phase type(%s) read from file(%s).\n",
                  szPhaseBuffer, szFileName);
		return(false);
  }
  memcpy(&pTable->ID, &Phases[pTable->ID.iNum], sizeof(Phase));

  /* read the Depth/Dist attributes */
	fread(&nD, sizeof(nD), 1, fttt);
	fread(&nZ, sizeof(nZ), 1, fttt);
	fread(&pTable->dDMin, sizeof(pTable->dDMin), 1, fttt);
	fread(&pTable->dDMax, sizeof(pTable->dDMax), 1, fttt);
	fread(&pTable->dZMin, sizeof(pTable->dZMin), 1, fttt);
	fread(&pTable->dZMax, sizeof(pTable->dZMax), 1, fttt);
	pTable->dDDelta = (pTable->dDMax - pTable->dDMin)/(nD-1);
	pTable->dZDelta = (pTable->dZMax - pTable->dZMin)/(nZ-1);

  /* allocate the Depth/Dist table */
  pTable->peDZTable = (TTEntry *) malloc(nD*nZ*sizeof(TTEntry));
  if(!pTable->peDZTable)
  {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  could not alloc %d bytes for Depth/Dist table"
                                " with %d rows and %d cols from file(%s).\n",
                 nD*nZ*sizeof(TTEntry), nZ, nD, szFileName);
		return(false);
  }

  /* read the Depth/Dist table */
	rc = fread(pTable->peDZTable, sizeof(TTEntry), nD*nZ, fttt);
  if(rc != (nD*nZ))
  {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  could not read entire Depth/Dist table"
                                " from file(%s).  Expecting (%d) entries, read(%d)\n",
                szFileName, nD*nZ, rc);
		return(false);
  }


  /* read the Depth/Time attributes (Depth info already read)*/
	fread(&nT, sizeof(nT), 1, fttt);
	fread(&pTable->dTMin, sizeof(pTable->dTMin), 1, fttt);
	fread(&pTable->dTMax, sizeof(pTable->dTMax), 1, fttt);
	pTable->dTDelta = (pTable->dTMax - pTable->dTMin)/(nT-1);

  /* allocate the Depth/Time table */
  pTable->peTZTable = (TTEntry *) malloc(nT*nZ*sizeof(TTEntry));
  if(!pTable->peTZTable)
  {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  could not alloc %d bytes for Depth/Time table"
                                " with %d rows and %d cols from file(%s).\n",
                 nD*nZ*sizeof(TTEntry), nZ, nT, szFileName);
		return(false);
  }

  /* read the Depth/Time table */
	rc = fread(pTable->peTZTable, sizeof(TTEntry), nT*nZ, fttt);
  if(rc != (nT*nZ))
  {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  could not read entire Depth/Time table"
                                " from file(%s).  Expecting (%d) entries, read(%d)\n",
                szFileName, nT*nZ, rc);
		return(false);
  }

  /* read the closing magic number to ensure a non-corrupted format */
	fread(&magic, sizeof(magic), 1, fttt);
	if(magic != TTT_FILE_FORMAT_VERSION_NUM) {
    reportTTError(TT_ERROR_FATAL,"ReadTTTFromFile(): ERROR!  Magic sequence at"
                                " end of file(%s) not found.  "
                                " Expecting (%d) found(%d)\n",
                szFileName, TTT_FILE_FORMAT_VERSION_NUM, magic);
		return(false);
	}
  /* close up the file, we're done */
	fclose(fttt);
  fttt = NULL;

  return(true);
}  /* end ReadTTTFromFile() */


int WriteTTTToFile(TTTableStruct * pTable, char * szFileName)
{
	FILE *fttt;
	int nD, nZ, nT;
	int magic = TTT_FILE_FORMAT_VERSION_NUM;
  int rc;
  char szPhaseBuffer[PHASE_BUFFER_FILESIZE];


  if(!(fttt = fopen(szFileName, "wb")))
  {
    reportTTError(TT_ERROR_FATAL,"WriteTTTToFile(): ERROR!  Could not open travel-time table file (%s)\n",
                  szFileName);
		return(false);
  }
	if(fwrite(&magic, sizeof(magic), 1, fttt) != 1)
  {
    reportTTError(TT_ERROR_FATAL,"WriteTTTToFile(): ERROR!  Could not write Magic sequence at"
                                " beginning of file(%s).\n",
                szFileName);
		return(false);
	}

  /* set the Depth/Dist attributes */
  nD = (int)((pTable->dDMax - pTable->dDMin)/pTable->dDDelta + 0.1) + 1;
  nZ = (int)((pTable->dZMax - pTable->dZMin)/pTable->dZDelta + 0.1) + 1;

  /* check the phase name */
  if(GetPhaseType(pTable->ID.szName) == PHASE_Unknown)
  {
    reportTTError(TT_ERROR_WARNING,
                  "WriteTTTToFile(): ERROR!  Table with Unrecognized phase type(%s)"
                  " being written to file(%s).\n",
                  pTable->ID.szName, szFileName);
		return(false);
  }

  /* write the phase name */
  memset(szPhaseBuffer, 0, sizeof(szPhaseBuffer));
  strncpy(szPhaseBuffer, pTable->ID.szName, sizeof(szPhaseBuffer)-1);
	fwrite(szPhaseBuffer, sizeof(szPhaseBuffer), 1, fttt);

  /* write the Depth/Dist attributes */
	fwrite(&nD, sizeof(nD), 1, fttt);
	fwrite(&nZ, sizeof(nZ), 1, fttt);
	fwrite(&pTable->dDMin, sizeof(pTable->dDMin), 1, fttt);
	fwrite(&pTable->dDMax, sizeof(pTable->dDMax), 1, fttt);
	fwrite(&pTable->dZMin, sizeof(pTable->dZMin), 1, fttt);
	fwrite(&pTable->dZMax, sizeof(pTable->dZMax), 1, fttt);

  /* write the Depth/Dist table */
	rc = fwrite(pTable->peDZTable, sizeof(TTEntry), nD*nZ, fttt);
  if(rc != (nD*nZ))
  {
    reportTTError(TT_ERROR_FATAL,"WriteTTTToFile(): ERROR!  could not write entire Depth/Dist table"
                                " to file(%s).  Expecting (%d) entries, wrote(%d)\n",
                szFileName, nD*nZ, rc);
		return(false);
  }

  /* set the Depth/Time attributes */
  nT = (int)((pTable->dTMax - pTable->dTMin)/pTable->dTDelta + 0.1) + 1;

  /* write the Depth/Time attributes (Depth info already written)*/
	fwrite(&nT, sizeof(nT), 1, fttt);
	fwrite(&pTable->dTMin, sizeof(pTable->dTMin), 1, fttt);
	fwrite(&pTable->dTMax, sizeof(pTable->dTMax), 1, fttt);

  /* write the Depth/Time table */
	rc = fwrite(pTable->peTZTable, sizeof(TTEntry), nT*nZ, fttt);
  if(rc != (nT*nZ))
  {
    reportTTError(TT_ERROR_FATAL,"WriteTTTToFile(): ERROR!  could not write entire Depth/Time table"
                                " to file(%s).  Expecting (%d) entries, wrote(%d)\n",
                szFileName, nT*nZ, rc);
		return(false);
  }

  /* write the closing magic number to ensure a non-corrupted format */
	fwrite(&magic, sizeof(magic), 1, fttt);

  /* close up the file, we're done */
	fclose(fttt);
  fttt = NULL;

  return(true);
}  /* end WriteTTTToFile() */
