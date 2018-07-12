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



/********************************************************************/
/********************************************************************/
int WriteCommentToHTMLFile(FILE * fpOut, CommentStruct * pComment, 
                           int bSameGroup, CommentStruct * CommentArray,
                           int iNumComments)
{

  int RetCode;

  if(!(bSameGroup && pComment->iCommentType == COMMENT_TYPE_CONSTANT))
  {
    /* write header */
    fprintf(fpOut, "<hr>\n");
  }

  /* print an anchor for hyperlink use */
  fprintf(fpOut, "<a name=%s></a>\n",GetCommentName(pComment));

  /* print the comment using the method appropriate for the comment type */
  switch(pComment->iCommentType)
  {
  case COMMENT_TYPE_UNDEFINED:
    {
      logit("e","%s: WARNING: comment type undefined.  Not writing comment.\n"
              "WriteCommentToHTMLFile()");
      RetCode = RETURN_WARNING;
      break;
    }
  case COMMENT_TYPE_CONSTANT:
    {
      
      RetCode = WriteConstantToHTMLFile(fpOut,
                                           (DefineConstantComment *) 
                                             (pComment->pComment)
                                          );
      if(RetCode = RETURN_SUCCESS)
        WriteLibraryInfoToHTMLFile(fpOut, &(pComment->Library));

      break;
    }

  case COMMENT_TYPE_TYPEDEF:
    {
      
      RetCode = WriteTypedefToHTMLFile(fpOut,
                                           (TypedefComment *) 
                                             (pComment->pComment)
                                          );
      WriteLibraryInfoToHTMLFile(fpOut, &(pComment->Library));

      break;
    }

  case COMMENT_TYPE_FUNCTION:
    {
      
      RetCode = WriteFunctionToHTMLFile(fpOut,
                                           (FunctionComment *) 
                                             (pComment->pComment)
                                          );
      WriteLibraryInfoToHTMLFile(fpOut, &(pComment->Library));

      break;
    }

  case COMMENT_TYPE_LIBRARY:
    {
      
      RetCode = WriteLibraryToHTMLFile(fpOut,
                                       (LibraryComment *) (pComment->pComment),
                                       &(pComment->Library),
                                       CommentArray, iNumComments
                                      );
      break;
    }
  default: 
    {
      logit("e","%s: WARNING: comment type is invalid(%d).  "
                     "Not writing comment.\n"
              "WriteCommentToHTMLFile()", pComment->iCommentType);
      RetCode = RETURN_WARNING;
      break;
    }
  }  /* end switch(pComment->iCommentType) */


  /* write trailer */
  fprintf(fpOut,"</table>\n\n<br><br>\n\n");

  /* all done */
  return(RetCode);
}  /* end WriteCommentToHTMLFile() */


int WriteLibraryInfoToHTMLFile(FILE * fpOut, LibraryInfoStruct * pLibrary)
{

  /* print a blank line for buffer between us and the above data. */
  fprintf(fpOut, " <tr><td><br></td></tr>\n");

  if(pLibrary->szLibraryName[0])
  {
    fprintf(fpOut, " <tr>\n <td><b>Group:</b></td>\n");
    fprintf(fpOut, "  <td>%s</td>\n </tr>\n",pLibrary->szLibraryName);
  }
  
  if(pLibrary->szSubLibraryName[0])
  {
    fprintf(fpOut, " <tr>\n <td><b>Sub Group:</b></td>\n");
    fprintf(fpOut, "  <td>%s</td>\n </tr>\n",pLibrary->szSubLibraryName);
  }

  if(pLibrary->szLanguage[0])
  {
    fprintf(fpOut, " <tr>\n <td><b>Language:</b></td>\n");
    fprintf(fpOut, "  <td>%s</td>\n </tr>\n",pLibrary->szLanguage);
  }

  /* Write the prototype location if available */
  if(pLibrary->szLocation[0])
  {
    fprintf(fpOut, " <tr>\n <td><b>Location:</b></td>\n");
    fprintf(fpOut, "  <td>%s</td>\n </tr>\n",pLibrary->szLocation);
  }

  return(RETURN_SUCCESS);
}  /* end of WriteLibraryInfoToHTMLFile() */


/********************************************************************/
/********************************************************************/
int WriteConstantToHTMLFile(FILE * fpOut, DefineConstantComment * pdccComment)
{

  static char szLastConstantGroup[60]="";
  int bNewGroup = FALSE;

  if(strcmp(szLastConstantGroup,pdccComment->szConstantGroup))
  {
    fprintf(fpOut,"<font size=+2>%s</font><br>\n<br>\n", pdccComment->szConstantGroup);
    strncpy(szLastConstantGroup, pdccComment->szConstantGroup, 
            sizeof(szLastConstantGroup));
    szLastConstantGroup[sizeof(szLastConstantGroup)-1] = 0x00;
    bNewGroup = TRUE;
  }

  fprintf(fpOut,"<font size=+1>%s</font><br>\n<br>\n", pdccComment->szConstantName);

  fprintf(fpOut,"<table>\n");

  fprintf(fpOut, " <tr>\n <td valign=top><b>Value:</b></td>\n");
  fprintf(fpOut,"  <td>%s</td>\n </tr>\n",pdccComment->szConstantValue);

  fprintf(fpOut, " <tr>\n <td valign=top><b>Description:</b></td>\n");
  fprintf(fpOut,"  <td>%s</td>\n </tr>\n",pdccComment->szConstantDescription);

  /* the trailer to close the table is written out by the caller,
     because they may want to add additional information, such as
     library info.
  */
  if(bNewGroup)
    return(RETURN_SUCCESS);
  else
    return(RETURN_WARNING);

}  /* end WriteConstantToHTMLFile() */


/********************************************************************/
/********************************************************************/
int WriteTypedefToHTMLFile(FILE * fpOut, TypedefComment * ptdComment)
{
  int i;

  fprintf(fpOut,"<font size=+1>%s</font><br>\n<br>\n", ptdComment->szTypedefName);

  fprintf(fpOut,"<table>\n");

  fprintf(fpOut, " <tr>\n <td valign=top><b>Type:</b></td>\n");
  fprintf(fpOut,"  <td>%s</td>\n </tr>\n",ptdComment->szTypedefDefinition);

  if(ptdComment->szTypedefDescription)
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Description:</b></td>\n");
    fprintf(fpOut,"  <td>%s</td>\n </tr>\n",ptdComment->szTypedefDescription);
  }

  if(ptdComment->szNote)
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Note:</b></td>\n");
    fprintf(fpOut,"  <td>%s</td>\n </tr>\n",ptdComment->szNote);
  }


  if(ptdComment->iNumMembers)
  {
    fprintf(fpOut, " <tr>\n <td colspan=2>\n");
    fprintf(fpOut,"   <table border=1 cellpadding=5>\n");
    fprintf(fpOut,"    <tr>\n     <td><b>Field</b></td>\n"
                  "     <td><b>Type</b></td>\n"
                  "     <td><b>Description</b></td>\n </tr>\n");
  }

  for(i=0; i< ptdComment->iNumMembers; i++)
  {
    /* write each MEMBER block */
    fprintf(fpOut,"    <tr>\n");
    fprintf(fpOut,"     <td>%s</td>\n",ptdComment->pcmMembers[i].szMemberName);
    fprintf(fpOut,"     <td>%s</td>\n",ptdComment->pcmMembers[i].szMemberType);
    fprintf(fpOut,"     <td>%s</td>\n",ptdComment->pcmMembers[i].szMemberDescription);
    fprintf(fpOut,"    </tr>\n");
  }
  
  if(ptdComment->iNumMembers)
  {
    fprintf(fpOut, "   </table>\n");
    fprintf(fpOut, "  </td>\n </tr>\n");
  }

  
  /* the trailer to close the table is written out by the caller,
     because they may want to add additional information, such as
     library info.
  */
  return(RETURN_SUCCESS);
}  /* end WriteTypedefToHTMLFile() */


/********************************************************************/
/********************************************************************/
int WriteFunctionToHTMLFile(FILE * fpOut, FunctionComment * pfcComment)
{
  int i;

  fprintf(fpOut,"<font size=+2>%s()</font><br>\n<br>\n", pfcComment->szFunctionName);

  fprintf(fpOut,"<table>\n");

  fprintf(fpOut, " <tr>\n <td valign=top><b>Prototype:</b></td>\n");

  
  /*******************
   Function Prototype 
  ********************/
  /* print a bunch of formatting junk */
  fprintf(fpOut,"  <td><table><tr><td valign=top><NOBR>"
                "<font size=-1 face=monospace>");

  /* if it's not a procedure print the return type */
  if(!pfcComment->bIsProcedure)
  {
    fprintf(fpOut,"%s ", pfcComment->szReturnType);
  }
  /* print the function name and some formatting junk */
  fprintf(fpOut, "%s(</NOBR></font></td>\n"
                 "     <td><font size=-1 face=monospace>",
          pfcComment->szFunctionName);

  /* if there are parameters, print them otherwise print VOID */
  if(pfcComment->iNumParams)
  {
    /* make the first case special because of the ',' between params */
    fprintf(fpOut,"<NOBR><i>%s</i> %s</NOBR>",
            pfcComment->pcmFunctionParams[0].szMemberType,
            pfcComment->pcmFunctionParams[0].szMemberName
           );
    for(i=1;i<pfcComment->iNumParams;i++)
    {
      fprintf(fpOut,", <NOBR><i>%s</i> %s</NOBR>",
              pfcComment->pcmFunctionParams[i].szMemberType,
              pfcComment->pcmFunctionParams[i].szMemberName
             );
    }
  }  /* end if pfcComment->iNumParams */
  else
  {
    fprintf(fpOut,"void");
  }
  fprintf(fpOut,"); ");
  fprintf(fpOut,"</font></td></tr></table></td>\n </tr>\n");

  /* source file */
  if(pfcComment->szSourceLocation[0])
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Source File:</b></td>\n");
    fprintf(fpOut,"  <td>%s</td>\n </tr>\n",pfcComment->szSourceLocation);
  }

  /* description */
  if(pfcComment->szFunctionDescription[0])
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Description:</b></td>\n");
    fprintf(fpOut,"  <td>%s</td>\n </tr>\n",pfcComment->szFunctionDescription);
  }

  /* note */
  if(pfcComment->szNote[0])
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Note:</b></td>\n");
    fprintf(fpOut,"  <td>%s</td>\n </tr>\n",pfcComment->szNote);
  }

  /* return code info */
  fprintf(fpOut, " <tr>\n  <td colspan=2><br><b>RETURN CODE INFO</b></td></tr>\n");

  /* return type */
  fprintf(fpOut," <tr>\n <td valign=top><b>Return Type:</b></td>\n");
  fprintf(fpOut,"  <td>%s</td>\n </tr>\n",pfcComment->szReturnType);

  /* return param number */
  if(pfcComment->bIsProcedure)
  {
    fprintf(fpOut," <tr>\n <td valign=top><b>Return Arg:</b></td>\n");
    fprintf(fpOut,"  <td>%d</td>\n </tr>\n",pfcComment->iReturnParam);
  }
  
  /* Table of Return Values */
  if(pfcComment->iNumReturnValues)
  {
    fprintf(fpOut," <tr><td colspan=2>");
    fprintf(fpOut,"<table border=1 cellpadding=5>\n");
    fprintf(fpOut," <tr>\n  <td><b>Return Value</b></td>\n"
            "  <td><b>Description</b></td>\n </tr>\n");

    for(i=0; i < pfcComment->iNumReturnValues; i++)
    {
      fprintf(fpOut," <tr>\n");
      fprintf(fpOut,"  <td>%s</td>\n",
              pfcComment->pcmReturnValues[i].szMemberName);
      fprintf(fpOut,"  <td>%s</td>\n",
              pfcComment->pcmReturnValues[i].szMemberDescription);
      fprintf(fpOut," </tr>\n");
    }
    
    fprintf(fpOut,"  </table></td></tr>\n");
  }  /* end if(pfcComment->iNumReturnValues)  
        meaning there are struct members to print. 
     *********************************************/

  /* parameter info */
  fprintf(fpOut, " <tr>\n  <td colspan=2><br><b>PARAMETER INFO</b></td></tr>\n");
  /* parameters */

  /* Function Parameters */
  if(pfcComment->iNumParams)
  {
    fprintf(fpOut," <tr><td colspan=2>");
    fprintf(fpOut,"<table border=1 cellpadding=5>\n");
    fprintf(fpOut," <tr>\n  <td><b>Parameter</b></td>\n"
            "  <td><b>Type</b></td>\n"
            "  <td><b>Description</b></td>\n </tr>\n");

    for(i=0; i < pfcComment->iNumParams; i++)
    {
      fprintf(fpOut," <tr>\n");
      fprintf(fpOut,"  <td>%s</td>\n",
              pfcComment->pcmFunctionParams[i].szMemberName);
      fprintf(fpOut,"  <td>%s</td>\n",
              pfcComment->pcmFunctionParams[i].szMemberType);
      fprintf(fpOut,"  <td>%s</td>\n",
              pfcComment->pcmFunctionParams[i].szMemberDescription);
      fprintf(fpOut," </tr>\n");
    }
    
    fprintf(fpOut,"  </table></td></tr>\n");
  }  /* end if(pfcComment->iNumParams)  
        meaning there are struct members to print. 
     *********************************************/
  
  /* the trailer to close the table is written out by the caller,
     because they may want to add additional information, such as
     library info.
  */
  return(RETURN_SUCCESS);

}  /* end WriteFunctionToHTMLFile() */


/********************************************************************/
/********************************************************************/
int WriteLibraryToHTMLFile(FILE * fpOut, LibraryComment * plcComment,
                           LibraryInfoStruct * pLibrary,
                           CommentStruct * CommentArray, int iNumComments)
{
  int i;
  int bPrintCommentTypeHeading;
  char szTempLanguage[255];
  int iTempCommentType;
  int bFirstPass = TRUE;

  if(pLibrary->szSubLibraryName[0])
  {
    /* we assume this is a sub-library record */
    fprintf(fpOut,"<font size=+2>%s</font><br>\n<br>\n", 
            pLibrary->szSubLibraryName);
  }
  else
  {
    /* we assume this is a library record */
    fprintf(fpOut,"<font size=+2>%s</font><br>\n<br>\n", 
            pLibrary->szLibraryName);
  }

  fprintf(fpOut,"<table>\n");

  if(pLibrary->szSubLibraryName[0])
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Group:</b></td>\n");
    fprintf(fpOut, "  <td>%s</td>\n </tr>\n",pLibrary->szLibraryName);
  }
  
  /* write the description block if available */
  if(plcComment->szLibraryDescription[0])
  {
    fprintf(fpOut, " <tr>\n <td valign=top><b>Description:</b></td>\n");
    fprintf(fpOut, "  <td>%s</td>\n </tr>\n",plcComment->szLibraryDescription);
  }

  /* write formatting stuff for the links */
  if(plcComment->iNumLinks)
  {
    fprintf(fpOut, " <tr>\n  <td colspan=2>\n   <table><tr>\n");
  }

  /* write each link */
  for(i=0; i<plcComment->iNumLinks; i++)
  {
    fprintf(fpOut, "<td><a href=%s>%s</a></td>\n", 
            plcComment->szLinks[i], plcComment->szLinkNames[i]);
  }

  /* write closing formatting stuff for the links */
  if(plcComment->iNumLinks)
  {
    fprintf(fpOut, "    </tr>\n   </table></td></tr>\n");
  }

  /* initialize Temp information */
  memset(szTempLanguage, 0, sizeof(szTempLanguage));
  iTempCommentType = 0;

  if(pLibrary->szSubLibraryName[0])
  {
    fprintf(fpOut, " <tr><td colspan=2>\n");
    fprintf(fpOut, "  <DIR>\n");

    /* this is a sub-library */
    for(i=0; i < iNumComments; i++)
    {
      if(!strcmp(pLibrary->szLibraryName, 
                 CommentArray[i].Library.szLibraryName))
      {
        if(!strcmp(pLibrary->szSubLibraryName, 
                   CommentArray[i].Library.szSubLibraryName))
        {
          if(CommentArray[i].pComment == plcComment)
          {
            /* we have found ourselves in the list.  skip! */
            continue;
          }

          if(strcmp(szTempLanguage, CommentArray[i].Library.szLanguage))
          {
            if(bFirstPass)
              bFirstPass = FALSE;
            else
              fprintf(fpOut,"     </DIR>\n   </DIR>\n");

            fprintf(fpOut,"    <LI>%s\n",CommentArray[i].Library.szLanguage);
            fprintf(fpOut,"     <DIR>\n");

            bPrintCommentTypeHeading = TRUE;
            strcpy(szTempLanguage, CommentArray[i].Library.szLanguage);
            iTempCommentType =  CommentArray[i].iCommentType;
          }
          if(iTempCommentType !=  CommentArray[i].iCommentType || 
             bPrintCommentTypeHeading)
          {
            /* print close list formatting */
            if(iTempCommentType !=  CommentArray[i].iCommentType)
            {
              fprintf(fpOut,"     </DIR>\n");
            }

            fprintf(fpOut,"      <LI>%s\n",FileTypeNames[CommentArray[i].iCommentType]);
            fprintf(fpOut,"       <DIR>\n");  /* list of comments */

            iTempCommentType = CommentArray[i].iCommentType;
            bPrintCommentTypeHeading = FALSE;
          }
          fprintf(fpOut,"        <LI>%s\n",GetCommentName(&CommentArray[i]));
        }  /* end if matching sublibrary name */
      }  /* end if matching library name */
    }  /* end for i in iNumComments (each comment) */

    fprintf(fpOut,"       </DIR>\n     </DIR>\n   </DIR>\n");
    fprintf(fpOut,"  </td>\n </tr>\n");
  }  /* end if this comment is sub-library */
  else
  {  /* this comment is full library */
    fprintf(fpOut, " <tr><td colspan=2>\n");
    fprintf(fpOut, "  <DIR>\n");

    /* this is a sub-library */
    for(i=0; i < iNumComments; i++)
    {
      if(CommentArray[i].iCommentType == COMMENT_TYPE_LIBRARY)
      { 
        if(!strcmp(pLibrary->szLibraryName, 
                   CommentArray[i].Library.szLibraryName))
        {
          /* we have a matching library record. */
          if(CommentArray[i].Library.szSubLibraryName[0])
          {
            /* it's a sublibrary */
            fprintf(fpOut,"    <LI>%s\n",GetCommentName(&CommentArray[i]));
          }
          else
          {
            /* this is the record for the current comment.  Ignore it. */
          }
        }   /* end if matching library name */
      }  /* end if commenttype = library */
    }  /* end for each comment */
    fprintf(fpOut,"   </DIR>\n");
    fprintf(fpOut,"  </td>\n </tr>\n");
  }

  return(RETURN_SUCCESS);
}  /* end WriteLibraryToHTMLFile() */

