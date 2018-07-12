/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_site.h 2094 2006-03-10 13:03:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2006/03/10 13:03:28  paulf
 *     upgraded to SCNL version 2.1.0, tested at Utah
 *
 *     Revision 1.2  2002/09/10 17:09:40  dhanych
 *     Stable scaffold
 *
 *     Revision 1.1  2001/04/11 21:05:23  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_site.h : Network parameter definitions.
 *
 */
#ifndef LM_SITE_H
#define LM_SITE_H

/* Define the structure that will hold the site table
 ****************************************************/
typedef struct {
        char    sta[6]; 
        char    net[3]; 
        char    comp[4];
        char    loc[3];
        char    staname[50];
        int     chanid;
        double  lat;
        double  lon;
        double  elev;
} SITE;

/* Prototypes for functions in site.c
 ************************************/
void set_maxsite( int );   /* set maxsite without calling site_com() */
int  site_com  ( void );                  /* process recognized commands   */
int  site_read ( char * );                /* read in a HYPOINV site file   */
SITE *find_site( char *, char *, char *, char *); /* Return pointer to a matching *
                                            * SITE structure               */
int strib( char* );


#endif
