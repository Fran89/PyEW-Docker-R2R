/*
 * gputil.c:
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

static char rcsid[] = "$Id: gputil.c 2151 2006-05-16 17:14:35Z paulf $";

/*
 * $Log$
 * Revision 1.4  2006/05/16 17:14:35  paulf
 * removed all Windows issues from the .c and .h and updated the makefile.sol
 *
 * Revision 1.3  2006/05/02 16:25:59  paulf
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

int
gp_uint8 (uint8_t * buf)
{
  return *buf;
}

int
gp_uint16 (uint8_t * buf)
{
  return (buf[0] << 8) | buf[1];
}

int
gp_uint24 (uint8_t * buf)
{
  return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

int
gp_uint32 (uint8_t * buf)
{
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

int
gp_int8 (uint8_t * buf)
{
  if ((*buf) & 0x80)
    {
      return ((int) *buf) - 0x100;
    }
  return gp_uint8 (buf);
}

int
gp_int16 (uint8_t * buf)
{
  if (buf[0] & 0x80)
    {
      return ((buf[0] << 8) | buf[1]) - 0x10000;
    }
  return gp_uint16 (buf);
}


int
gp_int24 (uint8_t * buf)
{

  if (buf[0] & 0x80)
    {
      return ((buf[0] << 16) | (buf[1] << 8) | buf[2]) - 0x1000000;
    }
  return gp_uint24 (buf);
}

int
gp_int32 (uint8_t * buf)
{
  int ret;
  if (buf[0] & 0x80)
    {

/*Bad voodoo below*/
      ret = ((buf[1] << 16) | (buf[2] << 8) | buf[3]) - 0x1000000;
      ret -= ((0xff - buf[0]) << 24);

      return ret;
    }
  return gp_uint32 (buf);
}


int
gp_a_to_base36 (char *a)
{
  int ret = 0;

  while (*a)
    {
      ret *= 36;
      if (((*a) >= '0') && ((*a) <= '9'))
        ret += (*a) - '0';
      if (((*a) >= 'a') && ((*a) <= 'z'))
        ret += 10 + (*a) - 'a';
      if (((*a) >= 'A') && ((*a) <= 'Z'))
        ret += 10 + (*a) - 'A';
      a++;
    }

  return ret;
}

char *
gp_base36_to_a (int i)
{
  static char ret[8];
  int j, k, pos = 7;

  ret[pos] = 0;

  /* bit 31 is the 'extended sysid' bit, if it is set then the next
     5 bits are used for other purposes and we only have 26 bits of
     base-36 characters */
  if(i & 0x80000000) i &= 0x03FFFFFF;

  while(i) {
    k = (int) i / 36;
    j = i - (k * 36);

    ret[--pos] = (j < 10) ? (j + '0') : (j - 10 + 'A');
    i = k;
  }

  return ret + pos;
}
