// configsource.cpp: implementation of the ConfigSource class.
//
//////////////////////////////////////////////////////////////////////

#include "configsource.h"

int ConfigSource::INVALID_INT    = INT_MIN;
long ConfigSource::INVALID_LONG  = LONG_MIN;
double ConfigSource::INVALID_DOUBLE = DBL_MIN;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ConfigSource::ConfigSource()
{
   ReadMode = CS_MODE_COMMAND;
   strcpy( Token, "" );
   strcpy( CurrentLine, "" );
   TokenIsNull    = true;
   LineParseIndex = 0;
   LastError      = 0;
   strcpy( LastMessage, "" );
}
//---------------------------------------------------------------------------
ConfigSource::~ConfigSource()
{
   Close();
}
//---------------------------------------------------------------------------
char* ConfigSource::NextToken()
{
   // Initialize token as empty string
   Token[0] = '\0';
   TokenIsNull = true;

	LastError = 0;
   strcpy( LastMessage, "" );

   bool _started  = false  // token start found, parsing token
      , _isquoted = false  // parsing a quoted token
      , _tokenend = false  // end of token found, leave parsing loop
      ;

   LineParseIndex++; // Advance off last location (whitespace) in CurrentLine


   int _tokenChIndex = 0 ;

   // Scan for beginning of token.
   // Note that first char in the buffer (CurrentLine) will
   // have been set to a space (token separator) which
   // is why we start with an increment of TokenParseIndex
   //
   int _linelen = strlen( CurrentLine );

	for( ; (! _tokenend) && LineParseIndex < _linelen ; LineParseIndex++ )
	{
		switch( CurrentLine[LineParseIndex] )
      {
        case ' ':
        case '\t':
        case ',':
             if ( _started )
             {
                if ( _isquoted )
                {
                   // space, tab or comma within quoted token, include the char
                   Token[_tokenChIndex++] = CurrentLine[LineParseIndex];
                   TokenIsNull = false;
                }
                else
                {
                   // space, tab or comma while parsing non-quoted token -- end of token
                   _tokenend = true;
                   // Step back line parse index to negate increment at loop termination
                   LineParseIndex--;
                }
             }
             // else  token not started, this is whitespace
             break;

        case '#':
             // comment started, assure no further parsing of this line
             LineParseIndex = _linelen;

        case '\n':
             if ( ! _started )
             {
                // end of line before token start found
                Token[0] = 0;  // token is empty string
                TokenIsNull = true;
             }
             // else  end of line after token started
             //       this is end of token (even if started with quote)

             _tokenend = true;

             break;

        case '"':
             if ( _isquoted )
             {
                // Already parsing a quoted token, this is the end thereof
                _tokenend = true;
             }
             else
             {
                // not yet parsing quoted token

                if ( _tokenChIndex == 0 )
                {
                   // First char in the token parsing,
                   // start a new token
                   _isquoted = true;
                   _started  = true;
//                   TokenIsNull = false;
                }
                // else  Not the first char in the token.
                //       In theory embed quote within token,
                //       but better to just skip it
             }
             break;

        default:
             // regular character, add to token (fld)
             Token[_tokenChIndex++] = CurrentLine[LineParseIndex];
             _started  = true;
             TokenIsNull = false;
		     break;
      }
   }


   Token[_tokenChIndex] = 0;  // null-terminate string

   return Token;

}
//---------------------------------------------------------------------------
// Get fixed position token, strips leading and trailing blanks
char* ConfigSource::GetToken(int n, int off) {
	unsigned int j;
	char ch;
	char fld[1024];
	int j1 = off;
	unsigned int j2 = off + n;
	if( j2 > strlen( CurrentLine ) )
   {
		j2 = strlen( CurrentLine );
	}
   for( j = j1 ; j < j2 ; j++ ) {
		ch = CurrentLine[j];
		if(ch == ' ')
			continue;
		break;
	}
	int nfld = 0;
	int jj = 0;
	for( ; j < j2 ; j++ ) {
		ch = CurrentLine[j];
		fld[jj++] = ch;
		if(ch == ' ')
			break;
		nfld = jj;
	}
	fld[nfld] = 0;
	strcpy( Token, fld );
	LineParseIndex = j2 - 1;
	return Token;
}
//---------------------------------------------------------------------------
char* ConfigSource::GetToken(int n) {
	return GetToken( n, LineParseIndex + 1 );
}
//---------------------------------------------------------------------------
// Load : Load string into command buffer, initialize for parse.
int ConfigSource::Load( const char * p_cmd)
{
   if ( p_cmd == NULL || MAX_LINE_LENGTH < strlen(p_cmd) )
   {
      return 0;
   }
	LastError = 0;
   strcpy( LastMessage, "" );
	strcpy( CurrentLine, p_cmd );
	LineParseIndex = -1;
	ReadMode = CS_MODE_COMMAND;
	return strlen( CurrentLine );
}
//---------------------------------------------------------------------------
int ConfigSource::Error( char** r_textual )
{
	int k = LastError;
   if ( *r_textual != NULL )
   {
      *r_textual = LastMessage;
   }
	LastError = 0;
	return k;
}
//---------------------------------------------------------------------------
char* ConfigSource::String()
{
	return ConfigSource::NextToken();
}
//---------------------------------------------------------------------------
int ConfigSource::Int()
{
   NextToken();
   if( 0 < strlen( Token ) )
   {
	  return atoi(Token);
   }
   return INVALID_INT;
}
//---------------------------------------------------------------------------
int ConfigSource::Int(int n) {
   return Int( n, LineParseIndex + 1 );
}
//---------------------------------------------------------------------------
int ConfigSource::Int(int n, int off) {
   GetToken(n, off);
   if( 0 < strlen( Token ) )
   {
      return atoi(Token);
   }
   return INVALID_INT;
}
//---------------------------------------------------------------------------
long ConfigSource::Long()
{
	NextToken();
	if( 0 < strlen( Token ) )
   {
		return atol(Token);
   }
	return INVALID_LONG;
}
//---------------------------------------------------------------------------
long ConfigSource::Long(int n) {
	return Long( n, LineParseIndex + 1 );
}
//---------------------------------------------------------------------------
long ConfigSource::Long(int n, int off) {
	GetToken(n, off);
	if( 0 < strlen( Token ) )
   {
      return atol(Token);
   }
	return INVALID_INT;
}
//---------------------------------------------------------------------------
double ConfigSource::Double()
{
	NextToken();
	if( 0 < strlen( Token ) )
   {
		return atof(Token);
	}
   return INVALID_DOUBLE;
}
//---------------------------------------------------------------------------
double ConfigSource::Double(int n) {
	return Double( n, LineParseIndex + 1 );
}
//---------------------------------------------------------------------------
double ConfigSource::Double(int n, int off) {
	GetToken(n, off);
	if( 0 < strlen( Token ) )
   {
	   return atof(Token);
   }
	return INVALID_DOUBLE;
}
//---------------------------------------------------------------------------
bool ConfigSource::Its(const char* p_str)
{
	return ( strcmp( Token, p_str ) == 0 );
}
//---------------------------------------------------------------------------

