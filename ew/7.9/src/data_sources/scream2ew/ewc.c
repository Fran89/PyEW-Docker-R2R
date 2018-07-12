/*
 * ewc.c:
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

static char rcsid[] = "$Id: ewc.c 3331 2008-06-26 23:09:59Z dietz $";

/*
 * $Log$
 * Revision 1.5  2008/06/26 23:09:59  dietz
 * Modified to issue a single Earthworm heartbeat at the end of ewc_init()
 * to make auto-restarts possible if the connection to scream fails.
 *
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
 * Revision 1.6  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"
#include "ewc_types.h"
#include "ewc.h"

static SHM_INFO Region={0};

static unsigned char InstId;
static unsigned char ModId;
static unsigned char TypeHeartBeat;
static unsigned char TypeError;
static unsigned char TypeTraceBuf;
static unsigned char TypeSOH;

void
ewc_init (const char *ModName, const char *RingName, int injectsoh)
{
  long RingKey;

  RingKey = GetKey ((char *) RingName);

  if (RingKey < 0)
    fatal (("Cannot get key for ring name `%s'", RingName));

  if (GetLocalInst (&InstId))
    fatal (("error getting local installation id"));

  if (GetModId ((char *) ModName, &ModId))
    fatal (("Cannot get ID for module name `%s'", ModName)); /*FIXME: module name */

  if (GetType ("TYPE_HEARTBEAT", &TypeHeartBeat))
    fatal (("Cannot get type number for TYPE_HEARTBEAT message"));

  if (GetType ("TYPE_ERROR", &TypeError))
    fatal (("Cannot get type number for TYPE_ERROR message"));

  if (GetType ("TYPE_TRACEBUF2", &TypeTraceBuf))
    fatal (("Cannot get type number for TYPE_TRACEBUF2 message"));

  if (injectsoh==1 && GetType ("TYPE_GCFSOH_PACKET", &TypeSOH))
    fatal (("Cannot get type number for TYPE_GCFSOH_PACKET message"));

  tport_attach (&Region, RingKey); /*Why is this void? */

/* The old code used to flush the input ring here - I can't see why
 * since we never use the input ring anywhere else
 */

  ewc_send_heartbeat ();

  return;
}



void
ewc_send_heartbeat (void)
{
  time_t now;
  MSG_LOGO msg;
  char buf[1024];
  int len;

  time (&now);

  len =
    snprintf (buf, sizeof (buf), "%ld %ld\n", (long) now, (long) getpid ());

  if (len < 0)
    {
      warning (("Buffer overflow"));
      return;
    }

  msg.instid = InstId;
  msg.mod = ModId;
  msg.type = TypeHeartBeat;

  if (tport_putmsg (&Region, &msg, len, buf) != PUT_OK)
    warning (("Error sending heartbeat."));

}


void
ewc_send_error (int errnum, char *text)
{
  time_t now;
  MSG_LOGO msg;
  char *buf;
  int len;

  time (&now);

  if (!text)
    return;

  buf = malloc ((len = 1024 + strlen (text)));
  if (!buf)
    {
      warning (("Malloc failed"));
      return;
    }

  len = snprintf (buf, len, "%ld %d %s\n", (long) now, errnum, text);

  if (len < 0)
    {
      free (buf);
      warning (("Buffer overflow"));
      return;
    }

  msg.instid = InstId;
  msg.mod = ModId;
  msg.type = TypeError;

  if (tport_putmsg (&Region, &msg, len, buf) != PUT_OK)
    warning (("Error sending error %d:%s", errnum, text));

}

int
ewc_terminate (void)
{
  int flag = tport_getflag (&Region);

  if (flag == TERMINATE)
    return 1;
  if (flag == getpid ())
    return 1;

  return 0;
}

void
ewc_shutdown (void)
{
  tport_detach (&Region);
}


void
ewc_send (uint8_t * buf, int len, int type)
{
  MSG_LOGO msg={0};

  msg.instid = InstId;
  msg.mod = ModId;
  switch(type) {
	case SEND_TYPE_TRACE:
     		msg.type = TypeTraceBuf;
		break;
  	case SEND_TYPE_SOH: 
		msg.type = TypeSOH;
		break;
	default:
    		warning (("Error sending data message, bad type specified to ewc_send"));
		return;
  }


  if (len > MAX_TRACEBUF_SIZ)
    fatal (("Assertion trace buf lengh <= MAX_TRACEBUF_SIZ failed"));

if (0) {
unsigned char *ptr=buf;
info(("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]));
ptr+=16;
info(("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]));
ptr+=16;
info(("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]));
ptr+=16;
info(("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15]));
}
 
  if (tport_putmsg (&Region, &msg, len, (char *) buf) != PUT_OK)
    warning (("Error sending data message"));

}
