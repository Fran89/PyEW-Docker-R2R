/*
 * dispatch.c:
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

static char rcsid[] = "$Id: dispatch.c 2431 2006-09-06 21:37:49Z paulf $";

/*
 * $Log$
 * Revision 1.4  2006/09/06 21:37:21  paulf
 * modifications made for InjectSOH
 *
 * Revision 1.3  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.2  2006/05/02 16:25:59  paulf
 *  new installment of scream2ew from GSL
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.7  2003/03/17 11:21:20  root
 * #
 *
 * Revision 1.6  2003/02/28 17:08:27  root
 * #
 *
 * Revision 1.5  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"
#include "ewc_types.h"

void
dispatch (gcf_block b)
{
  uint8_t *buf;
  TRACE2_HEADER *t;
  int len;



  buf =
    malloc (sizeof (TRACE2_HEADER) + (len = ((b->samples) * sizeof (int))));
  if (!buf)
    fatal (("malloc failed"));

  t = (TRACE2_HEADER *) buf;

  if (map_lookup (b, t)) {
    if (config.verbose) info(("Ignoring GCF block: could not find SCNL for %s %s", b->sysid, b->strid));
    free(buf);
    return;
  }

  /* process the SOH if sample_rate is 0 */
  if (config.injectsoh && !b->sample_rate) {
    int start_int;
    char network[3];
    char station[6];
    char *cptr_tmp;

    strcpy(network, t->net);
    strcpy(station, t->sta);
    buf[0]=0;
    b->buf[16+b->samples+1] = '\0';

    cptr_tmp = &(b->buf[16]);
    start_int = (int) b->estart;

    sprintf(buf, "%s-%s %d\n%s", network, station, start_int, cptr_tmp);
    len = strlen(buf);
    ewc_send (buf, len, SEND_TYPE_SOH);
    if (config.verbose) info(("Injecting GCF SOH block: %s %s", b->sysid, b->strid));
    free(buf);
    return;
  }


  t->version[0] = TRACE2_VERSION0;
  t->version[1] = TRACE2_VERSION1;
  t->endtime = t->starttime = (double) b->estart;
  t->endtime += ((double) (b->samples - 1)) / ((double) b->sample_rate);
  t->nsamp = b->samples;
  t->samprate = (double) b->sample_rate;
  strcpy (t->datatype, SCREAM2EW_DATATYPE);

  memcpy (t + 1, b->data, len);

  ewc_send (buf, len + sizeof (TRACE2_HEADER), SEND_TYPE_TRACE);

  if (config.verbose) 
      info (("Sent %d bytes: %s %s as net=%s sta=%s chan=%s pin=%d sr=%.2f", 
	    len+sizeof(TRACE2_HEADER), b->sysid, b->strid, t->net, t->sta,
	    t->chan, t->pinno, t->samprate,t->starttime,t->endtime));

  free (buf);
}
