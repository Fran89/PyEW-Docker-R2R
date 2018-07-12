
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: genericfilter.c 1834 2005-04-26 21:20:40Z dietz $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2005/04/26 21:20:40  dietz
 *     Added timestamping to logging in exportfilter_logmsg()
 *
 *     Revision 1.4  2005/04/22 17:14:18  dietz
 *     added msglen arg to exportfilter_logmsg() to correct logging of msgs.
 *
 *     Revision 1.3  2005/03/23 19:13:01  dietz
 *     Added more logging in "Verbose" mode using the new function
 *     exportfilter_logmsg().
 *
 *     Revision 1.2  2002/07/19 23:01:30  dietz
 *     changed to return EW_PRIORITY_x values.  Fixed bug in exportfilter() that caused
 *     exception on termination ('=' changed to 'memcpy')
 *
 *     Revision 1.1  2000/02/14 17:23:11  lucky
 *     Initial revision
 *
 *
 */

/*
 *  genericfilter.c  contains dummy filter functions
 *                   to be used with export.
 *
 *   981112 Lynn Dietz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <earthworm.h>
#include <kom.h>
#include <priority_level.h>   /* for EW_PRIORITY definitions only */
#include "exportfilter.h"

int FilterInit = 0;  /* initialization flag  */


/*********************************************************
 * exportfilter_com() processes all config-file commands *
 *                    related to the filter code.        *
 * Returns  1 if the command was recognized & processed  *
 *          0 if the command was not recognized          *
 * Note: this function may exit the process if it finds  *
 *       serious errors in any commands                  *
 *********************************************************/
int exportfilter_com(void)
{
   return( 0 );
}


/*********************************************************
 * exportfilter_init() Make sure all the required        *
 *  commands were found in the config file, do any other *
 *  startup things necessary to work properly            *
 *********************************************************/
int exportfilter_init(void)
{
   FilterInit = 1;
   return( 0 );
}


/**********************************************************
 * exportfilter() looks at the candidate message.         *
 *                Analyzes it and possibly reformats it.  *
 * Returns: EW_PRIORITY_DEF  if the resulting message is  *
 *                           to be exported               *
 *          EW_PRIORITY_NONE otherwise                    *
 **********************************************************/
int exportfilter( char  *inmsg,  long inlen,   unsigned char  intype, 
                  char **outmsg, long *outlen, unsigned char *outtype )
{
    if(!FilterInit) exportfilter_init();

    memcpy( *outmsg, inmsg, inlen );
   *outlen  = inlen;
   *outtype = intype;

   /*printf("genericfilter: accepting msgtype:%d  msg:%s\n",
           intype, inmsg);*/ /*DEBUG*/

    return( EW_PRIORITY_DEF );
}

/**********************************************************
 * exportfilter_logmsg()  simple logging of message       *
 **********************************************************/
void exportfilter_logmsg( char *msg, int msglen, 
                          unsigned char msgtype, char *note )
{
   char tmpstr[100];
   int endstr = (int)sizeof(tmpstr)-1;
   if( msglen < endstr ) endstr = msglen;
   strncpy( tmpstr, msg, endstr );
   tmpstr[endstr] = 0;
   logit("t","%s t%d %s\n", note, (int)msgtype, tmpstr );

   return;
}

/**********************************************************
 * exportfilter_shutdown()  frees allocated memory and    *
 *         does any other cleanup stuff                   *
 **********************************************************/
void exportfilter_shutdown(void)
{
   return;
}


