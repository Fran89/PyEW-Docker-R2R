/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2004/03/17 20:37:40  davidk
 *     extended max comment length from 1023 to 2047.
 *
 *     Revision 1.2  2001/07/20 16:36:10  davidk
 *     reformatted function prototypes
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <earthworm_simple_funcs.h> /* logit(), dirops_ew.c funcs */
#include <limits.h>
#ifdef _SOLARIS
# include <errno.h>
#endif  /* _SOLARIS */



/*************************************************/
/*************************************************/
/*         #DEFINE CONSTANTS                     */
/*************************************************/
/*************************************************/

/* Maximum length of a comment */
#define MAX_COMMENT_LEN 2047

/* TRUE/FALSE constants */
#define TRUE 1
#define FALSE 0

/* Comment String constants */
#define COMMENT_STRING_LINE1 "/************************************************"
#define COMMENT_STRING_LINE2 "************ SPECIAL FORMATTED COMMENT **********"
#define COMMENT_STRING_END2  "*************************************************"
#define COMMENT_STRING_END   "************************************************/"

/* Parse delimiter constants for use with strtok() */
#define PARSE_DELIMITERS ", \t"

/* Maximum number of files we will ever deal with,
   for use as a constant for size of filename array */
#define MAX_FILES 1500

/* Types of files we deal with */
#define NO_FILE_TYPE     0
#define OUTPUT_FILE_TYPE 1  /* Output file of type comment */
#define INPUT_FILE_TYPE  2  /* Input file of type comment */
#define LIST_FILE_TYPE   3  /* File containing list of input source files */


#define RETURN_GETNEXTLINE_EOF 9  /* good random arbitrary number */


/* Types of comments */
#define COMMENT_TYPE_UNDEFINED 0
#define COMMENT_TYPE_LIBRARY   1
#define COMMENT_TYPE_CONSTANT  2
#define COMMENT_TYPE_TYPEDEF   3
#define COMMENT_TYPE_FUNCTION  4

/* States for use in parsing of commments */
#define COMMENT_PARSE__NOT_IN_COMMENT      0
#define COMMENT_PARSE__AFTER_FIRST_LINE    1
#define COMMENT_PARSE__IN_UNKNOWN_COMMENT  2
/* there is no #3 */
#define COMMENT_PARSE__IN_DEFINE_COMMENT   4
#define COMMENT_PARSE__IN_TYPEDEF_COMMENT  5
#define COMMENT_PARSE__IN_FUNCTION_COMMENT 6
#define COMMENT_PARSE__IN_LIBRARY_COMMENT  7

/* Levels of comment differentiation */
#define COMPARE_COMMENTS_DIFFERENT_LIBRARY        0x20
#define COMPARE_COMMENTS_DIFFERENT_SUBLIBRARY     0x10
#define COMPARE_COMMENTS_DIFFERENT_LANGUAGE       0x08
#define COMPARE_COMMENTS_DIFFERENT_TYPE           0x04
#define COMPARE_COMMENTS_DIFFERENT_COMMENT_GROUP  0x02
#define COMPARE_COMMENTS_DIFFERENT_COMMENT_NAME   0x01


/* Function return code constants */
#define RETURN_SUCCESS  0
#define RETURN_WARNING  1
#define RETURN_ERROR   -1


/* FUNCTION MATURITY LEVELS */
#define FUNCTION_MATURITY_NEW        0
#define FUNCTION_MATURITY_CHANGED    1
#define FUNCTION_MATURITY_MATURE     4
#define FUNCTION_MATURITY_DEPRICATED 8

/* Tempfile Name */
#define TEMPFILE_NAME "temp_keywordfile.html"


/*************************************************/
/*************************************************/
/*          STRUCTURES AND TYPEDEFS              */
/*************************************************/
/*************************************************/


/* Struct containing information about which library
   and sublibrary that a comment belongs to */
typedef struct _LibraryInfoStruct
{
  char szLibraryName[255];
  char szSubLibraryName[255];
  char szLanguage[10];
  char szLocation[255];
} LibraryInfoStruct;


/* A Library type comment */
typedef struct _LibraryComment
{
  char szLibraryDescription[MAX_COMMENT_LEN];
  int    iNumLinks;
  char * szLinkNames[40];
  char * szLinks[40];
} LibraryComment;

/* A #define Constant type comment */
typedef struct _DefineConstantComment
{
  char szConstantName[255];
  char szConstantGroup[255];
  char szConstantValue[255];
  char szConstantDescription[1024];
  char szNote[MAX_COMMENT_LEN];
} DefineConstantComment;

/* A structure containing information about "struct" members
   and function parameters,
   for use with the Typedef and Function type comments */
typedef struct _CommentMember
{
  char szMemberName[255];
  char szMemberType[255];
  char szMemberDescription[1024];
} CommentMember;

/* A Typedef type comment */
typedef struct _TypedefComment
{
  char          szTypedefName[255];
  char          szTypedefDefinition[255];
  char          szTypedefDescription[1024];
  int           iNumMembers;
  CommentMember pcmMembers[50];
  char          szNote[MAX_COMMENT_LEN];
} TypedefComment;

/* A Function or Procedure type comment */
typedef struct _FunctionComment
{
  char          szFunctionName[255];
  char          szFunctionDescription[MAX_COMMENT_LEN];
  char          szReturnType[255];
  int           iNumReturnValues;
  CommentMember pcmReturnValues[50];
  int           iNumParams;
  CommentMember pcmFunctionParams[20];
  char          szNote[MAX_COMMENT_LEN];
  int           bIsProcedure;
  int           iReturnParam;
  int           iStability;
  char          szSourceLocation[255];
} FunctionComment;


/* CommentUnion */
typedef union _CommentUnion
{
  FunctionComment        fc;
  TypedefComment         td;
  DefineConstantComment  dcc;
  LibraryComment         lc;
} CommentUnion;

/* Keyword struct */
typedef struct _KeywordStruct
{
  char szKeyword[255];
  char szHyperlink[255];
} KeywordStruct;

/* Generic comment struct, to be
   used as element in global comment array.
   Contains common attributes of all comments */
typedef struct _CommentStruct
{
  void * pComment;
  int    iCommentType;
  LibraryInfoStruct Library;
  KeywordStruct Keyword;
} CommentStruct;



/*************************************************/
/*************************************************/
/*             FUNCTION PROTOTYPES               */
/*************************************************/
/*************************************************/


/* write_html_comments.c */
int GetBaseURLForComment(char * szBaseURL, 
                         unsigned int iBufferSize,
                         CommentStruct * pComment);

int AddKeywordHyperlinks(FILE * fpIn, FILE * fpTemp, 
                         CommentStruct * CommentArray,
                         int iNumComments,
                         CommentStruct * pBaseComment);

int CompareCommentsSearchingForKeywords(const void* p1, const void* p2);


/* api_doc_util.c */

int GetNextLine(char * pBuffer, int BufferSize, FILE * fptr);

int ReadCommentsFile(FILE * fpIn, CommentStruct ** pCommentsArray, 
                     int * piNumComments, int * piNumAllocComments);


int GetNextCommentFromSourceFile(FILE * fpIn, CommentStruct * pTemp_Comment);

/* not supported: GetNextCommentFromCommentFile() */
int GetNextCommentFromCommentFile(FILE * fpIn, CommentStruct * pTemp_Comment);

int ReplaceBuffer(void ** ppBuffer, int OldBufferSize, int NewBufferSize, 
                  int bCopyBuffer);

int strcmp1(const char *string1, const char *string2);

int CompareComments(const void* p1, const void* p2);

int ReplaceComment(CommentStruct * pDest, CommentStruct * pSource);

int GetCommentSize(int iCommentType);

int AllocateSpaceForAndCopyComment(CommentStruct * pCurrent, void ** ppComment);

int AddCommentToList(CommentStruct ** pCommentsArray, CommentStruct * pComment,
                     int * piNumComments, int * piNumAllocComments);

int ParseDefineComment(DefineConstantComment * pdccComment, 
                       LibraryInfoStruct * pLibrary);

int ParseTypedefComment(TypedefComment * ptdComment, 
                        LibraryInfoStruct * pLibrary);

int ParseLibraryComment(LibraryComment * plcComment, 
                        LibraryInfoStruct * pLibrary);

int ParseFunctionComment(FunctionComment * pfcComment, 
                         LibraryInfoStruct * pLibrary);

int ProcessGarbage(void);

int ParseListFile(char * szListFileName, int * piNumSourceFiles,
                  char * FileNameArray[]);

int ReadSourceFile(FILE * fpIn, CommentStruct ** pCommentsArray, 
                   int * piNumComments, int * piNumAllocComments,
                   char * szFileName);

int GetDescription(char * szBuffer, int iBufferSize, char * pTemp);

char * GetCommentName(CommentStruct * pComment);

char * TrimString(char * szString);

int CompareCommentsByKeyword(const void* p1, const void* p2);


/* parse_files_for_keywords.c */
int GetCommentFileName(char * szCurrentFilename, unsigned int iBufferSize,
                       char * szBaseDir, CommentStruct * pComment);

int ParseFilesForKeywords(CommentStruct * CommentArray, int iNumComments,
                          char * szBaseDir);


/* write_html_file.c */
int WriteCommentToHTMLFile(FILE * fpOut, CommentStruct * pComment, 
                           int bSameGroup, CommentStruct * CommentArray,
                           int iNumComments);

int WriteLibraryInfoToHTMLFile(FILE * fpOut, LibraryInfoStruct * pLibrary);

int WriteConstantToHTMLFile(FILE * fpOut, DefineConstantComment * pdccComment);

int WriteTypedefToHTMLFile(FILE * fpOut, TypedefComment * ptdComment);

int WriteFunctionToHTMLFile(FILE * fpOut, FunctionComment * pfcComment);

int WriteLibraryToHTMLFile(FILE * fpOut, LibraryComment * plcComment,
                           LibraryInfoStruct * pLibrary,
                           CommentStruct * CommentArray, int iNumComments);

/*************************************************/
/*************************************************/
/*                    EXTERNS                    */
/*************************************************/
/*************************************************/


extern int linectr;
extern FILE * fpIn;
extern const char FileTypeNames[][30];

