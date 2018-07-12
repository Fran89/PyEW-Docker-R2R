

/*************************************************/
/*************************************************/
/*               #INCLUDES                       */
/*************************************************/
/*************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parse_api_doc.h"

/*************************************************/
/*************************************************/
/*         #DEFINE CONSTANTS                     */
/*************************************************/
/*************************************************/

/* TRUE/FALSE constants */
#define TRUE 1
#define FALSE 0

/* Maximum length of a comment */
/*#define MAX_COMMENT_LEN 4096 */

/* Maximum length of tokens */
#define MAX_TOKENS 100

/* Comment String constants */
#define COMMENT_STRING_LINE1 "/************************************************"
#define COMMENT_STRING_LINE2 "************ SPECIAL FORMATTED COMMENT **********"
#define COMMENT_STRING_END   "************************************************/"

/* Parse delimiter constants for use with strtok() */
#define PARSE_DELIMITERS ", \t"

/* Function return code constants */
#define RETURN_SUCCESS  0
#define RETURN_WARNING  1
#define RETURN_ERROR   -1

/*************************************************/
/*************************************************/
/*             FUNCTION PROTOTYPES               */
/*************************************************/
/*************************************************/

int GetNextLine(char * pBuffer, int BufferSize, FILE * fptr);
/* GetNextLine reads the next line from FILE * fptr, and copies
   it into pBuffer.  It will always NULL terminate pBuffer and
   will remove the NEWLINE character.

    Return codes:
      RETURN_SUCCESS  Success
      RETURN_ERROR    Error in fgets()
      RETURN_WARNING  Warning: line too long for buffer, no newline char found
***********************************************************************/    


/*************************************************/
/*************************************************/
/*                  GLOBALS                      */
/*************************************************/
/*************************************************/

/* Counter for the current line in current file */
static int linectr=0;


/*************************************************/
/*************************************************/
/*                 FUNCTIONS                     */
/*************************************************/
/*************************************************/


/********************************************************************/
/*********************************************************************
  GetNextLine
    fills pBuffer with the next line from fptr.  It null terminates
    the line in place of a newline character. (like gets()).

    Return codes:
      RETURN_SUCCESS  Success
      RETURN_ERROR   Error in fgets
      RETURN_WARNING Warning: line too long for buffer, no newline char found
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
      logit("e","Reached end of file.  %d lines processed.\n",linectr);
    else
      logit("e","Error(%d) reading file near line %d.\n",
              errno,linectr);
    return(RETURN_ERROR);
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
  return(RETURN_SUCCESS);
}  /* End GetNextLine */


int CheckForArray(char * szName, char * szType)
{
  /* this function looks for array types, and moves the
     brackets ([]) from the name to the type */

  int iNameLen;

  iNameLen = strlen(szName);
  if(szName[iNameLen - 1] == ']')
  {
    /* we think we found an array */
    /* double check */
    if(szName[iNameLen - 2] == '[')
    {
      szName[iNameLen - 2] = 0x00;
      strcat(szType," []");
      return(TRUE);
    }
  }

  return(FALSE);
}


int main(int argc, char ** argv)
{   
  FILE *fptr;   
  char szBuffer[255];
  char szTotalBuffer[MAX_COMMENT_LEN] = "";
  int  iBufferSize = 255;
  int  iRetCode = RETURN_SUCCESS;
  int  bDone = FALSE;
  char szTempDest[MAX_COMMENT_LEN];
  char * pTotalBufferCurrent;
  char * pTempDestCurrent;
  char * pTemp;
  int  iTempLen;
  char *pCharString = "(,*)";           /* special characters to search for */
  char szDelimiters[]   = " \t\n";      /* token Delimiters */
  char * pTokenArray[MAX_TOKENS];
  int  i;                               /* loop counter */
  int  j = 0;
  int  iTokenNum;                      /* number of tokens */
  char szChars[]="(,);";
  char *pAddSpace =" ";
  int  iStateToSearchChar;
  int  iTheLastCharFound;
  FunctionComment fc;
  CommentStruct cs;  

  if(argc != 1)
  {
    printf("USAGE: protoparser < file1 > file2\n"
           "\n"
           "Where: \n"
           " file1 contains the prototype that you wish to parse, \n"
           "  terminated by a blank line.\n"
           "\n"
           " file2 is the output file where the program will write \n"
           " the resulting specially formatted comment.\n"
           "\n\n");
    return(-1);
  }

  fptr = stdin;

  /** Set file pointer to stdin **/
  /* if file pointer is NULL then return -1 */
  if(fptr == NULL)
  {
     logit("e", "Error occurred reading file!");
     return -1;
  }

  /** Read in the whole prototype **/
  /* keep getting next line if no error or no blank line encountered. 
  a blank line indicates the end of the prototype.*/
  while( iRetCode != RETURN_ERROR && !bDone  )
  {
    iRetCode = GetNextLine(szBuffer, iBufferSize, fptr);
    strcat (szTotalBuffer, szBuffer);
    
    /* if the first index of the buffer is 0x00 then we know a 
    blank line is encountered */
    if (szBuffer[0] == 0x00)
      bDone=TRUE;
  }
  
  /** Initialize a bunch of variables that we will use to parse 
      the prototype **/

  /* initialize pStringCurrent to point at the start of string */
  pTotalBufferCurrent = szTotalBuffer;
  
  /* initialize pTempDestCurrent to point at the start of tempDest */
  pTempDestCurrent = szTempDest;
  
  /* initialize tempDest buffer */
  memset(szTempDest, 0, sizeof(szTempDest));
  
  /* don't need to initialize pTemp, because it will be assigned later */
  
  /** Parse the prototype string for special characters.  Delimit those
      characters by putting spaces around them **/

  /* go through the string to find the 'special' characters (,) */
  while (*pCharString !='\0') 
  {
    /* searching for each of the special characters (,)  */
    while(pTemp = strchr(pTotalBufferCurrent, *pCharString))
    {
      
      iTempLen = pTemp - pTotalBufferCurrent;
      
      /* we want to copy the portion of string 
      (from pStringCurrent to pTemp) to pTempDestCurrent */
      memcpy(pTempDestCurrent, pTotalBufferCurrent, iTempLen);
      
      /* reset pStringCurrent and pTempDestCurrent */
      pTempDestCurrent += iTempLen;
      pTotalBufferCurrent = pTemp + 1;
      
      /* we want to copy <SPACE> ch <SPACE> to pTempDest in lieu of ch */
      *pTempDestCurrent = ' ';
      pTempDestCurrent++;
      *pTempDestCurrent = *pCharString;
      pTempDestCurrent++;
      *pTempDestCurrent = ' ';
      pTempDestCurrent++;
      
      
    }
    /* there are no more occurrences of ch in string */
    
    /* copy what's left of string to the end of tempdest */
    strcpy(pTempDestCurrent, pTotalBufferCurrent);  /* idealy we should use strncpy */
    *pCharString++; 
  }
  /* end of our block for iChar1*/
  

  /** Tokenize the resulting space delimited function prototype */
 
  /* Establish string and get the first token: */
  pTokenArray[0] = strtok( szTempDest, szDelimiters );  
  
  /* loops through the remaining of the string until
  strtok returns a NULL */
  for ( i = 1; (pTokenArray[i] = strtok(NULL, szDelimiters)) != NULL; i++)
  {  
  /* return value of strtok () is already assigned 
    to pTokenArray[i] */
  }

  /** Record the number of tokens found in the string **/
  iTokenNum = i;  /* keep track of number of tokens in the string */
  

  /** Initialize function comment members **/

  /* initialize fc struct */
  memset(&fc, 0, sizeof(fc));

  fc.iNumReturnValues = 1;
  strcpy(fc.pcmReturnValues[0].szMemberName,"Default_Return_Value");
  strcpy(fc.pcmReturnValues[0].szMemberDescription, 
         "Description of the default return value");

  strcpy(fc.szNote, "Optionally, write a note about \nthe function here.");
  fc.bIsProcedure = FALSE;
  strcpy(fc.szFunctionDescription, "Optionally, write a description of what "
                                   "the function does\n"
                                   "and how it behaves.");
  strcpy(fc.szSourceLocation, "THIS_FILE");

  fc.iStability = FUNCTION_MATURITY_NEW;

  fc.iNumParams = 0;
  iStateToSearchChar = 1;
  /* loops through the string to find appropriate tokens and 
  assign the to appropriate parameters. */
  for ( i = 0; i < iTokenNum; i++)
  {
    if (iStateToSearchChar == 1)
    {
      if (*pTokenArray[i] == '(')
      {
        strncat(fc.szFunctionName,pTokenArray[i-1], sizeof(fc.szFunctionName));
        for (j=0; j<i-1; j++)
        {
          strncat(fc.szReturnType,pTokenArray[j],sizeof(fc.szReturnType));
          strncat(fc.szReturnType, pAddSpace, 1);
        }
        CheckForArray(fc.szFunctionName, fc.szReturnType);
        iStateToSearchChar = 2;
        iTheLastCharFound = i;
      }
    } /* end if iStateToSearchChar == 1 */
    
    else if (iStateToSearchChar == 2)
    {
      if ((*pTokenArray[i] == ',') || (*pTokenArray[i] == ')'))
      {  
        strncat(fc.pcmFunctionParams[fc.iNumParams].szMemberName,pTokenArray[i-1], 
          sizeof(fc.pcmFunctionParams[fc.iNumParams].szMemberName));
        for (j=iTheLastCharFound + 1; j< i - 1; j++)
        {
          strncat(fc.pcmFunctionParams[fc.iNumParams].szMemberType,pTokenArray[j],
            sizeof(fc.pcmFunctionParams[fc.iNumParams].szMemberType));
          strncat(fc.pcmFunctionParams[fc.iNumParams].szMemberType, pAddSpace, 1);
        }
        CheckForArray(fc.pcmFunctionParams[fc.iNumParams].szMemberName, 
          fc.pcmFunctionParams[fc.iNumParams].szMemberType);

        sprintf(fc.pcmFunctionParams[fc.iNumParams].szMemberDescription,
                "Optional description of (%s)",
                fc.pcmFunctionParams[fc.iNumParams].szMemberName);

        fc.iNumParams++;                /* for each parameter */
        iTheLastCharFound = i;
      }
    } /* end if iStateToSearchChar == 2 */
  } /* end of for loop going thru tokens */
  
  
  memset(&cs,0,sizeof(CommentStruct));
  
  cs.pComment = &fc;
  cs.iCommentType = COMMENT_TYPE_FUNCTION;
  strcpy(cs.Library.szLibraryName,"EWDB_ORA_API");
  strcpy(cs.Library.szSubLibraryName,"UNKNOWN");
  strcpy(cs.Library.szLanguage,"C");
  strcpy(cs.Library.szLocation,"THIS_FILE");
  
  iRetCode = WriteCommentToCommentFile(stdout, &cs);
  
  return(-1);
  
}  /* end of main */