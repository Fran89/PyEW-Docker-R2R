
#include <string.h>
#include <earthworm.h>
#include "sendfilemt.h"

int Send_all( int sd, char buf[], int nbytes );
int Recv_all( int sd, char buf[], int nbytes, int TimeOut );


/****************************************************************
 *                      SendBlockToSocket()                     *
 *                                                              *
 *  Send a block to the remote system.  Send a zero-length      *
 *  block is sent when the transmission is over.                *
 ****************************************************************/

int SendBlockToSocket( int sd, char *buf, int blockSize )
{
   char blockSizeAsc[7];

/* Write block size to socket
   **************************/
   sprintf( blockSizeAsc, "%6u", blockSize );
   if ( Send_all( sd, blockSizeAsc, 6 ) == -1 )
   {
      logit( "et", "Send_all() error\n" );
      return SENDFILE_FAILURE;
   }

/* Send block bytes
   ****************/
   if ( Send_all( sd, buf, blockSize ) == -1 )
   {
      logit( "et", "Send_all() error\n" );
      return SENDFILE_FAILURE;
   }
   return SENDFILE_SUCCESS;
}


/****************************************************************
 *                      GetAckFromSocket()                      *
 *                                                              *
 *  Get an "ACK" from the remote system.                        *
 *  Return SENDFILE_SUCCESS on success, otherwise               *
 *     return SENDFILE_FAILURE.                                 *
 ****************************************************************/

int GetAckFromSocket( int sd )
{
   extern int AckTimeOut;     // Ack receive timeout
   char Ack[4];

/* Read the "ACK" from the socket
   ******************************/
    if ( Recv_all( sd, Ack, 3, AckTimeOut ) == -1)
    {
        logit( "et", "Recv_all() error. (no ACK if recv()==0)\n" );
        return SENDFILE_FAILURE;
    }

    Ack[3] = '\0';            // Null terminate

    if ( strcmp( Ack, "ACK" ) == 0 )
        return SENDFILE_SUCCESS;

    logit( "et", "Bad ACK string received: %s\n", Ack);
    return SENDFILE_FAILURE;
}
