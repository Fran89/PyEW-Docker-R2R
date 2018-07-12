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

/* We need two programs with parsing logic.

   Program #1 goes through the p3db_ora_api.h
   file and parses all of the specially formatted
   comments.  It then creates two sets of data:
   The first set is the documentation structures, 
   and the second set is the list of keyword hyperlinks.
   Presumably, each set of data can be written out to
   a file for later use.
   

   Program #2 goes through the documentation and
   adds hyperlinks for all of the keywords.

   For a first cut, these programs will be very dumb,
   and do the bare minimum!!!  davidk 05/02/00

   For a second, these programs will be dumb and do
   the bare minimum!!! davidk 02/02/01
*****************************************************/

/* This is program 1 (to the tune of: This is CNN) */

/*************************************************/
/*************************************************/
/*               #INCLUDES                       */
/*************************************************/
/*************************************************/
#include "parse_api_doc.h"


/*************************************************/
/*************************************************/
/*             FUNCTION PROTOTYPES               */
/*************************************************/
/*************************************************/

int GetNextLine(char * pBuffer, int BufferSize, FILE * fptr);
/**********************************************************************
  GetNextLine
    fills pBuffer with the next line from fptr.  It null terminates
    the line in place of a newline character. (like gets()).

    Return codes:
      RETURN_SUCCESS  Success
      RETURN_ERROR   Error in fgets
      RETURN_WARNING Warning: line too long for buffer, no newline char found
**********************************************************************/



int GetNextCommentFromSourceFile(FILE * fpIn, CommentStruct * pTemp_Comment);
int GetNextCommentFromCommentFile(FILE * fpIn, CommentStruct * pTemp_Comment);
int AddCommentToList(CommentStruct ** pCommentsArray, CommentStruct * pComment,
                     int * piNumComments, int * piNumAllocComments);


/* return codes 
    RETURN_SUCCESS   success, data found and retrieved
    RETURN_WARNING   no error, but no data found
    RETURN_ERROR     error, no data found
********************************************************/
int ParseFunctionComment(FunctionComment * pfcComment, 
                         LibraryInfoStruct * pLibrary);
int ParseTypedefComment(TypedefComment * ptdComment, 
                         LibraryInfoStruct * pLibrary);
int ParseDefineComment(DefineConstantComment * pdccComment, 
                         LibraryInfoStruct * pLibrary);
int ParseLibraryComment(LibraryComment * plcComment, 
                        LibraryInfoStruct * pLibrary);


int ParseListFile(char * szListFileName, int * piNumSourceFiles, 
                  char * FileNameArray[]);

int GetDescription(char * szBuffer, int iBufferSize, char * pTemp);

int ProcessGarbage(void);


int GetCommentSize(int iCommentType);


/*************************************************/
/*************************************************/
/*              GLOBAL VARIABLES                 */
/*************************************************/
/*************************************************/
FILE * fpDCC,* fpTDC, * fpFCC;
KeywordStruct KeywordHyperlinks[400];
int iNumKeywordHyperlinks=0;

/* Usage message issued when incorrect command line options are given */
const char USAGE_MESSAGE[]= "Usage: parse_api_doc [-O <output only comments file>]"
                            "                     [-I <input only comments file>]\n"
                            "                     [--INCLUDE <name of file that contains list\n"
                            "                                 of files to include (newline delimited)>]\n"
                            "                     [<source_file1>] [<source_file2>] [<source_fileN>]\n";


/* file pointers */
FILE * fpIn  = NULL;
FILE * fpOut = NULL;

/* Counter for the current line in current file */
int linectr=0;


/* Summary of command.  This is duplicated in the USAGE_MESSAGE */
/* 
Usage: parse_api_doc [-O <output only comments file>] 
                     [-I <input only comments file>] 
                     [--INCLUDE <name of file that contains list 
                                 of files to include (newline delimited)>] 
                     [<source_file1>] [<source_file2>] [<source_fileN>]
***************************************************************/


/*************************************************/
/*************************************************/
/*                 FUNCTIONS                     */
/*************************************************/
/*************************************************/


/********************************************************************/
/********************************************************************/
int main(int argc, char ** argv)
{
  /* array of source filenames passed in via command line */
  char * FileNameArray[MAX_FILES];

  /* output Comment file */
  char * szOutputFileName = NULL;

  /* input Comment file */
  char * szInputFileName  = NULL;

  /* input List file */
  char * szListFileName  = NULL;

  /* Return code variable */
  int RetCode;

  /* Number of source files in FileNameArray */
  int iNumSourceFiles = 0;

  /* Current source file index in FileNameArray */
  int iCurrentSourceFile = 0;

  /* counter */
  int i;

  /* number of Comments in comment array */
  int iNumComments      = 0;

  /* size of comment array */
  int iNumAllocComments = 0;

  /* Flag for parsing filenames from the command line */
  int iNextFileFlag = NO_FILE_TYPE;

  /* Comment array */
  CommentStruct * CommentArray   = NULL;

  /* i
  /*
  int bSuccess;
  int iStatus=0;
  int done;
  int bDataFound;
  */

  /* first:  parse the command line */
  /*  figure out what we got passed on the command line */
  for(i=1; i < argc; i++)
  {
    if(argv[i][0] == '-')
    {
      /* we are dealing with a flag, check it out */
      if(!strcmp(argv[i],"-O"))  /* -O for output file */
      {
        /* the next string should be an output file */
        iNextFileFlag = OUTPUT_FILE_TYPE;
      }
      else if(!strcmp(argv[i],"-I"))  /* -I for input comment file */
      {
        /* the next string should be an input file */
        iNextFileFlag = INPUT_FILE_TYPE;
      }
      else if(!strcmp(argv[i],"--INCLUDE"))  /* --INCLUDE for filelist file */
      {
        /* the next string should be a list file */
        iNextFileFlag = LIST_FILE_TYPE;
      }
      else
      {
        /* unrecognized flag.  Issue Usage message */
        logit("e", USAGE_MESSAGE);
        return(-1);
      }
    }
    else
    {
      /* not a flag, must be a normal name */
      if(iNextFileFlag == OUTPUT_FILE_TYPE)
        szOutputFileName = argv[i];
      else if(iNextFileFlag == INPUT_FILE_TYPE)
      {
        szInputFileName = argv[i];
      }
      else if(iNextFileFlag == LIST_FILE_TYPE)
        szListFileName = argv[i];
      else
      {
        FileNameArray[iNumSourceFiles] = argv[i];
        iNumSourceFiles++;
      }
      /* end if */

      iNextFileFlag = NO_FILE_TYPE;
    }
  }  /* end for i in argc (parsing command line) */

  if(szListFileName)
  {
    /* parse list file, retrieve all input names */
    RetCode = ParseListFile(szListFileName, &iNumSourceFiles, FileNameArray);

    /* we don't care what the function returned */
  }

  if((!iNumSourceFiles) && (szInputFileName == NULL))
  {
    logit("e","No source files found.  "
            "At least one source file must be specified!\n%s", USAGE_MESSAGE);
    return(-1);
  }

  /* Create Comments Array */
  CommentArray = (CommentStruct *)malloc(100 * sizeof(CommentStruct));
  iNumAllocComments = 100;

  /* if we need to read a comments file, read it first */
  if(szInputFileName)
  {
    fpIn = fopen(szInputFileName,"r");
    if(!fpIn)
    {
      logit("e","Error:  Could not open input file %s\n", szInputFileName);
      return(-1);
    }

    /* we now have an open comments file, read it in. */
    RetCode = ReadCommentsFile(fpIn, &CommentArray, &iNumComments, &iNumAllocComments);

    /* close the input comments file */
    fclose(fpIn);

    if(RetCode)
    {
      logit("e","ReadCommentsFile() failed with retcode = %d\n", RetCode);
      return(-1);
    }
  }  /* end if szInputFileName */

  /* Now read from each source file */
  for(i = 0; i < iNumSourceFiles; i++)
  {
    /* open the source file */
    fpIn = fopen(FileNameArray[i],"r");
    if(!fpIn)
    {
      logit("e","Warning:  Could not open input file %s,"
                     " skipping file and continuing.\n", 
              FileNameArray[i]);
      continue;
    }

    RetCode = ReadSourceFile(fpIn, &CommentArray, &iNumComments, 
                             &iNumAllocComments, FileNameArray[i]);
    
    if(RetCode == RETURN_SUCCESS)
    {
      logit("e","%s successfully processed!!!\n", FileNameArray[i]);
    }
    /* issue some sort of warning if we didn't get any comments from the file */
    else if(RetCode == RETURN_WARNING)
    {
      logit("e","WARNING:  No usable comments found in %s\n", FileNameArray[i]);
    }
    else /* RetCode == RETURN_ERROR */
    {
      logit("e","ERROR:  Error processing file %s\n", FileNameArray[i]);
    }

    /* close the open source file */
    fclose(fpIn);

  }  /* end for i in iNumSourceFiles */

  /* done with all input/source files */

  /* open output file */
  if(!szOutputFileName)
  {
    /* if there was no output file specified then dump stuff to stdout. */
    fpOut = stdout;
    if(!fpOut)
    {
      logit("e", "ERROR: Could not set output to stdout!  (No output file provided.)\n");
    }
  }
  else
  {
    fpOut = fopen(szOutputFileName,"w");
    if(!fpOut)
    {
      logit("e", "ERROR: Could not open output file %s\n",szOutputFileName);
    }
  }  /* end if !szOutputFileName */

  if(!fpOut)
  {
    return(-1);
  }

  /* write the output file */
  for(i=0; i < iNumComments; i++)
  {
    RetCode = WriteCommentToCommentFile(fpOut, &CommentArray[i]); 
  }

  /* close the output file */
  fclose(fpOut);

  /* go home */
  return(0);
}  /* end main() */
