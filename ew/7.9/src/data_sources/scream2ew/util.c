/*
 * util.c:
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

static char rcsid[] = "$Id: util.c 6803 2016-09-09 06:06:39Z et $";

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

static char *file;
static int line;
static char *what;
static int where;

void
dowhere (char *_file, int _line, char *_what, int _where)
{
  what = _what;
  file = _file;
  line = _line;
  where = _where;
}


void
doinfo (char *fmt, ...)
{
  va_list ap;
  char buf[2048], mfmt[2048], *ptr, *fptr;

/* Step 1 is to grok out the %m s in the format string*/

  ptr = fmt;

/*sprintf returns int, but someone seems to have defined it */
/*to return char? */
#if 0
  fptr =
    mfmt + sprintf (mfmt, "scream2ew: %s line %d: %s ", file, line, what);
#else
  sprintf (mfmt, "%s ", what);
  fptr = mfmt + strlen (mfmt);
#endif



  while (*ptr)
    {

      switch (*ptr)
        {
        case '\n':
          break;
        case '%':
          switch (*(ptr + 1))
            {
            case 'm':
              ptr++;
#if 0
              fptr +=
                sprintf (fptr, "(errno=%d %s)", errno, strerror (errno));
#else
              sprintf (fptr, "(errno=%d %s)", errno, strerror (errno));
              fptr += strlen (fptr);
#endif
#ifdef WIN32
              sprintf (fptr, " [WSAERROR=%d]", WSAGetLastError ());
              fptr += strlen (fptr);
#endif
              break;
            default:
              *(fptr++) = *ptr;
            }
          break;
        default:
          *(fptr++) = *ptr;
          ;
        }
      ptr++;
    }
  *fptr = 0;

  ptr = buf;

  va_start (ap, fmt);

#if 0
  ptr += vsprintf (ptr, mfmt, ap);
#else
  vsprintf (ptr, mfmt, ap);
  ptr += strlen (ptr);
#endif
  va_end (ap);

  *ptr = 0;

  if (where & MSG_EWLOGIT)
    logit ("et", "%s\n", buf);
  if (where & MSG_CONSOLE)
    fprintf (stderr, "%s\n", buf);

}
void
domsg (char *fmt, ...)
{
  va_list ap;
  char buf[2048], mfmt[2048], *ptr, *fptr;

/* Step 1 is to grok out the %m s in the format string*/

  ptr = fmt;

/*sprintf returns int, but someone seems to have defined it */
/*to return char? */
#if 0
  fptr =
    mfmt + sprintf (mfmt, "scream2ew: %s line %d: %s ", file, line, what);
#else
  sprintf (mfmt, "scream2ew: %s line %d: %s ", file, line, what);
  fptr = mfmt + strlen (mfmt);
#endif



  while (*ptr)
    {

      switch (*ptr)
        {
        case '\n':
          break;
        case '%':
          switch (*(ptr + 1))
            {
            case 'm':
              ptr++;
#if 0
              fptr +=
                sprintf (fptr, "(errno=%d %s)", errno, strerror (errno));
#else
              sprintf (fptr, "(errno=%d %s)", errno, strerror (errno));
              fptr += strlen (fptr);
#endif
#ifdef WIN32
              sprintf (fptr, " [WSAERROR=%d]", WSAGetLastError ());
              fptr += strlen (fptr);
#endif
              break;
            default:
              *(fptr++) = *ptr;
            }
          break;
        default:
          *(fptr++) = *ptr;
          ;
        }
      ptr++;
    }
  *fptr = 0;

  ptr = buf;

  va_start (ap, fmt);

#if 0
  ptr += vsprintf (ptr, mfmt, ap);
#else
  vsprintf (ptr, mfmt, ap);
  ptr += strlen (ptr);
#endif
  va_end (ap);

  *ptr = 0;

  if (where & MSG_EWLOGIT)
    logit ("et", "%s\n", buf);
  if (where & MSG_CONSOLE)
    fprintf (stderr, "%s\n", buf);

}

int
complete_read (SOCKET fd, char *buf, int n)
{
  int c = 0;
  int r;

  while (n)
    {
      r = recv (fd, buf, n, 0);
      if (r < 0)
        return r;
      if (!r)
        return c;

      n -= r;
      buf += r;
      c += r;
    }

  return c;
}
