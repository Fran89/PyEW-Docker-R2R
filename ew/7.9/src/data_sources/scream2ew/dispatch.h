/*
 * dispatch.h:
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
 * $Id: dispatch.h 2151 2006-05-16 17:14:35Z paulf $
 */

/*
 * $Log$
 * Revision 1.2  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.4  2003/02/19 16:00:18  root
 * #
 *
 */

#ifndef __DISPATCH_H__
#define __DISPATCH_H__

extern void dispatch (gcf_block b);
#endif /* __DISPATCH_H__ */
