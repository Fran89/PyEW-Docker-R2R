// mutableserverresult.cpp: implementation of the MutableServerResult class.
//
//////////////////////////////////////////////////////////////////////

#include "mutableserverresult.h"

#include "logger.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MutableServerResult::MutableServerResult()
{
   Status = MSB_RESULT_ERROR;
}
//
// -----------------------------------------------------
//
long MutableServerResult::BufferInitAlloc()
{
   return (long)(  MutableServerMessage::BufferInitAlloc()
                 + 3  // longest result = "-1\n"
                );
}
//
// -----------------------------------------------------
//
void MutableServerResult::FormatDerivativeData()
{
   MutableServerMessage::FormatDerivativeData();

   char _b[5];

   sprintf( _b, "%d\n", Status );

   // append the result status

   MessageBuffer.append( _b );
}
//
// -----------------------------------------------------
//
void MutableServerResult::ParseDerivativeData()
{
   Status = MSB_RESULT_ERROR;

   MutableServerMessage::ParseDerivativeData();

   long _index;

   if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
   {
      throw worm_exception("Unterminated message while parsing result code");
   }


   std::string _status = MessageBuffer.substr( 0 , _index );

   // Remove this line from the buffer

   MessageBuffer.erase( 0 , _index + 1 );

   Status = (short)atoi( _status.c_str() );
}
//
// -----------------------------------------------------
//
void MutableServerResult::SetStatus( short p_status )
{
   if (   p_status != MSB_RESULT_GOOD
       && p_status != MSB_RESULT_BUSY
       && p_status != MSB_RESULT_FAIL
       && p_status != MSB_RESULT_ERROR
      )
   {
      throw worm_exception("MutableServerResult::SetStatus(): invalid parameter");
   }
   Status = p_status;
}
//
// -----------------------------------------------------
//
short MutableServerResult::GetStatus() { return Status; }
//
// -----------------------------------------------------
//
