#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>

#include "earthworm_incl.h"
#include "die.h"
#include "options.h"
#include "misc.h"
#include "externs.h"
#include "heart.h"

#include "gcf/include/gcf.h"
#include "gcf_term.h"
#include "gcf_udp.h"     
#include "gcf_input_types.h"

#include "scn_map.h"
#include "convert.h"


#define POLL_NANO 250000000

#define DEFAULT_MSS_PORT 	3001
#define DEFAULT_GCFSERV_PORT 	45670
