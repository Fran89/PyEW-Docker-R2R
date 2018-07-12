/*
 * mainloop.c:
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

static char rcsid[] = "$Id: mainloop.c 2151 2006-05-16 17:14:35Z paulf $";

/*
 * $Log$
 * Revision 1.2  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.6  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"

void
main_loop (void)
{
  time_t now;
  time_t last_heartbeat = 0;

  for (;;)
    {

      if (ewc_terminate ())
        {
          ewc_shutdown ();
          warning (("Termination requested"));
          exit (0);
        }

      time (&now);

      if (difftime (now, last_heartbeat) > config.heartbeatinterval)
        {
          last_heartbeat = now;
          ewc_send_heartbeat ();
        }


      scm_dispatch ();

    }
}
