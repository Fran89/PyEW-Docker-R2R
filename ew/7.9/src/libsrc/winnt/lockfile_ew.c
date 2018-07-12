
#include <stdio.h>
#include <windows.h>


#define WIN_MAX_LOCKS 256
static int free_handle=1;
static HANDLE lock_handles[WIN_MAX_LOCKS];

int ew_lockfile_os_specific(char * fname) {

int fd;
DWORD  dw=0;

	fd = free_handle;

	if (fd == WIN_MAX_LOCKS) {
 		fprintf(stderr, "ew_lockfile():  unable to lock %s, %d files already locked in this process .\n", fname, WIN_MAX_LOCKS);
        	return (-1);
	}

    	lock_handles[fd] = CreateFile(fname, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);                   
    	if (lock_handles[fd] == INVALID_HANDLE_VALUE) {
 		fprintf(stderr, "ew_lockfile():  unable to open file %s for locking.\n", fname);
        	return (-1);
    	}
	if (LockFile(lock_handles[fd], dw, 0, dw+512, 0) == 0) {
		fprintf(stderr, "ew_lockfile(): %s already locked by another instance.\n", fname);
		CloseHandle(lock_handles[fd]);
        	return (-1);
 	}
	free_handle++;
 	return(fd);
}
int ew_unlockfile_os_specific(int fd) {
DWORD  dw=0;

	if (UnlockFile(lock_handles[fd], dw, 0, dw+512, 0)==0) {
 		fprintf(stderr, "ew_unlockfile():  unable to unlock fd %d.\n", fd);
		return(-1);
	}

	CloseHandle(lock_handles[fd]);
	return(1);
}
