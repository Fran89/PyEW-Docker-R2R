// mutableserverrequest.cpp: implementation of the MutableServerRequest class.
//
//////////////////////////////////////////////////////////////////////

#include "mutableserverrequest.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MutableServerRequest::MutableServerRequest()
{

}
//
// --------------------------------------------------------------------
//
long MutableServerRequest::BufferInitAlloc()
{
   long r_size = MutableServerMessage::BufferInitAlloc();

   for ( int _p = 0, _sz = Passport.size() ; _p < _sz ; _p++ )
   {
      r_size += Passport[_p].length();
      r_size += 1;  // line-terminating '\n'
   }
   return r_size;
}
//
// --------------------------------------------------------------------
//
void MutableServerRequest::FormatDerivativeData()
{
   MutableServerMessage::FormatDerivativeData();

   for ( int _p = 0, _sz = Passport.size() ; _p < _sz ; _p++ )
   {
      MessageBuffer.append( MSR_PASSPORT_TAG );
      MessageBuffer.append( Passport[_p] );
      MessageBuffer.append( "\n" );
   }
   // flag the start of the data
   //
      MessageBuffer.append( MSR_DATASTART_LINE );
      MessageBuffer.append( "\n" );
}
//
// --------------------------------------------------------------------
//
void MutableServerRequest::ParseDerivativeData()
{
   MutableServerMessage::ParseDerivativeData();

   ClearPassport();

   bool _parsing = true;

   long _index;

   std::string _ppline; // passport line

   do
   {
      if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
      {
         throw worm_exception("Unterminated message while parsing passport");
      }

// DEBUG remove this?
      // - 1 to exclude '\n'

      _ppline = MessageBuffer.substr( 0 , _index );


      // Remove this line from the buffer
      // + 1 to include '\n'

      MessageBuffer.erase( 0 , _index + 1 );

      // check for start of data

      if ( _ppline.compare( MSR_DATASTART_LINE ) == 0 )
      {
         _parsing = false;
         continue;
      }

      // minimally validate the passport line format

      
      if ( _ppline.compare( 0, strlen(MSR_PASSPORT_TAG),  MSR_PASSPORT_TAG ) != 0 )
      {
         worm_exception _expt("Invalid passport line: >");
                        _expt += _ppline;
                        _expt += "<";
         throw _expt;
      }

      // remove the tag from the line

      _ppline.erase( 0 , strlen(MSR_PASSPORT_TAG) );

      // Add the line to the passport

      AddPassportLine( _ppline );

   } while( _parsing );
   
}
//
// --------------------------------------------------------------------
//
void MutableServerRequest::ClearPassport() { Passport.clear(); }
//
// --------------------------------------------------------------------
//
void MutableServerRequest::AddPassportLine( const char * p_line )
{
   if ( p_line != NULL )
   {
      std::string _pp = p_line;
      Passport.push_back(_pp);
   }
}
//
// --------------------------------------------------------------------
//
void MutableServerRequest::AddPassportLine( std::string p_line )
{
   Passport.push_back(p_line);
}
//
// --------------------------------------------------------------------
//
int MutableServerRequest::GetPassportLineCount() { return Passport.size(); }
//
// --------------------------------------------------------------------
//
const char * MutableServerRequest::GetPassportLine( int p_index )
{
   if ( 0 <= p_index && p_index < Passport.size() )
   {
      return Passport[p_index].c_str();
   }
   else
   {
      return (char *)NULL;
   }
}
//
// --------------------------------------------------------------------
//
