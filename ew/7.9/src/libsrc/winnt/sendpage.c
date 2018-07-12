
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sendpage.c 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:53:30  lucky
 *     Initial revision
 *
 *
 */

  /**********************************************************************
   *                             sendpage.c                             *
   *                         Windows NT version                         *
   **********************************************************************/

#include <windows.h>
#include <stdio.h>
#include <earthworm.h>


 /**********************************************************************
  *            Send a Pager Request to PAGEIT via Serial Port          *
  *                          SendPage( buff )                          *
  *                                                                    *
  *     Will time out after four seconds if anything caused a hang.    *
  *     buff = String to output to serial port                         *
  *                                                                    *
  * Returns:                                                           *
  *      0 => All went well                                            *
  *     -1 => Time out - not in os2 or nt versions                     *
  *     -2 => Error while writing to port                              *
  **********************************************************************/

int SendPage( char *buff )
{
   HANDLE     comHandle;
   BOOL       success;
   DCB        dcb;
   DWORD      numWritten;

/* Set up the port with these parameters
   *************************************/
   const char SerialPort[] = "COM2";
   const DWORD baud        = 9600;
   const BYTE  parity      = EVENPARITY;
   const BYTE  databits    = 7;
   const BYTE  stopbits    = ONESTOPBIT;

/* Open the com port
   *****************/
   comHandle = CreateFile( SerialPort, GENERIC_READ | GENERIC_WRITE,
                           0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );

/* Get the current settings of the COMM port
   *****************************************/
   success = GetCommState( comHandle, &dcb );
   if ( !success )
   {
      logit( "e", "sendpage error getting the COM settings: %d\n", GetLastError() );
      return -2;
   }

/* Modify the COMM settings
   ************************/
   dcb.BaudRate = baud;
   dcb.Parity   = parity;
   dcb.ByteSize = databits;
   dcb.StopBits = stopbits;

/* Apply the new COMM port settings
   ********************************/
   success = SetCommState( comHandle, &dcb );
   if ( !success )
   {
      logit( "e", "sendpage error applying the COM settings: %d\n", GetLastError() );
      return -2;
   }

/* Send pager message to serial port
   *********************************/
   success = WriteFile( comHandle, buff, strlen(buff), &numWritten, 0 );
   if ( !success )
   {
      logit( "e", "sendpage error for port %s: %d\n", SerialPort, GetLastError() );
      CloseHandle( comHandle );
      return -2;
   }

   CloseHandle( comHandle );
   return 0;
}
