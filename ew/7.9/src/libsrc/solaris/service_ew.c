/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: service_ew.c 1716 2004-12-06 22:15:00Z mark $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2004/12/06 22:14:33  mark
 *     Added get_service_status
 *
 *     Revision 1.3  2004/08/03 18:48:06  mark
 *     Added dependencies param to install_service
 *
 *     Revision 1.2  2004/07/27 20:04:13  mark
 *     Fixed compile errors/warnings
 *
 *     Revision 1.1  2004/07/13 16:51:33  mark
 *     Initial checkin
 *
 */

#include "service_ew.h"

void install_service(const char *szServiceName, const char *szDisplayName, const char *szDependencies)
{
}

void uninstall_service(const char *szServiceName)
{
}

int start_service(char *szServiceName, LPSERVICE_MAIN_FUNCTION mainfunc)
{
	/* Call mainfunc with 0s as params. */
	mainfunc(0, (char **)0);

	return 0;
}

int init_service(char *szServiceName, int allow_pause, LPHANDLER_FUNCTION handler)
{
	return 0;
}

int set_service_status(int state, int error, int checkpoint, int hint)
{
	return 0;
}

int get_service_status(char *szServiceName, int *pState, int *pError)
{
	return 0;
}

