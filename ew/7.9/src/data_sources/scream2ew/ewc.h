/*
 * ewc.h:
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

/*
 * $Id: ewc.h 2431 2006-09-06 21:37:49Z paulf $
 */

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
 * Revision 1.5  2003/02/19 16:00:18  root
 * #
 *
 */

#ifndef __EWC_H__
#define __EWC_H__

extern void ewc_init (const char *ModName, const char *Ringname, int injectsoh);
extern void ewc_send_heartbeat (void);
extern void ewc_send_error (int errnum, char *text);
extern int ewc_terminate (void);
extern void ewc_shutdown (void);
extern void ewc_send (uint8_t * buf, int len, int type);
#endif /* __EWC_H__ */
