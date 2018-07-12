/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: threads.c 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
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
