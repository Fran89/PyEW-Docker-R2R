
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: index_util.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2007/02/26 21:41:33  paulf
 *     fixed long* that needed to be time_t *
 *
 *     Revision 1.6  2004/05/18 22:30:19  lombard
 *     Modified for location code
 *
 *     Revision 1.5  2001/06/29 22:23:59  lucky
 *     Implemented multi-level debug scheme where the user can specify how detailed
 *     (and large) the log files should be. If no Debug is specified, only
 *     errors are reported and logged.
 *
 *     Revision 1.4  2001/01/17 21:40:33  davidk
 *     added a comment, reformatted a logit, and updated the version stamp.
 *
 *     Revision 1.3  2000/07/08 19:01:07  lombard
 *     Numerous bug fies from Chris Wood
 *
 *     Revision 1.2  2000/06/28 23:45:27  lombard
 *     added signal handler for graceful shutdowns; numerous bug fixes
 *     See README.changes for complete list
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <transport.h>
#include "wave_serverV.h"
#include <time.h>
#include <trace_buf.h>

/*-----------------------------------------------------------------------*/
/*  Updates:
    DavidK 09/09/1998:  Changed NumOfFinishChunks to 
    (NumOfFinishChunks * sizeof(DATA_CHUNK)) in the size parameter of a
    memcpy call in CopyLIndexintoBuffer().  This should fix problems of
    corrupted index entries in index files.
    People were complaining that they could not change the number of index
    entries in the config file, once a tank existed, without blowing the
    tanks away.  They actually could.  The problem was that some of the
    indexes in the tank were probably corrupted(especially the last one),
    which tended to cause BuildLIndex() to search backwards through the
    entire tank, and thus rebuild the indexes for the entire tank, appending
    them to the existing index list, which usually caused the tank to run
    out of indexes again.  Even if it did not, the indexes were still 
    corrupted, and not all that useful. */
    /*-----------------------------------------------------------------------*/
/* Redundant File I/O description 

     To leave open, or closed!?! 
       There are two different ways that redundant file I/O could be 
       accomplished for indexes and tank structure files.
       
       1. Keep file pointers open.  This should limit File I/O overhead, but 
       limits the number of redundant tanks that could be handled to 80, if 
       there can only be 256 file handles.  For each tank there would be 3 
       files, 1 Tank file, and 2 Index files that contain matching data, so
       that if one becomes corrupt, then the other is still valid.
       
       2. Close all redundant fp's when not in use.  This increases File I/O
       overhead, but also raises the number of tanks that can be handled by
       one server to 240 or so, because now there is only one file per tank
       (Tank file) that is open all of the time.  The increased File I/O
       overhead is primarily CPU centric, and involves the extra memory
       work that needs to be done to create and remove active file handles.

       Choice 2 was chosen, because 80 file descriptors seemed to be a more 
       limiting factor in the performance of a wave server, than the additional
       I/O overhead.
       DK

     Alternating Files:
       If there are two files used by a redundant pointer, then the functions
       ReadRFile(), WriteRFile(), TestRFile() and others test and alternate
       between the redundant files.  The Redundant File Pointer (RFPtr), keeps
       track of the current file, which is the file most recently updated.  The
       CurrentFile is used for reads, while the file after the CurrentFile is
       used for writes, that way the most recent data is read, and the oldest
       data is overwritten.

  */


/*-----------------------------------------------------------------------*/
int InitializeFiles(RedundantFilePtr rfPtr)
{
  /* 
     Purpose:
     Determine the oldest file.  Set it to be the current file. 
     Determine the oldest file by a timestamp at the beginning of the file
     of type time_t.

     Return Values:
     1: If atleast 1 good file was found.
     0: If 0 good files were found, but all files now exist
     -1: If there was an unrecoverable error, such as not being able to create
     a file.

     Functions Called:
     System: time(),fopen(),fwrite(),fread(),fclose()
     Earthworm: logit()


     Additional Remarks:
    */
     
  time_t tstamp1=0,tstamp2=0,now=0;
  static char * MyFuncName = "InitializeFiles()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  time(&now);
  if((rfPtr->pCurrentFile=fopen( rfPtr->Filename1, "rb+" )) == NULL)
  {
    if (Debug > 1)
      logit("et","File: %s, not found, attempting to create it!\n",
          rfPtr->Filename1);

    if((rfPtr->pCurrentFile=fopen(rfPtr->Filename1,"wb")) == NULL)
    {
      logit("et","Error:  Could not create file: %s, returning error!\n",
            rfPtr->Filename1);
      return(-1);
    }
    else
    {
      if(fwrite(&now,sizeof(time_t),1,rfPtr->pCurrentFile) < 1)
      {
        logit("et","Error:  Could not write timestamp to new file: %s, returning error!\n",
              rfPtr->Filename1);
        return(-1);
      }
    }
  }
  else  /* Filename1 opened successfully */
  {
    if(!fread(&tstamp1, sizeof(time_t), 1, rfPtr->pCurrentFile))
      tstamp1=0;
  }
  fclose(rfPtr->pCurrentFile);

  if(rfPtr->NumOfFiles == 2)
  {
    if((rfPtr->pCurrentFile=fopen( rfPtr->Filename2, "rb+" )) == NULL)
    {
      if (Debug > 1)
        logit("et","File: %s, not found, attempting to create it!\n",
            rfPtr->Filename2);

      if((rfPtr->pCurrentFile=fopen(rfPtr->Filename2,"wb")) == NULL)
      {
        logit("et","Error:  Could not create file: %s, exiting!\n",
              rfPtr->Filename2);
        return(-1);
      }
      else
      {
        if(fwrite(&now,sizeof(time_t),1,rfPtr->pCurrentFile) < 1)
        {
          logit("et","Error:  Could not write timestamp to new file: %s, returning error!\n",
                rfPtr->Filename2);
          return(-1);
        }
      }
    }
    else  /* Filename2 opened successfully */
    {
      if(!fread(&tstamp2, sizeof(time_t), 1, rfPtr->pCurrentFile))
        tstamp2=0;
    }
    fclose(rfPtr->pCurrentFile);
  }

  if(tstamp1 > now)
  {
    tstamp1=0;
  }
  if(tstamp2 > now)
  {
    tstamp2=0;
  }
  if(tstamp1 == 0 && tstamp2 == 0)
  {
    /* Both files are empty, or should be! 
       Might as well make the current file #1*/
    rfPtr->CurrentFile=1;
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(0);  /* Both files were bogus! */
  }
  else
  {
    if(tstamp1 > tstamp2)
    {
      rfPtr->CurrentFile=1;
    }
    else
    {
      rfPtr->CurrentFile=2;
    }
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(1);
}
        

/*-----------------------------------------------------------------------*/
RedundantFilePtr CreateFilePtr(char* FName1, char* FName2)
{
  /* 
     Purpose:  
     Create a RedundantFilePtr from two filenames.  FName1
     must be a valid filename.  If FName2 is blank, it is ignored,
     and redundant files will not be used.  NULL Pointers are not
     acceptable values, and will cause the function to return
     with an error.

     Return Values:
     NULL:  The call failed.
     1:     An RFP was created with 1 filename.
     2:     An RFP was created with 2 filenames.

     Functions Called:
     System: malloc(),strncpy()
     Earthworm: logit()

     Additional Remarks:
    */


  RedundantFilePtr ret;   
  static char * MyFuncName = "CreateFilePtr()";

  if(Debug > 2)
    fprintf(stderr,"Entering %s\n",MyFuncName);
  

  /* Check for NULL */
  if(FName1 == 0 || FName2 == 0)
  {
    fprintf(stderr, "Null pointer passed to CreateFilePtr: FN1 %lld FN2 %lld\n",
          (long long) FName1,(long long) FName2);
    return(NULL);
  }

  /* Check for blank FName1 */
  if(FName1[0] == 0)
  {
    /* FName1 is required, but is invalid.  Choke */
    fprintf(stderr,"wave_serverV: FName1 is required, but is invalid.\n");
    return(NULL);
  }
  /* else */
  if((ret=(RedundantFilePtr)malloc(sizeof(RedundantFile))) != (RedundantFilePtr)NULL )
  {
    strncpy(ret->Filename1,FName1,MAX_TANK_NAME);
    if(FName2[0] != 0)
    {
      strncpy(ret->Filename2,FName2,MAX_TANK_NAME);
      ret->NumOfFiles=2;
    }
    else
    {
      ret->NumOfFiles=1;
    }
  }

  if(Debug > 2)
    fprintf(stderr,"Exiting %s\n",MyFuncName);
  
  return(ret);
}


/*-----------------------------------------------------------------------*/
int ReadRFile(RedundantFilePtr rfPtr,unsigned int Offset,
              int BytesToRead,char * pBuffer)
{
  /* 
     Purpose:
     Read data from a redundant file pointer into a buffer.

     Return Values:
     1: Read was successful
     0: File was opened for reading, but read was unsuccessful.
     -1: Function was unable to open file for reading.

     Functions Called:
     System: fopen(),fseek(),fread(),fclose()
     Earthworm: logit()

     Additional Remarks:
    */

  char * FName;
  static char * MyFuncName = "ReadRFile()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if(rfPtr->CurrentFile == 2)
  {
    FName=rfPtr->Filename2;
  }
  else
  {
    FName=rfPtr->Filename1;
  }

  if((rfPtr->pCurrentFile=fopen( FName, "rb" )) == NULL)
  {
    logit("et","ReadRFile:  Could not open %s\n",FName);
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(-1);
  }
  fseek(rfPtr->pCurrentFile,Offset,SEEK_SET);
  if(fread(pBuffer, BytesToRead, 1 /*Block*/, rfPtr->pCurrentFile) == 0)
  {
    logit("et","Unsuccessful read of %d bytes, at offset %d in %s\n",
          BytesToRead,Offset,FName);
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(0);
  }
  fclose(rfPtr->pCurrentFile);
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(1);
}

  
/*-----------------------------------------------------------------------*/
int WriteRFile(RedundantFilePtr rfPtr,unsigned int Offset,
               int BytesToWrite, char* pBuffer, int FinishedWithFile)
{
  /* 
     Purpose:
     Write data to a redundant file pointer into a buffer.

     Return Values:
     1: Write operation was successful.
     0: Write operation failed.
     -1: File operation such as fopen(), fclose(), or other failed.

     Functions Called:
     System: fopen(),fwrite(),fseek(),fclose()
     Earthworm: logit()

     Additional Remarks:
     This function writes to a file within a RFPtr.  It writes to the
     oldest file.  The newest file is indicated by CurrentFile.

     Cleanup:  This looks a bit confusing, because we are using
     pCurrentFile to do our file I/O, but it is not related to 
     CurrentFile.

    */

  char * FName;
  int retVal;
  static char * MyFuncName = "WriteRFile()";


  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if(rfPtr->CurrentFile == 1 && rfPtr->NumOfFiles > 1)
  {
    FName=rfPtr->Filename2;
  }
  else
  {
    FName=rfPtr->Filename1;
  }

  if((rfPtr->pCurrentFile=fopen( FName, "rb+" )) == NULL)
  {
    logit("et","WriteRFile:  Could not open %s\n",FName);
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(-1);
  }
  if(fseek(rfPtr->pCurrentFile,Offset,SEEK_SET) != 0)
  {
    logit("t","WriteRFile:  Could not fseek in %s\n",FName);
  }
  retVal = (int)fwrite(pBuffer, BytesToWrite, 1 /*Block*/, rfPtr->pCurrentFile);
  if(fclose(rfPtr->pCurrentFile) != 0)
  {
    logit("et","WriteRFile:  Could not close %s\n",FName);
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(-1);
  }
  if(FinishedWithFile)
  {
    /*Change CurrentFile to be the one we just wrote. */
    rfPtr->CurrentFile++;
    if(rfPtr->CurrentFile > rfPtr->NumOfFiles)
    {
      rfPtr->CurrentFile=1;
    }
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(retVal);
}

/*-----------------------------------------------------------------------*/
int CopyLIndexintoBuffer(TANK * pTSPtr, char* pBuffer)
{
  /* 
     Purpose:
     Copies the living index queue of a tank into a buffer.  The index may be 
     wrapped around a queue array, such that in the array, the last index is
     before the first.  In such a case, the functions copies from the start
     index to the end of the array, and then from the beginning of the array
     to the end index.

     0 1 2 3 4 5 6 7 8

     When start index is 2 and end index is 5, then elements 2,3,4,5 are 
     copied.

     When start index is 6 and end index is 2, then 6,7,8,0,1,2 are copied.

     Return Values:
     0: LIndex was copied successfully

     Functions Called:
     System: memcpy()
     Earthworm: logit()

     Additional Remarks:

    */

  unsigned int NumOfStartChunks,NumOfFinishChunks;
  static char * MyFuncName = "CopyLIndexintoBuffer()";


  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if(pTSPtr->indxStart == pTSPtr->indxFinish && 
     pTSPtr->chunkIndex[pTSPtr->indxStart].tEnd == 0.0)
  {
    /* List is empty, and there is nothing to copy */
  }
  else
  {
    if(pTSPtr->indxStart > pTSPtr->indxFinish)
    {

      NumOfStartChunks=(pTSPtr->indxMaxChnks - pTSPtr->indxStart);
      NumOfFinishChunks=(pTSPtr->indxFinish+1);
      memcpy(pBuffer,&(pTSPtr->chunkIndex[pTSPtr->indxStart]),
             NumOfStartChunks * sizeof(DATA_CHUNK));
      memcpy(pBuffer+NumOfStartChunks * sizeof(DATA_CHUNK),
             pTSPtr->chunkIndex,NumOfFinishChunks * sizeof(DATA_CHUNK));
    }
    else
    {
      NumOfStartChunks=1 + (pTSPtr->indxFinish - pTSPtr->indxStart);
      memcpy(pBuffer,&(pTSPtr->chunkIndex[pTSPtr->indxStart]),
             NumOfStartChunks * sizeof(DATA_CHUNK));
    }
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}

  
/*-----------------------------------------------------------------------*/
int InvalidateRFile(RedundantFilePtr rfPtr)
{
  /*  
      Purpose:
      Invalidate the current file.  All that currently
      involves is setting the CurrentFile to be the other file
      when redundant files are being used.

      Return Values:
      0: Success

      Functions Called:
      System: 
      Earthworm: logit()

      Additional Remarks:
    */
  static char * MyFuncName = "InvalidateRFile()";


  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if(rfPtr->NumOfFiles != 1)
  {
    rfPtr->CurrentFile--;
    if(!rfPtr->CurrentFile)
    {
      rfPtr->CurrentFile=2;
    }
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
int GetLatestTankStructures(TANKList * pTANKList)
     /* 
       Purpose:
        This here function will retrieve TANKs from the newest
        TANK(structures) file on disk.  It will build a list of TANKs
        in memory, and append them to pTANKList.

       Return Values:
        0: Tank list was created.  Reads may or may not have been successful.
           pTANKList may contain an empty list upon return.

       Functions Called:
        (not analyzed)

       Additional Remarks:
        Assuming:  pTANKList contains a valid value for pTSFile, the file
        pointer for the TankStructures file.  The file may not be opened
        already, but the pointer should be a relevant RedundantFilePtr.
        pTANKList should also contain valid values for Tank and Index
        file redundancy.  If there is an error reading from the RFPtr,
        the function assumes that the file being read is new and blank.

        This function is strange because no matter what goes wrong, we
        always return 0(Success).  Maybe this function cannot fail, or
        maybe the author and code reviewers just don't understand what
        this function is supposed to do....   DK 01/08/01
   */

{
  struct TankFileInfo tfiBuffer;
  char * pBuffer=0;
  time_t *pCheckTime;
  unsigned int i,NumOfTankFiles,BufferSize;
  int TANKFileIsValid;
  static char * MyFuncName = "GetLatestTankStructures()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  /* Initialization Activities */
  if(pTANKList->redundantTANKFiles)
  {
    NumOfTankFiles=2;
  }
  else
  {
    NumOfTankFiles=1;
  }
  TANKFileIsValid=0;
  if(InitializeFiles(pTANKList->pTSFile) < 0)
  {
    logit("et","wave_server:  Unrecoverable error during tank structure "
          "files initialization.\n  Wave_server may not start correctly!\n");
  }

  /* Get Valid TANKs */
  for(i=0; i<NumOfTankFiles && TANKFileIsValid==0; i++)
  {
    if(ReadRFile(pTANKList->pTSFile,0,sizeof(struct TankFileInfo),(char *)&tfiBuffer) != 1)
    {
      /* Something's wrong with this tank, lets assume it is new
         and empty, otherwise hopefully InitializeFiles() would have 
         complained.  DK
      */
      /* Times don't match, invalidate file */
      logit("et", "TANK File read failed, File was probably empty\n");
    }
    else
    {
      if(tfiBuffer.NumOfTanks <= MAX_TANKS)
      {
        BufferSize=tfiBuffer.NumOfTanks*sizeof(TANK) + sizeof(time_t);
        pBuffer = (char *) malloc(BufferSize);
        if(ReadRFile(pTANKList->pTSFile,
                     sizeof(time_t)+sizeof(int),BufferSize,pBuffer) != 1)
        {
          logit("t", "TANK File read failed.\n");
          continue;
        }
        pCheckTime=(time_t *)(pBuffer+BufferSize-sizeof(time_t));
        if(tfiBuffer.TimeStamp == *pCheckTime)
        {
          /* Time stamps match, file should be valid */
          pTANKList->pFirstTS=(TANK *)pBuffer;
          pTANKList->NumOfTS=tfiBuffer.NumOfTanks;
          TANKFileIsValid=1;
        }
        else
        {
          /* Times don't match, invalidate file */
          logit("et", "TANK File did not contain matching timestamps\n");
        }
      }
      else
      {
        /* Too many tanks, Invalidate file */
        logit("et", "TANK File was not valid\n");
      }
    } /* else ReadRFile() was successfull */
    if(!TANKFileIsValid)
    {
      InvalidateRFile(pTANKList->pTSFile);
      free(pBuffer);
    }
  }  /* end For all files  && NotValid */

  if(!TANKFileIsValid)
  {
    /* We couldn't find any good tank files on disk.  Return an empty list */
    pTANKList->pFirstTS=NULL;
    pTANKList->NumOfTS=0;
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
int TankIsInList(TANK * pTSPtr, TANKList * pConfigTankList)
{
  /* 
     Purpose:
     Determines whether the TANK at pTSPtr matches a TANK
     in pConfigTankList.  TANKs match when either the pin
     numbers on the tanks match, and the pin# is > 0, or
     when the sta,chan,net & loc, all match,  between the two tanks.

     Return Values:
     1: The tank is in the list, and the tank is not marked as new.
     0: The tank is not in the list.

     Functions Called:
     System: strcmp()
     Earthworm: logit()

     Additional Remarks:

    */

  unsigned int i;
  TANK * pTANKs;
  int TankFound=0;
  static char * MyFuncName = "TankIsInList()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  pTANKs=pConfigTankList->pFirstTS;
  for(i=0; (i < pConfigTankList->NumOfTS) && (!TankFound); i++)
  {
    if(((pTANKs[i].pin == pTSPtr->pin) && (pTANKs[i].pin > 0)) ||
       (!((strcmp(pTANKs[i].sta,pTSPtr->sta)) ||
          (strcmp(pTANKs[i].net,pTSPtr->net)) ||
	  (strcmp(pTANKs[i].loc,pTSPtr->loc)) ||
          (strcmp(pTANKs[i].chan,pTSPtr->chan))
          )) )
    {
      TankFound=1;
    }
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(TankFound);
}


/*-----------------------------------------------------------------------*/
int OpenTankFile(TANK * pTSPtr)
{
  /* 
     Purpose:
     Opens the Tank file for binary reading and writing.

     Return Values:
     0: The file is successfully opened for reading and writing.
     -1: Failure, the file could not be opened.

     Functions Called:
     System: fopen()
     Earthworm: logit()

     Additional Remarks:

    */

  static char * MyFuncName = "OpenTankFile()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if((pTSPtr->tfp=fopen( pTSPtr->tankName, "rb+" )) == 0)
  {
    logit( "e", "wave_serverV: Cannot open tank file <%s>\n",
           pTSPtr->tankName );
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(-1);
  }

  /* else, success */
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
int WriteLIndex(TANK * pTSPtr, int bUseMutexes, time_t CurrentTime)
{
  /* 
     Purpose:
     Make a copy of the living index for a given tank, and then write that
     copy to the index file on disk associated with the tank.  The writing
     is actually done to the RFPtr ifp.

     Return Values:
     0: Write was completed successfully
     -1: Write failed, error was logged.

     Functions Called:
     (not analyzed)

     Additional Remarks:
     Actions:
     1.  Figure out how many index chunks.
     2.  Lock the tank mutex, if applicable.
     2.  Malloc an area for time1 + NumOfChnks + Chunks + time2.
     3.  Grab the current time and copy it into time1 & time2.
     4.  Copy the chunks into Chunks.
     5.  Release the mutex, if applicable.
     6.  Write the index to file.
     */

  unsigned int NumOfChunks,BufferSize;
  time_t * pTStamp1,* pTStamp2;
  char * pBuffer;
  static char * MyFuncName = "WriteLIndex()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if(bUseMutexes)
  {
    RequestSpecificMutex(&(pTSPtr->mutex));
  }
  if(pTSPtr->indxStart == pTSPtr->indxFinish && 
     pTSPtr->chunkIndex[pTSPtr->indxStart].tEnd == 0.0)
  {
    NumOfChunks=0;
  }
  else
  {
    if(pTSPtr->indxStart > pTSPtr->indxFinish)
    {
      NumOfChunks=(pTSPtr->indxFinish+1) +
        (pTSPtr->indxMaxChnks - pTSPtr->indxStart);
    }
    else
    {
      NumOfChunks=1 + (pTSPtr->indxFinish - pTSPtr->indxStart);
    }
  }
  BufferSize=sizeof(time_t) + sizeof(NumOfChunks) + 
    NumOfChunks*sizeof(DATA_CHUNK) + sizeof(time_t);
  if((pBuffer = (char *) malloc(BufferSize)) == NULL)
  {
    logit("t","Malloc() failed in WriteLIndex\n");
    goto abortWriteLIndex;
  }
  if(CopyLIndexintoBuffer(pTSPtr,pBuffer+sizeof(time_t)+sizeof(NumOfChunks))
     != 0)
  {
    logit("t","CopyLIndexintoBuffer() failed in WriteLIndex\n");
    goto abortWriteLIndex;
  }
  pTSPtr->lastIndexWrite=CurrentTime;
  if(bUseMutexes)
  {
    ReleaseSpecificMutex(&(pTSPtr->mutex));
  }
  pTStamp1=(time_t *)pBuffer;
  pTStamp2=(time_t *)(pBuffer+BufferSize-sizeof(time_t));
  *pTStamp1=pTSPtr->lastIndexWrite;
  *pTStamp2=*pTStamp1;
  *((int *) (pBuffer+sizeof(time_t)))=NumOfChunks;
  if(WriteRFile(pTSPtr->ifp,0,BufferSize,pBuffer,1) != 1)
  {
    logit("t","WriteRFile() failed in WriteLIndex\n");
    goto abortWriteLIndex;
  }
  free(pBuffer);
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
 abortWriteLIndex:
  if(pBuffer)
  {
    free(pBuffer);
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(-1);
}


/*-----------------------------------------------------------------------*/
int CreateTankFile(TANK * pTSPtr)
{
  /* 
     Purpose:
     This should try to open (overwriting) a data file for the tank,
     and mark out the size of the file.

     Return Values:
     0: Tank file was successfully opened, and space was allocated.
     -1: Failure occurred while opening tank file and allocating space.

     Functions Called:
     System: fopen(),fseek(),putc(),fclose()
     Earthworm: logit()

     Additional Remarks:
     */

  static char * MyFuncName = "CreateTankFile()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  pTSPtr->tfp = fopen(pTSPtr->tankName, "wb+" );
  if ( pTSPtr->tfp == NULL )
  {
    logit( "e", "wave_serverV: Cannot create tank file <%s>\n",pTSPtr->tankName );
    goto abortCreateTankFile;
  }

  if (Debug > 1)
     logit ("et","wave_serverV: Created %s\n",pTSPtr->tankName);

  pTSPtr->firstPass = 1; /* declare first pass */
  pTSPtr->firstWrite = 1; /* declare first write */

    /* Grab the disk space
    **********************/

    /* make tank size even multiple of record size */
  pTSPtr->tankSize = pTSPtr->nRec * pTSPtr->recSize; 
  if( fseek( pTSPtr->tfp, pTSPtr->tankSize-1, SEEK_SET ) != 0 )
  {
    logit( "e","wave_serverV: Error on fseek to end of tank %s of %ld bytes; exiting!\n",
           pTSPtr->tankName,pTSPtr->tankSize-1 );
    goto abortCreateTankFile;
  }
  if( putc( 0, pTSPtr->tfp ) == EOF )       /* Write the last char in the file */
  {
    logit( "e", "wave_serverV: Error writing EOF to tank %s; exiting!\n",pTSPtr->tankName );
    goto abortCreateTankFile;
  }
  fclose(pTSPtr->tfp);
  if((pTSPtr->tfp = fopen(pTSPtr->tankName, "rb+" ))== NULL)
  {
    logit( "e", "wave_serverV: Error creating Tank file: %s!\n",pTSPtr->tankName);
    goto abortCreateTankFile;
  }

  /* Initialize insertion point to start of file */
  pTSPtr->inPtOffset=0;
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);

 abortCreateTankFile:
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(-1);
}


/*-----------------------------------------------------------------------*/
int MarkTankAsBad(TANK * pTSPtr)
{
  /* 
     Purpose:
     Mark a tank as being bad.

     Return Values:
     0: Success

     Functions Called:
     System: 
     Earthworm: logit()

     Additional Remarks:
     It is always a shame when a big standard header comes along and
     crushes a poor 1 line function.
    */

  static char * MyFuncName = "MarkTankAsBad()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  pTSPtr->isConfigured=-1;
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
int RemoveBadTanksFromList(TANKList * pTANKList)
{
  /* 
     Purpose:
     Removes all tanks marked as bad, from a TANK list.

     Return Values:
     0: If 0 good files were found, but all files now exist
     EXIT:
     Exits if there is a failure during the malloc() of a replacement
     list, or if there is at least one bad tank, and PleaseContinue is
     not set.

     Functions Called:
     (not analyzed)

     Additional Remarks:
     Function attempts to condense the list, removing all bad tanks.  It then
     mallocs a new list, which is just big enough to fit all of the good tank
     structures.  It then free's the older, larger list.  If the malloc fails
     to create the new list, then the function exits wave_server.
    */

  unsigned int i, NumberOfBadTanks=0;
  TANK * pTANK, * pNewFirstTS;
  static char * MyFuncName = "RemoveBadTanksFromList()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  for(i=0; i< pTANKList->NumOfTS; i++)
  {
    pTANK=&(pTANKList->pFirstTS[i]);
    if(pTANK->isConfigured == -1)
    {
      /* Tank is bad, log it and copy over it with a tank from end of list */
      logit("et","wave_server:  Error: Tank %s,%s,%s,%s was marked as bad, and is being removed\n",
            pTANK->sta,pTANK->chan,pTANK->net,pTANK->loc);
      memcpy(pTANK,&(pTANKList->pFirstTS[(pTANKList->NumOfTS)-1]),
             sizeof(TANK));
      pTANKList->NumOfTS--;
      NumberOfBadTanks++;
      i--;   /* We still have to inspect the tank we just copied into place */
    }
  }
  if(NumberOfBadTanks)
  {
    if(PleaseContinue)
    {
      if((pNewFirstTS = (TANK *) malloc(pTANKList->NumOfTS * sizeof(TANK))) == NULL)
      {
        /* Oh crud, we can't create a fresh list.  Logit and die */
        logit("et","Wave_serverV:  Error during malloc in RemoveBadTanks.\n");
        logit("et","Cannot recreate tank list.  Exiting.\n");
        exit(0);
      }
      memcpy(pNewFirstTS,pTANKList->pFirstTS,
             pTANKList->NumOfTS * sizeof(TANK));
      free(pTANKList->pFirstTS);
      pTANKList->pFirstTS=pNewFirstTS;
    }
    else
    {
      /* PleaseContinue is not set and we have atleast 1 bad tank.  Exit*/
      logit("et","%d tanks were found to be bad during initialization.  Exiting\n",
            NumberOfBadTanks);
      logit("et","To continue despite 1 or more tanks being bad, please set 'PleaseContinue' to 1 in the config file\n");
      exit(0);
    }
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
TANKList * CreatePersonalTANKBuffer(TANKList * pSrcList)
{
  /* 
     Purpose:
     Creates a full blank copy of a valid TANKList, and then copies
     the TANKList properties to the new list(Number of tanks,Redundant
     Index Files, etc...).  It allocates room for the same number of 
     TANKs that are in source list, but does not copy the contents of
     the TANKs over to the new list.

     Return Values:
     Valid TANKList *: 
     TANKBuffer successfully created
     NULL: 
     TANKBuffer creation failed, due to malloc() error.

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  TANKList* pRet;
  static char * MyFuncName = "CreatePersonalTANKBuffer()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if((pRet = (TANKList *) malloc(sizeof(TANKList))) == NULL)
  {
    goto abortMallocinCPTB;
  }
  *pRet=*pSrcList;
  if((pRet->pFirstTS = (TANK *) malloc(pSrcList->NumOfTS * sizeof(TANK))) == NULL)
  {
    goto abortMallocinCPTB;
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(pRet);

 abortMallocinCPTB:
  if(pRet)
  {
    if(pRet->pFirstTS)
    {
      free(pRet->pFirstTS);
    }
    free(pRet);
  }
  logit("t","malloc() error in CreatePersonalTANKBuffer\n");
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(NULL);
}


/*-----------------------------------------------------------------------*/
int GetTANKs(TANK * SrcTanks, TANK * DestTanks, int NumOfTanks,
             int UseMutexes)
{
  /* 
     Purpose:
     Copy the values of the TANKs in the source list, to the destination
     list.

     Return Values:
     0: Tanks Successfully copied.

     Functions Called:
     (not analyzed)

     Additional Remarks:  Set the UseMutexes variable to 0 to ignore
     the mutexes.  This is mainly for use by IndexMgr on shutdown,
     where there is a possibility that a server thread had locked
     a mutex, and then had been destroyed, thus leaving the mutex,
     permanently lost.  If UseMutexes is set to 0, the caller is
     responsible for dealing with potentially inconsistent data.

    */

  int i;
  static char * MyFuncName = "GetTANKs()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if(UseMutexes)
  {
    for(i=0;i<NumOfTanks;i++)
    {
      RequestSpecificMutex(&(SrcTanks[i].mutex));
      memcpy(&(DestTanks[i]),&(SrcTanks[i]),sizeof(TANK));
      ReleaseSpecificMutex(&(SrcTanks[i].mutex));
    }
  }
  else
  {
    for(i=0;i<NumOfTanks;i++)
    {
      memcpy(&(DestTanks[i]),&(SrcTanks[i]),sizeof(TANK));
    }
  }

  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
char * GetRedundantFileName(RedundantFilePtr rfPtr)
{
  /* Returns a pointer to the name of the current file for the rfPtr.
     This is dangerous because any modification to the contents of 
     the pointer will change the filename, but this prevents memory
     leaks by avoiding a malloc.  This function should only be used to
     report the name of a file in which an error occurred.
    */
    /* 
       Purpose:
        Returns a pointer to the name of the current file for the rfPtr.
        This is dangerous because any modification to the contents of 
        the pointer will change the filename, but this prevents memory
        leaks by avoiding a malloc.  This function should only be used to
        report the name of a file in which an error occurred.

       Return Values:
        char *: Pointer to the name of the name of the current file in the RFPtr

       Functions Called:
        (not analyzed)

       Additional Remarks:
    */

  static char * MyFuncName = "GetRedundantFileName()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if(rfPtr->CurrentFile == 1)
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(rfPtr->Filename1);
  }
  else
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(rfPtr->Filename2);
  }
}


/*-----------------------------------------------------------------------*/
TANK * ConfigTANK(TANK * pTSPtr, TANKList * pConfigTankList) 
{
  /* 
     Purpose:
     Configures a TANK in pConfigTankList with parameters taken
     from a comparable TANK structure(pTSPtr).  It returns a 
     pointer to the TANK structure that was modified.

     Return Values:
     Valid TANK *: 
     A matching tank was found and successfully configured.  The
     pointer points to the newly configured matching tank in the list.
     NULL: 
     The a matching tank was not found in the list.

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  unsigned int i;
  TANK * pTANKs, * pNewTank;
  int TankFound=-1;
  static char * MyFuncName = "ConfigTANK()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  
  pTANKs=pConfigTankList->pFirstTS;
  for(i=0; i<pConfigTankList->NumOfTS && TankFound<0; i++)
  {
    if(!( (strcmp(pTANKs[i].sta,pTSPtr->sta)) ||
          (strcmp(pTANKs[i].net,pTSPtr->net)) ||
	  (strcmp(pTANKs[i].loc,pTSPtr->loc)) ||
          (strcmp(pTANKs[i].chan,pTSPtr->chan))
          ) )
    {
      TankFound=i;
    }
  }
  if(TankFound >= 0)
  {
    /*We've found the TANK, now copy the info */
    pNewTank = &(pTANKs[TankFound]);
    if((pNewTank->ifp = (IndexFilePtr)malloc(sizeof(RedundantFile))) == NULL)
    {
      logit("t","malloc() failed in ConfigTANK()\n");
      goto configTANKfailure;
    }
    pNewTank->logo = pTSPtr->logo;
    strcpy(pNewTank->datatype,pTSPtr->datatype);
    pNewTank->pin = pTSPtr->pin;
    pNewTank->samprate = pTSPtr->samprate;
    pNewTank->tankSize = pTSPtr->tankSize;
    pNewTank->recSize = pTSPtr->recSize;
    pNewTank->nRec = pTSPtr->nRec;
    pNewTank->inPtOffset = pTSPtr->inPtOffset;
    pNewTank->isConfigured = 1;
    pNewTank->firstPass = pTSPtr->firstPass;
    pNewTank->firstWrite = pTSPtr->firstWrite;
    pNewTank->lastIndexWrite = pTSPtr->lastIndexWrite;

    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(pNewTank);
  }
  else
  {
    /* Tank not found, return error */
  configTANKfailure:
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(NULL);
  }
}


/*-----------------------------------------------------------------------*/
int CleanupIndexThread(TANKList * pTANKList)
{
  /* 
     Purpose:
     None

     Return Values:
     0: Success

     Functions Called:
     (not analyzed)

     Additional Remarks:
     This is a good idea for a function, but there's nothing to stick
     here at the moment.  DK    

  */
  static char * MyFuncName = "CleanupIndexThread()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
int InitTankList(TANKList ** ppConfigTankList,
                 TANK * pTankArray,
                 int RedundantTankStructFiles,
                 int RedundantIndexFiles,
                 int NumberOfTanks,
                 char * TankStructFile,
                 char * TankStructFile2)
{
  /* 
     Purpose:
     Creates a TANKList using the data passed in.  Creates a blank tank list, 
     creates a set of TANK structure files from the two TankStructFile names
     passed in.  Attatches an array of NumberOfTanks tanks, pointed to by 
     pTankArray, and sets the redundant file indicators.

     Return Values:
     0: The tank list is successfully initialized.  
     -1: Failure, due to an error.

     Functions Called:
     Can't call logit() here, it hasn't been initialized yet; use fprintf.

     Additional Remarks:
    */

  TANKList * pConfigTankList;
  static char * MyFuncName = "InitTankList()";

  if(Debug > 2)
    fprintf(stderr,"Entering %s\n",MyFuncName);
  

  /* Create space for the Config Tank List, and the create a quick
       reference to it.
    */
  if((*ppConfigTankList=(TANKList *) malloc(sizeof(TANKList))) == NULL)
  {
    fprintf(stderr,"malloc failed for tank list in InitTankList\n");
    if(Debug > 2)
      fprintf(stderr,"Exiting %s\n",MyFuncName);
    
    return(-1);
  }
  pConfigTankList=*ppConfigTankList;

  if(!RedundantTankStructFiles)
  {
    TankStructFile2=(char *) &RedundantTankStructFiles;  
    /* OK, I know this looks really wierd, but all that is happening, is
       that TankStructFile2 is being set to a blank string.  I Pass a blank 
       string(char pointer to an int=0) to CreateFilePtr if we are not using 
       RedundantTankStructFiles.  DavidK
    */
  }

  /* Create the TANKFile ptr */
  pConfigTankList->pTSFile = CreateFilePtr(TankStructFile,TankStructFile2);

  /* Attach the array of TANKs */
  pConfigTankList->pFirstTS=pTankArray;

  /* Set the number of Tanks */
  pConfigTankList->NumOfTS = NumberOfTanks;

  /* Set the redundant file flags */
  pConfigTankList->redundantIndexFiles=RedundantIndexFiles;
  pConfigTankList->redundantTANKFiles=RedundantTankStructFiles;

  if(Debug > 2)
    fprintf(stderr,"Exiting %s\n",MyFuncName);
  
  return(0);
}


/*-----------------------------------------------------------------------*/
int OpenIndexFile(TANK * pTSPtr, int UseRedundantIndexFiles,
                  int CreateNew)
{
  /* 
     Purpose:
     Creates the index file names from the tank file name.  Creates the index
     RedundantFilePtr.  Opens and validates/Creates the index files.  The newly
     created/test index file(s) are attached to the TANK pTSPtr as the index
     file pointer.

     Return Values:
     0: Success.  A RFPtr for index files was created and attached to the 
     TANK as the TANK's ifp.
     -1: Failure.  An error occured while creating and attaching the RFPtr.

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  char Filename1[MAX_TANK_NAME + 6];
  char Filename2[MAX_TANK_NAME + 6];
  int TankNameLength;
  static char * MyFuncName = "OpenIndexFile()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if(UseRedundantIndexFiles)
  {
    /* Create two files tank-1.inx tank-2.inx */
    strcpy(Filename1,pTSPtr->tankName);
    strcpy(Filename2,pTSPtr->tankName);
    TankNameLength = (int)strlen(Filename1);
    strcpy(&(Filename1[TankNameLength]),"-1.inx");
    strcpy(&(Filename2[TankNameLength]),"-2.inx");
  }
  else
  {
    /* Creates one file tank.inx */
    strcpy(Filename1,pTSPtr->tankName);
    Filename2[0]=0;
    TankNameLength = (int)strlen(Filename1);
    strcpy(&(Filename1[TankNameLength]),"-1.inx");
  }
  if(CreateNew)
  {
    /* Create the file(s) */
    pTSPtr->ifp= 
      CreateRFile(Filename1,Filename2,
                  sizeof(DATA_CHUNK)*pTSPtr->indxMaxChnks);
  }
  else
  {
    /* Open/Test the file(s), and mark the most current one */
    pTSPtr->ifp= 
      TestRFile(Filename1,Filename2,
                sizeof(DATA_CHUNK)*pTSPtr->indxMaxChnks);
    if(pTSPtr->ifp == NULL)
    {
      /* Try to create indexes.  Don't give up hope, the sun will
         come out tommorow! 
        */
      pTSPtr->ifp=
        CreateRFile(Filename1,Filename2,
                    sizeof(DATA_CHUNK)*pTSPtr->indxMaxChnks);
    }
  }

  if(pTSPtr->ifp == NULL)
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(-1);
  }
  else
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(0);
  }
}

/*-----------------------------------------------------------------------*/
RedundantFilePtr CreateRFile(char *Filename1, char *Filename2, int size)
{
  /* 
     Purpose:
     Creates atleast 1 file of size "size", and creates an RFPtr to point
     to the newly created file(s).  2 files are created if Filename2
     is not blank.

     Return Values:
     Valid RFPtr: 
     Redundant File Pointer, and file(s) successfully created.
     NULL: 
     Failure.  Error occured while creating files and RFPointer.

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  FILE * fPtr;
  RedundantFilePtr rfPtr;
  static char * MyFuncName = "CreateRFile()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if((rfPtr = (RedundantFilePtr) malloc(sizeof(RedundantFile))) == NULL)
  {
    logit("t","malloc() failed in CreateRFile()\n");
    goto abortCreateRFile;
  }
  if((fPtr=fopen(Filename1,"wb+")) == NULL)
  {
    logit("t","fopen() failed in CreateRFile()\n");
    goto abortCreateRFile;
  }
  if(fseek(fPtr,size-1,SEEK_SET) != 0)
  {
    logit("t","fseek() failed in CreateRFile()\n");
    goto abortCreateRFile;
  }
  if(putc(0,fPtr) == EOF)
  {
    logit("t","putc() failed in CreateRFile()\n");
    goto abortCreateRFile;
  }
  if(fclose(fPtr) != 0)
  {
    logit("t","fclose() failed in CreateRFile()\n");
    goto abortCreateRFile;
  }
  if(Filename2[0] != 0)
  {
    if((fPtr=fopen(Filename2,"wb+")) == NULL)
    {
      logit("t","fopen() failed in CreateRFile() for file 2\n");
      goto abortCreateRFile;
    }
    if(fseek(fPtr,size-1,SEEK_SET) != 0)
    {
      logit("t","fseek() failed in CreateRFile() for file 2\n");
      goto abortCreateRFile;
    }
    if(putc(0,fPtr) == EOF)
    {
      logit("t","putc() failed in CreateRFile() for file 2\n");
      goto abortCreateRFile;
    }
    if(fclose(fPtr) != 0)
    {
      logit("t","fclose() failed in CreateRFile() for file 2\n");
      goto abortCreateRFile;
    }
    rfPtr->NumOfFiles=2;
  }
  else
  {
    rfPtr->NumOfFiles=1;
  }
  strcpy(rfPtr->Filename1,Filename1);
  strcpy(rfPtr->Filename2,Filename2);
  rfPtr->CurrentFile=1;

  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(rfPtr);

 abortCreateRFile:
  if(rfPtr)
  {
    free(rfPtr);
  }

  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(NULL);
}

/*-----------------------------------------------------------------------*/
RedundantFilePtr TestRFile(char *Filename1, char *Filename2, int size)
{
  /* 
     Purpose:
     Creates an RFPtr to point the files named Filename1 and Filename2.
     If the files already exist, they are checked for a valid timestamp.
     If only one file exists, and both file names are valid, then a second
     file will be created to complete the set.  If neither file is valid,
     TestRFile gives up and goes home.

     Return Values:
     Valid RFPtr: 
     Redundant File Pointer, and file(s) successfully tested/created.
     NULL: 
     Failure.  Error occured while testing/creating files and RFPointer.

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  RedundantFilePtr rfPtr;
  static char * MyFuncName = "TestRFile()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if((rfPtr = (RedundantFilePtr) malloc(sizeof(RedundantFile))) == NULL)
  {
    logit("t","malloc() failed in TestRFile()\n");
    return(NULL);
  }
  strcpy(rfPtr->Filename1,Filename1);
  if(Filename2[0] == 0)
  {
    rfPtr->NumOfFiles=1;
  }
  else
  {
    strcpy(rfPtr->Filename2,Filename2);
    rfPtr->NumOfFiles=2;
  }
  if(InitializeFiles(rfPtr) < 1)
  {
    /* InitializeFiles either returned 0, which means that there was no
       useful info in the rfPtr files, or -1, which means there was an
       unrecoverable error.  Neither one of those is good for us!
      */
    free(rfPtr);
    logit("t","InitializeFiles() failed in TestRFile()\n");
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(NULL);
  }
  /* else */
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(rfPtr);
}


/*-----------------------------------------------------------------------*/
int BuildLIndex(TANK * pTSPtr)
{
  /* 
     Purpose:
     Build the living index, used by the wave serving threads and index 
     manager, from the disk index and tank file.  To build the index, we
     first read the most recent index from disk (.inx).  Once we have 
     loaded the index from disk, we go to the insertion point in the
     tank file, and we travel backwards, till we get to the last piece
     of trace data that was recorded by the index.  That point is the 
     starting_point.  Then we process all of the data between the
     starting_point and the insertion point.  TaaDaa, we have an up
     to date living index.

     Return Values:
     1: If atleast 1 good file was found.
     0: If 0 good files were found, but all files now exist
     -1: If there was an unrecoverable error, such as not being able to create
     a file.

     Functions Called:
     (not analyzed)

     Additional Remarks:
     Assumptions:
     We have a valid TankFile, we have a valid, put possibly outdated
     index file.

     What we do:
     We should first check to see if the Tank file is brand
     spanking new, if it is, then there is not much work to be done.  If
     not, we need to go to the trouble of seeking the insertion point, and
     working backwards in the tank until we find data that the index has 
     recorded.  We must then proceed to stride through the tank file, 
     processing each trace sample from the index-point to the current
     insertion point.  

     This is a BIG function!!!!!!
  */

  struct IndexInfo iiBuffer;
  int StartingPoint,CurrentPosition,NotDone,retVal;
  double IndexEnd, TanktEnd;
  TRACE2_HEADER * pCurrentRecord=0;
  int IndexesAreBad,i,NumOfIndexFiles;
  static char * MyFuncName = "InitializeFiles()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if((pTSPtr->chunkIndex = (DATA_CHUNK *)
      malloc(pTSPtr->indxMaxChnks * sizeof(DATA_CHUNK) + sizeof(time_t)))
     == NULL)
  {
    logit("t","malloc() failed while trying to allocate space for living index in BuildLIndex()\n");
    goto abortBuildLIndex;
  }
  if(pTSPtr->firstWrite)
  {
    /* Brand spanking new tank, build a blank index and go home. */
    if (Debug > 1)
        logit("et","BuildIndex(): First write for tank: %s,%s,%s,%s\n",
                pTSPtr->sta,pTSPtr->chan,pTSPtr->net,pTSPtr->loc);

    pTSPtr->indxStart=0;
    pTSPtr->indxFinish=0;
    pTSPtr->inPtOffset=0;
    pTSPtr->chunkIndex[pTSPtr->indxStart].tStart=0;
    pTSPtr->chunkIndex[pTSPtr->indxStart].tEnd=0;
  }
  else
  {

    if((NumOfIndexFiles=GetNumOfFiles(pTSPtr->ifp)) < 0)
    {
      logit("t","wave_serverV:  corrupt redundant file pointer in BuildLIndex() for Tank %s,%s,%s,%s\n",
            pTSPtr->sta,pTSPtr->chan,pTSPtr->net,pTSPtr->loc);
    }

    IndexesAreBad=1;
    for(i=0; (i < NumOfIndexFiles) && IndexesAreBad; i++)
    {
      if((pCurrentRecord=(TRACE2_HEADER *)malloc(pTSPtr->recSize)) == NULL)
      {
        logit("t","malloc() failed while trying to allocate pCurrentRecord in BuildLIndex()\n");
        goto abortBuildLIndex;
      }

      /*  Not a new tank, we've got some work to do to update the index. */
      if(ReadRFile(pTSPtr->ifp,0,sizeof(time_t)+sizeof(int),(char *)&iiBuffer) != 1)
      {
        logit("t","Timestamp discrepency in %s during %s.  File is invalid.\n",
              GetRedundantFileName(pTSPtr->ifp),"Wave_Server:BuildLIndex()");
        InvalidateRFile(pTSPtr->ifp);
        free(pCurrentRecord);
        continue;
      }

      /* There should be some logic here that checks iiBuffer.NumOfChunks
         against pTSPtr->indxMaxChnks, to prevent an overwrite if the
         index size on disk is too large. DavidK 7/7/98
      */
      if(iiBuffer.NumOfChunks > (unsigned int)pTSPtr->indxMaxChnks)
      {
        /* For some reason we have more index entries than
           we can handle.  Take only the number we can handle,
           and then ignore the rest.  DavidK 7/7/98
        */
        retVal=ReadRFile(pTSPtr->ifp,sizeof(time_t)+sizeof(int),
                         pTSPtr->indxMaxChnks* sizeof(DATA_CHUNK),
                         (char*)pTSPtr->chunkIndex);
        retVal=ReadRFile(pTSPtr->ifp,sizeof(time_t)+sizeof(int)+
                         iiBuffer.NumOfChunks* sizeof(DATA_CHUNK),
                         sizeof(time_t),
                         (char*)(&pTSPtr->chunkIndex[pTSPtr->indxMaxChnks]));
        logit("et","Wave_serverV:  Too many indexes in %s during %s."
              "Ignoring last %d indexes from index file.  Will be"
              " rebuilt later in update from Tankfile.\n",
              GetRedundantFileName(pTSPtr->ifp),
              "Wave_ServerV:BuildLIndex()",
              iiBuffer.NumOfChunks-pTSPtr->indxMaxChnks);

        /* change iiBuffer.NumOfChunks to reflect the actual number of index
           entries we read in and used.  */
        iiBuffer.NumOfChunks=pTSPtr->indxMaxChnks;
      }
      else
      {
        retVal=ReadRFile(pTSPtr->ifp,sizeof(time_t)+sizeof(int),
                         iiBuffer.NumOfChunks* sizeof(DATA_CHUNK) + sizeof(time_t),
                         (char*)pTSPtr->chunkIndex);
      }
      if(retVal != 1)
      {
        logit("et","Timestamp discrepancy in %s during %s.  File is invalid.\n",
              GetRedundantFileName(pTSPtr->ifp),"Wave_ServerV:BuildLIndex()");
        InvalidateRFile(pTSPtr->ifp);
        free(pCurrentRecord);
        continue;
      }

      if(iiBuffer.TimeStamp != 
         *((time_t *) (pTSPtr->chunkIndex + iiBuffer.NumOfChunks)))
      {
        logit("et","Timestamp discrepancy in %s during %s.  File is invalid.\n",
              GetRedundantFileName(pTSPtr->ifp),"Wave_Server:BuildLIndex()");
        InvalidateRFile(pTSPtr->ifp);
        free(pCurrentRecord);
      }
      else
      {
        IndexesAreBad=0;
      }
    }  /* for NumOfFiles */

    /* Set the index related variables relative to what we just read in.
       The problem here is that when we write the index that was save to dis
       we start at 0.  I guess we could start at indxStart, but that would
       be more complicated, so reset indxStart and indxFinish for the Tank. DK
    */
    if(IndexesAreBad || iiBuffer.NumOfChunks == 0)
    {
      /* Special case, no index entries on disk */
      if(IndexesAreBad)
      {
        logit("t","Rebuilding entire index from tank file %s, because disk indexes were corrupt.\n",pTSPtr->tankName);
      }
      else
      {
        logit("t","Rebuilding entire index from tank file %s, because disk indexes were blank.\n",pTSPtr->tankName);
      }

      pTSPtr->indxStart=0;
      pTSPtr->indxFinish=0;
      pTSPtr->chunkIndex[0].tEnd=0.0;
    }
    else
    {
      pTSPtr->indxStart=0;
      pTSPtr->indxFinish = 0 + iiBuffer.NumOfChunks - 1;
    }

    /* Now we have the latest disk index, we just need to update it.*/

    /* Find out the latest trace that the index knows about. */
    IndexEnd=pTSPtr->chunkIndex[pTSPtr->indxFinish].tEnd;

    /* Obtain the latest known entry in the tank */
    CurrentPosition = pTSPtr->inPtOffset - pTSPtr->recSize;
    if(CurrentPosition < 0)
    {
      CurrentPosition = pTSPtr->nRec * pTSPtr->recSize - pTSPtr->recSize;
    }

    if(fseek(pTSPtr->tfp,CurrentPosition,SEEK_SET))
    {
      logit("t","fseek() failed while seeking CurrentPosition in %s in %s\n"
            ,pTSPtr->tankName,MyFuncName);
      goto abortBuildLIndex;
    }

    if((pCurrentRecord=(TRACE2_HEADER *)malloc(pTSPtr->recSize)) == NULL)
    {
      logit("t","malloc() failed while allocating pCurrentRecord in %s\n",
            MyFuncName);
      goto abortBuildLIndex;
    }

    if(fread(pCurrentRecord, pTSPtr->recSize, 1, pTSPtr->tfp) != 1)
    {
      if(pTSPtr->firstWrite)  /* This shouldn't happen, but just in case */
      {
        pCurrentRecord->endtime=0.0;  /* Indicate a blank tank */
      }
      else
      {
        logit("t","wave_server: fread() failed in BuildLIndex() for Tank file %s\n",
              pTSPtr->tankName);
        goto abortBuildLIndex;
      }
    }
    TanktEnd=pCurrentRecord->endtime;

    if(!IndexEnd)
    {
      /* The Index is blank, we must completely rebuild it.*/
      if(pTSPtr->firstPass)
      {
        StartingPoint=0;
      }
      else
      {
        /* Cleanup, we are giving up the record at offset, to
           facilitate an easier recovery.  Hope no one needs it */
        StartingPoint=pTSPtr->inPtOffset+pTSPtr->recSize;
      }
      pTSPtr->firstWrite=1;
    }
    else
    {
      /* Find the starting point.  Search backwards from the last
         record written to the tank file. */

      NotDone = 1;
      for(CurrentPosition = pTSPtr->inPtOffset - pTSPtr->recSize;
          NotDone;
          CurrentPosition-=pTSPtr->recSize)
      {
        if(CurrentPosition < 0)
        {
          CurrentPosition = pTSPtr->nRec * pTSPtr->recSize - pTSPtr->recSize;
        }
        if(fseek(pTSPtr->tfp,CurrentPosition,SEEK_SET))
        {
          logit("t","wave_server: fseek() failed in BuildLIndex() for Tank file %s while searching for starting point.\n",
                pTSPtr->tankName);
          goto abortBuildLIndex;
        }
        if(fread(pCurrentRecord, pTSPtr->recSize, 1, pTSPtr->tfp) != 1)
        {
          logit("t","wave_server: fread() failed in BuildLIndex() for Tank file %s, while searching for starting point.\n",
                pTSPtr->tankName);
          goto abortBuildLIndex;
        }
        if ( ( NotDone=
               CheckForValidAndGreaterThanIndexEnd((char*)pCurrentRecord,IndexEnd,
                                                   TanktEnd,CurrentPosition,pTSPtr->inPtOffset) ) < 0 )
        {
          logit("", "\tBad record in tank %s\n", pTSPtr->tankName );
          NotDone = 0;
        }
        if( !NotDone) /*Done*/
        {
          StartingPoint=CurrentPosition + pTSPtr->recSize;
        }
      }  /* for CurrentPosition, moving backwards in tank file */
    }    /* else !IndexEnd */
       
    if(StartingPoint >= pTSPtr->nRec * pTSPtr->recSize)
    {
      StartingPoint=0;
    }
      
    /* Now restore index */
    for(CurrentPosition=StartingPoint; 
        CurrentPosition!=pTSPtr->inPtOffset;
        CurrentPosition+=pTSPtr->recSize)
    {
      if(CurrentPosition >= pTSPtr->nRec * pTSPtr->recSize)
      {
        CurrentPosition=0;
      }
      if(fseek(pTSPtr->tfp,CurrentPosition,SEEK_SET) != 0)
      {
        logit("t","wave_server: fseek() failed in BuildLIndex() for Tank file %s, while updatin index.\n",
              pTSPtr->tankName);
        goto abortBuildLIndex;
      }
      if(fread(pCurrentRecord, pTSPtr->recSize, 1, pTSPtr->tfp) != 1)
      {
        logit("t","wave_server: fread() failed in BuildLIndex() for Tank file %s, while updatin index.\n",
              pTSPtr->tankName);
        goto abortBuildLIndex;
      }

      if(pCurrentRecord->starttime < pCurrentRecord->endtime &&
         pCurrentRecord->starttime > 0 &&
         pCurrentRecord->endtime <= TanktEnd)
      {
        if(UpdateIndex(pTSPtr,pCurrentRecord,CurrentPosition, FALSE) == -1)
        {
          logit("t","wave_server: UpdateIndex() failed in BuildLIndex() for Tank file %s.\n",
                pTSPtr->tankName);
          goto abortBuildLIndex;
        }
      }
      else
      {
        logit("t","Bad record found in %s \n\tat %d: start: %lf end: %lf tank end %lf\n",
              pTSPtr->tankName, CurrentPosition, 
              pCurrentRecord->starttime, pCurrentRecord->endtime, TanktEnd);
      }
    }
    free(pCurrentRecord);
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(0);

 abortBuildLIndex:
  if(pTSPtr->chunkIndex)
  {
    free(pTSPtr->chunkIndex);
  }
  if(pCurrentRecord)
  {
    free(pCurrentRecord);
  }
  if(Debug > 2)
    logit("t","Exiting %s\n",MyFuncName);
  
  return(-1);
}
        

/*-----------------------------------------------------------------------*/
int CheckForValidAndGreaterThanIndexEnd(char * pCurrentRecord,
                                        double IndexEnd, double LastEnd,
                                        int CurrentPosition, int TankOffset)
{
  /* 
     Purpose:
     Determine whether a trace record is valid, and is later than the last
     record recorded in the index(time represented by IndexEnd).

     Return Values:
     1: Record is valid and greater(time wise) than IndexEnd.
     0: Record is (time wise) less than or equal to IndexEnd.
     -1: Bad record found

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  TRACE2_HEADER * pCurrentHeader=(TRACE2_HEADER *)pCurrentRecord;
  static char * MyFuncName = "CheckForValidAndGreaterThanIndexEnd()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  

  if(pCurrentHeader->endtime > LastEnd 
     || pCurrentHeader->starttime > LastEnd
     || pCurrentHeader->endtime == 0.0
     || pCurrentHeader->starttime == 0.0
     || pCurrentHeader->starttime > pCurrentHeader->endtime
     )
  {
    logit("t","Bad record: starttime: %f,endtime: %f,index endtime:%f, last tank record endtime %f\n",
          pCurrentHeader->starttime,pCurrentHeader->endtime,IndexEnd,LastEnd);
    logit("t","Bad rec (cont.) CurrentPos: %u, TankOffset %u\n",
          CurrentPosition,TankOffset);

    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    /* Bad record, logit, and return error */
    return(-1);
  }
  if(pCurrentHeader->endtime <= IndexEnd)
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    /* return false */
    return(0);
  }
  else
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    /* return true */
    return(1);
  }
}

/*-----------------------------------------------------------------------*/
int GetNumOfFiles(RedundantFilePtr rfPtr)
{
  /* 
     Purpose:
     Indicate the number of files being used in/by a redundant file 
     pointer(RFPtr)

     Return Values:
     >0: The number of files being used by a pointer
     -1:  An error occured while trying to determine the number of files.

     Functions Called:
     (not analyzed)

     Additional Remarks:
    */

  static char * MyFuncName = "GetNumOfFiles()";

  if(Debug > 2)
    logit("t","Entering %s\n",MyFuncName);
  
  if(rfPtr->NumOfFiles > 0 && rfPtr->NumOfFiles <= MAX_FILES_IN_REDUNDANT_SET)
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(rfPtr->NumOfFiles);
  }
  else
  {
    if(Debug > 2)
      logit("t","Exiting %s\n",MyFuncName);
    
    return(-1);
  }
}


int index_util_c_init()
{
  if (Debug > 1)
      logit("t","index_util.c:Version 0978978930\n");
  return(0);
}
/* <End of index_util.c> */

