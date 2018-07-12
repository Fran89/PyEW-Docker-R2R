/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/03/17 20:45:18  davidk
 *     Added additional debugging statements.
 *
 *     Revision 1.3  2001/07/20 16:37:38  davidk
 *     removed some c++ comments.
 *
 *     Revision 1.2  2001/07/20 16:34:36  davidk
 *     moved header stuff to parse_api_doc.h
 *     changed fprintf's to logit's
 *     reformatted some whitespace
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
#ifdef WIN32
# include <direct.h>  /* needed for getcwd() on NT */
#endif  /* WIN32 */
#include "parse_api_doc.h"


/*************************************************/
/*************************************************/
/*             FUNCTION PROTOTYPES               */
/*************************************************/
/*************************************************/




/*************************************************/
/*************************************************/
/*              GLOBAL VARIABLES                 */
/*************************************************/
/*************************************************/

/* globals externed in parse_api_doc.h */
/* file pointers */
FILE * fpIn  = NULL;
FILE * fpOut = NULL;

/* Counter for the current line in current file */
int linectr=0;

#define COMMENT_TYPE_UNDEFINED 0
#define COMMENT_TYPE_LIBRARY   1
#define COMMENT_TYPE_CONSTANT  2
#define COMMENT_TYPE_TYPEDEF   3
#define COMMENT_TYPE_FUNCTION  4

/* File name constants for different filetypes */
const char FileTypeNames[][30] = 
{
  "undefined_comments.html", /* COMMENT_TYPE_UNDEFINED */
  "library_comments.html",   /* COMMENT_TYPE_LIBRARY   */
  "constant_comments.html",  /* COMMENT_TYPE_CONSTANT  */
  "typedef_comments.html",   /* COMMENT_TYPE_TYPEDEF   */
  "function_comments.html"   /* COMMENT_TYPE_FUNCTION  */
};

/*************************************************/
/*************************************************/
/*              GLOBAL VARIABLES (to this file)  */
/*************************************************/
/*************************************************/


/* Usage message issued when incorrect command line options are given */
static const char USAGE_MESSAGE[]=
  "Usage: comment2html [-O <base output directory for html files>]"
  "                     [-I <input only comments file>]\n"
  "                     [--INCLUDE <name of file that contains list\n"
  "                                 of files to include (newline delimited)>]\n"
  "                     [<source_file1>] [<source_file2>] [<source_fileN>]\n";




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
  char * szOutputFileDir = NULL;

  /* output Comment file */
  char * szCurrentDir = ".";

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

  /* buffer for the base output directory */
  char szBaseDir[255];

  int bSameGroup = FALSE;
  int bGroupAlreadyChecked;

  int iDiff;

  char szCurrentOutputURLDirectory[512];
  char szCurrentOutputURLFileName[512];
  
  int bDirWorkDone;
  char testdir[512];
  CommentStruct * pCurr;


  logit_init("comment2html", 0, 1024, 1);
  logit("","/***************************/\n"
           "/*** comment2html startup **/\n"
           "/***************************/\n");
   
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
        logit("","Caught -O flag.\n");
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
        logit("e", "%s",USAGE_MESSAGE);
        return(-1);
      }
    }
    else
    {
      /* not a flag, must be a normal name */
      if(iNextFileFlag == OUTPUT_FILE_TYPE)
      {
        szOutputFileDir = argv[i];
        logit("","Setting Output directory to [%s]\n",szOutputFileDir);
      }
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

    if(RetCode == RETURN_ERROR)
    {
      logit("et","ParseListFile() failed.  Quitting!!!!\n");
      return(-1);
    }

    /* we don't care if the function returned a warning */
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
    linectr = 0;
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
    linectr = 0;
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
      logit("e","There are now %d comments and %d allocated.\n", 
              iNumComments, iNumAllocComments);
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
  if(!szOutputFileDir)
  {
    /* if there was no output file specified then dump stuff to the current directory. */
    szOutputFileDir = szCurrentDir;
        logit("","Setting Output directory to default[%s]\n",szOutputFileDir);
  }  /* end if !szOutputFileName */

  /* sort the comment records */
  qsort(CommentArray, iNumComments, sizeof(CommentStruct), CompareComments);

  /* Initialize iDiff to be the maximum possible difference,
     since there is no previous record */
  iDiff = INT_MAX;

  chdir_ew(szOutputFileDir);
  getcwd(szBaseDir, sizeof(szBaseDir));
  sprintf(szCurrentOutputURLDirectory,"./");


  /* write the output files */
  for(i=0; i < iNumComments; i++)
  {

    pCurr = &CommentArray[i];
    logit("e", "DEBUG:(%d, %s, %s, %s, %s, %d, iDiff=%d)\n", i,
            pCurr->Library.szLibraryName,
            pCurr->Library.szSubLibraryName,
            pCurr->Library.szLanguage,
            GetCommentName(pCurr),
            pCurr->iCommentType,
            iDiff
           );

    bDirWorkDone = FALSE;
    bGroupAlreadyChecked = FALSE;
    switch(iDiff)
    {
    case INT_MAX:
    case COMPARE_COMMENTS_DIFFERENT_LIBRARY:
      {
        chdir_ew(szBaseDir);
        sprintf(szCurrentOutputURLDirectory,"./");

        if(CommentArray[i].Library.szLibraryName[0])
        {
          CreateDir(CommentArray[i].Library.szLibraryName);
          chdir_ew(CommentArray[i].Library.szLibraryName);
          strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szLibraryName);
          strcat(szCurrentOutputURLDirectory,"/");
          bDirWorkDone = TRUE;
        }
      }
    case COMPARE_COMMENTS_DIFFERENT_SUBLIBRARY:
      {
        if(!bDirWorkDone)
        {
          /* change to base directory */
          chdir_ew(szBaseDir);
          sprintf(szCurrentOutputURLDirectory,"./");

          /* change to Lib directory if relevant */
          if(CommentArray[i].Library.szLibraryName[0])
          {
            chdir_ew(CommentArray[i].Library.szLibraryName);
            strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szLibraryName);
            strcat(szCurrentOutputURLDirectory,"/");
            /* create and change to SubLib directory if relevant */
            if(CommentArray[i].Library.szSubLibraryName[0])
            {
              CreateDir(CommentArray[i].Library.szSubLibraryName);
              chdir_ew(CommentArray[i].Library.szSubLibraryName);
              strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szSubLibraryName);
              strcat(szCurrentOutputURLDirectory,"/");
              bDirWorkDone = TRUE;
            }
          }
        }
        else
        {
          /* create and change to SubLib directory if relevant */
          if(CommentArray[i].Library.szSubLibraryName[0])
          {
            CreateDir(CommentArray[i].Library.szSubLibraryName);
            chdir_ew(CommentArray[i].Library.szSubLibraryName);
            strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szSubLibraryName);
            strcat(szCurrentOutputURLDirectory,"/");
          }
        }
      }
    case COMPARE_COMMENTS_DIFFERENT_LANGUAGE:
      {
        if(!bDirWorkDone)
        {
          /* change to base directory */
          chdir_ew(szBaseDir);
          sprintf(szCurrentOutputURLDirectory,"./");

          /* change to Lib directory if relevant */
          if(CommentArray[i].Library.szLibraryName[0])
          {
            chdir_ew(CommentArray[i].Library.szLibraryName);
            strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szLibraryName);
            strcat(szCurrentOutputURLDirectory,"/");

            /* create and change to SubLib directory if relevant */
            if(CommentArray[i].Library.szSubLibraryName[0])
            {
              chdir_ew(CommentArray[i].Library.szSubLibraryName);
              strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szSubLibraryName);
              strcat(szCurrentOutputURLDirectory,"/");
            }  /* end if valid sublib */
          }  /* end if valid lib */
        }  /* end if! dir work already done */
        if(CommentArray[i].Library.szLanguage[0])
        {
          CreateDir(CommentArray[i].Library.szLanguage);
          chdir_ew(CommentArray[i].Library.szLanguage);
          strcat(szCurrentOutputURLDirectory,CommentArray[i].Library.szLanguage);
          strcat(szCurrentOutputURLDirectory,"/");
        }

      }
    case COMPARE_COMMENTS_DIFFERENT_TYPE:
      {
        /* we must create a new file */
        if(fpOut)
        {
          /* write html trailer to old file*/
          
          /* close old file */
          fclose(fpOut);

        }

        /* open new file */
        logit("e","Current WD = %s\n",  getcwd(testdir, sizeof(testdir)));

        fpOut=fopen(FileTypeNames[CommentArray[i].iCommentType],"w+");
        sprintf(szCurrentOutputURLFileName,"%s%s", szCurrentOutputURLDirectory,
                FileTypeNames[CommentArray[i].iCommentType]);
                
        if(!fpOut)
        {
          logit("e","Could not create file %s\n",
                 FileTypeNames[CommentArray[i].iCommentType]);
          szCurrentOutputURLFileName[0] = 0x00;
        }

        /* write new header */
      }
    case COMPARE_COMMENTS_DIFFERENT_COMMENT_GROUP:
      {
         bSameGroup = FALSE;
         bGroupAlreadyChecked = TRUE;
      }
    case COMPARE_COMMENTS_DIFFERENT_COMMENT_NAME:
      {
        if(!bGroupAlreadyChecked)
          bSameGroup = TRUE;

        printf("Comment %3d: %s\n",i,GetCommentName(&CommentArray[i]));

        /* write comment */
        RetCode=WriteCommentToHTMLFile(fpOut, &CommentArray[i], 
                                       bSameGroup, CommentArray, iNumComments);

        /* record keyword w/hyperlink */
        strcpy(CommentArray[i].Keyword.szKeyword,GetCommentName(&CommentArray[i]));
        sprintf(CommentArray[i].Keyword.szHyperlink,"%s#%s",
                szCurrentOutputURLFileName, GetCommentName(&CommentArray[i]));
        break;
      }
    default:
      {
        logit("e","Duplicate copies of comment %s found\n",
                GetCommentName(&CommentArray[i]));
                
      }
    }  /* end switch(iDiff) */

    if(i < (iNumComments-1))
    {
      iDiff=CompareComments(&CommentArray[i+1],&CommentArray[i]);
    }
       
  } /* end for comments in array */

  /* close the output file */
  fclose(fpOut);


  RetCode = ParseFilesForKeywords(CommentArray, iNumComments,
                             szBaseDir);

  logit("e","ParseFilesForKeywords() returned %d\n",RetCode);


  sprintf(testdir, "%s/%s", szBaseDir, "index.html");
  fpOut = fopen(testdir, "w");
  if(!fpOut)
  {
    logit("e", "Could not open summary index file (%s)\n", testdir);
    return(-1);
  }

  fprintf(fpOut, "<html>\n<head>\n"
                 "<TITLE>Earthworm Modules and Libraries: Index</TITLE>\n"
                 "</head>\n<body>\n"
                 " <table>\n"
                 "   <tr>\n   <td width=40></td><td></td></tr>"
         );


  /* write the index output file */
  for(i=0; i < iNumComments; i++)
  {

    pCurr = &CommentArray[i];
    if(pCurr->iCommentType == COMMENT_TYPE_LIBRARY)
    {
      if(pCurr->Library.szSubLibraryName[0])
      {
        /* this is a sub-library */
        fprintf(fpOut, "<tr><td> </td><td><a href=%s/%s/%s>%s</a></td></tr>\n",
                pCurr->Library.szLibraryName, pCurr->Library.szSubLibraryName,
                FileTypeNames[COMMENT_TYPE_LIBRARY], 
                pCurr->Library.szSubLibraryName);
      }
      else
      {
        fprintf(fpOut, "<tr><td colspan=2><a href=%s/%s>%s</a></td></tr>\n",
                pCurr->Library.szLibraryName, 
                FileTypeNames[COMMENT_TYPE_LIBRARY], 
                pCurr->Library.szLibraryName);
      }
    }  /* end if comment is of type library */
  }  /* end for iNumComments */


  fprintf(fpOut, " </table>\n");
  fprintf(fpOut, "<hr>\n<a href=master_index.html>"
                 "All Comments Sorted in Alphabetic Order</a>\n"
                 "</body>\n"
                 "</html>\n"
         );


  fclose(fpOut);

  
  qsort(CommentArray, iNumComments, sizeof(CommentStruct), 
        CompareCommentsByKeyword);

  sprintf(testdir, "%s/%s", szBaseDir, "master_index.html");
  fpOut = fopen(testdir, "w");
  if(!fpOut)
  {
    logit("e", "Could not open master index file (%s)\n", testdir);
    return(-1);
  }

  fprintf(fpOut, "<html>\n<head>\n"
                 "<TITLE>Earthworm Modules and Libraries: Master Comment Index</TITLE>\n"
                 "</head>\n<body>\n"
         );


  /* write master_index.html file */
  for(i=0; i < iNumComments; i++)
  {

    pCurr = &CommentArray[i];
    fprintf(fpOut, "<a href=%s>%s</a>\n<br>\n",
            pCurr->Keyword.szHyperlink,
            pCurr->Keyword.szKeyword);
  }  /* end for iNumComments */


  fprintf(fpOut, "</body>\n"
                 "</html>\n"
         );


  fclose(fpOut);


              
  /* go home */
  return(0);
}  /* end main() */


