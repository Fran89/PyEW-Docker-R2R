/*! \file
 *
 * \brief Nanometrics Protocol Tool
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxptool_listen.c 4165 2011-01-24 15:19:28Z quintiliani $
 *
 */


/*
** p_server.c -- a stream socket server demo
*/

#include "config.h"

#ifndef HAVE_WINDOWS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#include <nmxp.h>
#include <nmxptool_listen.h>

extern void *nmxptool_print_info_raw_stream(void *arg);
extern void *nmxptool_print_params(void *arg);

/* #define MYPORT 3490	// the port users will be connecting to */

#define BACKLOG 3	 // how many pending connections queue will hold

/*
#include <sys/wait.h>
void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}
*/


#define MAX_LEN_HOSTCLIENT 100
#define MAX_OCC 30
static pthread_mutex_t mutex_occ = PTHREAD_MUTEX_INITIALIZER;
static int occ;
typedef struct {
    int v_new_fd[MAX_OCC];
    int last_command[MAX_OCC];
} DATA_SHARED;


/* return value needs to be freed */
char *nmxptool_command_clean(char *str_command) {
    int len = 0;
    char *command = NULL;

    len = strlen(str_command);

    if(len > 0) {
	command = NMXP_MEM_STRDUP(str_command);
	while(len > 0  &&
		(command[len-1] == '\r'  ||  command[len-1] == '\n'  ||  command[len-1] == ' ' ) ) {
	    len--;
	    command[len] = 0;
	}
    }

    if(len <= 0) {
	if(command) {
	    NMXP_MEM_FREE(command);
	    command = NULL;
	}
    }

    return command;
}

#define MAX_LEN_COMMAND 100
#define MAX_LEN_COMMAND_DESC 300
typedef struct {
    int command;
    char str_command[MAX_LEN_COMMAND];
    char str_command_desc[MAX_LEN_COMMAND_DESC];
} COMMAND_ITEM;

#define COMMAND_NULL    1
#define COMMAND_LIST    2
#define COMMAND_PRINT   3
#define COMMAND_EXIT    4
#define COMMAND_MEM     5
#define COMMAND_RAW     6
#define COMMAND_PARAMS  7
#define COMMAND_HELP    8

#define N_COMMAND       8

const COMMAND_ITEM list_cmd[N_COMMAND] = {
    {COMMAND_NULL,      "", 	""},
    {COMMAND_LIST,      "list", 	"List of the channels."},
    {COMMAND_PRINT,     "print", 	"Print processed packets."},
    {COMMAND_HELP,      "help", 	"Print this help"},
    {COMMAND_MEM,       "mem",		"Print memory size used."},
    {COMMAND_RAW,       "raw",		"Print info about data buffer."},
    {COMMAND_PARAMS,    "params", 	"Print parameter values."},
    {COMMAND_EXIT,      "exit", 	"Exit."}
};

int nmxptool_command(char *str_command) {
    int ret = -1;
    int i = 0;
    int len = 0;
    char *command_clean = NULL;

    command_clean = nmxptool_command_clean(str_command);

    /* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "'%s' ==> '%s'\n", NMXP_LOG_STR(str_command), NMXP_LOG_STR(command_clean)); */

    if(command_clean) {
	len = strlen(command_clean);
	i = 0;
	while(i < N_COMMAND  &&  strcmp(command_clean, list_cmd[i].str_command) != 0) {
	    i++;
	}
	if(i < N_COMMAND) {
	    ret = list_cmd[i].command;
	}
	NMXP_MEM_FREE(command_clean);
    } else {
	ret = COMMAND_NULL;
    }

    return ret;
}

static DATA_SHARED ds = {
    {0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0 },
    {0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0 }
};

int nmxptool_fd_add(int new_fd) {
    int ret_occ;

    pthread_mutex_lock (&mutex_occ);
    ds.v_new_fd[occ] = new_fd;
    ds.last_command[occ] = COMMAND_NULL;
    occ++;
    ret_occ = occ;
    pthread_mutex_unlock (&mutex_occ);

    return ret_occ;
}

int nmxptool_fd_rem(int new_fd) {
    int ret_occ;
    int i;

    pthread_mutex_lock (&mutex_occ);
    ret_occ = 0;
    while(ds.v_new_fd[ret_occ] != new_fd  &&  ret_occ < occ) {
	ret_occ++;
    }
    if(ret_occ < occ) {
	for(i=ret_occ; i < occ-1; i++) {
	    ds.v_new_fd[i] = ds.v_new_fd[i+1];
	    ds.last_command[i] = ds.last_command[i+1];
	}
	occ--;
    } else {
	ret_occ = -1;
    }
    pthread_mutex_unlock (&mutex_occ);

    return ret_occ;
}

int nmxptool_send_ctrl(int fd, char *msg) {
    int ret = 0;
    ret = send(fd, msg, strlen(msg), 0);
    if( ret == -1 ) {
	pthread_mutex_unlock (&mutex_occ);
	perror("send");
	nmxptool_fd_rem(fd);
	close(fd);
	pthread_exit(NULL);
    }
    return ret;
}

static pthread_mutex_t mutex_cur_fd = PTHREAD_MUTEX_INITIALIZER;
static int cur_fd = 0;
int nmxp_log_send_socket(char *msg) {
    int ret = 0;
    if(cur_fd != 0) {
	nmxptool_send_ctrl(cur_fd, msg);
    }
    return ret;
}

int nmxptool_fd_command(int new_fd, int command) {
    int ret_occ;
    int i;
    char str_command_not_found[] = "Command not found!\n";
    char str_tot_mem[30];
    char msg[1024];

    pthread_mutex_lock (&mutex_occ);
    ret_occ = 0;
    while(ds.v_new_fd[ret_occ] != new_fd  &&  ret_occ < occ) {
	ret_occ++;
    }
    if(ret_occ < occ) {
	ds.last_command[ret_occ] = command;
    } else {
	ret_occ = -1;
    }
    pthread_mutex_unlock (&mutex_occ);

    switch(command) {

	case COMMAND_MEM:
	    snprintf(str_tot_mem, 30, "%d\n", NMXP_MEM_PRINT_PTR(0, 1));
	    nmxptool_send_ctrl(new_fd, str_tot_mem);
	    break;

	case COMMAND_LIST:
	    break;

	case COMMAND_RAW:
	case COMMAND_PARAMS:
	    pthread_mutex_lock (&mutex_cur_fd);
	    cur_fd = new_fd;
	    nmxp_log_add(nmxp_log_send_socket, nmxp_log_send_socket);
	    if(command == COMMAND_RAW) {
		nmxptool_print_info_raw_stream(NULL);
	    } else {
		nmxptool_print_params(NULL);
	    }
	    nmxp_log_rem(nmxp_log_send_socket, nmxp_log_send_socket);
	    cur_fd = 0;
	    pthread_mutex_unlock (&mutex_cur_fd);
	    break;

	case COMMAND_HELP:
	    for(i=0; i<N_COMMAND; i++) {
		if(list_cmd[i].command != COMMAND_NULL) {
		    snprintf(msg, 1024, "%-20s %s\n", list_cmd[i].str_command, list_cmd[i].str_command_desc);
		    nmxptool_send_ctrl(new_fd, msg);
		}
	    }
	    break;

	case COMMAND_NULL:
	    break;

	default:
	    nmxptool_send_ctrl(new_fd, str_command_not_found);
	    break;
    }

    return ret_occ;
}


typedef struct {
    int fd;
    char hostclient[MAX_LEN_HOSTCLIENT];
} FD_HC;

void *nmxptool_p_man_sockfd(void *arg) {
    FD_HC *fd_hc = (FD_HC *) arg;
    char command[MAX_LEN_COMMAND];
    int i;
    int last_command = -1;
    char *prompt = "> ";
    char *welcome_message = "Welcome aboard nmxptool! Type 'help' for command list.\n";
    char *last_str_command = NULL;

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY,
			"server: got connection from %s (%d) (%d)\n", fd_hc->hostclient, fd_hc->fd, nmxptool_fd_add(fd_hc->fd));

    pthread_mutex_lock (&mutex_occ);
    nmxptool_send_ctrl(fd_hc->fd, welcome_message);
    pthread_mutex_unlock (&mutex_occ);
	
    while(last_command != COMMAND_EXIT) {

	for(i=0; i<MAX_LEN_COMMAND; i++) {
	    command[i] = 0;
	}

	pthread_mutex_lock (&mutex_occ);
	nmxptool_send_ctrl(fd_hc->fd, prompt);
	/* TODO if error NMXP_MEM_FREE(fd_hc); */
	pthread_mutex_unlock (&mutex_occ);
	
	if(read(fd_hc->fd, command, MAX_LEN_COMMAND) == -1) {
	    /* ERROR */
	}

	if( (last_command = nmxptool_command(command)) != -1 ) {
	    last_str_command = nmxptool_command_clean(command);
	}

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY,
			"server: got command from %s (%d): '%s' (%d) \n", fd_hc->hostclient, fd_hc->fd, NMXP_LOG_STR(last_str_command), last_command);

	if(last_str_command) {
	    NMXP_MEM_FREE(last_str_command);
	    last_str_command = NULL;
	}

	nmxptool_fd_command(fd_hc->fd, last_command);
	
    }

    nmxptool_fd_rem(fd_hc->fd);

    close(fd_hc->fd);
    NMXP_MEM_FREE(fd_hc);
    pthread_exit(NULL);
}

#define MAX_LEN_MSG 1000
int nmxptool_listen_print_seq_no(NMXP_DATA_PROCESS *pd) {
    int ret = 0;
    char str_time[200];
    char msg[MAX_LEN_MSG];
    int i;
    nmxp_data_to_str(str_time, pd->time);

    snprintf(msg, MAX_LEN_MSG, "Process %s.%s.%s %2d %d %d %s %dpts lat. %.1fs\n",
	    NMXP_LOG_STR(pd->network),
	    NMXP_LOG_STR(pd->station),
	    NMXP_LOG_STR(pd->channel),
	    pd->packet_type,
	    pd->seq_no,
	    pd->oldest_seq_no,
	    NMXP_LOG_STR(str_time),
	    pd->nSamp,
	    nmxp_data_latency(pd)
	    );

    pthread_mutex_lock (&mutex_occ);
    for(i=0; i < occ; i++) {
	if(ds.last_command[i] == COMMAND_PRINT) {
	    nmxptool_send_ctrl(ds.v_new_fd[i], msg);
	}
    }
    pthread_mutex_unlock (&mutex_occ);


    return ret;
}

void *nmxptool_listen(void *arg)
{
	int port_socket_listen = *(int*)arg;
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct sockaddr_in my_addr;	// my address information
	struct sockaddr_in their_addr; // connector's address information
	socklen_t sin_size;
	int yes=1;
	FD_HC *fd_hc = NULL;

	pthread_t threads;
	pthread_attr_t attr;
	int rc;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}
	
	my_addr.sin_family = AF_INET;		 // host byte order
	my_addr.sin_port = htons(port_socket_listen);	 // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	/*
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	*/

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}
		fd_hc = NMXP_MEM_MALLOC(sizeof(FD_HC));
		strcpy(fd_hc->hostclient, inet_ntoa(their_addr.sin_addr));
		fd_hc->fd = new_fd;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		rc = pthread_create(&threads, &attr, nmxptool_p_man_sockfd, (void *)fd_hc);
		pthread_attr_destroy(&attr);
		if (rc){
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY,
			    "ERROR; return code from pthread_create() is %d\n", rc);
		    exit(-1);
		}
		/*
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
				perror("send");
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
		*/
	}

	pthread_exit(NULL);
}


#endif



