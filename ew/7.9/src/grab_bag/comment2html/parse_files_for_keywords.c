#include "parse_api_doc.h"

int GetCommentFileName(char * szCurrentFilename, unsigned int iBufferSize,
                       char * szBaseDir, CommentStruct * pComment)
{

  char szTemp[255];


  szTemp[0] = 0x00;
  if(pComment->Library.szLibraryName[0])
  {
    strcat(szTemp,pComment->Library.szLibraryName);
    strcat(szTemp,"/");
    if(pComment->Library.szSubLibraryName[0])
    {
      strcat(szTemp,pComment->Library.szSubLibraryName);
      strcat(szTemp,"/");
    }
  }
  if(pComment->Library.szLanguage[0])
  {
    strcat(szTemp,pComment->Library.szLanguage);
    strcat(szTemp,"/");
  }
  sprintf(szCurrentFilename,"%s/%s%s", szBaseDir, szTemp,
          FileTypeNames[pComment->iCommentType]);

  if(strlen(szCurrentFilename) >= iBufferSize)
    return(RETURN_ERROR);
  else
    return(RETURN_SUCCESS);
}  /* end GetCommentFileName() */


int ParseFilesForKeywords(CommentStruct * CommentArray, int iNumComments,
                          char * szBaseDir)
{
  /* counter */
  int i;

  /* return code */
  int rc;

  /* file name buffer */
  char szCurrentFilename[255];

  /* Sample Comment */
  CommentStruct SampleComment;
  CommentStruct * CommentArray2;


  int iDiff = INT_MAX;

  FILE * fpTemp, *fpIn;

  CommentArray2 = malloc(iNumComments * sizeof(CommentStruct));
  if(!CommentArray2)
  {
    logit("e", "ParseFilesForKeywords() ERROR!! malloc of %d bytes failed!\n",
            iNumComments * sizeof(CommentStruct));
    return(RETURN_ERROR);
  }

  memcpy(CommentArray2, CommentArray, iNumComments * sizeof(CommentStruct));


  /* re-sort the comments via a different sorting mechanism */
  qsort(CommentArray2, iNumComments, sizeof(CommentStruct), 
        CompareCommentsSearchingForKeywords);

  /* parse the html files and add hyperlinks where appropriate */
  for(i=0; i < iNumComments; i++)
  {
    switch(iDiff)
    {
    case INT_MAX:
    case COMPARE_COMMENTS_DIFFERENT_LIBRARY:
    case COMPARE_COMMENTS_DIFFERENT_SUBLIBRARY:
    case COMPARE_COMMENTS_DIFFERENT_LANGUAGE:
    case COMPARE_COMMENTS_DIFFERENT_TYPE:
      {
        /* get new filename */
        rc = GetCommentFileName(szCurrentFilename, sizeof(szCurrentFilename),
                    szBaseDir, &CommentArray[i]);

        if(rc != RETURN_SUCCESS)
        {
          logit("e","GetCommentFileName() failed.\n");
          return(RETURN_ERROR);
        }

        /* open new file */
        fpIn=fopen(szCurrentFilename,"r");
        linectr = 0;
                
        if(!fpIn)
        {
          logit("e","WARNING!!!!: Could not open file %s for reading!\n",
                  szCurrentFilename);
          break;
        }

        /* open temp file */
        fpTemp=fopen(TEMPFILE_NAME,"w");
        if(!fpTemp)
        {
          logit("e","ERROR!!!!: Could not open tempfile for writing!\n",
                  szCurrentFilename);
          return(RETURN_ERROR);
        }

        /* parse the input file for keywords*/
        memcpy(&SampleComment, &CommentArray[i], sizeof(CommentStruct));
        rc = AddKeywordHyperlinks(fpIn,fpTemp,CommentArray2,iNumComments,
                                  &SampleComment);

        fclose(fpIn);
        fclose(fpTemp);

        if(rc == RETURN_SUCCESS)
        {
          rc = rename_ew(TEMPFILE_NAME, szCurrentFilename);
          if(rc)
          {
            logit("e","ParseFilesForKeywords(): ERROR: Failed to replace %s \n"
                           "with tempfile w/keyword hyperlinks\n",
                    szCurrentFilename);
          }
        }
        else
        {
          logit("e","ParseFilesForKeywords(): ERROR: AddKeywordHyperlinks()"
                         " failed for %s.\n",
                  szCurrentFilename);
        }
        if(rc == RETURN_SUCCESS)
          logit("e", "%s successfully processed.\n", szCurrentFilename);
        break;
      }  /* end case COMPARE_COMMENTS_DIFFERENT_TYPE */
    case COMPARE_COMMENTS_DIFFERENT_COMMENT_GROUP:
    case COMPARE_COMMENTS_DIFFERENT_COMMENT_NAME:
    default:
      {
        break;
      }
    }  /* end switch(iDiff) */

    if(i < (iNumComments-1))
    {
      iDiff=CompareComments(&CommentArray[i+1],&CommentArray[i]);
    }
       
  } /* end for comments in array */

  /* go home */
  return(RETURN_SUCCESS);
}  /* end ParseFilesForKeywords() */
