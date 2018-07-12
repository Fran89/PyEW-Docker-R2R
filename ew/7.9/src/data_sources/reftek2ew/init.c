/* @(#)init.c	1.2 07/01/98 */
/*======================================================================
 *
 * Initialization routine
 *
 *====================================================================*/
#include "import_rtp.h"

#define MY_MOD_ID IMPORT_RTP_INIT

/* Log exit status (for errors prior to log facility in place) */

static VOID fexit(INT32 status)
{
    fprintf(stderr, "exit %ld\n", status);
    exit((int) status);
}

#define BUFLEN 1024

VOID init(INT32 argc, CHAR **argv, struct param *par)
{
SHM_INFO *shm;
INT32 status, buflen = BUFLEN;
CHAR buffer[BUFLEN];

#ifdef WINNT
WSADATA lpWSAData;
    if (WSAStartup(MAKEWORD(2,2), &lpWSAData) != 0) {
        perror("WSAStartup");
        exit(1);
    }
#endif /* WINNT */

/* Initialize the global earthworm mutex */

    CreateMutex_ew();

/* Set the program name */
   par->prog = argv[0];
/* Get InstId from the environment */

    if ((status = GetLocalInst(&par->InstId)) < 0) {
        fprintf(stderr, "%s: FATAL ERROR: GetLocalInst error %d\n",
            argv[0], status
        );
        fexit(MY_MOD_ID + 1);
    }

/* Require the name of the parameter file as the single argument */

    if (argc != 2) {
        fprintf(stderr, "usage: %s param_file\n", argv[0]);
        fprintf(stderr, "%s: FATAL ERROR: illegal comand line\n",
            argv[0]
        );
        fexit(MY_MOD_ID + 2);
    }

/* Start the logging facility(s) */

    logit_init(argv[1], 0, 1024, 1);

/* Load the parameter file */

    if (!read_params
        (
            argv[0], argv[1], buffer, buflen, par
        )
    ) fexit(MY_MOD_ID + 3);

/* re-Start the logging facility(s) */

    logit_init(argv[1], par->Mod, 1024, 1);
    log_params(argv[0], argv[1], par);

	if (par->debug) {
		rtp_loginit(NULL, -1, NULL, argv[0]);
		rtp_loglevel(par->debug);
	}

/* Attach to the output ring(s) */

    if (par->RawRing.defined) {
        tport_attach(&par->RawRing.shm, par->RawRing.key);
        logit("t", "attached to %s\n", par->RawRing.name);
    }

    if (par->WavRing.defined) {
        tport_attach(&par->WavRing.shm, par->WavRing.key);
        logit("t", "attached to %s\n", par->WavRing.name);
    }

/* Initialize packet forwarders */

    if (!init_senders(par)) terminate(MY_MOD_ID + 4);

/* Start heartbeat thread */

    if (par->WavRing.defined) {
        shm = &par->WavRing.shm;
    } else if (par->RawRing.defined) {
        shm = &par->RawRing.shm;
    }

    if (!start_hbeat(shm, par)) terminate(MY_MOD_ID + 5);
    if (!notify_init(shm, par)) terminate(MY_MOD_ID + 6);
}
