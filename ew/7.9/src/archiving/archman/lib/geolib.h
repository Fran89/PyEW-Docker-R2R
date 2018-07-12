/* geolib.h
 *
 * include file for geomagnetic library functions
 *
 * all constant definitions begin with GEO_ to indicate that they are part
 * of this library and to prevent usuage of commonly used definitions
 * (such as TRUE and FALSE).
 */

/* this is a cut down version of the file found on QNX sdas systems */

/* prevent double inclusion */
#ifndef __GEOLIB_DEF_
#define __GEOLIB_DEF_

/* stdio.h  and others required for function prototypes */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <float.h>
#include <stdlib.h>
#include <time.h>

/*  #ifndef WIN32 */
#ifdef _SOLARIS
#include <mqueue.h>
#include <unistd.h>
#include <sys/ipc.h>
#endif

/******************************************************************************/
/*** things added for WINDOWS ***/
#ifdef WIN32
typedef int pid_t;
typedef short int mqd_t;

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

int init_winsock (void);
#endif
/*** end of things added for WINDOWS ***/
/******************************************************************************/

/* we don't want to use debugging memory allocation in EW */
#define NO_DEBUG_ALLOC

/* these have different names QNX */
#ifndef MAXFLOAT
#define MAXFLOAT	FLT_MAX
#endif
#ifndef MINFLOAT
#define MINFLOAT	FLT_MIN
#endif

/* standard boolean definitions */
#define	GEO_TRUE	1
#define GEO_FALSE	0

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* null string pointer */
#define GEO_NULL_STRING	(char*)0

/* general error code when GEO_FALSE cannot be used */
#define GEO_ERROR	-1

/* definitions for debugging memory allocation */
#ifndef NO_DEBUG_ALLOC
#define calloc(a,b)		geolib_debug_calloc((a),(b))
#define free(a)			geolib_debug_free((a))
#define malloc(a)		geolib_debug_malloc((a))
#define realloc(a,b)	geolib_debug_realloc((a),(b))
#endif

/* the value used to indicate a "missing data point" */
#define MISSING_DATA_VALUE	999999l

#define GEO_INTERMAG_LEN	126	/* length of an intermagnet packet */

/* this structure is used by the queue management routines */
struct Queue_details
{
  char entry_name [40];
  char cmd_name [60];
  char data_name [60];
  char queue_dir [100];
};

/* file type used in File_info below */
#define FILE_NORMAL	1
#define FILE_DIR	2
#define FILE_OTHER	3

/* structure to hold the description of a file */
struct File_info
{
  char name [200];
  long size;
  time_t creation_time;
  time_t modification_time;
  time_t access_time;
  int file_type;
};

/* these constants are used by the queue management routines */
/* the following is the name of the environment variable
 * that holds the name of the queue files */
#define GEO_QUEUE_DIR_NAME	"GIN_BIN_QUEUE_DIR"
/* the following a prefix and suffix values for temporary
 * files in the queue directory */
#define GEO_QUEUE_PREFIX	"queue_"
#define GEO_QUEUE_DATA_SUFFIX	".dat"
#define GEO_QUEUE_CMD_SUFFIX	".cmd"

/* this structure is used by the get_date routine */
struct Date
{
  int sec, min, hour;
  int day, month, year;
};

/* the data type of the contents of each node in the "tree" routines */
#define DATA_TYPE	char

/* the node structure */
struct Node
{
  struct Node *next;		/* linked list of the nodes at this level */
  struct Node *child;		/* link to next level */
  int ret_val;			/* return value - if the link to the next
				 * level is blank then this is a leaf node
				 * and should have a return value */
  DATA_TYPE contents;		/* the contents of this node */
};

/* an empty tree */
#define NULL_TREE	((struct Node *)0)

/* symbols for different types of match when walking a tree */
#define NO_MATCH	0
#define PARTIAL_MATCH	1
#define COMPLETE_MATCH	2

/* symbols used with the command tail processor */
#define NOT_OPT		0
#define NO_PAR		1
#define OPT_PAR		2
#define MAND_PAR	3
#define SYNONYM		4
#define HELP_PAR	5
#define END_PAR		-1

/* the structure used to describe the command line */
struct Command_line_details
{
  char letter;		/* the letter used to mark this option*/
  int params;		/* what parameters are allowed */
  int repeat;		/* repeat count OR synonym substitute */
};

/* the structure used to describe standard stream re-directions
 * when using run_programme () - to use this with run-programme
 * declare a three element array:
 *	struct Redirection redir[3]
 * element 0 corrsponds to stdin, 1 to stdout, 2 to stderr */
struct Redirection
{
  char *name;		/* name of file to redirect to or NULL
			 * to use already open file (from handle) */
  int append;		/* if TRUE, append, otherwise create
			 * (not used on input streams) */
  int handle;		/* handle to an open file -
 			 * if name is blank and handle is
			 * not negative then use handle
			 * for redirection */
};

/* dereferencing constants for a three element redir array */
#define REDIR_ARRAY_LEN		3
#define REDIR_STDIN		0
#define REDIR_STDOUT		1
#define REDIR_STDERR		2

/* return values from the parse_date and parse_time routines */
#define PDATE_ERR	0
#define PDATE_DAY_OK	1
#define PDATE_MONTH_OK	2
#define PDATE_OK	3
#define PTIME_ERR	0
#define PTIME_HOUR_OK	1
#define PTIME_MINUTE_OK	2
#define PTIME_OK	3
#define PTIME_SECOND_OK	4

/* operation codes for ftp () */
#define FTP_GET		1
#define FTP_PUT		2
#define FTP_APPEND	3

/* return codes for ftp () */
#define FTP_OK			0
#define FTP_PARAMETER_ERROR	1
#define FTP_LOCAL_FILE_ERROR	2
#define FTP_TEMP_FILE_ERROR	3
#define FTP_ERROR		4
#define FTP_BAD_HOST		5
#define FTP_LOGIN_REJECTED	6
#define FTP_APPEND_ERROR	7

/* codes for ftp2 */
/* return values from public routines */
#define FTP2_OK			0
#define FTP2_UNKNOWN_HOST	1
#define FTP2_NO_CONNECT		2
#define FTP2_UNAVAILABLE	3
#define FTP2_BAD_USER		4
#define FTP2_NOT_OK		5
#define FTP2_SERVER_SHUTDOWN	6
#define FTP2_COMM_ERR		7
#define FTP2_SYSTEM_FAULT	8
#define FTP2_TYPE_NOT_SUP	9
#define FTP2_NOT_LOGGED_IN	10
#define FTP2_BAD_FILE		11
#define FTP2_MISSING_FILE	12
#define FTP2_REMOTE_FILE	13
#define FTP2_LOCAL_FILE		14
#define FTP2_NO_SPACE		15
#define FTP2_NO_MEM		16
/* bit mapped codes for the ftp_send_file  and ftp_receive_file command
 * parameter - note that some combinations are incompatible:
 * FTP_APPEND | FTP_GENERATE_NAME - can't generate a name for existing file */
#define FTP2_CREATE		0x00	/* create/overwrite or create/append */
#define FTP2_APPEND		0x01
#define FTP2_USE_GIVEN_NAME	0x00	/* use given name or generate a name */
#define FTP2_GENERATE_NAME	0x02
#define FTP2_TYPE_ASCII		0x00	/* ASCII or IMAGE type */
#define FTP2_TYPE_IMAGE		0x04
/* bit positions to decode the above command codes */
#define FTP2_BIT_COMMAND	0x01	/* bit position of command */
#define FTP2_BIT_NAME		0x02	/* bit position of name generation */
#define FTP2_BIT_TYPE		0x04	/* bit position of type */

/* codes for telnet */
#define TELNET_OK		0
#define TELNET_UNKNOWN_HOST	1
#define TELNET_NO_CONNECT	2
#define TELNET_UNAVAILABLE	3
#define TELNET_BAD_USER		4
#define TELNET_SYSTEM_FAULT	5
#define TELNET_COMM_ERR		6
#define TELNET_BAD_FILE		7
#define TELNET_NOT_LOGGED_IN	8

/* return codes for some of the network functions - these MUST be -ve */
#define NET_DISCONNECT		-1
#define NET_ERROR			-2
#define NET_CONNECT_PARENT	-3
#define NET_CONNECT_CHILD	-4
#define NET_TIMEOUT			-5
#define NET_RESOURCE_ERR	-6

/* codes for the error message passing functions in errmsg.c */
/* the severity codes (least severe first) - these codes may be used
 * to de-reference an array so they must be contiguous, starting from
 * 0 - ERR_NCODES holds the number of codes */
#define ERR_UNKNOWN	0
#define ERR_DEBUG	1
#define ERR_INFO	2
#define ERR_WARNING	3
#define ERR_ERROR	4
#define ERR_FATAL	5
#define ERR_NCODES	6
/* the destination for errors */
#define ERR_STDERR	1
#define ERR_LOGGER	2

/* the error logs created by the errlog demon can be read into
 * this structure */
struct Errlog
{
  int day, month, year;
  int hour, min, sec;
  char hostname [50];
  char utility_name [50];
  pid_t pid;
  int severity;				/* see ERR_xxx codes in geolib.h */
  char message [150];
};

/* codes for the return value from copy_file () */
#define COPY_OK			0
#define COPY_SELF		1
#define COPY_BAD_SOURCE		2
#define COPY_DEST_EXISTS	3
#define COPY_BAD_DEST		4
#define COPY_WRITE_ERR		5

/* codes for the return value from substitute_variables() */
#define SV_RUNNING		-1
#define SV_OK			0
#define SV_BAD_VAR		1
#define SV_GRAMMER		2

/* macro functions */
#define year_with_century(y)    ((y>99) ? y : ((y>50) ? y+1900 : y+2000))
#define days_in_year(y)         (is_leap_year(y)?366:365)

/* the encrypt and decrypt routines have been renamed using the
 * prefix gin_ to avoid a name clash on linux - redefine them
 * here */
#define encrypt(s)		gin_encrypt(s)
#define decrypt(s)		gin_decrypt(s)

/* the following are used with the lpt port routines */
#define LPT_IN_BUSY			1
#define LPT_IN_ACK			2
#define LPT_IN_PE			3
#define LPT_IN_SLCT			4
#define LPT_IN_ERROR		5

#define LPT_IRQ_EN			1
#define LPT_OUT_SLCT_IN		2
#define LPT_OUT_INIT    	3
#define LPT_OUT_AUTO_FD_XT	4
#define LPT_OUT_STROBE		5
#define LPT_OUT_ALL			6
#define LPT_DIRECTION		7

#define LPT_ERROR			-1
#define LPT_BIT_CLEAR		0
#define LPT_BIT_SET			1
#define LPT_BIT_TOGGLE		2

/* this structure is used by the ring buffer routines */
struct Ring_buffer
{
  void *data;
  int element_size;
  int n_elements;
  int in_pointer;
  int out_pointer;
};

/* this structure is used to load printers */
struct Printer_Definition
{
  char *name;		/* name of the printer */
  char *command;	/* command for the printer - it must accept
					 * data on stdin */
  char *memory;		/* the memory used for the name and command -
					 * can be freed when the definitions are removed */
};

/* the following is used as a flag to indicate to data -
 * its value must be outside the range of data for a
 * seismic data value */
#define PRINT_DATA_NONE				65536L


/* return codes from get_pci_basic_info */
#define GPCI_OK				0
#define GPCI_NO_PROG		1
#define GPCI_NO_BIOS		2
#define GPCI_NO_CARD		3
#define GPCI_SYS_FAULT		4

/* this structure is used to hold information retrieved from the get_pci
 * program */
struct Pci_basic_info
{
  int vendor_id;		/* the vendors unique id */
  int device_id;			/* the unique id for the device */
  int card_index;		/* 0 for first card installed, 1 for 2nd ... */

  int bus_no;			/* the PCI bus for the card */
  int device_no;		/* the device/function number for the card */

  int irq;				/* the hardware interrupt (if any) */
  int base_address [6];	/* base address registers */
};

/* process priority codes for config_backgr() - these
 * codes are arbitary numbers, config_backgr() converts
 * them to operating system priorities */
#define PRIO_HIGHEST			9
#define PRIO_IRQ_HANDLER		8
#define PRIO_DEV_HANDLER		7
#define PRIO_SER_HANDLER		6
#define PRIO_ACQUISITION		5
#define PRIO_SUPER_NORMAL		4
#define PRIO_NORMAL				3
#define PRIO_SUB_NORMAL			2
#define PRIO_LOWEST				1
#define PRIO_NO_CHANGE			-1

/* bit mapped options for config_backgr2() */
#define CB_CLOSE_STDIN			0x01
#define CB_CLOSE_STDOUT			0x02
#define CB_CLOSE_STDERR			0x04

/* filter types for the IIR_Filter.type field */
#define IIR_ALL_PASS		0
#define IIR_LOW_PASS		1
#define IIR_HIGH_PASS		2
#define IIR_BAND_PASS		3
#define IIR_BAND_REJECT		4

/* IIR time series data types */
#define IIR_INT			1
#define IIR_FLOAT		2
#define IIR_DOUBLE		3

/* a structure to hold an IIR filter, including coefficients from previous filter
 * use, allowing the filter to be used with data in packets or segments */
struct IIR_Filter
{
  int order;			/* the filter order (0 - 10) */
  int type;			/* see constants above */
  double flow;			/* low cutoff frequency in Hz */
  double fhigh;			/* high cutoff frequency in Hz */
  double tdel;			/* sampling interval in seconds */
  int zero_phase;		/* flag - if TRUE apply filter in reverse over
				 *        data to achieve 0 phase shift */
  double missing_data;	/* this contains the value used to indicate
						 * a gap in the data */

  int nsects;			/* number of filter sections */
  double *sn, *sd;		/* filter coefficients */

  double *x1, *x2, *y1, *y2;	/* intermediate results stored for processing
								 * data in packets or segments */
};

/* destination codes for log_msg() (must be bit-mapped) */
#define LM_CONSOLE      0x01
#define LM_STDOUT       0x02
#define LM_STDERR       0x04
#define LM_ERRLOG       0x08
#define LM_SYSLOG       0x10

/* return codes for get_ping_result() */
#define PING_OK					1
#define PING_UNKNOWN_HOST		2
#define PING_NO_ICMP			3
#define PING_NOT_ROOT			4
#define PING_SOCKET_ERROR		5
#define PING_NO_MORE_RESULTS	6
/* the following codes are not used by get_ping_result(), but may be
 * used by its callers */
#define PING_STATE_UNKNOWN		100


/* geolib library function prototypes */
int add_clock (int *, int *, int *, int *, int *, int *, long);
void add_days (int *, int *, int *, int);
int add_milli (int *milli, int *sec, int *min, int *hou, int *da, int *mon, int *yea,
               long increment);
struct Node *add_object (struct Node *, DATA_TYPE *, int, int);
int add_pinger (char *address, int frequency);
int add_to_ring_buffer (struct Ring_buffer *ring_buffer, void *data,
						int allow_overflow);
int alloc_more_mem (void **block, int new_size, int *current_size);
int capture_programme (char *arguments, int use_path, char *str_stdin,
                       char **str_stdout, char **str_stderr);
int chsize2 (int handle, int size, int byte);
int chsize3 (int handle, int size, int byte);
int check_clock (int, int, int, int, int, int);
int check_data (char, int, float *, int *, float *, float *, float *);
int check_dir (char *dir);
int check_number (char *, int, int);
void close_database (void);
int close_gse_file (void);
void close_socket (int sock);
int comm_tail (int, char* [], struct Command_line_details *,
               void (*) (void), char *, char *);
int compare_clocks (int, int, int, int, int, int, int, int, int,
		    int, int, int);
int compare_files (const void *r1, const void *r2);
void compress_retransmit_list (void);
int connect_to_server (char *dest_host, int dest_port, int src_port, 
                       int tmout, int allow_reuse, int *socket_handle);
void config_backgr (void (*term_sig) (int), void (*hup_sig) (int),
                    int prio);
void config_backgr2 (void (*term_sig) (int), void (*hup_sig) (int),
                     int prio, char *prio_env_name, int options);
int copy_file (char *, char *, int);
struct Ring_buffer *create_ring_buffer (int element_size, int n_elements);
int current_year (int century);
void date_from_seconds (unsigned long n_secs,
					    int *sec, int *min, int *hour,
					    int *day, int *month, int *year);
int date_to_day_of_year (int, int, int);
int day_diff (int day_1, int month_1, int year_1,
			  int day_2, int month_2, int year_2);
int day_of_year_to_date (int, int, int *, int *);
int days_in_month (int, int);
void decode_intermagnet (unsigned char *, int, int, int *, int *, int *,
			 char *, int *, int *, long *, char *, long *,
			 long *, long *, long *);
int destroy_emsg_queue (void);
void destroy_printer_list (struct Printer_Definition *printer_def, int n_printers);
void destroy_ring_buffer (struct Ring_buffer *ring_buffer);
void disconnect_from_server (int socket_handle);
void display_ftp_error (char *);
int delete_posix_mqueue (char *name);
void delete_queue_entry (struct Queue_details *queue_details);
char *env_to_dir (char *env_var, char *errmsg);
void empty_ring_buffer (struct Ring_buffer *ring_buffer);
int eofgetline (FILE *fp, char s[], int lim, int *flag);
int establish_server (int port, int queue_len, int catch_client_death,
				      int bind_timeout, int *sock);
int evaluate (float *variable, char *expression);
int extract_from_ring_buffer (struct Ring_buffer *ring_buffer,
							  int n_elements, void *data);
int format_error (char *, int, char *, char *, pid_t, char *, int);
int format_log_filename (char *format, char *string, int maxlen);
int fprintf_component_6 (FILE *, long);
void free_dic (void);
int ftp (int operation, char *hostname, char *username, char *password,
	 char *directory, char *filename, char *copy_filename,
	 int binary, int delete, int overwrite);
int ftp_logon (char *host, char *user, char *password, char *account, int tm);
int ftp_logoff (void);
int ftp_change_dir (char *directory);
int ftp_send_file (int command, char *local_file, char *remote_file);
int ftp_receive_file (int command, char *local_file, char *remote_file);
void *geolib_debug_calloc (size_t n_elements, size_t element_size);
void geolib_debug_free (void *user_data);
void *geolib_debug_malloc (size_t size);
void *geolib_debug_realloc (void *user_data, size_t size);
char *get_data_font (char *env_var);
int get_database_line_number (void);
void get_date (struct Date *);
char *get_home_dir (void);
int get_id (char *, char *, char *, char *);
char *get_month_name (int);
int get_n_days_data_online (char *env_var);
struct File_info *get_next_file (char *path);
char *get_next_line (char *string);
int get_password (char *prompt, char *password);
int get_pci_basic_info (int vendor_id, int device_id, int card_index,
                        struct Pci_basic_info *info);
int get_ping_result (int first, char **address,
					 int *n_pings_sent, int *n_pings_received,
					 time_t *last_sent_time);
int get_queue_entry (int first, struct Queue_details *queue_details);
int get_ring_buffer_length (struct Ring_buffer *ring_buffer);
char *get_tty_font (char *env_var);
char *get_window_font (char *env_var);
int get_word (char *string, int number, int max_word_size, char *word);
void gin_decrypt (unsigned char *string);
void gin_encrypt (unsigned char *string);
char *get_server_name (void);
void hdz_calculate (long, long, long, long, long, long, long, long, long *,
        	   long *, long *);
void hdz_calculate2 (double, double, double, double, double, double, double,
		     double *, double *, double *);
void heap_dump (FILE *fp, int short_form);
struct IIR_Filter *iir_create (int iord, int type, double fl, double fh,
			       double freq, int zero_phase, double missing_data);
void iir_destroy (struct IIR_Filter *iir);
void iir_reset(struct IIR_Filter *iir);
void iir_apply (void *data, int nsamps, int contiguous, int data_type,
                struct IIR_Filter *iir);
void init_lpt_port (int ba);
int is_leap_year (int);
int is_posix_mqueue_working (void);
int is_process_alive (int pid);
void log_errors_to (int, char *);
void log_error (int, char *);
void log_msg (int destination, int severity, char *progname,
              char *format, ...);
char *make_ordinal_number (int number);
int make_printer_list (char *env_base, struct Printer_Definition **printer_def);
void make_synth_wave (double sample_rate, double n_seconds, double freq,
                      double max_amp, int reset, int *data);
int memchk (void *ptr, int byte, int len);
int min_of_day_to_time (int, int *, int *);
long milli_diff (int milli1, int sec1, int min1, int hour1,
                int day1, int month1, int year1,
                int milli2, int sec2, int min2, int hour2,
                int day2, int month2, int year2);
long minute_diff (int sec_1, int min_1, int hour_1, int day_1, int month_1,
				  int year_1, int sec_2, int min_2, int hour_2, int day_2,
				  int month_2, int year_2);
int monitor_process (pid_t pid, void (*exit_handler) (pid_t, int, void *),
					 void *callback, int kill_on_exit);
int net_partial_read (int handle, char *msg, int max_len);
int net_read (int handle, char *msg, int max_len);
int net_read_line (int handle, char *msg, int max_len, char *term_chars,
                   int term_char_len);
int net_write (int handle, char *msg, int max_len);
int open_gse_file (char *filename, char *msg_id, char *source);
int open_gse_file2 (FILE *fp, char *msg_id, char *source);
int open_posix_mqueue (char *name, int oflag, int n_msgs, int msg_size);
int parse_date (char *, int *, int *, int *);
int parse_time (char *, int *, int *, int *);
int peek_at_ring_buffer (struct Ring_buffer *ring_buffer,
		   		  	     int n_elements, void *data);
int ping (char *hostname, int timeout);
int power (int base, int number);
int print_data (int day, int month, int year, 
                int hour, int min, int sec, int duration,
                int *range, int *offset, int *data[], FILE *fp,
                int (*progress) (int percent) );
int print_dv (int day, int month, int year, 
              int hour, int min, int sec, long *dv,
		      float *red_data_percent, float *blue_data_percent,
		      float block_data_percent, FILE *fp);
int print_errors (int day, int month, int year,
				  struct Errlog *errlog, int n_recs, FILE *fp);
int print_file (char *filename, char *prn_cmd,
				int n_copies, char *status_msg);
int print_stream (FILE *fp, char *prn_cmd,
				  int n_copies, char *status_msg);
int print_string (char *string, char *prn_cmd,
				int n_copies, char *status_msg);
int process_retransmit_list (int queue);
int queue (char *command_line, char *working_dir, int use_fp, FILE *fpin);
int read_encrypted_file (char *filename, unsigned char *string);
int read_lpt_control_bit (int bit);
int read_lpt_data_port (void);
int read_lpt_status_bit (int bit);
int read_struct (char *, int, int, void *);
int receive_error (int, int *, char *, char *, pid_t *);
int restart_initialise (int argc, char *argv[], int sig_no);
void restart_process (void);
void restart_signal_handler (int sig_no);
/* long round (double amount); */
int run_programme (char *, int, int, struct Redirection *);
int run_queue_entry (struct Queue_details *queue_details, int silent);
int search_tree (struct Node *, DATA_TYPE, int, int *);
int search_word (char *string, char *substr, int number, int abs_match,
		 int case_ind);
int sec_of_day_to_time (long daysec, int *sec, int *min, int *hour);
long second_diff (int sec_1, int min_1, int hour_1, int day_1, int month_1,
				  int year_1, int sec_2, int min_2, int hour_2, int day_2,
				  int month_2, int year_2);
unsigned long seconds_since_1970 (int sec, int min, int hour,
								  int day, int month, int year);
int send_mail (char *, char *, char *, int);
int send_mail2 (char *, char *, char *);
int send_mail3 (FILE *, char *, char *);
int send_bin_mail (char *, char *, char *, char *);
void set_debug_alloc_dump_level (int level);
void set_error_pid (pid_t pid);
int strlist (char *, char *, int);
char *strlwr (char *);
#ifndef WIN32
int strnicmp (char *, char *, int);
#endif
char *strupr (char *);
int stricmp2 (char *, char *);
char *stristr (char *, char *);
int strilist (char *, char *, int);
char *strtok2 (char *string, char *seps);
int substitute_variables (char *in_string, int remove_double_slash,
						  char *out_string);
int telnet_logon (char *host, char *user, char *password, char *prmpt, int tm);
int telnet_logoff (void);
int telnet_send_command (char *command, ...);
int telnet_get_response (int max_len, struct Redirection *redir, char *buffer);
void telnet_debug (int);
char *temp_filename (char *, char *);
char *terminate_path (char *, int);
int time_to_min_of_day (int, int);
char *trim_leading_whitespace (char *string);
char *trim_trailing_whitespace (char *string);
int wait_for_client_connection (int sock, int tmout, int do_fork, int *call_sock,
                                char *client_ip_addr, int *client_ip_port,
                                pid_t *child_pid);
void wildcard_to_regexp (char *wildcard, char *regexp);
int write_encrypted_file (char *filename, unsigned char *string);
int write_error (FILE *, int, char *, char *, pid_t);
int write_gse_channel_header (int start_year, int start_month, int start_day,
                              int start_hour, int start_min, int start_sec,
                              char *chan_name, char *chan_type, char *aux_id,
                              long n_samps,
                              double frequency, double calib_value,
                              double calib_period, char *instrum_name,
                              double horiz_angle, double vert_angle);
int write_gse_channel_data (int n_samps, long *data, long missing_val);
int write_gse_channel_trailer (void);
void write_lpt_data_port (int value);
int write_lpt_control_port (int bit, int value);
int write_noirq(int handle, const char *buffer, int size);
int write_struct (char *, int, int, void *);

int initialise_raster (int, int, int, int);
void clr_raster (void);
int divide_raster (int, int);
void draw_raster_magneto (int, char *, char *, float *, int *, int, int, float, float);
void draw_raster_top_title (char *, char *);
void draw_raster_bottom_title (int, int);
int save_raster (char *);
int save_raster_sun (char *);

int open_termcap (char *);
void close_termcap (void);
void clear_screen (void);
int get_nrows (void);
int get_ncols (void);
void move_to_xy (int row, int col);
void draw_box (int row1, int col1, int row2, int col2);
void bold_on (void);
void blink_on (void);
void underline_on (void);
void inverse_video_on (void);
void all_attributes_off (void);
void bold_off (void);
void blink_off (void);
void underline_off (void);
void inverse_video_off (void);


#endif




