/*
 * main.c:
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

static char rcsid[] = "$Id: main.c 5772 2013-08-09 15:03:20Z paulf $";

/*
 * $Log$
 * Revision 1.7  2007/02/19 22:07:38  stefan
 * logit_init fix per Richard Luckett
 *
 * Revision 1.6  2006/09/06 21:37:21  paulf
 * modifications made for InjectSOH
 *
 * Revision 1.5  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.4  2006/05/02 16:25:59  paulf
 *  new installment of scream2ew from GSL
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.5  2003/02/28 17:05:37  root
 * #
 *
 */

/*
 * original version developed by:
 *
 *	Kent Lindquist
 *	Geophysical Institute
 *	U. of Alaska
 *	Sept. 1998
 *
 *	Murray McGowan, Guralp Systems, Inc.
 *
 * Complete re-write Jan 2003 to fix compiler issues and
 * add TCP support.
 *
 */

/* 
 * scream2ew module  
 *    Supports listening for UDP gcf packets from SCREAM!
 *    -or-
 *    Conecting to SCREAM! and fetching TCP packets
 *
 */


#include "project.h"

#define VERSION_STR "1.0.1 - 2013.02.25"
int
main (int argc, char **argv)
{
  if (argc != 2)
    fatal (("Usage:\n\t%s configfile\n Version: %s\n", argv[0], VERSION_STR));

  /* Start logging to screen initially */
  logit_init (argv[1], 0, 256, 1);
  
  /* Parse the configuration file */
  parse_config (argv[1]);

  /* Possibly change log options */
  logit_init (argv[1], 0, 256, config.writelog);
  info (("Read configuration file %s\n", argv[1]));
  info (("Program Version: %s\n", VERSION_STR));

  /*Initialize communication with ew */
  ewc_init (config.modulename, config.ringname, config.injectsoh);

  scm_init (config.protocol, config.server, config.port);


  main_loop ();

  return(0);
}
