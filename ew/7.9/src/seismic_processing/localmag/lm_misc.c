/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_misc.c 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_misc.c: A bunch of miscellaneous functions used for local_mag (and maybe
 *            other modules as well.)
 *    Pete Lombard; Sept, 2000
 */


#include <math.h>
#include <ctype.h>
#include <string.h>
#include "lm_misc.h"

/*
 * utmcal: Taken from utmcal.for in hyp2000, translated to c using f2c
 *         and cleaned up to remove most of the fortran stench.
 */
double utmcal(double orlat, double orlon, double dlat, double dlon)
{
    /* Initialized data */

  static double a = 6378206.4;        /* Earth radius in meters */
  static double esq = 0.00676866;
  static double ako = 1.0;
  static double degrad = 0.0174532;   /*     degrad = 3.14159/180. */
  
  /* Local variables */
  static double biga, bigc, bigm, bign, alat, clat, bigt, alon, olat, 
    slat, tlat, olon, epsq, biga2, biga3, biga4, biga5, biga6, bigc2, 
    bigt2, alatd, x, y, bigmo, olatd;
  
/* --CALCULATE THE POSITION OF ONE POINT RELATIVE TO ANOTHER ON A UNIVERSAL */
/*  TRANSVERSE MERCATOR GRID */
/*     ORLAT AND ORLON ARE THE LATITUDE AND LONGITUDE OF THE REFERENCE POINT */
/*        IN DECIMAL DEGREES */
/*     DLAT AND DLON ARE THE LATITUDE AND LONGITUDE OF THE POINT */
/*        TO BE PROJECTED IN DECIMAL DEGREES */
/*     DIST IS THE DISTANCE BETWEEN THE TWO POINTS, returned by utmcal */
/*     DATA AKO /0.9996/ */
  olatd = orlat;
  olat = olatd * degrad;
  olon = orlon * degrad;
  alatd = dlat;
  alat = alatd * degrad;
  alon = dlon * degrad;
  slat = sin(alat);
  clat = cos(alat);
  tlat = tan(alat);
  epsq = esq / (1.0 - esq);
  bign = a / sqrt(1.0 - esq * slat * slat);
  bigt = tlat * tlat;
  bigc = epsq * clat * clat;
  biga = (alon - olon) * clat;
  bigm = alatd * 111132.0894 - sin(alat * 2.0) * 16216.94 + 
    sin(alat * 4.0) * 17.21 - sin(alat * 6.0) * 0.02;
  bigmo = olatd * 111132.0894 - sin(olat * 2.0) * 16216.94 + 
    sin(olat * 4.0) * 17.21 - sin(olat * 6.0) * 0.02;
  biga2 = biga * biga;
  biga3 = biga2 * biga;
  biga4 = biga3 * biga;
  biga5 = biga4 * biga;
  biga6 = biga5 * biga;
  bigt2 = bigt * bigt;
  bigc2 = bigc * bigc;
  x = ako * bign * (biga + (1.0 - bigt + bigc) * biga3 / 6.0 + 
                    (5.0 - bigt * 18.0 + bigt2 + bigc * 72.0 - epsq * 58.0) * 
                    biga5 / 120.0);
  y = ako * (bigm - bigmo + bign * tlat * 
             (biga2 / 2.0 + (5.0 - bigt + bigc * 9.0 + bigc2 * 4.0) * 
              biga4 / 24.0 + (61.0 - bigt * 58.0 + bigt2 + bigc * 600.0 - 
                              epsq * 330.0) * biga6 / 720.0));
  return (sqrt(x * x + y * y) / 1000.0);
}


/*
 * fmtFilename: format a filename: filename points to a pre-allocated buffer
 *   with buflen bytes of empty space (one byte will be used for the 
 *   null terminator.) "format" is a character string describing the format
 *   in a manner analogous to printf(): `%' preceeds a special character
 *   of which "sScCnNlL%" are defined. S, C, L, and N indicate that the station,
 *   component, location or network names, respectively, are added to the filename, 
 *   with their case preserved (normally it will already be upper case.)
 *   s, c, l, and n are similar except that station, component, location, or network names
 *   are converted to lower case before being appended to filename. `%' 
 *   following `%' is used to indicate itself. Other characters following `%'
 *   will generate an error. All characters not preceeded by `%' are added
 *   as is to filename.
 *  Returns: 0 on success
 *          -1 when filename would overflow
 *          -2 on parse errors
 *      No logging is done here.
 */
int fmtFilename(char *sta, char *comp, char *net, char *loc, char *filename, 
                int buflen, char *format)
{
  char *c, *s, *f;
  
  f = &filename[strlen(filename)];
  c = format;
  while (c < format + strlen(format))
  {
    if (*c == '%')
    {
      c++;
      switch(*c)
      {
      case 'S':
        if (strlen(sta) < (size_t)buflen)
        {
          strcat(f, sta);
          f += strlen(sta);
          buflen -= strlen(sta);
        }
        else
          return -1;
        c++;
        break;
      case 's':
        if (strlen(sta) < (size_t)buflen)
        {
          s = sta;
          while (s < sta + strlen(sta))
          {
            *f++ = tolower(*s++);
            buflen--;
          }
        *f = '\0';
        }
        else
          return -1;
        c++;
        break;
      case 'C':
        if (strlen(comp) < (size_t)buflen)
        {
          strcat(f, comp);
          f += strlen(comp);
          buflen -= strlen(comp);
        }
        else
          return -1;
        c++;
        break;
      case 'c':
        if (strlen(comp) < (size_t)buflen)
        {
          s = comp;
          while (s < comp + strlen(comp))
          {
            *f++ = tolower(*s++);
            buflen--;
          }
        *f = '\0';
        }
        else
          return -1;
        c++;
        break;
      case 'L':
        if (strlen(loc) < (size_t)buflen)
        {
          strcat(f, loc);
          f += strlen(loc);
          buflen -= strlen(loc);
        }
        else
          return -1;
        c++;
        break;
      case 'l':
        if (strlen(loc) < (size_t)buflen)
        {
          s = comp;
          while (s < loc + strlen(loc))
          {
            *f++ = tolower(*s++);
            buflen--;
          }
        *f = '\0';
        }
        else
          return -1;
        c++;
        break;
      case 'N':
        if (strlen(net) < (size_t)buflen)
        {
          strcat(f, net);
          f += strlen(net);
          buflen -= strlen(net);
        }
        else
          return -1;
        c++;
        break;
      case 'n':
        if (strlen(net) < (size_t)buflen)
        {
          s = net;
          while (s < net + strlen(net))
          {
            *f++ = tolower(*s++);
            buflen--;
          }
        *f = '\0';
        }
        else
          return -1;
        c++;
        break;
      case '%':
        if (1 < buflen)
        {
          *f++ = '%';
          buflen--;
        }
        else
          return -1;
        c++;
        break;
      default:
        return -2;
      }
    }
    else
    {
      if (1 < buflen)
      {
        *f++ = *c++;
        buflen--;
      }
      else
        return -1;
    }
  }
  return 0;
}
          
  
/*
 * isMatch: does the file `name' match the `format' string?
 *          Special format characters are introduced by `%'. The special
 *          characters are sScCnNLl%. The letters s or S indicate a station
 *          name which must start with an alphabetic character, contain
 *          alphabetic characters or numbers, be up to 6 characters long,
 *          and for s has no upper-case letters; for S has no lower-case.
 *          Similarly, c or C search for a component name, up to 8 characters,
 *          and n or N search for a network name, up to 8 characters long.
 *          These odd name lengths are base on Earthworm tracebuf packets.
 *          The symbol % must match itself, as do all characters not preceeded
 *          by %.
 *   Returns: 1 on a match
 *            0 on failed match
 *           -1 on bad format string; error is not logged
 */
int isMatch(char *name, char *format)
{
  char *n, *f;
  enum State { norm, pct, ssta, sSTA, scomp, sCOMP, snet, sNET, sloc, sLOC};
  enum State state = norm;
  int ns, nc, nn;

  n = name;
  f = format;
  
  while(*n != 0 && (*f != 0 || state > pct) )
  {
    switch (state)
    {
    case norm:   /* scanning normal characters */
      if (*f == '%')  /* Found the format introducer */
      {
        f++;
        state = pct;
        continue;
      }
      if (*f++ != *n++)  /* regular chars in format must match chars in name */
        return 0;
      break;
    case pct:   /* Looking for format char after '%' */
      switch(*f)
      {
      case 'S':
        f++;
        state = sSTA;
        ns = 0;
        break;
      case 's':
        f++;
        state = ssta;
        ns = 0;
        break;
      case 'C':
        f++;
        state = sCOMP;
        nc = 0;
        break;
      case 'c':
        f++;
        state = scomp;
        nc = 0;
        break;
      case 'N':
        f++;
        state = sNET;
        nn = 0;
        break;
      case 'n':
        f++;
        state = snet;
        nn = 0;
        break;
      case 'L':
        f++;
        state = sLOC;
        nn = 0;
        break;
      case 'l':
        f++;
        state = sloc;
        nn = 0;
        break;
      case '%':
        f++;
        if (*n++ != '%')
          return 0;
        break;
      default:
        return -1;   /* Bad format string */
      }
      break;
    case sSTA:  /* Looking for upper-case station name in name string */
      if (ns == 0)
      {
        if (isupper(*n++))
        {
          ns++;
        }
        else
          return 0;
      }
      else if (isupper(*n) || isdigit(*n))  
      {                       /* don't increment n until *n is checked */
        if (++ns == STA_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case ssta:  /* Looking for lower-case station name in name string */
      if (ns == 0)
      {
        if (islower(*n++))
        {
          ns++;
        }
        else
          return 0;
      }
      else if (islower(*n) || isdigit(*n))  
      {                       /* don't increment n until *n is checked */
        if (++ns == STA_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case sCOMP:   /* Looking for upper-case component name in name string */
      if (nc == 0)
      {              /* First character must be a letter */
        if (isupper(*n++))
        {
          nc++;
        }
        else
          return 0;
      }
      /* remaining chars may be letters or numbers */
      else if (isupper(*n) || isdigit(*n)) 
      {                          /* don't increment n until *n is checked */
        if (++nc == COMP_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case scomp:   /* Looking for lower-case component name in name string */
      if (nc == 0)
      {
        if (islower(*n++))
        {
          nc++;
        }
        else
          return 0;
      }
      else if (islower(*n) || isdigit(*n)) 
      {                          /* don't increment n until *n is checked */
        if (++nc == COMP_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case sLOC:   /* Looking for upper-case location name in name string */
      if (nn == 0)
      {
        if (isupper(*n++))
        {
          nn++;
        }
        else
          return 0;
      }
      else if (isupper(*n) || isdigit(*n) )
      {                           /* don't increment n until *n is checked */
        if (++nn == LOC_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case sloc:   /* Looking for lower-case location name in name string */
      if (nn == 0)
      {
        if (islower(*n++))
        {
          nn++;
        }
        else
          return 0;
      }
      else if (islower(*n) || isdigit(*n) )
      {                           /* don't increment n until *n is checked */
        if (++nn == LOC_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case sNET:   /* Looking for upper-case network name in name string */
      if (nn == 0)
      {
        if (isupper(*n++))
        {
          nn++;
        }
        else
          return 0;
      }
      else if (isupper(*n) || isdigit(*n) )
      {                           /* don't increment n until *n is checked */
        if (++nn == NET_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    case snet:   /* Looking for lower-case network name in name string */
      if (nn == 0)
      {
        if (islower(*n++))
        {
          nn++;
        }
        else
          return 0;
      }
      else if (islower(*n) || isdigit(*n) )
      {                           /* don't increment n until *n is checked */
        if (++nn == NET_LEN)
          state = norm;
        n++;
      }
      else
      {
        state = norm;
      }
      break;
    default:  /* Unknown state */
      return -2;
    }
  }
  if ( *n == *f )
    return 1;
  
  return 0;
  
}

/********************************************************************
 * strappend() append second null-terminated character string to    *
 * the first as long as there's enough room in the target buffer    * 
 * for both strings an the null-byte                                *
 ********************************************************************/
int strappend( char *s1, int s1max, char *s2 )
{
   if( (int)strlen(s1)+(int)strlen(s2)+1 > s1max ) return( -1 );
   strcat( s1, s2 );
   return( 0 );
}
