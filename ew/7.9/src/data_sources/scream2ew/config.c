/*
 * config.c:
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

static char rcsid[] = "$Id: config.c 2431 2006-09-06 21:37:49Z paulf $";

/*
 * $Log$
 * Revision 1.3  2006/09/06 21:37:21  paulf
 * modifications made for InjectSOH
 *
 * Revision 1.2  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.5  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"

struct config_struct config = { 0 };

void
parse_config (const char *filename)
{
  int fh;
  char *tok;

  config.injectsoh=0;


  fh = k_open ((char *) filename);

  if (!fh)
    fatal (("Can't open configuration file %s"));


  while (fh)
    {
      while (k_rd ())
        {
          tok = k_str ();


          if ((!tok) || (*tok == '#'))
            continue;


          if (*tok == '@')
            {
              fh = k_open (tok + 1);
              if (!fh)
                fatal (("Can't open configuration file %s"));

              continue;
            }

          if (k_its ("LogFile"))
            {
              config.writelog = k_int ();
            }
          else if (k_its ("MyModuleId"))
            {
              char *arg = k_str ();
              if (arg)
                config.modulename = strdup (arg);
            }
          else if (k_its ("RingName"))
            {
              char *arg = k_str ();
              if (arg)
                config.ringname = strdup (arg);
            }
          else if (k_its ("HeartBeatInterval"))
            {
              config.heartbeatinterval = (double) k_long ();
            }
          else if (k_its ("PortNumber"))
            {
              config.port = k_int ();
            }
          else if (k_its ("ChanInfo"))
            {
              char *arg = k_str ();
              map_add (arg);
            }
          else if (k_its ("Verbose"))
            {
	      config.verbose=k_int();
            }
          else if (k_its ("InjectSOH"))
            {
	      config.injectsoh=k_int();
            }
          else if (k_its ("SleepInterval"))
            {
              warning (("SleepInterval option ignored provided only for backward compatability"));
            }
          else if (k_its ("Protocol"))
            {
              char *arg = k_str ();

              if (!strcasecmp (arg, "UDP"))
                {
                  config.protocol = SCM_PROTO_UDP;
                }
              else if (!strcasecmp (arg, "TCP"))
                {
                  config.protocol = SCM_PROTO_TCP;
                }
              else
                {
                  fatal (("Unknown argument to Protocol statement %s", arg));
                }
            }
          else if (k_its ("Server"))
            {
              char *arg = k_str ();
              if (arg)
                config.server = strdup (arg);
              config.protocol = SCM_PROTO_TCP;
            }
          else
            {
              fatal (("Unknown command %s in configuration file", tok));
            }
        }

      fh = k_close ();
    }



  if (!config.modulename)
    fatal (("Missing MyModuleId in configuration file"));

  if (!config.port)
    fatal (("No PortNumber specificed in configuration file"));

  if ((config.protocol == SCM_PROTO_TCP) && (!config.server))
    fatal (("TCP specified but no Server set in configuration file"));

  if (config.server && (config.protocol != SCM_PROTO_TCP))
    fatal (("Server specified for UDP connection in configuration file"));




}
