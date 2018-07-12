/*
 * gcf.c:
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

static char rcsid[] = "$Id: gcf.c 2431 2006-09-06 21:37:49Z paulf $";

/*
 * $Log$
 * Revision 1.4  2006/09/06 21:37:21  paulf
 * modifications made for InjectSOH
 *
 * Revision 1.3  2006/05/02 16:25:59  paulf
 *  new installment of scream2ew from GSL
 *
 * Revision 1.1  2003/03/27 18:07:18  alex
 * Initial revision
 *
 * Revision 1.4  2003/02/28 17:05:37  root
 * #
 *
 */

#include "project.h"

static time_t
gcf_time_to_time_t (gcf_time * t)
{
  time_t ret = GCF_EPOCH;

  ret += (t->day) * 24 * 60 * 60;
  ret += (t->sec);

  return ret;
}

static int
extract_8 (gcf_block b)
{
  uint8_t *ptr = b->buf;
  int *optr = b->data;

  int val;
  int n = b->samples;

  ptr += 16;

  val = gp_int32 (ptr);
  ptr += 4;

  if (*ptr) {
    warning(("First difference is not zero"));
    return 1;
  }
  while (n--)
    {
      val += gp_int8 (ptr++);
      *(optr++) = val;
    }
  b->fic = val;
  b->ric = gp_int32 (ptr);

  if (b->fic != b->ric) {
    warning(("Fic!=Ric"));
    return 1;
  }

  return 0;
}

static int
extract_16 (gcf_block b)
{
  uint8_t *ptr = b->buf;
  int *optr = b->data;

  int val;
  int n = b->samples;

  ptr += 16;

  val = gp_int32 (ptr);
  ptr += 4;

  if (*ptr) {
    warning(("First difference is not zero"));
    return 1;
  }

  while (n--)
    {
      val += gp_int16 (ptr);
      ptr += 2;
      *(optr++) = val;
    }
  b->fic = val;
  b->ric = gp_int32 (ptr);

  if (b->fic != b->ric) {
    warning(("Fic!=Ric"));
    return 1;
  }
  
  return 0;
}

static int
extract_24 (gcf_block b)
{
  uint8_t *ptr = b->buf;
  int *optr = b->data;

  int val;
  int n = b->samples;

  ptr += 16;

  if ((*ptr) && (0xff != *ptr)) {
    warning(("claimed 24 bit data isn't"));
    return 1;
  }

  val = gp_int32 (ptr);
  ptr += 4;

  if (gp_int24 (ptr)) {
    warning(("First difference is not zero"));
    return 1;
  }

  while (n--)
    {
      val += gp_uint24 (ptr);
      ptr += 3;
      while (val >= 0x800000L)
        val -= 0x1000000L;
      *(optr++) = val;
    }
  b->fic = val;
  if ((*ptr) && (0xff != *ptr)) {
    warning(("claimed 24 bit data isn't"));
    return 1;
  }

  b->ric = gp_int32 (ptr);


  if (b->fic != b->ric) {
    warning(("Fic!=Ric"));
    return 1;
  }

  return 0;
}

static int
extract_32 (gcf_block b)
{
  uint8_t *ptr = b->buf;
  int *optr = b->data;

  uint32_t uval;
  int32_t val;

  int n = b->samples;

  ptr += 16;

  uval = gp_uint32 (ptr);
  ptr += 4;

  if (*ptr) {
    warning(("First difference is not zero"));
    return 1;
  }

  while (n--)
    {
      uval += gp_uint32 (ptr);
      ptr += 4;

      if (uval == 0x80000000UL)
        {
          val = -0x80000000L;
        }
      else if (uval & 0x80000000UL)
        {
          val = -1 - (int32_t) (0xffffffffUL - uval);
        }
      else
        {
          val = (int32_t) uval;
        }

      *(optr++) = (int) val;
    }

  b->fic = (int) val;
  b->ric = gp_int32 (ptr);

  if (b->fic != b->ric) {
    warning(("Fic!=Ric"));
    return 1;
  }
  
  return 0;
}



int
gcf_dispatch (uint8_t * buf, int sz)
{
  int i;
  struct gcf_block_struct block;
  uint8_t *ptr;
  int err = 0;

  block.buf = buf;
  block.size = sz;
  block.csize = 0;

  strcpy (block.sysid, gp_base36_to_a (gp_uint32 (buf)));
  strcpy (block.strid, gp_base36_to_a (gp_uint32 (buf + 4)));

  i = gp_uint16 (buf + 8);
  block.start.day = i >> 1;
  i &= 1;
  i <<= 16;
  block.start.sec = i | gp_uint16 (buf + 10);

  block.estart = gcf_time_to_time_t (&block.start);

  block.sample_rate = buf[13];
  block.format = buf[14] & 7;
  block.records = buf[15];
  block.samples = (block.format) * (block.records);

  block.text = buf + 16;

  if (block.sample_rate)
    {
      switch (block.format)
        {
        case 4:
          block.csize = 24 + block.samples;
          err = extract_8 (&block);
          break;
        case 2:
          block.csize = 24 + (2 * block.samples);
          err = extract_16 (&block);
          break;
        case 1:
          if ((sz - 24) == (4 * block.samples))
            {
              block.csize = 24 + (4 * block.samples);
              err = extract_32 (&block);
            }
          else if ((sz - 24) == (3 * block.samples))
            {
              block.csize = 24 + (3 * block.samples);
              err = extract_24 (&block);
            }
          else
            {
              /* Guess 32 */
              err = extract_32 (&block);
            }
          break;
        default:
          warning(("unknown GCF compression format"));
          err = 1;
        }
    }

  if(!err) dispatch (&block);
  return err;
}
