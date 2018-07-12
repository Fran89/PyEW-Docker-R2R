/*
 * map.c:
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

static char rcsid[] = "$Id: map.c 5774 2013-08-09 17:16:23Z paulf $";

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
 * Revision 1.5  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"


typedef struct map_struct
{
  struct map_struct *next;
  char *system;
  char *stream;
  char *network;
  char *station;
  char *channel;
  char *location;
  int pinno;
  int ignore;                   /*Use to generate warnings */
}
 *Map;

static Map map = NULL;

/* this include terminating NULL */
#define SCREAM_SYSID_LEN 12
#define SCREAM_STREAM_LEN 12

void
map_add (char *mapping)
{
  Map new;
  char system[SCREAM_SYSID_LEN];
  char stream[SCREAM_STREAM_LEN];
  char network[TRACE2_NET_LEN];
  char station[TRACE2_STA_LEN];
  char channel[TRACE2_CHAN_LEN];
  char location[TRACE2_LOC_LEN];
  int pinno, j;
  char dummy;

  if ((j = sscanf (mapping, "%11s %11s %8s %6s %2s %3s %d%c",
                   system, stream, network, station, location, channel, 
                   &pinno, &dummy)) != 7)
    fatal (("Failed to parse ChanInfo '%s' (got %d fields expected %d)\n",
            mapping, j, 7));

  new = malloc (sizeof (struct map_struct));

  new->system = strdup(system);
  new->stream = strdup(stream);
  new->network = strdup(network);
  new->station = strdup(station);
  new->location = strdup(location);
  new->channel = strdup(channel);
  new->pinno = pinno;
  new->ignore = 0;
  new->next = map;
  map = new;

}

int
map_lookup (gcf_block b, TRACE2_HEADER* h)
{
  Map m = map;

  while (m)
    {
      if ((!strcmp (b->sysid, m->system)) && (!strcmp (b->strid, m->stream)))
        {

          if (m->ignore)
            return 1;

          strncpy (h->loc, m->location, TRACE2_LOC_LEN);
          strncpy (h->net, m->network, TRACE2_NET_LEN);
          strncpy (h->sta, m->station, TRACE2_STA_LEN);
          strncpy (h->chan, m->channel, TRACE2_CHAN_LEN);
          h->pinno = m->pinno;

          return 0;

        }
      m = m->next;
    }

  m = malloc (sizeof (struct map_struct));

  m->system = strdup (b->sysid);
  m->stream = strdup (b->strid);
  m->ignore = 1;

  warning (("block from unmapped channel %s %s", b->sysid, b->strid));

  m->next = map;

  map = m;

  return 1;
}
