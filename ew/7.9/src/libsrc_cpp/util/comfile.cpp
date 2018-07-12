//---------------------------------------------------------------------------

#include "comfile.h"


//---------------------------------------------------------------------------
TComFileParser::TComFileParser()
{
   ReadMode       = CS_MODE_FILE;
   OpenFileCount  = 0;
   for ( int _f = 0 ; _f < MAX_COM_FILE ; _f++ )
   {
      files[_f] = NULL;
   }
}
//---------------------------------------------------------------------------
TComFileParser::~TComFileParser()
{
}
//---------------------------------------------------------------------------
bool TComFileParser::Open(const char* p_filename)  // open first file
{
   Close();
   LastError = 0;
   strcpy( LastMessage, "" );
   eof = false;
   if( (files[OpenFileCount] = fopen( p_filename, "r")) == NULL )
   {
      sprintf( LastMessage, "File <%s> not found.\n", p_filename );
      return false;
   }
   ReadMode = CS_MODE_FILE;
   OpenFileCount++;  // now 1
   return true;
}
//---------------------------------------------------------------------------
bool TComFileParser::DesignateArchive(FILE* p_file)
{
   if ( p_file == NULL )
   {
      return false;
   }
	ReadMode = CS_MODE_ARCHIVE;
	eof = false;
	Archive = p_file;
	return true;
}
//---------------------------------------------------------------------------
// close all opened files
void TComFileParser::Close()
{
   while( 0 < OpenFileCount )
   {
      OpenFileCount--;
      if ( files[OpenFileCount] != NULL )
      {
		 fclose( files[OpenFileCount] );
         files[OpenFileCount] = NULL;
      }
   }
}
//---------------------------------------------------------------------------
//
// RETURNS        0 - n  = length of line read
//         COMFILE_EOF   = eof
//         COMFILE_ERROR = error
//
int TComFileParser::ReadLine()
{
   int r_value = COMFILE_EOF;

   bool _readyToReturn = false;

	char  buf[MAX_LINE_LENGTH+1];   // read buffer

	LastError = 0;
   strcpy( LastMessage, "" );

	if( eof )
   {
      // Already finished parsing all comfiles, return EOF
		return COMFILE_EOF;
	}


   switch( ReadMode )
   {
     case CS_MODE_FILE:   // file:

          while( ! _readyToReturn )
          {
             // Get string from current file
	          if( (fgets(buf, sizeof(buf) - 1, files[OpenFileCount-1])) != NULL )
	          {
                // able to read line from file

		          LineParseIndex = -1; // i = -1; start of line, no tokens yet

                switch( buf[0] )
                {
                  case '@':
                       if ( OpenFileCount == MAX_COM_FILE )
                       {
                          // failed opening new file
                          sprintf( LastMessage, "Attempt to open more than %d command files.", MAX_COM_FILE );
                          r_value = COMFILE_ERROR;
                          _readyToReturn = true;
                       }
                       else
                       {
                          // Found indicator of new file to open
			              buf[0] = ' ';       // replace file name indicator with token spacer
                       strcpy( CurrentLine, buf);  // make line with filename available to tokenizer code
			              char* filename = NextToken(); // get filename from current line

                          // open new file
	                      if( (files[OpenFileCount] = fopen(filename, "r")) != NULL )
                          {
				             OpenFileCount++;
                             // loop again to get next [first] line from [newly-opened] file for parsing
                          }
                          else
                          {
                             // failed opening new file
				                 sprintf( LastMessage, "File <%s> not found.\n", filename );
                             r_value = COMFILE_ERROR;
                             _readyToReturn = true;
                          }
                       }
                       break;

                  default:
                       // remove terminating newline
                       if ( buf[strlen(buf)-1] == '\n' )
                       {
                          buf[strlen(buf)-1] = 0;  // replace end-of-line with end-of-string
                       }

                       // check for empty spaces or comment start
                       for ( int _c = 0, _sz = strlen(buf) ; _c < _sz ; _c++ )
                       {
                          switch( buf[_c] )
                          {
                            case ' ':
                            case '\t':
                                 break;

                            case '\n':
                            case '#':
                                 // empty line or comment, go to next line
                                 _c = _sz;
                                 break;

                            default:
                                 // not an indication of new file, return info about current line
                                 strcpy( CurrentLine, buf );
                                 r_value = strlen( CurrentLine );
                                 _readyToReturn = true;
                                 LineParseIndex = _c - 1;
                                 _c = _sz;
                                 break;
                          }
                       }

                }
             }
             else
             {
                // nothing read from current file, must be end of current file
                fclose( files[--OpenFileCount] );
                files[OpenFileCount] = NULL;

	             if( OpenFileCount == 0 )
                {
                   // no files remain open, return indication of end-of-file
                   strcpy( Token , "eof" );
                   eof = true;
                   r_value = COMFILE_EOF;
                   _readyToReturn = true;
                }
                // else a file remains open, get the next line from that file
             }
          } // while( ! _notReadyToReturn )
          break;


     case CS_MODE_ARCHIVE:  // This mode reads archive files char-by-char into CurrentLine
          {
	          char  chr;        // read char
	          int   charsread = 0;   // chars read from archive file

             while( ! _readyToReturn )
             {
                switch( (chr = (char)fgetc( Archive )) )
                {
                  case EOF:
                       // could also be an error
                       strcpy( Token , "eof" );
                       eof = true;
                       if ( charsread == 0 )
                       {
                          // encountered EOF with no line data in buffer
                          r_value = COMFILE_EOF;
                          _readyToReturn = true;
                          break;
                       }
                       // encountered EOF without preceeding end-of-line NULL termination
                       // unlikely, but....
                       // fall through

                  case '\n':
                       LineParseIndex = -1; // i = -1;
                       buf[charsread] = 0;         // null-terminate
                       strcpy( CurrentLine, buf);
                       r_value = strlen( CurrentLine );
                       _readyToReturn = true;
                       break;

                  case '\r':
                       // drop this char, read the next one
                       //goto archive;
                       break;

                  default:
                       // append this char
                       buf[charsread++] = chr;
                       // continue loop to get next one
                       break;
                }
             } // while( ! _readyToReturn )

          }
          break;
   }

   return r_value;
}
