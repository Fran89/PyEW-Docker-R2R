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
int WriteCommentToCommentFile(FILE * fpOut, CommentStruct * pComment)
{

  int RetCode;

  /* write header */
  fprintf(fpOut, "\n");
  fprintf(fpOut, "%s\n", COMMENT_STRING_LINE1);
  fprintf(fpOut, "%s\n", COMMENT_STRING_LINE2);

  switch(pComment->iCommentType)
  {
  case COMMENT_TYPE_UNDEFINED:
    {
      logit("e","%s: WARNING: comment type undefined.  Not writing comment.\n"
              "WriteCommentToCommentFile()");
      RetCode = RETURN_WARNING;
      break;
    }
  case COMMENT_TYPE_CONSTANT:
    {
      
      fprintf(fpOut, "TYPE DEFINE\n\n");

      WriteLibraryInfoToCommentFile(fpOut, &(pComment->Library));

      RetCode = WriteConstantToCommentFile(fpOut,
                                           (DefineConstantComment *) 
                                             (pComment->pComment)
                                          );
      break;
    }

  case COMMENT_TYPE_TYPEDEF:
    {
      
      fprintf(fpOut, "TYPE TYPEDEF\n\n");

      WriteLibraryInfoToCommentFile(fpOut, &(pComment->Library));

      RetCode = WriteTypedefToCommentFile(fpOut,
                                           (TypedefComment *) 
                                             (pComment->pComment)
                                          );
      break;
    }

  case COMMENT_TYPE_FUNCTION:
    {
      
      fprintf(fpOut, "TYPE FUNCTION_PROTOTYPE\n\n");

      WriteLibraryInfoToCommentFile(fpOut, &(pComment->Library));

      RetCode = WriteFunctionToCommentFile(fpOut,
                                           (FunctionComment *) 
                                             (pComment->pComment)
                                          );
      break;
    }

  case COMMENT_TYPE_LIBRARY:
    {
      
      fprintf(fpOut, "TYPE LIBRARY\n\n");

      WriteLibraryInfoToCommentFile(fpOut, &(pComment->Library));

      RetCode = WriteLibraryToCommentFile(fpOut,
                                           (LibraryComment *) 
                                             (pComment->pComment)
                                          );
      break;
    }
  default: 
    {
      logit("e","%s: WARNING: comment type is invalid(%d).  "
                     "Not writing comment.\n"
              "WriteCommentToCommentFile()", pComment->iCommentType);
      RetCode = RETURN_WARNING;
      break;
    }
  }  /* end switch(pComment->iCommentType) */


  /* write trailer */
  fprintf(fpOut, "\n");
  fprintf(fpOut, "%s\n", COMMENT_STRING_END2);
  fprintf(fpOut, "%s\n", COMMENT_STRING_END);

  /* all done */
  return(RetCode);
}



int WriteLibraryInfoToCommentFile(FILE * fpOut, LibraryInfoStruct * pLibrary)
{


  if(pLibrary->szLibraryName[0])
  {
    fprintf(fpOut, "LIBRARY %s\n\n", pLibrary->szLibraryName);
  }

  if(pLibrary->szSubLibraryName[0])
  {
    fprintf(fpOut, "SUB_LIBRARY %s\n\n", pLibrary->szSubLibraryName);
  }

  if(pLibrary->szLanguage[0])
  {
    fprintf(fpOut, "LANGUAGE %s\n\n", pLibrary->szLanguage);
  }

  /* Write the prototype location if available */
  if(pLibrary->szLocation[0])
  {
    fprintf(fpOut, "LOCATION %s\n\n", pLibrary->szLocation);
  }

  return(RETURN_SUCCESS);
}  /* end of WriteLibraryInfoToCommentFile() */


/********************************************************************/
/********************************************************************/
int WriteConstantToCommentFile(FILE * fpOut, DefineConstantComment * pdccComment)
{

  if(pdccComment->szConstantGroup[0])
    fprintf(fpOut, "CONSTANT_GROUP %s\n\n", pdccComment->szConstantGroup);

  /* write the Constant block */
  fprintf(fpOut, "CONSTANT_NAME %s\n", pdccComment->szConstantName);
  fprintf(fpOut, "VALUE %s\n", pdccComment->szConstantValue);
  if(pdccComment->szConstantDescription[0])
    fprintf(fpOut, "DESCRIPTION %s\n", pdccComment->szConstantDescription);
  fprintf(fpOut, "\n");

  return(RETURN_SUCCESS);
}  /* end WriteConstantToCommentFile() */


/********************************************************************/
/********************************************************************/
int WriteTypedefToCommentFile(FILE * fpOut, TypedefComment * ptdComment)
{
  int i;

  /* write the main Typedef block */
  fprintf(fpOut, "TYPEDEF %s\n", ptdComment->szTypedefName);
  fprintf(fpOut, "TYPE_DEFINITION %s\n", ptdComment->szTypedefDefinition);
  fprintf(fpOut, "\n");

  /* write the description block if available */
  if(ptdComment->szTypedefDescription[0])
    fprintf(fpOut, "DESCRIPTION %s\n\n", ptdComment->szTypedefDescription);

  /* write the NOTE block if available */
  if(ptdComment->szNote[0])
    fprintf(fpOut, "NOTE %s\n\n", ptdComment->szNote);

  for(i=0; i< ptdComment->iNumMembers; i++)
  {
    /* write each MEMBER block */
    fprintf(fpOut, "MEMBER %s\n", ptdComment->pcmMembers[i].szMemberName);
    fprintf(fpOut, "MEMBER_TYPE %s\n", ptdComment->pcmMembers[i].szMemberType);
    if(ptdComment->pcmMembers[i].szMemberDescription[0])
      fprintf(fpOut, "MEMBER_DESCRIPTION %s\n", 
              ptdComment->pcmMembers[i].szMemberDescription);
    fprintf(fpOut, "\n");
  }

  return(RETURN_SUCCESS);
}  /* end WriteTypedefToCommentFile() */


/********************************************************************/
/********************************************************************/
int WriteFunctionToCommentFile(FILE * fpOut, FunctionComment * pfcComment)
{
  int i;

  /* write the main Function block */
  if(pfcComment->bIsProcedure)
  {
    fprintf(fpOut, "\nPROCEDURE %s\n", pfcComment->szFunctionName);
    fprintf(fpOut, "RETURN_PARAMETER %d\n", pfcComment->iReturnParam);
  }
  else
  {
    fprintf(fpOut, "\nFUNCTION %s\n", pfcComment->szFunctionName);
  }
  fprintf(fpOut, "\n");

  /* write the stability block */
  fprintf(fpOut, "STABILITY ");

  switch(pfcComment->iStability)
  {
  case FUNCTION_MATURITY_NEW:
    fprintf(fpOut, "NEW\n\n");
    break;
  case FUNCTION_MATURITY_CHANGED:
    fprintf(fpOut, "CHANGED\n\n");
    break;
  case FUNCTION_MATURITY_MATURE:
    fprintf(fpOut, "MATURE\n\n");
    break;
  case FUNCTION_MATURITY_DEPRICATED:
    fprintf(fpOut, "DEPRICATED\n\n");
    break;
  default:
    break;
  }

  /* Write the source location if available */
  if(pfcComment->szSourceLocation[0])
    fprintf(fpOut, "SOURCE_LOCATION %s\n\n", pfcComment->szSourceLocation);

  /* Write the return type if available */
  if(pfcComment->szReturnType[0])
    fprintf(fpOut, "RETURN_TYPE %s\n\n", pfcComment->szReturnType);

  /* Write the RETURN_VALUEs */
  for(i=0; i< pfcComment->iNumReturnValues; i++)
  {
    /* write each RETURN_VALUE block */
    fprintf(fpOut, "RETURN_VALUE %s\n", pfcComment->pcmReturnValues[i].szMemberName);
    if(pfcComment->pcmReturnValues[i].szMemberDescription[0])
      fprintf(fpOut, "RETURN_DESCRIPTION %s\n", 
              pfcComment->pcmReturnValues[i].szMemberDescription);
    fprintf(fpOut, "\n");
  }

  /* Write the PARAMETERs */
  for(i=0; i< pfcComment->iNumParams; i++)
  {
    /* write each PARAMETER block */
    fprintf(fpOut, "PARAMETER %d\n", i+1);
    fprintf(fpOut, "PARAM_NAME %s\n", pfcComment->pcmFunctionParams[i].szMemberName);
    fprintf(fpOut, "PARAM_TYPE %s\n", pfcComment->pcmFunctionParams[i].szMemberType);
    if(pfcComment->pcmFunctionParams[i].szMemberDescription[0])
      fprintf(fpOut, "PARAM_DESCRIPTION %s\n", 
              pfcComment->pcmFunctionParams[i].szMemberDescription);
    fprintf(fpOut, "\n");
  }

  /* write the description block if available */
  if(pfcComment->szFunctionDescription[0])
    fprintf(fpOut, "DESCRIPTION %s\n\n", pfcComment->szFunctionDescription);

  /* write the NOTE block if available */
  if(pfcComment->szNote[0])
    fprintf(fpOut, "NOTE %s\n\n", pfcComment->szNote);

  return(RETURN_SUCCESS);
}  /* end WriteFunctionToCommentFile() */


/********************************************************************/
/********************************************************************/
int WriteLibraryToCommentFile(FILE * fpOut, LibraryComment * plcComment)
{
  int i;

  /* write the description block if available */
  if(plcComment->szLibraryDescription[0])
    fprintf(fpOut, "DESCRIPTION %s\n\n", plcComment->szLibraryDescription);

  for(i=0; i<plcComment->iNumLinks; i++)
  {
      fprintf(fpOut, "LINK_NAME %s\n", 
              plcComment->szLinkNames[i]);
      fprintf(fpOut, "LINK %s\n", plcComment->szLinks[i]);
      fprintf(fpOut, "\n");
  }


  return(RETURN_SUCCESS);
}



