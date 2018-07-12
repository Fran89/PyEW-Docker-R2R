/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_site.c 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.3  2002/09/10 17:09:40  dhanych
 *     Stable scaffold
 *
 *     Revision 1.2  2002/01/15 21:23:03  lucky
 *     *** empty log message ***
 *
 *     Revision 1.1  2001/04/11 21:05:23  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_site.c : Station parameter routines.
 *   Modified from the libsrc/utils version: strips trailing blanks off
 *   of SCNL strings, allows lookup by SN with C and L empty, since all the 
 *   components should be at the same location.
 *   Changed `name' to `sta' to match SCNL convention.
 *   Removed the `site' command from site_com(), since it read only
 *     station names, not component or net.
 *   Removed site_load() since it seemed unnecessary.
 *   Added calls to qsort() and bsearch() for faster access.
 *   Functions no longer call exit() on error.
 *   Return values standardized to 0 for success, < 0 on error.
 *   fprintf() replaced by logit().
 *
 *  Pete Lombard, October 2000
 * modified for Location codes Feb 2006 - PAF
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <earthworm.h>
#include <kom.h>
#include "lm_site.h"

/* Initialization constants
 **************************/
static int initSite = 0;

static int maxSite  = 1800;  /* default size;                     *
                             ** Use <maxsite> command to increase
                             **
                             ** 20020516 dbh - previous statement
                             **                invalid.  lm_config.c
                             **                never calls site_com(),
                             **                thus never sets maxSite.
                             **                Added function set_maxsite()
                             **                which allows lm_config.c
                             **                to set maxsite when
                             **                it obtains the
                             **                appropriate value.          
                             */
SITE *Site;
int nSite;

/* Internal function prototypes */
static int site_init(void);
static int CompareSiteSCNLs( const void *s1, const void *s2 );

/**************************************************************************
 * site_init()  Allocate the site table                                   *
 *    Returns: 0 on success                                               *
 *            -1 on error                                                 *
 **************************************************************************/
static int site_init(void)
{
  if(initSite)
    return 0;
  nSite = 0;
  Site = (SITE *)calloc(maxSite, sizeof(SITE));
  if (!Site) 
  {
    logit("", "site_init:  Could not allocate site table\n");
    return -1;
  }
  initSite = 1;
  return 0;
}


/**************************************************************************
 *  site_read(filename)  Read in a HYPOINVERSE format, universal station  *
 *                       code file                                        *
 *           Returns: 0 on success                                        *
 *                   -1 on error                                          *
 **************************************************************************/

/* Sample station line:
R8075 MN  BHZ  41 10.1000N121 10.1000E   01.0     0.00  0.00  0.00  0.00 1  0.00
0123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 

note that location code is at position 8 and 9 - paulf

*/

int site_read(char *filename)
{
  FILE  *stafile;
  char   line[256];
  int    dlat, dlon, elev;
  float  mlat, mlon;
  char   comp, ns, ew;
  int    n;

  /* initialize site table */
  if (site_init() < 0)
    return -1;

  /* open station file */
  if( (stafile = fopen( filename, "r" )) == (FILE *) NULL )
  {
    logit("", "site_read: Cannot open site file <%s>\n", filename);
    return -1;
  }

  /* read in one line of the site file at a time */
  while( fgets( line, sizeof(line), stafile ) != (char *) NULL )
  {

    /* see if internal site table has room left */
    if( nSite >= maxSite ) 
    {
      fprintf( stderr,
               "site_read: Site table full; cannot load entire file <%s>\n", 
               filename );
      fprintf( stderr,
               "site_read: Use <maxsite> command to increase table size.\n" );
      return -1;
    }

    /* decode each line and strip trailing blanks */

    strncpy( Site[nSite].sta, &line[0],  5);
    strib(Site[nSite].sta);

    strncpy( Site[nSite].net,  &line[6],  2);
    strib(Site[nSite].net);

    strncpy( Site[nSite].loc,  &line[8],  2);
    if (strcmp(Site[nSite].loc, "  ") == 0) 
    {
	strcpy(Site[nSite].loc, "--");
    }
    strib(Site[nSite].loc);

    strncpy( Site[nSite].comp, &line[10], 3);
    strib(Site[nSite].comp);

    comp = line[9];

    line[42] = '\0';
    n = sscanf( &line[38], "%d", &elev );
    if( n < 1 ) 
    {
      fprintf( stderr,
               "site_read: Error reading elevation from station file line:\n%s\n",
               line );
      continue;
    }

    ew       = line[37];
    line[37] = '\0';
    n = sscanf( &line[26], "%d %f", &dlon, &mlon );
    if( n < 2 ) 
    {
      fprintf( stderr,
               "site_read: Error reading longitude from station file line:\n%s\n",
               line );
      continue;
    }

    ns       = line[25];
    line[25] = '\0';
    n = sscanf( &line[15], "%d %f", &dlat, &mlat );
    if ( n < 2 ) 
    {
      fprintf( stderr,
               "site_read: Error reading latitude from station file line:\n%s\n",
               line );
      continue;
    }

    /*      printf( "%-5s %-2s %-3s %d %.4f%c%d %.4f%c%4d\n",
            Site[nSite].sta, Site[nSite].net, Site[nSite].comp,
            dlat, mlat, ns,
            dlon, mlon, ew, elev ); */ /*DEBUG*/

    /* use one-letter component if there is no 3-letter component given */
    if ( strlen(Site[nSite].comp) == 0)
      sprintf( Site[nSite].comp, "%c", comp );

    /* convert to decimal degrees */
    if ( dlat < 0 ) dlat = -dlat;
    if ( dlon < 0 ) dlon = -dlon;
    Site[nSite].lat = (double) dlat + (mlat/60.0);
    Site[nSite].lon = (double) dlon + (mlon/60.0);

    /* make south-latitudes and west-longitudes negative */
    if ( ns=='s' || ns=='S' )
      Site[nSite].lat = -Site[nSite].lat;
    if ( ew=='w' || ew=='W' || ew==' ' )
      Site[nSite].lon = -Site[nSite].lon;
    Site[nSite].elev = (double) elev/1000.;

    /*      printf("%-5s %-2s %-3s %.4f %.4f %.0f\n\n",
            Site[nSite].sta, Site[nSite].net, Site[nSite].comp,
            Site[nSite].lat, Site[nSite].lon, Site[nSite].elev ); */ /*DEBUG*/

    /* update the total number of stations loaded */
    ++nSite;

  } /*end while*/

  fclose( stafile );

  /* Order the file for rapid access using bsearch() */
  qsort(Site, nSite, sizeof(SITE), CompareSiteSCNLs);
  
  return 0;
}

/**********************************************************************
** set_maxsite() set maxsite without calling site_com()
**********************************************************************/
void set_maxsite( int p_max )
{
   if ( maxSite < p_max )
   {
      maxSite = p_max;
   }
}

/**********************************************************************
 * site_com(): Process a com-style site command: maxsite, site_file   *
 *       Returns: 0 on success,                                       *
 *                1 on unknown command,                               *
 *               -1 on errors                                         *
 **********************************************************************/

int site_com( void )
{
  char *name;

  if(k_its("maxsite")) 
  {
    if(initSite) 
    {
      fprintf( stderr, "site_com:  Error: site table already allocated.\n" );
      fprintf( stderr,
               "site_com:  Use <maxsite> before any <site> or <site_file> commands" );
      return -1;
    }
    maxSite = k_int();
    return 0;
  }

  if(k_its("site_file")) 
  {    /* added command to read in a HYPOINVERSE format */
    name = k_str();     /* "universal code" station file.     950831:ldd */
    if(!name) 
    {
      logit("", "site_com: filename missing from site_file command\n");
      return -1;
    }
    return (site_read(name));
  }

  return 1;
}


/*************************************************************************
 * find_site(sta, comp, net): Returns pointer to the SITE structure, or  *
 *                            NULL if not found                          *
 *************************************************************************/

SITE * find_site(char *sta, char *comp, char *net, char *loc)
{
  static SITE key;
  
  memset(&key, 0, sizeof(key));
  
  if ( site_init() != 0)
    return (SITE *)NULL;
  
  if (sta)
    strncpy(key.sta, sta, sizeof(key.sta));
  if (net)
    strncpy(key.net, net, sizeof(key.net));
  if (comp)
    strncpy(key.comp, comp, sizeof(key.comp));
  if (comp)
    strncpy(key.loc, comp, sizeof(key.loc));

  return ((SITE *)bsearch(&key, Site, nSite, sizeof(SITE), CompareSiteSCNLs));
  
}


/*
 * strib: strips trailing blanks (space, tab, newline)
 *    Returns: resulting string length.
 */
int strib( char *string )
{
  int i, length = 0;
  
  if ( string && (length = strlen( string )) > 0)
  {
    for ( i = length-1; i >= 0; i-- )
    {
      switch ( string[i])
      {
      case ' ':
      case '\n':
      case '\t':
        string[i] = '\0';
        break;
      default:
        return ( i+1 );
      }
    }
  }
  
  return length;
}


/*************************************************************
 *                   CompareSiteSCNLs()                       *
 *                                                           *
 *  This function is passed to qsort() and bsearch().        *
 *  We use qsort() to sort the site list by SCN numbers,     *
 *  and we use bsearch to look up an SCN in the list.        *
 *  If the comp field is empty in either SITE struct, it is  *
 *  ignored.                                                 *
 *************************************************************/
static int CompareSiteSCNLs( const void *s1, const void *s2 )
{
   int rc;
   SITE *t1 = (SITE *) s1;
   SITE *t2 = (SITE *) s2;

   rc = strcmp( t1->sta, t2->sta );
   if ( rc != 0 ) return(rc);
   rc = strcmp( t1->net,  t2->net );
   return(rc);
}
