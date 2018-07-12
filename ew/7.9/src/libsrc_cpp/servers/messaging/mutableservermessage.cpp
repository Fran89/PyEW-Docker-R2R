// mutableserverresult.cpp: implementation of the MutableServerResult class.
//
//////////////////////////////////////////////////////////////////////

#include "mutableservermessage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MutableServerMessage::MutableServerMessage()
{
}
//
// --------------------------------------------------------------------
//
void MutableServerMessage::FormatBuffer()
{
   MessageBuffer.resize(0);
   MessageBuffer.reserve( BufferInitAlloc() );
   FormatDerivativeData();
   MessageBuffer.append( "\n" );
}
//
//-------------------------------------------------------------------
//
long MutableServerMessage::GetBufferLength() const
{
   return MessageBuffer.size();
}
//
//-------------------------------------------------------------------
//
const char * MutableServerMessage::GetBuffer() const
{
   return MessageBuffer.c_str();
}
//
//-------------------------------------------------------------------
//
bool MutableServerMessage::ParseMessageLine( const char * p_buffer
                                           ,       bool   p_append_nl   /* = true  */
                                           ,       bool   p_clearbuffer /* = false */
                                           )
{
   if ( p_buffer == NULL )
   {
      throw worm_exception("MutableServerMessage::ParseMessageLine() Error: buffer parameter NULL");
   }

   if ( p_clearbuffer )
   {
      MessageBuffer.reserve( BufferInitAlloc() );
      MessageBuffer = "";
   }

   if ( strlen(p_buffer) == 0 )
   {
      if ( p_append_nl )
      {
         MessageBuffer.append( "\n" );
      }
      ParseDerivativeData();
      return true;
   }
   else
   {
      MessageBuffer.append( p_buffer );
      if ( p_append_nl )
      {
         MessageBuffer.append( "\n" );
      }
      return false;
   }
}
//
//-------------------------------------------------------------------
//
void MutableServerMessage::ParseFromBuffer( const char * p_buffer )
{
   if ( p_buffer == NULL )
   {
      throw worm_exception("MutableServerMessage::ParseFromBuffer() Error: buffer parameter NULL");
   }

   MessageBuffer = p_buffer;
   MessageBuffer.append( "\n" );
   ParseDerivativeData();
}
//
// --------------------------------------------------------------------
//
