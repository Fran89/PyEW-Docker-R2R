/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2001/07/20 16:34:36  davidk
 *     changed fprintf's to logit's
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

#define KEYWORD_STRTOK_DELIMITERS "\t=,.!? ><\n[]()"


int GetBaseURLForComment(char * szBaseURL, 
                         unsigned int iBufferSize,
                         CommentStruct * pComment)
{


  szBaseURL[0] = 0x00;
  if(pComment->Library.szLibraryName[0])
  {
    strcat(szBaseURL,"../");
    if(pComment->Library.szSubLibraryName[0])
    {
      strcat(szBaseURL,"../");
    }
  }
  if(pComment->Library.szLanguage[0])
  {
    strcat(szBaseURL,"../");
  }
  /* yes, I know this is wrong, but.... */
  if(strlen(szBaseURL) >= iBufferSize)
    return(RETURN_ERROR);
  else
    return(RETURN_SUCCESS);
}  /* end GetBaseURLForComment() */


/********************************************************************/
/********************************************************************/
int AddKeywordHyperlinks(FILE * fpIn, FILE * fpTemp, 
                         CommentStruct * CommentArray,
                         int iNumComments,
                         CommentStruct * pBaseComment)
{

  static char szTemp[2550], szTemp2[2550], szTempOut[5120], szHeading[255];
  static char szBaseURL[255];
  char * pLocation, * pOldLocation, * pNextToken, * pOrigin;

  CommentStruct * pComment;

  int RetCode;

  GetBaseURLForComment(szBaseURL, sizeof(szBaseURL), pBaseComment);

  while(!feof(fpIn))
  {
    RetCode=GetNextLine(szTemp,sizeof(szTemp),fpIn);
    if(RetCode!= RETURN_GETNEXTLINE_EOF && RetCode != RETURN_SUCCESS)
    {
      logit("e","AddKeywordHyperlinks(): ERROR processing file.\n"); 
      continue;
    }

    /* first check for an anchor (heading) */
    strcpy(szTempOut,szTemp);
    pLocation=strstr(szTempOut,"<a name=");
    if(pLocation)
    {
      /* we have a match, get the new section name */
      pNextToken=strtok(&pLocation[strlen("<a name=")],KEYWORD_STRTOK_DELIMITERS);
      strcpy(szHeading,pNextToken);
    }

    /* now parse the line, one token at a time. */

    /* initialize everything */
    strcpy(szTemp2,szTemp);
    pOrigin=szTemp2;
    pOldLocation=szTemp2;
    szTempOut[0]=0x00;
    pNextToken=strtok(szTemp2, KEYWORD_STRTOK_DELIMITERS);
    while(pNextToken)
    {
      if(strcmp(pNextToken,szHeading))  /* don't process token 
                                           if it matches the heading */
      {
        strcpy(pBaseComment->Keyword.szKeyword,pNextToken);
        pComment=bsearch(pBaseComment, CommentArray, iNumComments, 
                         sizeof(CommentStruct),
                         CompareCommentsSearchingForKeywords);
        if(pComment)
        {
          /* we found a match! */
          /* first copy everything up to the current token! */
          strncat(szTempOut,&szTemp[pOldLocation-pOrigin],pNextToken-pOldLocation);

          /* then print the hyperlink in lieu of the current token. */
          sprintf(&(szTempOut[strlen(szTempOut)]),
                  "<a href=%s%s>%s</a>",
                  szBaseURL,
                  pComment->Keyword.szHyperlink,
                  pComment->Keyword.szKeyword
                 );

          /* then adjust the pOldLocation pointer, so that it points to 
             just after the end of the current token. */
          pOldLocation=pNextToken+strlen(pNextToken);
        }  /* end if strcmp(pNextToken, szHeading) */
      }

      pNextToken = strtok(NULL,  KEYWORD_STRTOK_DELIMITERS);
    }  /* end while strtok() of current line */
    /* copy everything that is left */
    strcat(szTempOut,&szTemp[pOldLocation-pOrigin]);

    /* now write the new line out */
    fprintf(fpTemp, szTempOut);
    fprintf(fpTemp, "\n");
  }  /* end while(!feof(fpIn)) */

  return(RETURN_SUCCESS);

}  /* end AddKeywordHyperlinks() */


/********************************************************************/
/********************************************************************/
int CompareCommentsSearchingForKeywords(const void* p1, const void* p2)
{

  const CommentStruct * pComment1 = p1;
  const CommentStruct * pComment2 = p2;

  int rc;

  /* first check Library names */
  if(rc = strcmp(pComment1->Library.szLibraryName,pComment2->Library.szLibraryName))
    return(rc * COMPARE_COMMENTS_DIFFERENT_LIBRARY);

  /* then check the keyword */
  rc = strcmp(pComment1->Keyword.szKeyword, pComment2->Keyword.szKeyword);
  return(rc * COMPARE_COMMENTS_DIFFERENT_COMMENT_NAME);
}  /* end CompareCommentsSearchingForKeywords() */


