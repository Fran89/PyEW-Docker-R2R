/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: service_ew.c 5796 2013-08-13 23:10:16Z stefan $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2005/06/08 17:39:03  mark
 *     Added wait_for_dependencies
 *
 *     Revision 1.3  2004/12/06 22:15:00  mark
 *     Added get_service_status
 *
 *     Revision 1.2  2004/08/03 18:48:45  mark
 *     Added dependencies param to install_service
 *
 *     Revision 1.1  2004/07/13 16:47:12  mark
 *     Initial checkin
 *
 */

#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
#include "service_ew.h"
#include "earthworm_simple_funcs.h"

static SERVICE_STATUS serv_status;
static SERVICE_STATUS_HANDLE serv_handle = 0;

/*******************
 * install_service
 *
 * Installs this app as a service.  This only needs to be called once, period;
 * not once per time you run.  After this is called, the service will be set
 * to start automatically at every reboot.
 *
 * Params:
 *	szServiceName - internal name given to this service
 *	szDisplayName - name displayed in the Service Manager
 ******************/
void install_service(const char *szServiceName, const char *szDisplayName, const char *szDependencies)
{
	SC_HANDLE schSCManager, schService;
	char szPath[512];
	char szPathQuoted[514];
	int err;

	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_CREATE_SERVICE); 
	if (schSCManager == NULL)
	{
		DWORD ErrNum;
		ErrNum = GetLastError();
		if (ErrNum == 5)
		{
			fprintf(stderr, "OpenSCManager failed; error code: %d. You must install startstop_service in an Administrator console.", ErrNum);
		}
		else {
			fprintf(stderr, "OpenSCManager failed; error code: %d", ErrNum);
		}
		return;
	}

	
	if (GetModuleFileName(NULL, szPath, 512) == 0)
	{
		fprintf(stderr, "Unable to get binary path; error code: %d", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	szPathQuoted[0]=0;
	strcat(szPathQuoted, "\"");
	strcat(szPathQuoted, szPath);
	strcat(szPathQuoted, "\"");

	schService = CreateService( schSCManager,		/* SCManager database */
			szServiceName,							/* name of service */
			szDisplayName,							/* service name to display */
			SERVICE_ALL_ACCESS,						/* desired access */
			SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,	/* service type */
			SERVICE_AUTO_START,						/* start type */
			SERVICE_ERROR_NORMAL,					/* error control type */
			szPathQuoted,									/* service's binary */
			NULL,									/* no load ordering group */
			NULL,									/* no tag identifier */
			szDependencies,							/* any dependencies */
			NULL,									/* LocalSystem account */
			NULL									/* no password */
	);
	if (schService == NULL) 
	{
		err = GetLastError();
		if (err == ERROR_SERVICE_EXISTS)
		{
			fprintf(stderr, "This service has already been installed.");
		}
		else
		{
			fprintf(stderr, "Unable to install service; error code: %d", GetLastError());
		}
	}
	else
	{
		fprintf(stderr, "%s service successfully installed.", szServiceName);

		CloseServiceHandle(schService); 
	}

	CloseServiceHandle(schSCManager);
}

/*******************
 * uninstall_service
 *
 * Uninstalls this app as a service.
 *
 * Params:
 *	szServiceName - internal name given to this service, used during install_service
 ******************/
void uninstall_service(const char *szServiceName)
{
	SC_HANDLE schSCManager, schService;
	int err;
	
	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager == NULL)
	{
		fprintf(stderr, "OpenSCManager failed; error code: %d", GetLastError());
		return;
	}
	
	schService = OpenService( schSCManager, szServiceName, SERVICE_ALL_ACCESS);
	if (schService == NULL) 
	{
		err = GetLastError();
		if (err == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			fprintf(stderr, "The service has already been uninstalled!  That was easy...");
		}
		else
		{
			fprintf(stderr, "Unable to open service; error code: %d", err);
		}
		CloseServiceHandle(schSCManager);
		return;
	}

	if(!DeleteService(schService)) 
	{
		fprintf(stderr, "Unable to delete service; error code: %d", GetLastError());
	}
	else 
	{
		fprintf(stderr, "%s service successfully removed", szServiceName);
	}

	CloseServiceHandle(schService); 
	CloseServiceHandle(schSCManager);	
}

/******************
 * start_service
 *
 * Starts running the service.  This function should be called within the main() function.
 * Its purpose it to register this app as running with the Windows Service Manager, then
 * call another function which will act as a second main() function.
 *
 * Params;
 *	szServiceName - internal name given to this service, used during install_service
 *	mainfunc - new "main" function.
 * Returns:
 *	0 if all is well, otherwise an error value.  This function will not return until the
 *	"mainfunc" function has completed and returned.
 *****************/
int start_service(char *szServiceName, LPSERVICE_MAIN_FUNCTION mainfunc)
{
	SERVICE_TABLE_ENTRY serviceTable[] =
	{
		{ szServiceName, mainfunc },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(serviceTable))
	{
		fprintf(stderr, "Error %d from StartServiceCtrlDispatcher\n", GetLastError() );
		return 1;
	}

	return 0;
}

/******************
 * init_service
 *
 * Initializes the service.  This function should be called towards the beginning of
 * the mainfunc() function passed to start_service.
 *
 * Params:
 *	szServiceName - internal name given to this service, used during install_service
 *					and start_service
 *	allow_pause - non-zero if this service can pause/continue; zero if it can't
 *	handler - function that is called when the Windows Service Manager needs to issue
 *				STOP, PAUSE, or INTERROGATE commands
 * Returns:
 *	0 if all is well, otherwise an error value.
 ******************/
int init_service(char *szServiceName, int allow_pause, LPHANDLER_FUNCTION handler)
{
    serv_status.dwServiceType        = SERVICE_WIN32_OWN_PROCESS; 
	if (allow_pause == 0)
		serv_status.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	else
		serv_status.dwControlsAccepted   = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE;
    serv_status.dwServiceSpecificExitCode = 0; 

    serv_handle = RegisterServiceCtrlHandler(szServiceName, handler);
	if (serv_handle == 0)
	{
		fprintf(stderr, "Unable to register service control handler; error code: %d", GetLastError());
		return -1;
	}

	return 0;
}

/******************
 * set_service_status
 *
 * Informs the Windows Service Manager (and anybody else who's listening via the WSM)
 * the current state of the service.  See the Win32 SetServiceStatus() function for
 * more details.
 *
 * Params:
 *	state - current state of the service (e.g. SERVICE_RUNNING), or 0 to re-send the
 *			last known state.
 *	error - Win32 error to report, or 0 if everything is fine
 *	checkpoint - current checkpoint for _PENDING states.  Start with a 0 value when you
 *				enter one of these states, then increment by 1 with subsequent set_service_status
 *				calls until you're no longer pending.
 *	hint - how long the Service Manager can expect to wait until you're no longer pending,
 *			or until the next "checkpoint" status is sent (in ms).
 * Returns:
 *	0 if all is well, otherwise an error value.
 ******************/
int set_service_status(int state, int error, int checkpoint, int hint)
{
	if (state != 0)
	{
		serv_status.dwCurrentState       = state; 
		serv_status.dwWin32ExitCode      = error; 
		serv_status.dwCheckPoint         = checkpoint; 
		serv_status.dwWaitHint           = hint;
	}

	if (SetServiceStatus(serv_handle, &serv_status))
		return -1;

	return 0;
}

/******************
 * get_service_status
 *
 * Determines the status of a given service.  This can be used to determine if all service
 * dependencies are running.
 *
 * Params:
 *	szServiceName - internal name given to this service, used during install_service
 *					and start_service (does not have to have registered as an earthworm/hydra
 *					service, but can refer to any service)
 *  pState - return value referring to current service state (e.g. SERVICE_RUNNING)
 *  pError - return value referring to current service error value, or 0 if all is well.
 *
 * Returns:
 *	0 if all is well, otherwise an error value.
 ******************/
int get_service_status(char *szServiceName, int *pState, int *pError)
{
	SC_HANDLE schSCManager, schService;
	SERVICE_STATUS status;
	int retval = 0;

	schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS); 
	if (schSCManager == NULL)
	{
		retval = GetLastError();
		return retval;
	}
	schService = OpenService(schSCManager, szServiceName, GENERIC_READ);
	if (schService == NULL)
	{
		retval = GetLastError();
		CloseServiceHandle(schSCManager);
		return retval;
	}

	if (!QueryServiceStatus(schService, &status))
	{
		retval = GetLastError();
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return retval;
	}

	*pState = status.dwCurrentState;
	*pError = status.dwWin32ExitCode;

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 0;
}

/******************
 * wait_for_dependencies
 *
 * Waits for all of the given dependencies to start running.  This is used when a service has
 * more than one dependency, since Windows only waits for one service to start running rather
 * than all of them.
 *
 * Params:
 *	szDependencyList - List of dependant services given to install_service; each service name
 *			is NULL terminated, and the entire array is double-NULL terminated.
 *  pCheckpoint -	On entry, the next checkpoint to pass to set_service_status.
 *					On exit, the next checkpoint for the calling function to pass to
 *						set_service_status.
 *
 * Returns:
 *	0 if all is well, otherwise an error value.
 ******************/
int wait_for_dependencies(char *szDependencyList, int *pCheckpoint)
{
	char *szDeps = szDependencyList;
	int i, state, err, retval;

	while (*szDeps != 0) // step through all dependencies
	{
		while (TRUE) // wait for each dependency
		{
			set_service_status(SERVICE_START_PENDING, 0, *pCheckpoint, 1500);
			*pCheckpoint++;

			// Get the status of this dependency.
			retval = get_service_status(szDeps, &state, &err);
			if (retval != 0)
			{
				printf("Error %d trying to find status of %s!\n", retval, szDeps);
				return -1;
			}
			// If this is running properly, continue to the next dependency.
			if (state == SERVICE_RUNNING)
				break;

			// Wait for another second for this dependency to come up...
			sleep_ew(1000);
		} // while we wait for a single dependency

		// Skip to just past the next null character.
		i = strlen(szDeps);
		szDeps += (i + 1);
	} // while we step through all dependencies

	return 0;
}
