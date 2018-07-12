
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pageout.c 6838 2016-10-15 00:08:28Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2006/04/26 19:24:13  dietz
 *     removed some extern variables which are in statmgr.h
 *
 *     Revision 1.3  2006/04/26 00:25:34  dietz
 *     + Modified to allow up to 10 pagegroup commands in statmgr config file.
 *     + Modified descriptor files to allow optional module-specific settings for
 *     pagegroup (up to 10) and mail (up to 10) recipients. Any pagegroup or
 *     mail setting in a descriptor file override the statmgr config settings.
 *     + Modified logfile name to use the name of the statmgr config file (had
 *     been hard-coded to 'statmgr*'.
 *     + Modified logging of configuration and descriptor files.
 *
 *     Revision 1.2  2002/07/09 23:08:49  dietz
 *     added optional command pagegroup to descriptor file.
 *     If it exists, it overrides the pagegroup command in statmgr config
 *
 *     Revision 1.1  2000/02/14 19:39:55  lucky
 *     Initial revision
 *
 *
 */

/*************************************************************************
                                PageOut()

                Send a pager message to the transport ring.

   Will construct a PAGEIT message to the specified pager group
   with the specified message text.

   Returns:
        0 => All went well
       -1 => Error
 *************************************************************************/

#include <stdio.h>
#include <string.h>
#include <transport.h>
#include <earthworm.h>
#include "statmgr.h"

int PageOut( SHM_INFO *region, char group[][MAXRECIPLEN], 
             int ngroup, char *text )
{
   char     msg[250];
   MSG_LOGO logo;
   int      ig;
   int      rc;

/* Set logo values of pager message
 **********************************/
   logo.type   = TypePage;
   logo.mod    = MyModId;
   logo.instid = InstId;

/* For each pagegroup in the list
 ********************************/
   for( ig = 0; ig< ngroup; ig++ )
   {
   /* Assemble pageit message in msg
    ********************************/
      sprintf( msg, "group: %s %s#", group[ig], text );

   /* Write to transport ring
    *************************/
      if ( tport_putmsg( region, &logo, strlen(msg), msg ) == PUT_OK )
         rc = 0;
      else
      {
         logit( "e", "Error sending pageit message to transport region.\n" );
         rc = -1;
      }
   }

   return( rc );
}


/*************************************************************************
                             SendPageHeart()

                 Send a heartbeat to the pageit system
                        via the transport ring.

 *************************************************************************/
int SendPageHeart( SHM_INFO *region, char* sysName )
{
   char     msg[50];
   MSG_LOGO logo;

/* Assemble the heartbeat message
   ******************************/
   strcpy(msg,"alive:");
   strcat(msg,sysName);
   strcat(msg,"#");  /* Pageit's termination symbol */

/* Set logo values of pager message
   ********************************/
   logo.type   = TypePage;
   logo.mod    = MyModId;
   logo.instid = InstId;

/* Write to transport ring
   ***********************/
   if ( tport_putmsg( region, &logo, strlen(msg), msg ) == PUT_OK )
      return( 0 );
   else
   {
      logit( "e", "Error sending pageit hbeat to transport region.\n" );
      return( -1 );
   }
}
