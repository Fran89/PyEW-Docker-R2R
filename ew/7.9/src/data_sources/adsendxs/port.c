
       /*******************************************************
        *                        port.c                       *
        *                                                     *
        *  Get Trimble GPS data through a serial com port.    *
        *                                                     *
        *  This is Windows-specific code!                     *
        *******************************************************/


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <earthworm.h>
#include <time_ew.h>
#include "adsendxs.h"

static HANDLE handle = 0;
void LogLastSystemError( void );


/*****************************************************
  OpenPort

  Initialize the PC com port.
 *****************************************************/

int OpenPort( int ComPort, int BaudRate, int ReadFileTimeout )
{
   COMMTIMEOUTS timeouts = {0};
   DCB          dcb;                    // Data control block
   char         portName[PORTNAMESIZE]; // Com port name

/* Sanity checks
   *************/
   if ( ComPort < 1 || ComPort > 18 )
   {
      logit( "et", "Invalid value of ComPort (%d).\n", ComPort );
      return -1;
   }

   switch ( BaudRate )
   {
      case   1200: break;
      case   2400: break;
      case   4800: break;
      case   9600: break;
      case  14400: break;
      case  19200: break;
      case  38400: break;
      case  57600: break;
      case 115200: break;
      case 128000: break;
      case 256000: break;
      default:
         logit( "et", "Invalid baud rate: %d\n", BaudRate );
         return -1;
   }


/* Open the com port for asynchronous input/output.
   Share mode is zero, which allows exclusive access only.
   ******************************************************/
   sprintf_s( portName, PORTNAMESIZE, "COM%d", ComPort );
   handle = CreateFile( portName,
                        GENERIC_READ|GENERIC_WRITE,     // Desired access
                        0,                              // Share mode
                        NULL,                           // Default security attributes
                        OPEN_EXISTING,                  // Create parameter
                        FILE_ATTRIBUTE_NORMAL,          // Flags and attributes
                        NULL );                         // Template file

   if ( handle == INVALID_HANDLE_VALUE )
   {
      LogLastSystemError();
      logit( "et", "Error opening port %s\n", portName );
      return -1;
   }

/* Set PC serial port parameters to the default values
   for a Trimble Thunderbolt E GPS receiver.
   ***************************************************/
   if ( GetCommState(handle, &dcb) == 0 )
   {
      logit( "et", "Error getting state of comm port.\n" );
      LogLastSystemError();
      return -1;
   }

   dcb.BaudRate        = (DWORD)BaudRate;
   dcb.ByteSize        = 8;            // Hardwired
   dcb.Parity          = NOPARITY;     // Hardwired
   dcb.StopBits        = ONESTOPBIT;   // Hardwired
   dcb.fOutxCtsFlow    = FALSE;        // Disable RTS/CTS flow control
   dcb.fOutxDsrFlow    = FALSE;        // Disable DTR/DSR flow control
   dcb.fDsrSensitivity = FALSE;        // Driver ignores DSR
   dcb.fOutX           = FALSE;        // Disable XON/XOFF out flow control
   dcb.fInX            = FALSE;        // Disable XON/XOFF in flow control
   dcb.fNull           = FALSE;        // Don't discard null bytes
   dcb.fAbortOnError   = FALSE;        // Don't abort rd/wr on error

   if ( SetCommState(handle, &dcb) == 0 )
   {
      LogLastSystemError();
      logit( "et", "Error setting comm port state.\n" );
      return -1;
   }

/* Set read and write timeouts on the com port.
   If there any bytes in the input buffer, ReadFile returns
      immediately with the bytes in the buffer.
   If there are no bytes in the input buffer, ReadFile waits
      until a byte arrives and then returns immediately.
   If no bytes arrive within ReadTotalTimeoutConstant
      milliseconds, ReadFile times out.
   Timeouts are not used for write operations.
   *********************************************************/
   timeouts.ReadIntervalTimeout         = MAXDWORD;
   timeouts.ReadTotalTimeoutMultiplier  = MAXDWORD;
   timeouts.ReadTotalTimeoutConstant    = ReadFileTimeout;   // Milliseconds
   timeouts.WriteTotalTimeoutMultiplier = 0;
   timeouts.WriteTotalTimeoutConstant   = 0;

   if ( SetCommTimeouts( handle, &timeouts ) == 0 )
   {
      LogLastSystemError();
      logit( "et", "Error setting serial port timeouts.\n" );
      return -1;
   }
   return 0;
}


/******************************************************
  PurgeComInput

  Purge the input com port buffer of any leftover
  characters from the last read.
 ******************************************************/
int PurgeComInput( void )
{
   if ( PurgeComm(handle, PURGE_RXCLEAR) == 0 )
   {
      LogLastSystemError();
      logit( "et", "Error purging input serial port buffer.\n" );
      return -1;
   }
   return 0;
}


/********************************************************
  ReadGpsPacket

  Read one packet from the GPS receiver via com port.

  Returns number of bytes read (could be 0).
  Returns -1 if an error occurred.

 The com port is left open on return.
 ********************************************************/

int ReadGpsPacket( UINT8 packet[] )
{
   UINT8 chr;
   int   npacketchar = 0;
   int   nDLE = 0;
   int   first = 1;

/* Read a packet from the GPS receiver, one byte at a time.
   The TSIP packet format is:
   <DLE><id><data string bytes><DLE><ETX>
   *******************************************************/
   while ( 1 )
   {
      DWORD numRead = 0;
      UINT8 buf[2] = {0};

      if ( ReadFile( handle, buf, 1, &numRead, NULL ) == FALSE )
      {
         LogLastSystemError();
         logit( "et", "ReadFile error in ReadGpsPacket.\n" );
         return -1;
      }

/* Did we get one byte from the GPS, as requested?
   **********************************************/
      if ( numRead == 0 )
      {
         logit( "et", "Error.  No bytes received from GPS receiver.\n" );
         return -1;
      }

/* The first character read must be a DLE
   **************************************/
      chr = buf[0];
      if ( first && (chr != DLE) )
      {
         logit( "et", "ReadGpsPacket error.  First byte in packet is not DLE.\n" );
         return -1;
      }
      else
         first = 0;

/* Count the total DLE's received
   ******************************/
      if ( chr == DLE ) nDLE++;
      if ( (chr == ETX) && (nDLE % 2 == 0) ) break;

/* Check for buffer overflow
   *************************/
      if ( npacketchar + 1 > MAXGPSPACKETSIZE )
      {
         logit( "et", "ReadGpsPacket buffer overflow.\n" );
         return -1;
      }

/* Save the byte
   *************/
      packet[npacketchar++] = chr;
   }
   packet[npacketchar++] = chr;   // Save the final ETX character
   return npacketchar;            // Return number of bytes read
}


/***************************************************************
  DecodeGpsPacket

  Extract packet id and data string bytes from the GPS packet.
  dsb  = Pointer to data string bytes
  ndsb = Number of data string bytes
 ***************************************************************/

void DecodeGpsPacket( UINT8 packet[], int packetsize, UINT8 *packet_id, UINT8 dsb[], int *ndsb )
{
   *packet_id = packet[1];
   *ndsb = packetsize - 4;
   memcpy( dsb, packet+2, *ndsb );
   return;
}


/********************************************************************
  RemoveDleBytes

  Remove any extra (stuffed) DLE bytes from the packet data string.
 ********************************************************************/

void RemoveDleBytes( UINT8 dsb[], int *ndsb )
{
   int newNdsb = *ndsb;
   int i;

   for ( i = 1; i < newNdsb; i++ )
   {
      if ( (dsb[i-1] == DLE) && (dsb[i] == DLE) )
      {
         newNdsb--;
         memmove( &dsb[i], &dsb[i+1], (newNdsb - i) );
      }
   }
   *ndsb = newNdsb;
   return;
}


/************************************************
  ClosePort

  Close the serial port.
 ************************************************/

void ClosePort( void )
{
   if ( handle != 0 )
      CloseHandle( handle );
   return;
}


/************************************************
  WriteGpsPacket

  Write packet to GPS receiver via com port.

  Returns  0 if all ok,
          -1 if a write error occurred
 ************************************************/

int WriteGpsPacket( UINT8 packet[], int nchar )
{
   DWORD numWritten;

   if ( WriteFile( handle, packet, nchar, &numWritten, 0 ) == 0 )
   {
      LogLastSystemError();
      logit( "et", "Error writing GPS packet.\n" );
      return -1;
   }

   if ( (int)numWritten < nchar )
   {
      logit( "et", "Requested write of %d characters to serial port.  Only %u were written.\n",
              nchar, numWritten );
      return -1;
   }
   return 0;
}


/*********************************************************
  DisableAutoPackets

  Disable automatic output packets from the GPS.
  By default, the primary and supplemental timing
  packets are sent after every one-second pulse.
 *********************************************************/

int DisableAutoPackets( void )
{
   UINT8 gpsPacket[] = {DLE, 0x8E, 0xA5, 0, 0, 0, 0, DLE, ETX};
   int   packetsize;
   UINT8 inPacket[MAXGPSPACKETSIZE];

/* Purge com port input buffer
   ***************************/
   if ( PurgeComInput() < 0 )
   {
      logit( "et", "PurgeComInput error.\n" );
      return -1;
   }

/* Disable packet broadcasts
   *************************/
   if ( WriteGpsPacket( gpsPacket, 9 ) < 0 )
   {
      logit( "et", "WriteGpsPacket error in DisableAutoPackets.\n" );
      return -1;
   }

/* Read the response packet
   ************************/
   if ( (packetsize = ReadGpsPacket(inPacket)) < 0 )
   {
      logit( "et", "ReadGpsPacket error in DisableAutoPackets.\n" );
      return -1;
   }
   if ( packetsize != 9 )
   {
      logit( "et", "Error.  DisableAutoPacket packet length: %d\n", packetsize );
      logit( "et", "Packet length should be 9.\n" );
      return -1;
   }
   return 0;
}


/*************************************************************
  SetUtcMode

  Put the GPS into "UTC mode", instead of "GPS mode".
  This will cause times to be reported in UTC, and the
  one-PPS pulse will be referenced to UTC.
  Save the timing settings to non-volatile memory.
 *************************************************************/

int SetUtcMode( void )
{
   UINT8 modePacket[] = {DLE, 0x8E, 0xA2, 0x03, DLE, ETX};
   UINT8 savePacket[] = {DLE, 0x8E, 0x4C, 0x06, DLE, ETX};

   if ( WriteGpsPacket( modePacket, 6 ) < 0 )
   {
      logit( "et", "WriteGpsPacket error setting UTC mode.\n" );
      return -1;
   }

   if ( WriteGpsPacket( savePacket, 6 ) < 0 )
   {
      logit( "et", "WriteGpsPacket error saving timing settings to NVM.\n" );
      return -1;
   }
   return 0;
}


/************************
  Query PPS output status
  ***********************/

int GetPpsOutputStatus( void )
{
   static UINT8 requestPacket[] = {DLE, 0x8E, 0x4E, DLE, ETX};
   int    packetsize;
   UINT8  inPacket[MAXGPSPACKETSIZE];
   UINT8  packet_id;                // GPS packet id
   UINT8  dsb[10];
   int    ndsb;
   const  int nptp = 2;            // Num bytes in PPS status packet

/* Purge com port input buffer
   ***************************/
   if ( PurgeComInput() < 0 )
   {
      logit( "et", "PurgeComInput error.\n" );
      return -1;
   }

/* Request PPS characteristics
   ***************************/
   if ( WriteGpsPacket( requestPacket, 5 ) < 0 )
   {
      logit( "et", "WriteGpsPacket error.\n" );
      return -1;
   }

/* Read the response packet
   ************************/
   if ( (packetsize = ReadGpsPacket(inPacket)) < 0 )
   {
      logit( "et", "ReadGpsPacket error in GetPpsOutputStatus.\n" );
      return -1;
   }
   if ( packetsize < 4 )
   {
      logit( "et", "Error.  GetPpsOutputStatus packet length: %d\n", packetsize );
      return -1;
   }

/* Extract packet id and data-string bytes from the incoming packet.
   dsb  = Data string bytes from GPS packet
   ndsb = Number of data string bytes
   ****************************************************************/
   DecodeGpsPacket( inPacket, packetsize, &packet_id, dsb, &ndsb );

   if ( packet_id != 0x8F )           // Wrong packet id
   {
      logit( "et", "Error. packet_id = %02X  (should be 0x8E)\n", packet_id );
      return -1;
   }

/* Remove any stuffed DLE bytes from the data string bytes.
   Then, check dsp bytes for sanity.
   *******************************************************/
   RemoveDleBytes( dsb, &ndsb );

   if ( ndsb != nptp )        // Wrong number of bytes in packet data string
   {
      int i;
      logit( "et", "Error. Number of data bytes in packet= %d (should be %d)\n", ndsb, nptp );
      for ( i = 0; i < ndsb; i++ )    // For debugging
         logit( "e", " %02X", dsb[i] );
      logit( "e", "\n" );
      return -1;
   }

   if ( dsb[0] != requestPacket[2] )  // Wrong packet subcode
   {
      logit( "et", "Error. Packet subcode for PTP = %02X  (should be %02X)\n",
             dsb[0], requestPacket[2] );
      return -1;
   }
   if ( dsb[1] & 0x02 )
      logit( "et", "The 1-PPS pulse is always on.\n" );
   else
      logit( "et", "1-PPS status byte: 0x%02x\n", dsb[1] );
   return 0;
}


/********************************************************************
  GetPrimaryTimingPacket

  Request that the primary timing packet be returned immediately.
  Returns 0 if successful.
 ********************************************************************/

int GetPrimaryTimingPacket( UINT8 dsb[] )
{
   static UINT8 requestPacket[] = {DLE, 0x8E, 0xAB, 0x00, DLE, ETX};
   int    packetsize;
   UINT8  inPacket[MAXGPSPACKETSIZE];
   UINT8  packet_id;                // GPS packet id
   int    ndsb;
   const  int nptp = 17;            // Num bytes in primary timing packet

/* Purge com port input buffer
   ***************************/
   if ( PurgeComInput() < 0 )
   {
      logit( "et", "PurgeComInput error.\n" );
      return -1;
   }

/* Request primary timing packet
   *****************************/
   if ( WriteGpsPacket( requestPacket, 6 ) < 0 )
   {
      logit( "et", "WriteGpsPacket error.\n" );
      return -1;
   }

/* Read the primary timing packet
   ******************************/
   if ( (packetsize = ReadGpsPacket(inPacket)) < 0 )
   {
      logit( "et", "ReadGpsPacket error in GetPrimaryTimingPacket.\n" );
      return -1;
   }
   if ( packetsize < 4 )
   {
      logit( "et", "Error.  GetPrimaryTimingPacket packet length: %d\n", packetsize );
      return -1;
   }

/* Extract packet id and data-string bytes from the incoming packet.
   dsb  = Data string bytes from GPS packet
   ndsb = Number of data string bytes
   ****************************************************************/
   DecodeGpsPacket( inPacket, packetsize, &packet_id, dsb, &ndsb );

   if ( packet_id != 0x8F )           // Wrong packet id
   {
      logit( "et", "Error. packet_id = %02X  (should be 0x8F)\n", packet_id );
      return -1;
   }

/* Remove any stuffed DLE bytes from the data string bytes.
   Then, check dsp bytes for sanity.
   *******************************************************/
   RemoveDleBytes( dsb, &ndsb );

   if ( ndsb != nptp )        // Wrong number of bytes in packet data string
   {
      int i;
      logit( "et", "Error. Number of data bytes in packet= %d (should be %d)\n", ndsb, nptp );
      for ( i = 0; i < ndsb; i++ )    // For debugging
         logit( "e", " %02X", dsb[i] );
      logit( "e", "\n" );
      return -1;
   }

   if ( dsb[0] != requestPacket[2] )  // Wrong packet subcode
   {
      logit( "et", "Error. Packet subcode for PTP = %02X  (should be %02X)\n",
             dsb[0], requestPacket[2] );
      return -1;
   }
   return 0;
}


/**********************************************************************
  GetSupplementalTimingPacket

  Request that the supplemental timing packet be returned immediately.
  Returns 0 if successful.
 **********************************************************************/

int GetSupplementalTimingPacket( UINT8 dsb[] )
{
   static UINT8 requestPacket[] = {DLE, 0x8E, 0xAC, 0x00, DLE, ETX};
   int    packetsize;
   UINT8  inPacket[MAXGPSPACKETSIZE];
   UINT8  packet_id;                // GPS packet id
   int    ndsb;
   const  int nstp = 68;            // Num bytes in supplemental timing packet

/* Purge com port input buffer
   ***************************/
   if ( PurgeComInput() < 0 )
   {
      logit( "et", "PurgeComInput error.\n" );
      return -1;
   }

/* Request supplemental timing packet
   **********************************/
   if ( WriteGpsPacket( requestPacket, 6 ) < 0 )
   {
      logit( "et", "WriteGpsPacket error.\n" );
      return -1;
   }

/* Read the supplemental timing packet
   ***********************************/
   if ( (packetsize = ReadGpsPacket(inPacket)) < 0 )
   {
      logit( "et", "ReadGpsPacket error in GetSupplementalTimingPacket.\n" );
      return -1;
   }
   if ( packetsize < 4 )
   {
      logit( "et", "Error.  GetSupplementalTimingPacket packet length: %d\n", packetsize );
      return -1;
   }

// for ( i = 0; i < packetsize; i++ )       // For debugging
//    logit( "", " %02X", dsb[i] );
// logit( "", "\n" );

/* Extract packet id and data-string bytes from the incoming packet.
   dsb  = Data string bytes from GPS packet
   ndsb = Number of data string bytes
   ****************************************************************/
   DecodeGpsPacket( inPacket, packetsize, &packet_id, dsb, &ndsb );

   if ( packet_id != 0x8F )           // Wrong packet id
   {
      logit( "et", "Error. packet_id = %02X  (should be 0x8F)\n", packet_id );
      return -1;
   }

/* Remove any stuffed DLE bytes from the data string bytes.
   Then, check dsp bytes for sanity.
   *******************************************************/
   RemoveDleBytes( dsb, &ndsb );

   if ( ndsb != nstp )        // Wrong number of bytes in packet data string
   {
      logit( "et", "Error. Number of data bytes in packet= %d (should be %d)\n", ndsb, nstp );
      return -1;
   }

   if ( dsb[0] != requestPacket[2] )  // Wrong packet subcode
   {
      logit( "et", "Error. Packet subcode for STP = %02X  (should be %02X)\n",
             dsb[0], requestPacket[2] );
      return -1;
   }
   return 0;
}


/* Retrieve and Log the Last System Error Message
   **********************************************/
void LogLastSystemError( void )
{
   LPVOID lpMsgBuf;
   DWORD dw = GetLastError();

   FormatMessage
   (
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR) &lpMsgBuf,
      0, NULL
    );

    logit( "et", "%s", lpMsgBuf );
    LocalFree( lpMsgBuf );
    return;
}
