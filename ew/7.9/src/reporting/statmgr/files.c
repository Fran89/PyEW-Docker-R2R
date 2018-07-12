
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: files.c 2145 2006-04-26 00:25:34Z dietz $
 *
 *    Revision history:
 *     $Log$
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

      /**************************************************************
       *                           files.c                          *
       *                                                            *
       * Functions to print descriptor files & configuration file.  *
       **************************************************************/

#include <stdio.h>
#include <time.h>
#include <earthworm.h>
#include "statmgr.h"

            /****************************************
             *               printdf                *
             *                                      *
             * Function to print descriptor arrays. *
             ****************************************/
void PrintDf( DESCRIPTOR *desc, int ndesc )
{
   int i, j;

   logit( "", "\n       *****  Descriptor Files  *****\n" );

   logit( "", "---------------------------------------------------\n" );
   for( i=0; i<ndesc; i++ )
   {
      logit( "", "# Descriptor file #%d\n", i+1 );
      logit( "", "# Module description:\n" );
      logit( "", " modName    %s\n",   desc[i].modName   );
      logit( "", " modId      %u\n",   desc[i].modId     );
      logit( "", " instId     %u\n",   desc[i].instId    );
      logit( "", " system     %s\n",   desc[i].sysName   );
      logit( "", "# Module-specific pagegroups (%d):\n", desc[i].npagegroup );
      for( j=0; j<desc[i].npagegroup; j++ ) {
        logit( "", " pagegroup  %s\n", desc[i].pagegroup[j] );
      }
      logit( "", "# Module-specific email recipients (%d):\n", desc[i].nmail );
      for( j=0; j<desc[i].nmail; j++ ) {
        logit( "", " mail       %s\n", desc[i].mail[j] );
      }

      logit( "", "# Heartbeat:\n" );
      logit( "", " tsec: %d page: %d mail: %d\n",
             desc[i].hbeat.tsec,
             desc[i].hbeat.page,
             desc[i].hbeat.mail );

      logit( "", "# Errors:\n" );
      for( j=0; j<desc[i].nerr; j++ )
      {
         logit( "", " err: %hd nerr: %d tsec: %d page: %d mail: %d\n",
               desc[i].err[j].err,
               desc[i].err[j].nerr,
               desc[i].err[j].tsec,
               desc[i].err[j].page,
               desc[i].err[j].mail );
         logit( "", " text: \"%s\"\n", desc[i].err[j].text );
      }
      logit( "", "---------------------------------------------------\n" );
   }
   return;
}

             /*******************************************
              *                PrintCnf                 *
              *                                         *
              *  Function to print configuration file.  *
              *******************************************/
void PrintCnf( CNF cnf )
{
   int i;

   logit( "", "\n       *****  Statmgr Configuration file  *****\n" );

   logit( "", " RingName        %s\n", cnf.ringName );

   logit( "", " heartbeatPageit %d\n", cnf.heartbeatPageit );

   logit( "", " LogFile         %d\n", cnf.logswitch );

   logit( "", "System-wide pagegroups (%d):\n", cnf.npagegroup );
   for( i = 0; i < cnf.npagegroup; i++ ) {
      logit( "", " pagegroup       %s\n", cnf.pagegroup[i] );
   }

   logit( "", "System-wide email recipients (%d):\n", cnf.nmail );
   for( i = 0; i < cnf.nmail; i++ ) {
      logit( "", " mail            %s\n", cnf.mail[i] );
   }
   return;
}
