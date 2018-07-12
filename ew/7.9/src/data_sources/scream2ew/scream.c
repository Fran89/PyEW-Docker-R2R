/*
 * scream.c:
 *
 * Copyright (c) 2003 Guralp Systems Limited
 * Author James McKenzie, contact <software@guralp.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

static char rcsid[] = "$Id: scream.c 6803 2016-09-09 06:06:39Z et $";

/*
 * $Log$
 * Revision 1.3  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.2  2006/05/02 16:25:59  paulf
 *  new installment of scream2ew from GSL
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.7  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"

static SOCKET sockfd = (SOCKET)-1;
static int protocol;

/* _protocol is one of SCM_PROTO_... */
/* For SCM_PROTO_UDP, server is ignored, port is the */
/*   local port to listen on */
/* For SCM_PROTO_TCP, server is the remote server to connect */
/*   to, and port is the port on that server, the local port */
/*   is assigned by the OS*/

void
scm_init (int _protocol, char *server, int port)
{

  struct sockaddr_in local = { 0 }, remote =
  {
  0};

  SocketSysInit ();

  protocol = _protocol;

  switch (protocol)
    {
    case SCM_PROTO_TCP:
      sockfd = socket (PF_INET, SOCK_STREAM, 0);


      local.sin_family = AF_INET;
      local.sin_port = 0;
      local.sin_addr.s_addr = INADDR_ANY;

      if (bind (sockfd, (struct sockaddr *) &local, sizeof (local)))
        fatal (("bind failed: %m"));

      remote.sin_family = AF_INET;
      remote.sin_port = (u_short)htons ((u_short)port);

      {
        struct hostent *he = gethostbyname (server);
        if (!he)
          fatal (("gethostbyname(%s) failed: %m", server));

        if (he->h_addrtype != AF_INET)
          fatal (("gethostbyname returned a non-IP address"));

        memcpy (&remote.sin_addr.s_addr, he->h_addr, he->h_length);

      }


      if (connect (sockfd, (struct sockaddr *) &remote, sizeof (remote)))
        fatal (("connect failed: %m"));

      {
        uint8_t cmd = SCREAM_CMD_START_XMIT;
        if (send (sockfd, (const char *)&cmd, 1, 0) != 1)
          fatal (("write to socket failed: %m"));

      }
      info (("connected TCP socket"));

      break;

    case SCM_PROTO_UDP:
      sockfd = socket (PF_INET, SOCK_DGRAM, 0);
      /*For the UDP binding we want to be able */
      /*to rebind to this listening port */

      {
        int on = 1;

        if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, (void *) &on,
                        sizeof (on)))
          warning (("Error setting SO_REUSEADDR: %m"));
      }

      local.sin_family = AF_INET;
      local.sin_port = (u_short)htons ((u_short)port);
      local.sin_addr.s_addr = INADDR_ANY;

      if (bind (sockfd, (struct sockaddr *) &local, sizeof (local)))
        fatal (("bind failed: %m"));
      break;
    default:
      fatal (("Unknown protocol"));
      break;
    }


}

void
scm_dispatch ()
{
  uint8_t buf[SCREAM_MAX_LENGTH];
  struct sockaddr_in sin = { 0 };
  int sinlen = sizeof(sin);

  switch (protocol)
    {
    case SCM_PROTO_UDP:
	memset(buf,0,sizeof(buf));
      if (recvfrom(sockfd, (char *)buf, SCREAM_MAX_LENGTH, 0, (struct sockaddr*)&sin, &sinlen) < 0)
        {
#if defined(WIN32)
/*Shoot them now, shoot them often, there seems to be no way to */
/*turn off delayed error notification, in win2k - hip hip horay */
/*we'll just "Carry on regardless(tm)" */

          if (WSAGetLastError () == WSAECONNRESET)
            return;
          else
            fatal (("recv failed: %m"));
#else
          fatal (("recv failed: %m"));
#endif
        }

      break;
    case SCM_PROTO_TCP:
/*This horrible hack is because the protocol version isn't until later...*/
      if (complete_read (sockfd, (char *) buf, SCREAM_INITIAL_LEN) !=
          SCREAM_INITIAL_LEN)
        fatal (("read failed: %m"));

      switch (buf[GCF_BLOCK_LEN])
        {
        case 31:
          if (complete_read
              (sockfd, (char *) buf + SCREAM_INITIAL_LEN,
               SCREAM_V31_SUBSEQUENT) != SCREAM_V31_SUBSEQUENT)
            fatal (("read failed: %m"));
          break;
        case 40:
          if (complete_read
              (sockfd, (char *) buf + SCREAM_INITIAL_LEN,
               SCREAM_V40_SUBSEQUENT) != SCREAM_V40_SUBSEQUENT)
            fatal (("read failed: %m"));
          break;
        default:
          fatal (("Unknown version of scream protocol at remote end"));
          break;
        }
      break;
    }

  if(gcf_dispatch (buf, GCF_BLOCK_LEN)) {
    switch(protocol) {
    case SCM_PROTO_UDP:
        warning(("GCF parser failed on block from %s port %d\n",
                inet_ntoa(sin.sin_addr),ntohs(sin.sin_port)));
        break;
        
    case SCM_PROTO_TCP:
        warning(("GCF parser failed on block from TCP connection\n"));
        break;
        
    default:
        warning(("GCF parser failed on block from unknown source\n"));
        break;
    }
  }

}
