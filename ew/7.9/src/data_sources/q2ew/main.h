#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include "earthworm_incl.h"
#include "comserv_incl.h"
#include "die.h"
#include "options.h"
#include "misc.h"
#include "misc_seed_utils.h"
#include "externs.h"
#include "heart.h"
#include "convert.h"


/****************************************************************/
/* 

    COMSERV defines and vars : Note, no externs since all is done
	in main() thread
*/

#define DATA_MASK CSIM_DATA|CSIM_MSG /* for this implementation we only 
					care about messages and data */
#define COM_OUT_BUF_SIZE 6000
#define POLL_NANO 250000000

char client_name[5] = "Q2EW" ;
char server_name[5] = "*" ;

static PTR_STA_CLIENT   sta;
PTR_CLIENT       shbuf;
static PTR_DATA         data;

/* some qlib2 defines */

#define MIN_SEED_BLK_SIZE 256
