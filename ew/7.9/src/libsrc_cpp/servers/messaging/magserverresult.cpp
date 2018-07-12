// magserverresult.cpp: implementation of the MagServerResult class.
//
//////////////////////////////////////////////////////////////////////

#include "magserverresult.h"

//  0123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789
//            1         2         3         4         5         6         7         8         9         0         1         2         3         4         5         6
//
//  EEEEEEE OOOOOOO T M.MMMMM EE.EEEE AAAAAAAAA\n
//
//  IDIDIDIDID  CHCHCHCHCH CPCPCPCP SSSS CCC NNN LLLL la.ttttt lon.nnnnn elev.vvv azm.mm dip.ppp T AA.AAAA amp1.DDDD AMP1TIMEEEE AMP1PERIOD amp2.DDDD AMP2TIMEEEE AMP2PERIOD
// 
#define MSR_HEADER_LINE_SZ_EST  50
#define MSR_CHAN_LINE_SZ_EST   180

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MagServerResult::MagServerResult()
{
   EventId    = 0L;
   OriginId   = 0L;
   MagType    = MAGTYPE_UNDEFINED;
   MagAverage = 0.0;
   MagError   = 0.0;

   Author     = "000000000";
}
//
//-------------------------------------------------------------------
//
void MagServerResult::SetMagnitudeInfo(       long    p_eventid
                                      ,       long    p_originid
                                      ,       int     p_magtype
                                      ,       float   p_average
                                      ,       float   p_error
                                      , const char  * p_author
                                      )
{
   EventId    = p_eventid;
   OriginId   = p_originid;
   MagType    = p_magtype;
   MagAverage = p_average;
   MagError   = p_error;

   Author     = ( p_author == NULL ? "000000000" : p_author );
}
//
//-------------------------------------------------------------------
//
void MagServerResult::ClearChannels()
{
   Channels.clear();
}
//
//-------------------------------------------------------------------
//
void MagServerResult::AddChannel( MAG_SRVR_CHANNEL p_channelinfo )
{
   Channels.push_back( p_channelinfo );
}
//
//-------------------------------------------------------------------
//
const long MagServerResult::GetEventId() const { return EventId; }
//
//-------------------------------------------------------------------
//
const long MagServerResult::GetOriginId() const { return OriginId; }
//
//-------------------------------------------------------------------
//
const int MagServerResult::GetMagType() const { return MagType; }
//
//-------------------------------------------------------------------
//
const float MagServerResult::GetAverage() const { return MagAverage; }
//
//-------------------------------------------------------------------
//
const float MagServerResult::GetError() const { return MagError; }
//
//-------------------------------------------------------------------
//
const char * MagServerResult::GetAuthor() const { return Author.c_str(); }
//
//-------------------------------------------------------------------
//
const int MagServerResult::GetChannelCount() { return Channels.size(); }
//
//-------------------------------------------------------------------
//
bool MagServerResult::GetChannel( unsigned int       p_index
                                , MAG_SRVR_CHANNEL * r_channel
                                ) const
{
   if (   Channels.size() <= p_index
       || r_channel == NULL
      )
   {
      return false;
   }

   *r_channel = Channels[p_index];

   return true;
}
//
//-------------------------------------------------------------------
//
long MagServerResult::BufferInitAlloc()
{
   // add estimated sizes for the magnitude and
   // and channel lines.
   return MutableServerResult::BufferInitAlloc()
        + MSR_HEADER_LINE_SZ_EST
        + Channels.size() * MSR_CHAN_LINE_SZ_EST
        ;
}
//
//-------------------------------------------------------------------
//
void MagServerResult::FormatDerivativeData()
{
   MutableServerResult::FormatDerivativeData();

   char _wrk[240];

   // Build the header line
   //
   // EEEEEEE OOOOOOO T M.MMMMM EE.EEEE AAAAAAAAA\n
   //
   sprintf( _wrk
          , "%d %d %d %f %f %f %s\n"
          , EventId
          , OriginId
          , MagType
          , MagAverage
          , MagError
          , Author.c_str()
          );

   MessageBuffer += _wrk;

   for ( int _c = 0 ; _c < Channels.size() ; _c++ )
   {
      // build a channel line
      //
      sprintf( _wrk
             , "%d %d %d %s %s %s %s %f %f %f %f %f %d %f %f %f %f %f %f\n"
             , Channels[_c].ampid
             , Channels[_c].channelid
             , Channels[_c].componentid
             , Channels[_c].sta
             , Channels[_c].comp
             , Channels[_c].net
             , Channels[_c].loc
             , Channels[_c].lat
             , Channels[_c].lon
             , Channels[_c].elev
             , Channels[_c].azm
             , Channels[_c].dip
             , Channels[_c].magnitude
             , Channels[_c].amp1
             , Channels[_c].amp1time
             , Channels[_c].amp1period
             , Channels[_c].amp2
             , Channels[_c].amp2time
             , Channels[_c].amp2period
             );

      MessageBuffer += _wrk;
   }

}
//
//-------------------------------------------------------------------
//
void MagServerResult::ParseDerivativeData()
{
   MutableServerResult::ParseDerivativeData();

   long _index;

   if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
   {
      throw worm_exception("Unterminated message while parsing MagServerResult header");
   }

   if ( _index < 2 ) 
   {
      throw worm_exception("Incomplete header line while parsing MagServerResult");
   }

   std::string _readline = MessageBuffer.substr( 0 , _index );

   // Remove this line from the buffer

   MessageBuffer.erase( 0 , _index + 1 );

   // parse the header line

   char _author[60];

   if ( sscanf( _readline.c_str()
              , "%ld %ld %d %f %f %f %s\n"
              , &EventId
              , &OriginId
              , &MagType
              , &MagAverage
              , &MagError
              ,  _author
              ) != 6 )
   {
      throw worm_exception("Invalid header line while parsing MagServerResult");
   }


   // parse the channel lines

   MAG_SRVR_CHANNEL _channel;

   do
   {
      if ( (_index = MessageBuffer.find("\n")) == MessageBuffer.npos )
      {
         throw worm_exception("Unterminated message while parsing MagServerResult channel");
      }

      if ( 0 < _index ) 
      {
         _readline = MessageBuffer.substr( 0 , _index );

         // Remove this line from the buffer

         MessageBuffer.erase( 0 , _index + 1 );

         // parse the channel line

         if ( sscanf( _readline.c_str()
                    , "%ld %ld %s %s %s %s %f %f %f %f %f %d %f %f %lf %f %f %lf %f\n"
                    , &_channel.ampid
                    , &_channel.channelid
                    , &_channel.componentid
                    ,  _channel.sta
                    ,  _channel.comp
                    ,  _channel.net
                    ,  _channel.loc
                    , &_channel.lat
                    , &_channel.lon
                    , &_channel.elev
                    , &_channel.azm
                    , &_channel.dip
                    , &_channel.magnitude
                    , &_channel.amp1
                    , &_channel.amp1time
                    , &_channel.amp1period
                    , &_channel.amp2
                    , &_channel.amp2time
                    , &_channel.amp2period
                    ) != 19 )
         {
            throw worm_exception("Invalid channel line while parsing MagServerResult");
         }

      } //  have a line with content 

   } while ( 0 < _index );

}
//
//-------------------------------------------------------------------
//
