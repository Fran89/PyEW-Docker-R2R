#ifndef GCF_UDP_H
#define GCF_UDP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "gcf.h"

/* default gcfserv UDP port */
#define DEFAULT_PORT 45670
#define DEFAULT_SCREAM_PORT 1567

/* the following are types returned from gcf_udp_client_recv() */
#define GCF_SCREAM 1
#define GCF_UNCOMPRESSED 2
#define GCF_CMD 3

#define GCF_SERVER_RESTART_TIMEOUT 60

/* the base command set for the DM24 */
#define GCF_NULL 0
#define GCF_SEND_DATA 1
#define GCF_STOP_DATA 2
#define GCF_OBTAIN_CONTROL 3
#define GCF_DM24_CMD 4
#define GCF_DM24_OCCUPIED 5
#define GCF_RELEASE_CONTROL 6
#define GCF_UDP_ACK 7
#define GCF_UDP_PING 8
#define GCF_SERVER_CLOSED 9
#define GCF_SEND_UNCOMPRESSED_DATA 10
#define GCF_AUTH_DENIED 11



/* data transmitted from Scream 3.1 */

typedef struct gcf_scream_udp_packet {
	char 	gcf_block[GCF_BLOCK];
	int	version;
	int	id_strlen;
	char 	id_str[33];
	unsigned short 	sequence;
	int	byte_order;	/* 1 = motorola 2= intel */
	int 	type;		/* type for gcfhdr_read() call arg */
} GCFscreamudp;

/* the following is as of Scream 3.1 */
#define GCF_SCREAM_UDP_SIZE 1063
#define GCF_SCREAM_ID_SIZE 32
#define SCREAM_VERSION 31

#define GCF_SCREAM_MOTOROLA 	1
#define GCF_SCREAM_INTEL 	2

/* make sure the next array is only used once */

#ifdef USE_CMD_STRS

	/* for now these are 4 char cmds preceeded by GCF */
static char *gcf_udp_cmds[] = {
		"GCFNULL", 
		"GCFSEND", 
		"GCFSTOP",
		"GCFCTRL",
		"GCFCMD ",
		"GCFOCCU",
		"GCFREL ",
		"GCFACKN",
		"GCFPING",
		"GCFNOSV",
		"GCFSDUN",
		"GCFNOAU"
};

/* 2000.349 - changed GCFACK to GCFACKN to support Murray's changes of HNY protocol 
	used by PULL Client 
*/

#define GCF_NUM_UDP_CMDS  (sizeof(gcf_udp_cmds)/sizeof(char*))
#endif

#define MAXGCFUDPCMD 9

/* 
	the GCFudpclient structure below is all the information 
	necessary that a server should cache on a client. Eventually,
	this will also have a TTL - time to live field for a duration
	over which the client's command should be maintained.
*/

typedef struct udp_client {
	struct sockaddr_in fsin;	/* socket address */
	int fd;				/* output file descriptor */
	int cmd;			/* client requested command */
	int native_byte_order;		/* client's native byte order */
	struct udp_client *next;	/* the next client in the list */
	int last_packet_was_error;	/* flag for next member */
	int consecutive_send_to_errors;	/* the number of consecutive send_to_errors */
	int total_send_to_errors;	/* the total number of send_to_errors */
	int total_packets_sent;		/* the total number of packets sent */
	int since_packets_sent;		/* the number of packets sent since last data cmd */
	char *hostname;			/* the hostname of the client */
} GCFudpclient;

/* the following structure is for MULTI_THREADED servers
	using POSIX threads (PTHREADS) to pass two arguments
	to the gcf_scream_ () calls.
*/

typedef struct scream_args {
	GCFudpclient *client; 
	char *packet;
} SCREAM_ARGS;

#ifdef STDC

int gcf_udp_server_init(int port, int block);
GCFudpclient * gcf_udp_server_request(int fd); 
int gcf_udp_sendtoclient_uncompressed( GCFudpclient *client, GCFhdr *hdr );
int gcf_udp_sendtoclient_scream(GCFudpclient *client, char *out_scream_packet);
void  gcf_udp_sendtoclient_scream_pthread(void *scream_args);
int gcf_udp_notifyclient( GCFudpclient *client, int cmd);
int gcf_udp_client_recv(int fd, void **ptr);
int gcf_udp_client_init(char *host, int port);
int gcf_mss_client_init(char *host, int port, int ms_timeout);
int gcf_udp_client_sendcmd(int fd,  int cmd);
int gcf_udp_client_sendrecvcmd(int fd,  int sendcmd, void **result_ptr);
int gcf_udp_ping(int fd);
int get_udpcmd(char str[], GCFudpclient *client);
int get_udpcmd_num(char str[]);
int gcf_tcp_server_control_init(int block);
int gcf_tcp_server_control_accept(int fd);
int gcf_tcp_client_connect(char * host, int port);
int gcf_redo_socket(int fd);
int gsockread(int fd, char *trans_buf, int buf_max, int timeout_msecs);

/* scream calls */
int gcf_scream_server_init(char *host, int port);
int gcf_enable_broadcast(int fd);
int gcf_construct_scream_packet(GCFhdr *in_hdr, 
	char * in_gcf_block, char * out_scream_packet, int out_byte_order, 
	int com_port);
void gcf_decode_scream_packet( GCFscreamudp * scream_packet, char udp_buf[]);
int gcf_scream_server_send(int out_fd, char * out_scream_packet);
int gcf_scream_client_init(char *host, int port);
int gcf_scream_client_recv(int fd, GCFhdr *hdr);
int gcf_scream_client_recv2(int fd, GCFscreamudp **scream_pack);
int gcf_scream_client_tcp_conn_init(int *oldest_sequence);
int gcf_scream_client_tcp_get_seq(int sock_fd, unsigned short seq, GCFscreamudp **scream_packet);
int get_last_scream_sender_port();
char * get_last_scream_sender_name();

/* client struct managment funcs */
int gcf_client_add(GCFudpclient *client);
int gcf_client_del(GCFudpclient *client);
void gcf_client_free(GCFudpclient *client);

/* get the comm error string */
char * gcf_getcomm_error();

#else

int 		gcf_udp_server_init();
GCFudpclient * 	gcf_udp_server_request();
int 		gcf_udp_sendtoclient_uncompressed();
int 		gcf_udp_sendtoclient_scream();
void  		gcf_udp_sendtoclient_scream_pthread();
int 		gcf_udp_notifyclient();
int 		gcf_udp_client_init();
int 		gcf_mss_client_init();
int 		gcf_udp_client_recv();
int 		gcf_udp_client_sendcmd();
int 		gcf_udp_client_sendrecvcmd();
int 		gcf_udp_ping();
int 		get_udpcmd();
int 		get_udpcmd_num();
int 		gcf_tcp_server_control_init();
int 		gcf_tcp_server_control_accept();
int 		gcf_tcp_client_connect();

int 		gcf_scream_server_init();
int 		gcf_enable_broadcast();
int 		gcf_construct_scream_packet();
void 		gcf_decode_scream_packet(); 
int 		gcf_scream_server_send();
int 		gcf_scream_client_init();
int 		gcf_scream_client_recv();
int 		gcf_scream_client_recv2();
int 		gcf_scream_client_tcp_conn_init();
int 		gcf_scream_client_tcp_get_seq();

int 		gcf_client_add();
int 		gcf_client_del();
void 		gcf_client_free();
int 		gcf_redo_socket();
int 		gsockread();

int 		get_last_scream_sender_port();
char * 		get_last_scream_sender_name();
char * 		gcf_getcomm_error();

#endif


#endif /*GCF_UDP_H*/
