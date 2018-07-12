
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sender.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2005/04/08 17:18:02  dietz
 *     minor initialization change
 *
 *     Revision 1.1  2000/02/14 19:11:50  lucky
 *     Initial revision
 *
 *
 */


          /********************************************
           *                 sender.c                 *
           *                                          *
           *   Socket functions for UDP broadcasting  *
           ********************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <ew_packet.h>

static SOCKET soko = 0;

void SocketSysInit( void );
void SocketClose( SOCKET );
void SocketPerror( char * );


     /*****************************************************
      *                     SocketInit                    *
      *                                                   *
      *   Open a socket and bind it to a port number      *
      *   Returns -1 if error; 0 if all ok                *
      *****************************************************/

int SocketInit( void )
{
   int optval;

/* Initialize socket system
   ************************/
   SocketSysInit();

/* Open a socket
   *************/
   soko = socket( AF_INET, SOCK_DGRAM, 0 );
   if ( soko == -1 )
   {
      SocketPerror( "Socket function error" );
      return( -1 );
   }

/* Enable broadcasting on the socket
   *********************************/
   optval = 1;
   if ( setsockopt( soko, SOL_SOCKET, SO_BROADCAST, (char *)&optval,
                    sizeof(int) ) == -1 )
   {
      SocketPerror( "Setsockopt SO_BROADCAST error" );
      SocketClose( soko );
      return( -1 );
   }
   return( 0 );
}


   /********************************************************
    *                      SendPacket                      *
    *               Broadcast one UDP packet               *
    *                                                      *
    *  Returns length of message sent.                     *
    ********************************************************/

int SendPacket( PACKET *packet,            /* Pointer to packet to send */
                int    packetSize,         /* Length of packet in bytes */
                char   *OutAddress,        /* IP address of destination */
                int    OutPortNumber )
{
   int lenSent;
   static struct sockaddr_in name;

   memset( &name, 0, sizeof(name) );              /* Fill with zeros */
   name.sin_family      = AF_INET;
   name.sin_port        = htons( (unsigned short)OutPortNumber );
   name.sin_addr.s_addr = inet_addr( OutAddress );

   lenSent = sendto( soko, (char *)packet, packetSize, 0,
             (struct sockaddr *)&name, sizeof(name) );

   if ( lenSent == -1 )
      SocketPerror( "Sendto error" );

   return( lenSent );
}

   /********************************************************
    *         SocketShutdown  - closes socket              *
    ********************************************************/
void SocketShutdown( void )
{
   if( soko ) SocketClose( soko );
   return;
}
