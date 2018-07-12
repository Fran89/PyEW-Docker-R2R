/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2001/07/20 16:33:41  davidk
 *     changed fprintf's to logit's
 *     reformatted some whitespace
 *     Added code to insure that the list-file parser
 *      doesn't overflow the file array.
 *     Added code close the listfile.
 *
 *     Revision 1.1  2001/07/10 23:14:21  davidk
 *     Initial revision
 *
 ************************************************************/



/*************************************************/
/*************************************************/
/*               #INCLUDES                       */
/*************************************************/
/*************************************************/
#include "parse_api_doc.h"



/********************************************************************/
/*********************************************************************
  GetNextLine
    fills pBuffer with the next line from fptr.  It null terminates
    the line in place of a newline character. (like gets()).

    Return codes:
      RETURN_SUCCESS  Success
      RETURN_ERROR   Error in fgets
      RETURN_WARNING Warning: line too long for buffer, no newline char found
      RETURN_GETNEXTLINE_EOF  End Of File
*********************************************************************/
int GetNextLine(char * pBuffer, int BufferSize, FILE * fptr)
{
  int RetCode;
  int i;

  errno=0;
  RetCode=(int)fgets(pBuffer, BufferSize,fptr);
  linectr++;
  if(!RetCode)
  {
    if(feof(fptr))
    {
      logit("e","Reached end of file.  %d lines processed.\n",linectr);
      return(RETURN_GETNEXTLINE_EOF);
    }
    else
    {
      logit("e","Error(%d) reading file near line %d.\n",
              errno,linectr);
      return(RETURN_ERROR);
    }
  }
  else
  {
    if(pBuffer[strlen(pBuffer)-1] != '\n')
    {
      logit("e","Warning:  Line too long, or parsing error at line %d\n",
              linectr);
      logit("e","Buffer retrieved: %s\n\n",pBuffer);
      return(RETURN_WARNING);
    }
    else
    {
      pBuffer[strlen(pBuffer)-1]=0x00;  /* null terminate without the 
                                           newline like gets would */
    }
    /* right trim the string for SPACES, TABS, and ??NULLS?? */
    for(i = strlen(pBuffer) -1; i >= 0; i--)
    {
      if(pBuffer[i] == ' ' || pBuffer[i] == '\t' || pBuffer[i] == 0x00)
        pBuffer[i] = 0x00;
      else
        break;
    }
  }
/*  logit("e","Debug: At line (%5d) in GetNextLine()\n", linectr); */

  return(RETURN_SUCCESS);
}  /* End GetNextLine */



/********************************************************************/
/********************************************************************/
int ReadCommentsFile(FILE * fpIn, CommentStruct ** pCommentsArray, 
                     int * piNumComments, int * piNumAllocComments)
{
  return(ReadSourceFile(fpIn, pCommentsArray, piNumComments, piNumAllocComments,
                        "COMMENTS_INPUT_FILE"));
}

/********************************************************************/
/********************************************************************/
int GetNextCommentFromSourceFile(FILE * fpIn, CommentStruct * pTemp_Comment)
{
  static int iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
  int done = FALSE;
  int RetCode;

  /* Comment structure variables */
  DefineConstantComment dccComment;
  TypedefComment        tdComment;
  FunctionComment       fcComment;
  LibraryComment        lcComment;

  /* Pointer for the current token retrieved from strtok() */
  char * pToken;

  /* Buffer to hold the current line */
  char plb[MAX_COMMENT_LEN];  /* line buffer */

  /* make sure plb is NULL terminated */
  plb[sizeof(plb) - 1] = 0x00;

  if(iStatus == COMMENT_PARSE__NOT_IN_COMMENT)
  {
    /* retrieve the next line from the file! */
    RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
  }

  while(!done && RetCode != RETURN_ERROR && RetCode != RETURN_GETNEXTLINE_EOF)
  {
    switch(iStatus)
    {
    case COMMENT_PARSE__NOT_IN_COMMENT:
      {

        if(!strncmp(plb,COMMENT_STRING_LINE1, sizeof(COMMENT_STRING_LINE1) - 1))
          iStatus=1;

        break;
      }
    case COMMENT_PARSE__AFTER_FIRST_LINE:
      {
        if(strncmp(plb,COMMENT_STRING_LINE2, sizeof(COMMENT_STRING_LINE2) - 1))
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        else
          iStatus = COMMENT_PARSE__IN_UNKNOWN_COMMENT;
        break;
      }
    case COMMENT_PARSE__IN_UNKNOWN_COMMENT:
      {
        if(!strcmp(plb,COMMENT_STRING_END))
        {
          /* No data was recorded, so just reset the state */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
          break;
        }
        /* Now we are trying to figure 
           out what type of comment it is.
           Search for "TYPE "
        *******************************/
        pToken=strtok(plb,PARSE_DELIMITERS);
        if(strcmp(pToken,"TYPE"))
        {
          /* no match, break */
          break;
        }
        else
        {
          /* we've got a match, so check the next token */
          pToken=strtok(NULL,PARSE_DELIMITERS);

          if(!strcmp(pToken,"DEFINE"))
          {
            /* This is a #define comment */
            iStatus = COMMENT_PARSE__IN_DEFINE_COMMENT;
            break;
          }
          else if(!strcmp(pToken,"TYPEDEF"))
          {
            /* This is a typedef comment */
            iStatus = COMMENT_PARSE__IN_TYPEDEF_COMMENT;
            break;
          }
          else if(!strcmp(pToken,"FUNCTION_PROTOTYPE"))
          {
            /* This is a function prototype comment */
            iStatus = COMMENT_PARSE__IN_FUNCTION_COMMENT;
            break;
          }
          else if(!strcmp(pToken,"LIBRARY"))
          {
            /* This is a function prototype comment */
            iStatus = COMMENT_PARSE__IN_LIBRARY_COMMENT;
            break;
          }
          else
          {
            logit("e","ERROR:  Unexpected comment type (%s) encountered!\n",
                    pToken);
            break;
          }
        }
      }
    case COMMENT_PARSE__IN_DEFINE_COMMENT:  /* define comment */
      {
        /* initialize the library struct */
        memset(&(pTemp_Comment->Library), 0, sizeof(LibraryInfoStruct));

        RetCode = ParseDefineComment(&dccComment, &(pTemp_Comment->Library));
        if(RetCode == RETURN_SUCCESS)
        {
          /* copy the TypedefComment from temp to permanent storage */
          memcpy(pTemp_Comment->pComment, &dccComment, sizeof(DefineConstantComment));
          
          /* indicate to the caller that the comment is of type constant */
          pTemp_Comment->iCommentType = COMMENT_TYPE_CONSTANT;

          /* indicate that we got something */
          done = TRUE;

          /* do not reset status to "not in a comment" */
        }
        else if(RetCode == RETURN_WARNING)
        {
          /*no longer in function comment block */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else /* RetCode == RETURN_ERROR */
        {
          /* logit, error in parse comment at line ???? */
          logit("e", "%s:  Error parsing constant comment.\n",
                  "GetNextCommentFromSourceFile()");
          done = TRUE;
        }
        break;
      }
      
    case COMMENT_PARSE__IN_TYPEDEF_COMMENT:  /* typedef comment */
      {
        /* initialize the library struct */
        memset(&(pTemp_Comment->Library), 0, sizeof(LibraryInfoStruct));

        RetCode = ParseTypedefComment(&tdComment, &(pTemp_Comment->Library));
        if(RetCode == RETURN_SUCCESS)
        {
          /* copy the TypedefComment from temp to permanent storage */
          memcpy(pTemp_Comment->pComment, &tdComment, sizeof(TypedefComment));
          
          /* indicate to the caller that the comment is of type typedef */
          pTemp_Comment->iCommentType = COMMENT_TYPE_TYPEDEF;

          /* indicate that we got something */
          done = TRUE;

          /* reset status to "not in a comment" */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else if(RetCode == RETURN_WARNING)
        {
          /*no longer in function comment block */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else /* RetCode == RETURN_ERROR */
        {
          /* logit, error in parse comment at line ???? */
          logit("e", "%s:  Error parsing typedef comment.\n",
                  "GetNextCommentFromSourceFile()");
          done = TRUE;
        }
        break;
      }

    case COMMENT_PARSE__IN_FUNCTION_COMMENT:
      {
        /* initialize the library struct */
        memset(&(pTemp_Comment->Library), 0, sizeof(LibraryInfoStruct));

        RetCode = ParseFunctionComment(&fcComment, &(pTemp_Comment->Library));
        if(RetCode == RETURN_SUCCESS)
        {
          /*logit("e", "DEBUG: Successfully parsed function comment!\n"); */
          /* copy the FunctionComment from temp to permanent storage */
          memcpy(pTemp_Comment->pComment, &fcComment, sizeof(FunctionComment));
          /*logit("e", "DEBUG: Copied function comment to permanent storage!\n"); */
          
          /* indicate to the caller that the comment is of type function */
          pTemp_Comment->iCommentType = COMMENT_TYPE_FUNCTION;

          /* indicate that we got something */
          done = TRUE;

          /* reset status to "not in a comment" */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else if(RetCode == RETURN_WARNING)
        {
          /*no longer in function comment block */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else /* RetCode == RETURN_ERROR */
        {
          /* logit, error in parse comment at line ???? */
          logit("e", "%s:  Error parsing function comment.\n",
                  "GetNextCommentFromSourceFile()");
          done = TRUE;
        }
        break;
      }

    case COMMENT_PARSE__IN_LIBRARY_COMMENT:
      {
        /* initialize the library struct */
        memset(&(pTemp_Comment->Library), 0, sizeof(LibraryInfoStruct));

        RetCode = ParseLibraryComment(&lcComment, &(pTemp_Comment->Library));
        if(RetCode == RETURN_SUCCESS)
        {
          /* copy the FunctionComment from temp to permanent storage */
          memcpy(pTemp_Comment->pComment, &lcComment, sizeof(LibraryComment));
          
          /* indicate to the caller that the comment is of type function */
          pTemp_Comment->iCommentType = COMMENT_TYPE_LIBRARY;

          /* indicate that we got something */
          done = TRUE;

          /* reset status to "not in a comment" */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else if(RetCode == RETURN_WARNING)
        {
          /*no longer in function comment block */
          iStatus = COMMENT_PARSE__NOT_IN_COMMENT;
        }
        else /* RetCode == RETURN_ERROR */
        {
          /* logit, error in parse comment at line ???? */
          logit("e", "%s:  Error parsing library comment.\n",
                  "GetNextCommentFromSourceFile()");
          done = TRUE;
        }
        break;
      }
      
    default: break;
    }  /* end switch(iStatus) */

    if((iStatus == COMMENT_PARSE__NOT_IN_COMMENT    ||
        iStatus == COMMENT_PARSE__AFTER_FIRST_LINE  ||
        iStatus == COMMENT_PARSE__IN_UNKNOWN_COMMENT   )
       && !done
      )
    {
      /* retrieve the next line from the file! */
      RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
    }
  } /* end while */
  return(RetCode);
}


/********************************************************************/
/********************************************************************/
int GetNextCommentFromCommentFile(FILE * fpIn, CommentStruct * pTemp_Comment)
{
  return(-1);
}


/*************************************************************************
*************************************************************************/
int ReplaceBuffer(void ** ppBuffer, int OldBufferSize, int NewBufferSize, 
                  int bCopyBuffer)
{
  /*******************************************************
  ReplaceBuffer() creates a new buffer in place of an old
  one.  ReplaceBuffer() frees the buffer pointed to by 
  (*ppBuffer), creates a new buffer of size NewBufferSize
  bytes, and sets (*ppBuffer) to point at the new buffer.
  It's like a realloc, but the buffer pointer changes.
  *******************************************************/

  void * pBufferOld, *pBufferNew;
  
  /* check input params, we require that the old buffer be valid */
  if(!ppBuffer || !(*ppBuffer))
  {
    logit("e","ReplaceBuffer(): ERROR NULL inputs!\n");
    return(RETURN_ERROR);
  }
  
  pBufferOld = *ppBuffer;
  
  /* allocate the new buffer */
  pBufferNew = malloc(NewBufferSize);
  if(!pBufferNew)
  {
    logit("e","ReplaceBuffer(): malloc failed for %d bytes.\n", NewBufferSize);
    return(RETURN_ERROR);
  }
  /* else */
  
  /* Copy the contents of the old buffer to the new buffer if desired */
  if(bCopyBuffer)
  {
    memcpy(pBufferNew,pBufferOld,OldBufferSize);
  }
  
  /* free the old buffer */
  free(pBufferOld);
  
  /* point the caller's pointer at the new buffer */
  *ppBuffer = pBufferNew;
  
  return(RETURN_SUCCESS);
}  /* end ReplaceBuffer() */



/********************************************************************/
/********************************************************************/
int strcmp1(const char *string1, const char *string2)
{
  int rc;

  rc = strcmp(string1, string2);
  if(rc < 0)
    return(-1);
  else if(rc > 0)
    return(1);
  else
    return(0);
}  /* end strcmp1() */

/********************************************************************/
/********************************************************************/
int CompareComments(const void* p1, const void* p2)
{

  const CommentStruct * pComment1 = p1;
  const CommentStruct * pComment2 = p2;

  int rc;

  /* first check Library names */
  if(rc = strcmp1(pComment1->Library.szLibraryName,pComment2->Library.szLibraryName))
    return(rc * COMPARE_COMMENTS_DIFFERENT_LIBRARY);

  /* next check SubLibrary names */
  else if(rc = strcmp1(pComment1->Library.szSubLibraryName, pComment2->Library.szSubLibraryName))
    return(rc * COMPARE_COMMENTS_DIFFERENT_SUBLIBRARY);

  /* next check the language */
  else if(rc = strcmp1(pComment1->Library.szLanguage,pComment2->Library.szLanguage))
    return(rc * COMPARE_COMMENTS_DIFFERENT_LANGUAGE);

  /* next check CommentType */
  else if(pComment1->iCommentType < pComment2->iCommentType)
    return(-1 * COMPARE_COMMENTS_DIFFERENT_TYPE);
  else if (pComment1->iCommentType > pComment2->iCommentType)
    return(1 * COMPARE_COMMENTS_DIFFERENT_TYPE);

  /* now we know we have two of the same type comments from the 
     same sub-library/language */

  /* next check CommentName */

  /* we have to split up here based on comment type */
  switch(pComment1->iCommentType)
  {
  case COMMENT_TYPE_UNDEFINED:
    {
      /* uh this is bad, issue a warning and return 0 */
      logit("e", "CompareComments():  WARNING: comments have "
                      "a comment type of UNDEFINED.   BUG!!!\n");
      return(0);
    }
  case COMMENT_TYPE_CONSTANT:
    {
      rc = strcmp1(((DefineConstantComment *)(pComment1->pComment))->szConstantGroup,
                    ((DefineConstantComment *)(pComment2->pComment))->szConstantGroup
                 );
      if(rc)
      {
        return(rc * COMPARE_COMMENTS_DIFFERENT_COMMENT_GROUP);
      }
      else
      {
        rc = strcmp1(((DefineConstantComment *)(pComment1->pComment))->szConstantName,
                    ((DefineConstantComment *)(pComment2->pComment))->szConstantName
                   );
      }
      break;
    }
  case COMMENT_TYPE_TYPEDEF:
    {
      rc = strcmp1(((TypedefComment *)(pComment1->pComment))->szTypedefName,
                    ((TypedefComment *)(pComment2->pComment))->szTypedefName
                 );
      break;
    }
  case COMMENT_TYPE_FUNCTION:
    {
      rc = strcmp1(((FunctionComment *)(pComment1->pComment))->szFunctionName,
                    ((FunctionComment *)(pComment2->pComment))->szFunctionName
                 );
      break;
    }
  case COMMENT_TYPE_LIBRARY:
    {
      /* we have already been through the library names up above */
      return(0);
    }
  default:
    /* uh this is not good, we have a bad CommentType */
    logit("e", "CompareComments():  WARNING: comments have "
                    "an invalid comment type(%d).   BUG!!!\n",
            pComment1->iCommentType);
    return(0);
  }  /* end switch */

  return(rc * COMPARE_COMMENTS_DIFFERENT_COMMENT_NAME);
}

/********************************************************************/
/********************************************************************/
int ReplaceComment(CommentStruct * pDest, CommentStruct * pSource)
{
  int    iCommentSize;
  void * pTemp;

  /* save the pointer to the actual comment */
  pTemp = pDest->pComment;

  /* get the size of the actual comment */
  iCommentSize = GetCommentSize(pDest->iCommentType);
  /* check for error */
  if(iCommentSize <= 0)
  {
    return(RETURN_ERROR);
  }

  /* copy the new CommentStruct over the top of the old one */
  memcpy(pDest, pSource, sizeof(CommentStruct));

  /* reset the pointer to the actual comment */
  pDest->pComment = pTemp;

  /* copy the contents of the new actual comment over the contents 
     of the old actual comment */
  memcpy(pDest->pComment, pSource->pComment, iCommentSize);

  /* success */
  return(RETURN_SUCCESS);
}

int GetCommentSize(int iCommentType)
{
  int iSize;

  switch(iCommentType)
  {
  case COMMENT_TYPE_CONSTANT:
    iSize = sizeof(DefineConstantComment);
    break;
  case COMMENT_TYPE_TYPEDEF:
    iSize = sizeof(TypedefComment);
    break;
  case COMMENT_TYPE_FUNCTION:
    iSize = sizeof(FunctionComment);
    break;
  case COMMENT_TYPE_LIBRARY:
    iSize = sizeof(LibraryComment);
    break;
  default:
    return(RETURN_ERROR);
    break;
  }

  return(iSize);

}  /* end GetCommentSize() */

int AllocateSpaceForAndCopyComment(CommentStruct * pCurrent, void ** ppComment)
{
  int iSize;
  void * pComment;

  iSize = GetCommentSize(pCurrent->iCommentType);


  pComment = malloc(iSize);

  if(pComment)
  {
    memcpy(pComment, pCurrent->pComment, iSize);
    *ppComment = pComment;
    return(RETURN_SUCCESS);
  }
  else
  {
    return(RETURN_ERROR);
  }

}  /* end AllocateSpaceForAndCopyComment() */


/********************************************************************/
/********************************************************************/
int AddCommentToList(CommentStruct ** pCommentsArray, CommentStruct * pComment,
                     int * piNumComments, int * piNumAllocComments)
{
  int i;
  int RetCode;

  int iNumComments =      *piNumComments;
  int iNumAllocComments = *piNumAllocComments;
  CommentStruct * pCurrent;


  for(i=0; i < iNumComments; i++)
  {
    pCurrent = &((*pCommentsArray)[i]);
    if(!CompareComments(pCurrent,pComment))
    {
      /* we have a match, do an update */
      RetCode = ReplaceComment(pCurrent, pComment);
      return(RetCode);
    }
  }  /* end for i in iNumComments */

  /* we didn't find a matching comment, so append */
  if(iNumComments >= iNumAllocComments)
  {
    logit("e", "DEBUG: AddCommentToList():allocating 50 more comments\n");
    /* increment iNumAllocComments by 50 */
    *piNumAllocComments += 50;

    RetCode = ReplaceBuffer((void **)pCommentsArray, 
                            iNumAllocComments * sizeof(CommentStruct), 
                            *piNumAllocComments * sizeof(CommentStruct),
                            TRUE);
    if(RetCode != RETURN_SUCCESS)
      return(RetCode);
  }

  /* append the comment */
  pCurrent = &((*pCommentsArray)[iNumComments]);
  memcpy(pCurrent, pComment, sizeof(CommentStruct));

  RetCode = AllocateSpaceForAndCopyComment(pCurrent, &(pCurrent->pComment));
  if(RetCode != RETURN_SUCCESS)
    return(RETURN_ERROR);


  iNumComments++;
  
  /* copy iNumComments back to caller buffer */
  *piNumComments = iNumComments;

  /* we are done */
  return(RETURN_SUCCESS);
}



/* return codes 
    RETURN_SUCCESS   success, data found and retrieved
    RETURN_WARNING   no error, but no data found
    RETURN_ERROR     error, no data found
********************************************************/
/********************************************************************/
/********************************************************************/
int ParseDefineComment(DefineConstantComment * pdccComment, 
                       LibraryInfoStruct * pLibrary)
{

  /* Buffer to hold the current line */
  char plb[MAX_COMMENT_LEN];  /* line buffer */

  int done = 0;
  size_t strlen_plb = 0;
  size_t strlen_szCD = 0;
  int RetCode;
  int OUT_RetCode = 0;
  char * pToken;
  static char szConstantGroup[255]="";
  static LibraryInfoStruct lisLibrary;

  char * Temp_szDescription;
  int    Temp_iDescriptionSize;
  char * Temp_plb;


  /* bSuccess indicates whether we successfully obtained a clean
     comment.  If it is not true, then there is a cleanup routine
     that will push us through any remaining garbage until 
     we hit a blank line. 
  ***************************************************************/
  int bSuccess = TRUE;


  /* initialize the library info struct */
  if(szConstantGroup[0] == 0x00)
  {
    /* we are at the top of a new group of constants */
    memset(&lisLibrary, 0, sizeof(lisLibrary));
  }

  /* Get next line */
  RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
  if(RetCode == RETURN_ERROR)
  {
    return(RetCode);
  }

  /* we are done when we either:
     1) Run out of lines in the file
     2) Hit an end comment line
     3) Gather a complete DEFINE comment
  ***************************************/
  while(!done)
  {
    /* reinitialize RetCode */
    RetCode = RETURN_SUCCESS;

    /* check for blank line */
    if(!strlen(plb)) /* blank line */
    {
      goto GetAnotherLine;
    }

    /* check for End Comment line that ends a comment section */
    else if(!strcmp(plb,COMMENT_STRING_END))
    {
      /* we reached the end of this comment */  
      szConstantGroup[0] = 0x00;
      OUT_RetCode        = RETURN_WARNING;
      done               = TRUE;
      bSuccess           = TRUE;
    }
    else
    {
      /* grab the strlen, so that we can still decipher contents after
         strtok destroys the string */
      strlen_plb = strlen(plb);

      /* grab the first token off this line */
      pToken=strtok(plb,PARSE_DELIMITERS);

      /* Check for "CONSTANT_GROUP" which names a group of constants */
      if(!strcmp(pToken,"CONSTANT_GROUP"))
      {
        /* get the CONSTANT_GROUP name */
        if(strlen_plb > strlen(pToken) + 1)
        {
          /* OK, there is enough string left after the 
             token, that we can grab something */
          strncpy(szConstantGroup, (char *)pToken+strlen(pToken) + 1, 
                  sizeof(szConstantGroup));
          szConstantGroup[sizeof(szConstantGroup) - 1] = 0x00;
        }
        else  /* line was blank after "CONSTANT_GROUP" token */
        {
          szConstantGroup[0] = 0x00;
        }
        /* the comment was clean */
        bSuccess = TRUE;
        goto GetAnotherLine;
      }


      /* Check for "LIBRARY" which starts a block */
      else if(!strcmp(pToken,"LIBRARY"))
      {
        strcpy(pLibrary->szLibraryName,
               &(plb[strlen("LIBRARY")+1]));
        TrimString(pLibrary->szLibraryName);
        strcpy(lisLibrary.szLibraryName, pLibrary->szLibraryName);
      }

      /* Check for "SUB_LIBRARY" which starts a block */
      else if(!strcmp(pToken,"SUB_LIBRARY"))
      {
        strcpy(pLibrary->szSubLibraryName,
               &(plb[strlen("SUB_LIBRARY")+1]));
        TrimString(pLibrary->szSubLibraryName);
        strcpy(lisLibrary.szSubLibraryName, pLibrary->szSubLibraryName);
      }

      /* Check for "LANGUAGE" which starts a block */
      else if(!strcmp(pToken,"LANGUAGE"))
      {
        strcpy(pLibrary->szLanguage,
               &(plb[strlen("LANGUAGE")+1]));
        TrimString(pLibrary->szLanguage);
        strcpy(lisLibrary.szLanguage, pLibrary->szLanguage);
      }

      /* Check for "LOCATION" which starts a block */
      else if(!strcmp(pToken,"LOCATION"))
      {
        strcpy(pLibrary->szLocation,
               &(plb[strlen("LOCATION")+1]));
        TrimString(pLibrary->szLocation);
      }

      /* Check for "CONSTANT" which starts a block */
      if(!strcmp(pToken,"CONSTANT"))
      {
        /* initialize our struct */
        memset(pdccComment,0,sizeof(DefineConstantComment));

        /* copy the ConstantGroup name */
        strcpy(pdccComment->szConstantGroup, szConstantGroup);

        /* copy the Library information */
        memcpy(pLibrary, &lisLibrary, sizeof(lisLibrary));

        /* Constant Name */
        pToken=strtok(NULL,PARSE_DELIMITERS);
        strcpy(pdccComment->szConstantName,pToken);

        /* Get next line */
        RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
        if(RetCode || *plb == 0x00)
        {
          logit("e","Error during parse of #define "
                  "Constant comment %s\n", pdccComment->szConstantName);
          bSuccess = FALSE;  /* unexpected garbage */
          goto GetAnotherLine;
        }

        pToken=strtok(plb,PARSE_DELIMITERS);

        if(!strcmp(pToken,"VALUE"))
        {
          /* Constant Value */
          strcpy(pdccComment->szConstantValue,&(plb[strlen("VALUE")+1]));

          /* Get next line */
          RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
          strlen_plb = strlen(plb);

          if(RetCode || *plb == 0x00)
          {
            /* not what we were hoping for, but good enough */
            /* End of "CONSTANT" block.  Write the comment to file */
            OUT_RetCode = RETURN_SUCCESS;
            done = TRUE;
            bSuccess = TRUE;
          }
          else
          {
            /* look for a DESCRIPTION */
            pToken=strtok(plb,PARSE_DELIMITERS);
            /* Check for "DESCRIPTION" which starts a block */
            if(!strcmp(pToken,"DESCRIPTION"))
            {
              /* Get the description */
              
              /* initialize the params to pass to GetDescription() 
              *****************************************************/
              
              /* if there something after "DESCRIPTION"  then
              pass the rest of the line, otherwise pass NULL.  */
              if(strlen_plb > (strlen(plb) + 1))
                Temp_plb = &(plb[strlen(plb) + 1]);
              else
                Temp_plb = NULL;
              
              Temp_szDescription = 
                pdccComment->szConstantDescription;
              
              Temp_iDescriptionSize = 
                sizeof(pdccComment->szConstantDescription) - 1;
              
              /* Get the Description */
              RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
              if(RetCode == RETURN_SUCCESS)
                bSuccess=TRUE;
              else
                bSuccess = FALSE;

              /* End of "CONSTANT" block.  Write the comment to file */
              OUT_RetCode = RetCode;
              done        = TRUE;
            }  /* end if token = "DESCRIPTION" */
            else
            {
              /* somebody threw in some junk we weren't expecting.
                 write what we've got and throw out whatever we
                 just found.
              ********************************************************/
              /* End of "CONSTANT" block.  Write the comment to file */
              OUT_RetCode = RETURN_SUCCESS;
              bSuccess    = FALSE;
              done        = TRUE;
            } /* if strcmp "DESCRIPTION" */
          }  /* if GetNextLine bombed for "DESCRIPTION" */
        }  /* if strcmp "VALUE" */
        else
        {
          bSuccess = FALSE;  /* we were expecting "VALUE" */
        }  /* if strcmp "VALUE" */
      }  /* if strcmp "CONSTANT" */
      else
      {
        /* garbage */
        if(strncmp("****",pToken,4))
          bSuccess = FALSE;
      }
    }    /* end else not blank line or end comment */

    GetAnotherLine:
    if(!bSuccess)
    {
      /* we found something that we weren't expecting, and we are
         now in the middle of it.  Run through it so that it
         doesn't get in our way again.
      ************************************************************/
      RetCode  = ProcessGarbage();
      bSuccess = TRUE;
    }  /* if !bSuccess */
    
    if(!done)
    {
      /* Get next line */
      RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
      if(RetCode == RETURN_ERROR)
      {
        done        = TRUE;
        OUT_RetCode = RETURN_WARNING;
      }
    }
  }   /* end while !done */

  return(OUT_RetCode);
}  /* end ParseDefineComment() */


/* return codes 
    RETURN_SUCCESS   success, data found and retrieved
    RETURN_WARNING   no error, but no data found
    RETURN_ERROR     error, no data found
********************************************************/
/********************************************************************/
/********************************************************************/
int ParseTypedefComment(TypedefComment * ptdComment, 
                        LibraryInfoStruct * pLibrary)
{

  /* Buffer to hold the current line */
  char plb[MAX_COMMENT_LEN];  /* line buffer */

  int done = 0;
  size_t strlen_plb = 0;
  int RetCode;
  int OUT_RetCode = 0;
  char * pToken;

  /* bSuccess indicates whether we successfully obtained a clean
     comment.  If it is not true, then there is a cleanup routine
     that will push us through any remaining garbage until 
     we hit a blank line. 
  ***************************************************************/
  int bSuccess   = TRUE;

  int bDataFound = FALSE;
  
  char * Temp_szDescription;
  int    Temp_iDescriptionSize;
  char * Temp_plb;


  /* initialize the library info struct */
  memset(pLibrary, 0, sizeof(LibraryInfoStruct));

  /* Get next line */
  RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
  if(RetCode == RETURN_ERROR)
  {
    return(RetCode);
  }

  /* we are done when we either:
     1) Run out of lines in the file
     2) Hit an end comment line
  ***************************************/
  while(!done)
  {
    /* reinitialize RetCode */
    RetCode = RETURN_SUCCESS;

    /* check for blank line */
    if(!strlen(plb)) /* blank line */
    {
      goto GetAnotherLine;
    }

    /* check for End Comment line that ends a comment section */
    else if(!strcmp(plb,COMMENT_STRING_END))
    {
      if(bDataFound)
      {
        OUT_RetCode = RETURN_SUCCESS;
        done        = TRUE;
      }
      else
      {
        /* no useful comment found */  
        OUT_RetCode = RETURN_WARNING;
        done        = TRUE;
      }
    }  /* end if not EndOfComment string */
    else
    {
      /* grab the strlen, so that we can still decipher contents after
         strtok destroys the string */
      strlen_plb = strlen(plb);
      
      /* grab the first token off this line */
      pToken=strtok(plb,PARSE_DELIMITERS);

      if(!bDataFound)
      {
        /* Check for "TYPEDEF" which starts a block */
        if(!strcmp(pToken,"TYPEDEF"))
        {
          /* initialize our struct */
          memset(ptdComment,0,sizeof(TypedefComment));
          
          /* Typedef Name */
          pToken=strtok(NULL,PARSE_DELIMITERS);
          strcpy(ptdComment->szTypedefName,pToken);
          
          /* Get next line */
          RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
          if(RetCode || *plb == 0x00)
          {
            logit("e","Error during parse of typedef comment.\n");
            bDataFound = FALSE;
            goto GetAnotherLine;
          }
          
          pToken=strtok(plb,PARSE_DELIMITERS);
          if(!strcmp(pToken,"TYPE_DEFINITION"))
          {
            /* Type Definition */
            strcpy(ptdComment->szTypedefDefinition,
              &(plb[strlen("TYPE_DEFINITION")+1]));
            bDataFound = TRUE;
          }
          else
          {
            logit("e","Error during parse of typedef comment: "
              "TYPE_DEFINITION.\n Found: (%s) instead\n\n",
              plb);
            bSuccess = FALSE;
            goto GetAnotherLine;
          }
        }  /* if "TYPEDEF" not found */
        /* Check for "LIBRARY" which starts a block */
        else if(!strcmp(pToken,"LIBRARY"))
        {
          strcpy(pLibrary->szLibraryName,
                 &(plb[strlen("LIBRARY")+1]));
          TrimString(pLibrary->szLibraryName);
        }

        /* Check for "SUB_LIBRARY" which starts a block */
        else if(!strcmp(pToken,"SUB_LIBRARY"))
        {
          strcpy(pLibrary->szSubLibraryName,
                 &(plb[strlen("SUB_LIBRARY")+1]));
          TrimString(pLibrary->szSubLibraryName);
        }

        /* Check for "LANGUAGE" which starts a block */
        else if(!strcmp(pToken,"LANGUAGE"))
        {
          strcpy(pLibrary->szLanguage,
                 &(plb[strlen("LANGUAGE")+1]));
          TrimString(pLibrary->szLanguage);
        }

        /* Check for "LOCATION" which starts a block */
        else if(!strcmp(pToken,"LOCATION"))
        {
          strcpy(pLibrary->szLocation,
                 &(plb[strlen("LOCATION")+1]));
          TrimString(pLibrary->szLocation);
        }

        else
        {
          bSuccess = FALSE;
          goto GetAnotherLine;
        }
      }  /* !bDataFound */
      else
      {
        /* Check for "DESCRIPTION" which starts a block */
        if(!strcmp(pToken,"DESCRIPTION"))
        {
          /* Get the description */

          /* initialize the params to pass to GetDescription() 
          *****************************************************/
          
          /* if there something after "DESCRIPTION"  then
          pass the rest of the line, otherwise pass NULL.  */
          if(strlen_plb > (strlen(plb) + 1))
            Temp_plb = &(plb[strlen(plb) + 1]);
          else
            Temp_plb = NULL;
          
          Temp_szDescription = 
            ptdComment->szTypedefDescription;
          
          Temp_iDescriptionSize = 
            sizeof(ptdComment->szTypedefDescription) - 1;
          
          /* Get the Description */
          RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
          if(RetCode == RETURN_SUCCESS)
            bSuccess=TRUE;
          else
            bSuccess = FALSE;
        }  /* end if token = "DESCRIPTION" */

        /* Check for "MEMBER" which starts a block */
        else if(!strcmp(pToken,"MEMBER"))
        {
          /* Member Name */
          pToken=strtok(NULL,PARSE_DELIMITERS);
          strcpy(ptdComment->pcmMembers[ptdComment->iNumMembers].szMemberName,
            pToken);
          
          /* Get next line */
          RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
          if(RetCode || *plb == 0x00)
          {
            logit("e","Error during parse of typedef "
              "struct member \n");
            
            /* WRITE WHAT WE'VE GOT  and set bSuccess=FALSE */
            OUT_RetCode = RETURN_SUCCESS;
            bSuccess    = FALSE;
          }
          
          pToken=strtok(plb,PARSE_DELIMITERS);
          if(!strcmp(pToken,"MEMBER_TYPE"))
          {
            /* Member Type */
            strcpy(ptdComment->pcmMembers[ptdComment->iNumMembers].szMemberType,
              &(plb[strlen("MEMBER_TYPE")+1]));
            
            /* Get next line */
            RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
            if(RetCode || *plb == 0x00)
            {
              /* not what we were hoping for, but good enough */
              /* End of "MEMBER" block.  Increment the member counter */
              ptdComment->iNumMembers++;
              OUT_RetCode = RETURN_SUCCESS;
              bSuccess    = TRUE;
            }
            else
            {
              /* look for a MEMBER_DESCRIPTION */
              strlen_plb = strlen(plb);
              
              pToken=strtok(plb,PARSE_DELIMITERS);
              if(!strcmp(pToken,"MEMBER_DESCRIPTION"))
              {
                
              /* initialize the params to pass to GetDescription() 
                *****************************************************/
                
                /* if there something after "MEMBER_DESCRIPTION"  then
                pass the rest of the line, otherwise pass NULL.  */
                if(strlen_plb > (strlen(plb) + 1))
                  Temp_plb = &(plb[strlen(plb) + 1]);
                else
                  Temp_plb = NULL;
                
                Temp_szDescription = 
                  ptdComment->pcmMembers[ptdComment->iNumMembers].szMemberDescription;
                
                Temp_iDescriptionSize = 
                  sizeof(ptdComment->pcmMembers[ptdComment->iNumMembers].szMemberDescription) - 1;
                
                /* Get the Description */
                RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
                if(RetCode == RETURN_SUCCESS)
                {
                  ptdComment->iNumMembers++;
                  bSuccess=TRUE;
                }
                else
                {
                  bSuccess = FALSE;
                }
              }
              else
              {
              /* somebody threw in some junk we weren't expecting.
              note what we've got and throw out whatever we
              just found.
                ********************************************************/
                ptdComment->iNumMembers++;
                bSuccess = FALSE;
                
              } /* if strcmp "MEMBER_DESCRIPTION" */
            }   /* if GetNextLine bombed for "MEMBER_DESCRIPTION" */
          }     /* if strcmp "MEMBER_TYPE" */
        }       /* if strcmp "MEMBER" */

        /* Check for "NOTE" which starts a block */
        else if(!strcmp(pToken,"NOTE"))
        {
          /* Get the Note */

          /* initialize the params to pass to GetDescription() 
          *****************************************************/
          
          /* if there something after "NOTE"  then
          pass the rest of the line, otherwise pass NULL.  */
          if(strlen_plb > (strlen(plb) + 1))
            Temp_plb = &(plb[strlen(plb) + 1]);
          else
            Temp_plb = NULL;
          
          Temp_szDescription = 
            ptdComment->szNote;
          
          Temp_iDescriptionSize = 
            sizeof(ptdComment->szNote) - 1;
          
          /* Get the Note */
          RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
          if(RetCode == RETURN_SUCCESS)
            bSuccess=TRUE;
          else
            bSuccess = FALSE;
        }  /* end if token = "NOTE" */

        else
        {
          /* we didn't find what we were expecting, this must
             be a garbage comment.  Get rid of it. 
          ***************************************************/
          if(strncmp("****",pToken,4))
            bSuccess = FALSE;
        }
      }  /* end else !bDataFound */
    }    /* end else not blank line or end comment */

    GetAnotherLine:
    if(!bSuccess)
    {
      /* we found something that we weren't expecting, and we are
         now in the middle of it.  Run through it so that it
         doesn't get in our way again.
      ************************************************************/
      RetCode  = ProcessGarbage();
      bSuccess = TRUE;
    }  /* if !bSuccess */
    
    if(!done)
    {
      /* Get next line */
      RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
      if(RetCode == RETURN_ERROR)
      {
        done        = TRUE;
        OUT_RetCode = RETURN_WARNING;
      }
    }
  }   /* end while !done */

  return(OUT_RetCode);
}  /* end ParseTypedefComment() */







/* return codes 
    RETURN_SUCCESS   success, data found and retrieved
    RETURN_WARNING   no error, but no data found
    RETURN_ERROR     error, no data found
********************************************************/
/********************************************************************/
/********************************************************************/
int ParseLibraryComment(LibraryComment * plcComment, 
                        LibraryInfoStruct * pLibrary)
{

  /* Buffer to hold the current line */
  char plb[MAX_COMMENT_LEN];  /* line buffer */

  int done = 0;
  size_t strlen_plb = 0;
  int RetCode;
  int OUT_RetCode = 0;
  char * pToken;

  /* bSuccess indicates whether we successfully obtained a clean
     comment.  If it is not true, then there is a cleanup routine
     that will push us through any remaining garbage until 
     we hit a blank line. 
  ***************************************************************/
  int bSuccess   = TRUE;

  int bDataFound = FALSE;
  
  char * Temp_szDescription;
  int    Temp_iDescriptionSize;
  char * Temp_plb;
  char * pTemp;


  /* initialize the library info struct */
  memset(pLibrary, 0, sizeof(LibraryInfoStruct));

  /* initialize our struct */
  memset(plcComment,0,sizeof(LibraryComment));

  /* Get next line */
  RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
  if(RetCode == RETURN_ERROR)
  {
    return(RetCode);
  }

  /* we are done when we either:
     1) Run out of lines in the file
     2) Hit an end comment line
  ***************************************/
  while(!done)
  {
    /* reinitialize RetCode */
    RetCode = RETURN_SUCCESS;

    /* check for blank line */
    if(!strlen(plb)) /* blank line */
    {
      goto GetAnotherLine;
    }

    /* check for End Comment line that ends a comment section */
    else if(!strcmp(plb,COMMENT_STRING_END))
    {
      if(bDataFound)
      {
        OUT_RetCode = RETURN_SUCCESS;
        done        = TRUE;
      }
      else
      {
        /* no useful comment found */  
        OUT_RetCode = RETURN_WARNING;
        done        = TRUE;
      }
    }  /* end if not EndOfComment string */
    else
    {
      /* grab the strlen, so that we can still decipher contents after
         strtok destroys the string */
      strlen_plb = strlen(plb);
      
      /* grab the first token off this line */
      pToken=strtok(plb,PARSE_DELIMITERS);

      /* Check for "DESCRIPTION" which starts a block */
      if(!strcmp(pToken,"DESCRIPTION"))
      {
        /* Get the description */
        
        /* initialize the params to pass to GetDescription() 
        *****************************************************/
        
        /* if there something after "MEMBER_DESCRIPTION"  then
        pass the rest of the line, otherwise pass NULL.  */
        if(strlen_plb > (strlen(plb) + 1))
          Temp_plb = &(plb[strlen(plb) + 1]);
        else
          Temp_plb = NULL;
        
        Temp_szDescription = 
          plcComment->szLibraryDescription;
        
        Temp_iDescriptionSize = 
          sizeof(plcComment->szLibraryDescription) - 1;
        
        /* Get the Description */
        RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
        if(RetCode == RETURN_SUCCESS)
          bSuccess=TRUE;
        else
          bSuccess = FALSE;
      }  /* end if token = "DESCRIPTION" */

      /* Check for "LIBRARY" which starts a block */
      else if(!strcmp(pToken,"LIBRARY"))
      {
        strcpy(pLibrary->szLibraryName,
          &(plb[strlen("LIBRARY")+1]));
        TrimString(pLibrary->szLibraryName);

        bDataFound = TRUE;
      }
      
      /* Check for "SUB_LIBRARY" which starts a block */
      else if(!strcmp(pToken,"SUB_LIBRARY"))
      {
        strcpy(pLibrary->szSubLibraryName,
          &(plb[strlen("SUB_LIBRARY")+1]));
        TrimString(pLibrary->szSubLibraryName);
      }
      
      /* Check for "LANGUAGE" which starts a block */
      else if(!strcmp(pToken,"LANGUAGE"))
      {
        strcpy(pLibrary->szLanguage,
          &(plb[strlen("LANGUAGE")+1]));
        TrimString(pLibrary->szLanguage);
      }

      /* Check for "LOCATION" which starts a block */
      else if(!strcmp(pToken,"LOCATION"))
      {
        strcpy(pLibrary->szLocation,
               &(plb[strlen("LOCATION")+1]));
        TrimString(pLibrary->szLocation);
      }

      /* Check for "LINK_NAME" which starts a block */
      else if(!strcmp(pToken,"LINK_NAME"))
      {
        pTemp = malloc(strlen(&(plb[strlen("LINK_NAME")+1])));
        if(!pTemp)
        {
          logit("e","malloc error of for LINK_NAME\n");
          return(RETURN_ERROR);
        }
        strcpy(pTemp, &(plb[strlen("LINK_NAME")+1]));
        plcComment->szLinkNames[plcComment->iNumLinks] = pTemp;

        RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
        
        /* grab the first token off this line */
        pToken=strtok(plb,PARSE_DELIMITERS);

        if(!strcmp(pToken,"LINK"))
        {
          pTemp = malloc(strlen(&(plb[strlen("LINK")+1])));
          if(!pTemp)
          {
            logit("e","malloc error of for LINK\n");
            return(RETURN_ERROR);
          }
          strcpy(pTemp, &(plb[strlen("LINK")+1]));
          plcComment->szLinks[plcComment->iNumLinks] = pTemp;
          plcComment->iNumLinks++;
        }
        else
        {
          bSuccess = FALSE;
        }
      }  /* end if LINK_NAME */

      else
      {
        if(strncmp("****",pToken,4))
          bSuccess = FALSE;
      }
    }    /* end else not blank line or end comment */

    GetAnotherLine:
    if(!bSuccess)
    {
      /* we found something that we weren't expecting, and we are
         now in the middle of it.  Run through it so that it
         doesn't get in our way again.
      ************************************************************/
      RetCode  = ProcessGarbage();
      bSuccess = TRUE;
    }  /* if !bSuccess */
    
    if(!done)
    {
      /* Get next line */
      RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
      if(RetCode == RETURN_ERROR)
      {
        done        = TRUE;
        OUT_RetCode = RETURN_WARNING;
      }
    }
  }   /* end while !done */

  return(OUT_RetCode);
}  /* end ParseLibraryComment() */


/* return codes 
    RETURN_SUCCESS   success, data found and retrieved
    RETURN_WARNING   no error, but no data found
    RETURN_ERROR     error, no data found
********************************************************/
/********************************************************************/
/********************************************************************/
int ParseFunctionComment(FunctionComment * pfcComment, LibraryInfoStruct * pLibrary)
{

  /* Buffer to hold the current line */
  char plb[MAX_COMMENT_LEN];  /* line buffer */

  int done = 0;
  size_t strlen_plb = 0;
  int RetCode;
  int OUT_RetCode = 0;
  char * pToken;

  /* bSuccess indicates whether we successfully obtained a clean
     comment.  If it is not true, then there is a cleanup routine
     that will push us through any remaining garbage until 
     we hit a blank line. 
  ***************************************************************/
  int bSuccess   = TRUE;

  int bDataFound = FALSE;
  
  char * Temp_szDescription;
  int    Temp_iDescriptionSize;
  char * Temp_plb;


  /* initialize the library info struct */
  memset(pLibrary, 0, sizeof(LibraryInfoStruct));

  /* Get next line */
  RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
  if(RetCode == RETURN_ERROR)
  {
    return(RetCode);
  }

  /* we are done when we either:
     1) Run out of lines in the file
     2) Hit an end comment line
  ***************************************/
  while(!done)
  {
    /* reinitialize RetCode */
    RetCode = RETURN_SUCCESS;

    /* check for blank line */
    if(!strlen(plb)) /* blank line */
    {
      goto GetAnotherLine;
    }

    /* check for End Comment line that ends a comment section */
    else if(!strcmp(plb,COMMENT_STRING_END))
    {
      if(bDataFound)
      {
        OUT_RetCode = RETURN_SUCCESS;
        done        = TRUE;
      }
      else
      {
        /* no useful comment found */  
        OUT_RetCode = RETURN_WARNING;
        done        = TRUE;
      }
    }  /* end if not EndOfComment string */
    else
    {
      /* grab the strlen, so that we can still decipher contents after
         strtok destroys the string */
      strlen_plb = strlen(plb);
      
      /* grab the first token off this line */
      pToken=strtok(plb,PARSE_DELIMITERS);

      if(!bDataFound)
      {
        /* Check for "FUNCTION" which starts a block */
        if(!strcmp(pToken,"FUNCTION"))
        {
          /* initialize our struct */
          memset(pfcComment,0,sizeof(FunctionComment));
          
          /* Typedef Name */
          pToken=strtok(NULL,PARSE_DELIMITERS);
          strcpy(pfcComment->szFunctionName,pToken);

          if(!strcmp("P3DB_Associate_Chan_w_Comp", pfcComment->szFunctionName))
          {
            logit("","Got to reading assoc_chan_w_comp(), num params is %d\n",
              pfcComment->iNumParams);
          }

          
          bDataFound = TRUE;
        }  /* if "FUNCTION" found */
        /* Check for "PROCEDURE" which starts a block */
        else if(!strcmp(pToken,"PROCEDURE"))
        {
          /* initialize our struct */
          memset(pfcComment,0,sizeof(FunctionComment));
          
          /* Procedure Name */
          pToken=strtok(NULL,PARSE_DELIMITERS);
          strcpy(pfcComment->szFunctionName,pToken);
          
          RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
          pToken=strtok(plb,PARSE_DELIMITERS);

          if(!strcmp(pToken,"RETURN_PARAMETER"))
          {
            pfcComment->iReturnParam = atoi(&(plb[strlen(pToken) + 1]));

            /* Set procedure flag */
            pfcComment->bIsProcedure = TRUE;
            /* Set data found flag */
            bDataFound = TRUE;
          }
          else
          {
            /* this is garbled, do a reset */
            pfcComment->szFunctionName[0] = 0x00;
          }  /* else not "RETURN_PARAMETER" */
        }  /* else "PROCEDURE" found */

        /* Check for "LIBRARY" which starts a block */
        else if(!strcmp(pToken,"LIBRARY"))
        {
          strcpy(pLibrary->szLibraryName,
                 &(plb[strlen("LIBRARY")+1]));
          TrimString(pLibrary->szLibraryName);
        }

        /* Check for "SUB_LIBRARY" which starts a block */
        else if(!strcmp(pToken,"SUB_LIBRARY"))
        {
          strcpy(pLibrary->szSubLibraryName,
                 &(plb[strlen("SUB_LIBRARY")+1]));
          TrimString(pLibrary->szSubLibraryName);
        }

        /* Check for "LANGUAGE" which starts a block */
        else if(!strcmp(pToken,"LANGUAGE"))
        {
          strcpy(pLibrary->szLanguage,
                 &(plb[strlen("LANGUAGE")+1]));
          TrimString(pLibrary->szLanguage);
        }
        /* Check for "LOCATION" which starts a block */
        else if(!strcmp(pToken,"LOCATION"))
        {
          strcpy(pLibrary->szLocation,
                 &(plb[strlen("LOCATION")+1]));
          TrimString(pLibrary->szLocation);
        }
        else
        {
          bSuccess = FALSE;
          goto GetAnotherLine;
        }
      }  /* !bDataFound */
      else
      {
        /* Check for "DESCRIPTION" which starts a block */
        if(!strcmp(pToken,"DESCRIPTION"))
        {
          /* Get the description */

          /* initialize the params to pass to GetDescription() 
          *****************************************************/
          
          /* if there something after "MEMBER_DESCRIPTION"  then
          pass the rest of the line, otherwise pass NULL.  */
          if(strlen_plb > (strlen(plb) + 1))
            Temp_plb = &(plb[strlen(plb) + 1]);
          else
            Temp_plb = NULL;
          
          Temp_szDescription = 
            pfcComment->szFunctionDescription;
          
          Temp_iDescriptionSize = 
            sizeof(pfcComment->szFunctionDescription) - 1;
          
          /* Get the Description */
          RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
          if(RetCode == RETURN_SUCCESS)
          {
            bSuccess=TRUE;
          }
          else
          {
            bSuccess = FALSE;
          }
        }  /* end if token = "DESCRIPTION" */

        /* Check for "PARAMETER" which starts a block */
        else if(!strcmp(pToken,"PARAMETER"))
        {
           /* don't read the parameter number, we are going
              to do something stupid and assume they are in order.
          *******************************************************/
          RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
          pToken=strtok(plb,PARSE_DELIMITERS);
          
          /* Member Name */
          if(!strcmp(pToken,"PARAM_NAME"))
          {
            pToken=strtok(NULL,PARSE_DELIMITERS);
            strcpy(pfcComment->pcmFunctionParams[pfcComment->iNumParams].szMemberName,
                   pToken);
          
            /* Get next line */
            RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
            if(RetCode || *plb == 0x00)
            {
              logit("e","Error during parse of function member \n");
            
              /* WRITE WHAT WE'VE GOT  and set bSuccess=FALSE */
              OUT_RetCode = RETURN_SUCCESS;
              bSuccess    = FALSE;
            }
            
            pToken=strtok(plb,PARSE_DELIMITERS);
            if(!strcmp(pToken,"PARAM_TYPE"))
            {
              /* Param Type */
              strcpy(pfcComment->pcmFunctionParams[pfcComment->iNumParams].szMemberType,
                     &(plb[strlen("PARAM_TYPE")+1]));
            
              /* Get next line */
              RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
              if(RetCode || *plb == 0x00)
              {
                /* not what we were hoping for, but good enough */
                /* End of "PARAMETER" block.  Increment the member counter */
                pfcComment->iNumParams++;
                OUT_RetCode = RETURN_SUCCESS;
                bSuccess    = TRUE;
              }
              else
              {
                /* look for a PARAM_DESCRIPTION */
                strlen_plb = strlen(plb);
              
                pToken=strtok(plb,PARSE_DELIMITERS);
                if(!strcmp(pToken,"PARAM_DESCRIPTION"))
                {
                
                  /* initialize the params to pass to GetDescription() 
                  *****************************************************/
                
                  /* if there something after "PARAM_DESCRIPTION"  then
                     pass the rest of the line, otherwise pass NULL.  */
                  if(strlen_plb > (strlen(plb) + 1))
                    Temp_plb = &(plb[strlen(plb) + 1]);
                  else
                    Temp_plb = NULL;
                
                  Temp_szDescription = 
                    pfcComment->pcmFunctionParams[pfcComment->iNumParams].szMemberDescription;
                
                  Temp_iDescriptionSize = 
                    sizeof(pfcComment->pcmFunctionParams[pfcComment->iNumParams].szMemberDescription) - 1;
                
                  /* Get the Description */
                  RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
                  pfcComment->iNumParams++;
                  if(RetCode == RETURN_SUCCESS)
                    bSuccess=TRUE;
                  else
                    bSuccess = FALSE;
                }
                else  /* if line does not start with "DESCRIPTION" */
                {
                  /* somebody threw in some junk we weren't expecting.
                     note what we've got and throw out whatever we
                     just found.
                  ********************************************************/
                  pfcComment->iNumParams++;
                  bSuccess = FALSE;
                
                } /* else strcmp "MEMBER_DESCRIPTION" */
              }   /* else GetNextLine bombed for "MEMBER_DESCRIPTION" */
            }     /* if strcmp "PARAM_TYPE" */
            else
            {
              bSuccess = FALSE;
            }
          }       /* if strcmp "PARAM_NAME" */
          else
          {
            bSuccess = FALSE;
          }
        }         /* if strcmp "PARAMETER" */
        /* Check for "NOTE" which starts a block */
        else if(!strcmp(pToken,"NOTE"))
        {
          /* Get the Note */

          /* initialize the params to pass to GetDescription() 
          *****************************************************/
          
          /* if there something after "MEMBER_DESCRIPTION"  then
          pass the rest of the line, otherwise pass NULL.  */
          if(strlen_plb > (strlen(plb) + 1))
            Temp_plb = &(plb[strlen(plb) + 1]);
          else
            Temp_plb = NULL;
          
          Temp_szDescription = 
            pfcComment->szNote;
          
          Temp_iDescriptionSize = 
            sizeof(pfcComment->szNote) - 1;
          
          /* Get the Description */
          RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
          if(RetCode == RETURN_SUCCESS)
          {
            bSuccess=TRUE;
          }
          else
          {
            bSuccess = FALSE;
          }
        }  /* end if token = "NOTE" */

        /* Check for "RETURN_TYPE" which starts a block */
        else if(!strcmp(pToken,"RETURN_TYPE"))
        {
          strcpy(pfcComment->szReturnType,
                 &(plb[strlen("RETURN_TYPE")+1]));
        }
        /* Check for "RETURN_VALUE" which starts a block */
        else if(!strcmp(pToken,"RETURN_VALUE"))
        {
          strcpy(pfcComment->pcmReturnValues[pfcComment->iNumReturnValues].szMemberName,
                 &(plb[strlen("RETURN_VALUE")+1]));
          
          /* Get next line */
          RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
          if(RetCode || *plb == 0x00)
          {
            /* not what we were hoping for, but good enough */
            /* End of "RETURN_VALUE" block.  Increment the 
            return value counter */
            pfcComment->iNumReturnValues++;
            bSuccess=TRUE;
          }
          else
          {
            /* look for a RETURN_DESCRIPTION */
            strlen_plb = strlen(plb);
            
            pToken=strtok(plb,PARSE_DELIMITERS);
            if(!strcmp(pToken,"RETURN_DESCRIPTION"))
            {
              
              /* initialize the params to pass to GetDescription() 
              *****************************************************/
              
              /* if there something after "RETURN_DESCRIPTION"  then
                 pass the rest of the line, otherwise pass NULL.  */
              if(strlen_plb > (strlen(plb) + 1))
                Temp_plb = &(plb[strlen(plb) + 1]);
              else
                Temp_plb = NULL;
              
              Temp_szDescription = 
                pfcComment->pcmReturnValues[pfcComment->iNumReturnValues].szMemberDescription;
              
              Temp_iDescriptionSize = 
                sizeof(pfcComment->pcmReturnValues[pfcComment->iNumReturnValues].szMemberDescription) - 1;
              
              /* Get the Description */
              RetCode = GetDescription(Temp_szDescription, Temp_iDescriptionSize, Temp_plb);
              if(RetCode == RETURN_SUCCESS)
              {
                pfcComment->iNumReturnValues++;
                bSuccess=TRUE;
              }
              else
              {
                bSuccess = FALSE;
              }
            }
            else  /* if line does not start with "RETURN_DESCRIPTION" */
            {
              /* somebody threw in some junk we weren't expecting.
                 note what we've got and throw out whatever we
                 just found.
              ********************************************************/
              pfcComment->iNumReturnValues++;
              bSuccess = FALSE;
              
            } /* else strcmp "RETURN_DESCRIPTION" */
          }   /* else GetNextLine bombed for "RETURN_DESCRIPTION" */
        }     /* end else from if "DESCRIPTION", "RETURN_VALUE", "PARAM" */
        /* Check for "SOURCE_LOCATION" which starts a block */
        else if(!strcmp(pToken,"SOURCE_LOCATION"))
        {
          strcpy(pfcComment->szSourceLocation,
                 &(plb[strlen("SOURCE_LOCATION")+1]));
        }
        else
        {
          /* we didn't find what we were expecting, this must
             be a garbage comment.  Get rid of it. 
          ***************************************************/
          if(strncmp("****",pToken,4))
            bSuccess = FALSE;
        }     /* end else from if Description, etc..  */
      }       /* end else !bDataFound */
    }         /* end else not blank line or end comment */

    GetAnotherLine:
    if(!bSuccess)
    {
      /* we found something that we weren't expecting, and we are
         now in the middle of it.  Run through it so that it
         doesn't get in our way again.
      ************************************************************/
      RetCode  = ProcessGarbage();
      bSuccess = TRUE;
    }  /* if !bSuccess */
    
    if(!done)
    {
      /* Get next line */
      RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
      if(RetCode == RETURN_ERROR || RetCode == RETURN_GETNEXTLINE_EOF)
      {
        done        = TRUE;
        OUT_RetCode = RETURN_WARNING;
      }
    }
  }   /* end while !done */

  return(OUT_RetCode);
}  /* end ParseFunctionComment() */


/*********************************************************************
*********************************************************************/
int ProcessGarbage(void)
{
  int RetCode = 0;
  char plb[MAX_COMMENT_LEN] = "NOT BLANK";

  while(!(RetCode || *plb == 0x00 || !strncmp(plb,"****",4)))
    RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);

  return(RetCode);
}


/********************************************************************/
/********************************************************************/
int ParseListFile(char * szListFileName, int * piNumSourceFiles,
                  char * FileNameArray[])
{

  FILE * fp;
  int iNumSourceFiles = *piNumSourceFiles;
  int RetCode;
  int iRetCode = RETURN_SUCCESS;
  char * pToken;

  /* Buffer to hold the current line */
  char plb[MAX_COMMENT_LEN];  /* line buffer */

  /* initialize plb */
  plb[sizeof(plb)-1] = 0x00;

  /* attempt to open the file */
  fp = fopen(szListFileName,"r");

  /* verify the file was opened */
  if(!fp)
  {
    logit("e","ParseListFile: Could not open list file %s for reading\n",szListFileName);
    return(RETURN_ERROR);
  }

  /* we now have an open file */

  /* get the next line */
  while((RetCode=GetNextLine(plb,sizeof(plb)-1,fp)) != RETURN_ERROR
         && RetCode != RETURN_GETNEXTLINE_EOF)
  {

    /* Don't exceed the max num of files */
    if(iNumSourceFiles >= MAX_FILES)
    {
      logit("et", "MAXIMUM NUMBER OF FILES %d reached before end of "
                  "include file (%s)!  Some files will not be parsed!\n",
            MAX_FILES, szListFileName);
      iRetCode = RETURN_WARNING;
      break;
    }

    pToken = strtok(plb,PARSE_DELIMITERS);
    if(pToken)
    {
      FileNameArray[iNumSourceFiles] = malloc(strlen(pToken) + 1);
      if(!FileNameArray[iNumSourceFiles])
      {
        logit("e","ParseListFile: Memory allocation error!\n");
        return(RETURN_ERROR);
      }
      strcpy(FileNameArray[iNumSourceFiles], pToken);
      iNumSourceFiles++;
    }
  }

  /* copy number of source files back to caller's var */
  *piNumSourceFiles = iNumSourceFiles;

  if(fp)
  {
    fclose(fp);
    fp = NULL;
  }

  return(iRetCode);
}  /* end ParseListFile() */



/********************************************************************/
/********************************************************************/
int ReadSourceFile(FILE * fpIn, CommentStruct ** pCommentsArray, 
                   int * piNumComments, int * piNumAllocComments,
                   char * szFileName)
{

  int done = FALSE;
  CommentStruct Temp_Comment;
  int bUsefulCommentFound = FALSE;
  int RetCode;
  FunctionComment * pfcComment;
  CommentUnion Comment;
  
  Temp_Comment.pComment = &Comment;

  while(!done)
  {
    RetCode = GetNextCommentFromSourceFile(fpIn, &Temp_Comment);
    if(RetCode == RETURN_SUCCESS)
    {
      /* we got a comment */
      if(!strcmp(Temp_Comment.Library.szLocation, "THIS_FILE"))
      {
        strcpy(Temp_Comment.Library.szLocation, szFileName);
      }

      if(Temp_Comment.iCommentType == COMMENT_TYPE_FUNCTION)
      {
        pfcComment = (FunctionComment *)(Temp_Comment.pComment);
        if(!strcmp(pfcComment->szSourceLocation, "THIS_FILE"))
        {
          strcpy(pfcComment->szSourceLocation, szFileName);
        }

      }  /* end if iCommentType == COMMENT_TYPE_FUNCTION */

      /* Add the comment to the list */
      RetCode = AddCommentToList(pCommentsArray, &Temp_Comment,
                                 piNumComments, piNumAllocComments);
      if(RetCode == RETURN_ERROR)
      {
        logit("e","ReadSourceFile(): error in AddCommentToList()\n");
        return(RETURN_ERROR);
      }

      bUsefulCommentFound = TRUE;
    }
    else if(RetCode == RETURN_GETNEXTLINE_EOF)
    {
      done = TRUE;
    }
    else
    {
      logit("e", "Error encountered at line %d of %s\n",linectr, szFileName);
      done = TRUE;
    }
  }  /* end while not done */

  if(RetCode == RETURN_ERROR)
    return(RETURN_ERROR);
  else
    if(bUsefulCommentFound)
      return(RETURN_SUCCESS);
    else
      return(RETURN_WARNING);
}


int GetDescription(char * szBuffer, int iBufferSize, char * pTemp)
{
  char plb[MAX_COMMENT_LEN];
  int RetCode = RETURN_SUCCESS;
  int strlen_buffer = 0;
  
  if(pTemp)
  {
    strncpy(plb, pTemp, sizeof(plb));
    plb[sizeof(plb)-1] = 0x00;
  }
  else
  {
    RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
  }
  /* NULL terminate the buffer in case strncpy cannot */
  szBuffer[iBufferSize - 1] = 0x00;

  strlen_buffer = 0;

  while(RetCode == RETURN_SUCCESS && *plb != 0x00 )
  {
    strncpy(&(szBuffer[strlen_buffer]), plb,
            iBufferSize - 1 - (strlen_buffer));

    if(strlen(szBuffer) <
      strlen_buffer + strlen(plb) )
    {
    /* we truncated the last line.  We must be out of room in 
      szConstantDescription */
      RetCode = RETURN_WARNING;
    }
    else
    {
      strlen_buffer = strlen(szBuffer);
      szBuffer[strlen_buffer] = ' ';  
      strlen_buffer++;
      RetCode=GetNextLine(plb,sizeof(plb)-1,fpIn);
    }
  }  /* end while not done with comment */

  if(RetCode == RETURN_SUCCESS)
    szBuffer[strlen_buffer - 1] = 0x00;

  return(RetCode);
}  /* end GetDescription() */


char * GetCommentName(CommentStruct * pComment)
{
  static char BLANK[] = "";
  switch(pComment->iCommentType)
  {
  case COMMENT_TYPE_UNDEFINED:
    {
      /* uh this is bad, issue a warning and return 0 */
      logit("e", "GetCommentName():  WARNING: comment has "
                      "a comment type of UNDEFINED.   BUG!!!\n");
      return(BLANK);
    }
  case COMMENT_TYPE_CONSTANT:
    {
      return(((DefineConstantComment *)(pComment->pComment))->szConstantName);
      break;
    }
  case COMMENT_TYPE_TYPEDEF:
    {
      return(((TypedefComment *)(pComment->pComment))->szTypedefName);
      break;
    }
  case COMMENT_TYPE_FUNCTION:
    {
      return(((FunctionComment *)(pComment->pComment))->szFunctionName);
      break;
    }
  case COMMENT_TYPE_LIBRARY:
    {
      if(pComment->Library.szSubLibraryName[0])
        return(pComment->Library.szSubLibraryName);
      else
        return(pComment->Library.szLibraryName);
      break;
    }
  default:
    /* uh this is not good, we have a bad CommentType */
    logit("e", "GetCommentName():  WARNING: comment has "
                    "an invalid comment type(%d).   BUG!!!\n",
            pComment->iCommentType);
    return(BLANK);
  }  /* end switch */
}  /* end of GetCommentName() */


char * TrimString(char * szString)
{
  int i,j,k;

  for(i=0; szString[i] != 0x00; i++)
  {
    if(!(szString[i] == ' ' || szString[i] == '\t'))
    {
      j = i;
      break;
    }
  }

  k = strlen(szString) - 1;  /* set k to last char in string */

  for(i=k; ; i++)
  {
    if(!(szString[i] == ' ' || szString[i] == '\t'))
    {
      k = i;
      break;
    }
  }
  
  /* now the proper string is between j and k */

  if(j == 0)
  {
    szString[k+1] = 0x00;
    return(szString);
  }

  for(i=0; i <= k-j ; i++)
  {
    szString[i] = szString[j+i];
  }

  szString[i] = 0x00;

  return(szString);
}  /* end TrimString() */


/********************************************************************/
/********************************************************************/
int CompareCommentsByKeyword(const void* p1, const void* p2)
{

  const CommentStruct * pComment1 = p1;
  const CommentStruct * pComment2 = p2;

  /* compare keywords */
  return(strcmp1(pComment1->Keyword.szKeyword, pComment2->Keyword.szKeyword));
}  /* end CompareCommentsByKeyword() */
