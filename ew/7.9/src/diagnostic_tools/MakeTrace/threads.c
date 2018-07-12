/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: threads.c 1147 2002-11-25 22:55:49Z alex $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2002/11/25 22:55:49  alex
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:00:43  lucky
 *     Initial revision
 *
 *
 */

   /******************************************************************
    *              Dummy thread functions for adsend.                *
    *  adsend is single-threaded so it must link to these functions. *
    ******************************************************************/


int _beginthread()
{
   return 0;
}


void _endthread()
{
   return;
}
