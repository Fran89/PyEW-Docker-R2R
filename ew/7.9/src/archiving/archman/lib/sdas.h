/* sdas.h - include file for all SDAS library routines */

/* this is a cut down version of the file found on QNX sdas systems */

/* prevent double inclusion */
#ifndef __SDAS_H__
#define __SDAS_H__

/************************************************************************
 * section 1 - general stuff
 ************************************************************************/

#include <stdio.h>
#include <limits.h>
#include <time.h>

#include <sys/types.h>

#include "geolib.h"

/* a code that is used in the SDAS ring-buffer to show missing data -
 * this code must:
 *     1) be the same byte for all 4 bytes of a long int
 *     2) be outside the range -2**24 -> 2**24 */
#define SDAS_MISSING_DATA_BYTE	0x79
#define SDAS_MISSING_DATA		0x79797979l

/* the format statement that controls the format of a data record in
 * the most recent block file - also a string that is the same length
 * as a string would be when formatted for the most recent block file and
 * a string used to show missing data in the file */
#define MRB_FORMAT_STRING   "%4d %02d-%02d-%04d %02d:%02d:%02d\n"
#define	MRB_DUMMY_REC       "xxxx xx-xx-xxxx xx:xx:xx\n"
#define MRB_MISSING_REC     "                        \n"

/* return codes for get_most_recent_block_time() */
#define GMRBT_OK				0
#define GMRBT_FILE_ERR			2
#define GMRBT_NO_CHAN			3
#define GMRBT_NO_MEM			4
#define GMRBT_REM_ERR           5

/* the maximum rate that data can be sampled - used to set the length
 * of a 1 second data array */
#define MAX_SAMPLE_RATE			1000

/* the maximum number of seconds in a long, allowing for expansion to
* milliseconds */
#define MAX_DURATION_SECS		(LONG_MAX/1000)

/* enumeration of possible time base values */
enum Sample_time_base
{
  SAMPLE_TIME_BASE_SECONDS, SAMPLE_TIME_BASE_MINUTES,
  SAMPLE_TIME_BASE_HOURS
};

/* return codes for init_sdas_reading() */
#define ISR_OK			0
#define ISR_MISS_ENV	1
#define ISR_NO_MEM		2
#define ISR_BAD_DIR		3
#define ISR_NO_DATA		4
#define ISR_BAD_CONF	5
#define ISR_NO_CONN     10
#define ISR_SERR        11

/* return code for get_sdas_data() - must be -ve */
#define GSD_BAD_SR		-1
#define GSD_BAD_CD		-2
#define GSD_IO_ERR		-3
#define GSD_REM_ERR     -4
#define GSD_NO_MEM      -5

/* return codes for find_sdas_gap() */
#define FSG_GAP         1
#define FSG_DATA        2
#define FSG_DONE        3
#define FSG_IO_ERR      4

/* the default internet port for the data server */
#define DEFAULT_INET_PORT		6801

/* codes for the type of pid file */
#define SDAS_PID_FILE           1
#define SDAS_DAEMON_PID_FILE    2

/* return codes for load_sdas_pid_file() */
#define LPF_OK          1
#define LPF_NO_MEM      2
#define LPF_NO_FILE     3
#define LPF_FILE_ERR    4

/* return codes from errlog_load_data() */
#define ERRLOG_NOFILE		1
#define ERRLOG_BADFILE		2
#define ERRLOG_NOMEM		4
#define ERRLOG_OK			5

/* codes for the errlog data orders */
#define ERRLOG_ORDER_NONE               0
#define ERRLOG_ORDER_TIME				1
#define ERRLOG_ORDER_SEVERITY_TIME		2
#define ERRLOG_ORDER_PID_TIME           3

/* the error logs can be read into this structure */
struct Errlog_store
{
  int day, month, year;
  int hour, min, sec;
  char hostname [50];
  char utility_name [50];
  pid_t pid;
  int severity;				/* see ERR_xxx codes in geolib.h */
  char message [150];
};

/* error flag information (for real-time error monitoring) is read
 * using this structure */
struct Err_flag_info
{
  char errmsg [200];
  int flag_on;
  int year, month, day, hour, min, sec;
  int severity;
};

/* return codes from copy_remote_file() */
#define CRF_OK          0
#define CRF_NO_FILE     1
#define CRF_REMOTE_ERR  2
#define CRF_LOCAL_ERR   3

/* return codes from load_remote_directory() - must be negative */
#define LRD_NO_DIR      -1
#define LRD_REMOTE_ERR  -2
#define LRD_NO_MEM      -3

/* types of ltasta algorithm, for LTASTA_Data.type */
#define LTASTA_REX_ALLEN    1
#define LTASTA_SEISLOG      2

/* this structure holds an lta/sta pair, its associated configuration
 * parameters and internal calculation variables for the lta/sta
 * on a single trace */
struct LTASTA_Data
{
  /* current lta/sta/trigger values */
  float dc;                     /* the DC level of the signal */
  float sta;                    /* the short term average */
  float lta;                    /* the long term average */
  float ratio;                  /* STA / LTA */
  int is_triggered;             /* TRUE when the system triggers */
  int is_state_changed;         /* TRUE when the state changes from
                                 * triggered to non-triggered, or vv */
  
  /* configuration information */
  long lta_duration;            /* length of lta in seconds */
  long sta_duration;            /* length of lta in seconds */
  float trigger_threshold;      /* level at which a trigger occurs */
  float detrigger_threshold;    /* level at which a trigger occurs */
  int lta_off_in_trigger;       /* TRUE to stop calculating lta
                                 * while a channel is triggered */
  float sample_rate;            /* sample rate in Hz */
  int type;                     /* one of LTASTA_REX_ALLEN or LTASTA_SEISLOG */

  /* for type == LTASTA_REX_ALLEN:
   *    Allen's constants - see the paper for a description
   * for type == LTASTA_SEISLOG:
   *    C1 = sta length in samples
   *    C2 = lta length in samples
   *    C3, C4 not used */
  float C1;
  float C2;
  float C3;
  float C4;
    
  /* local storage for the Rex Allen method */
  long previous_data;           /* value of the previous data sample */
  float previous_real_data;     /* previous (DC removed) real data */

  /* start up variables for the seislog method */  
  int initial_sample_count;     /* used to count samples so that
                                 * triggers are not declared until
                                 * a minimum amount of data has been
                                 * collected */
  int state;                    /* 0 = calculating initial DC level
                                 * 1 = calculating initial sta/lta
                                 * 2 = fully operational */
  
};

/* this structure holds a group of channels */
struct Channel_group
{
  char group_name [100];
  int n_channels;
  int *channel_list;
};

/* information type codes for get_writer_queue_size() */
#define GWQS_MAX_SIZE       1
#define GWQS_CUR_SIZE       2
#define GWQS_FREE_SPACE     3

/* codes for the type of operation in write_uptime_message */
#define UPTIME_RESTART      1
#define UPTIME_LOG          2

/* codes for the type of read in read_uptime_message() */
#define UPTIME_FIRST        1
#define UPTIME_NEXT         2
#define UPTIME_LAST         3

/* codes for the return value from read_uptime_message() */
#define UPTIME_ERROR        -1
#define UPTIME_OK           0
#define UPTIME_EOF          1


/************************************************************************
 * section 2 - definitions for things to do with writing data
 ************************************************************************/

/* enumeration of possible acquisition channel types - only the
 * first two can appear in the acquire.cfg database - the NO_UPDATE
 * types tell the writing software not to update the most_recent_block
 * file */
enum Channels_acq_type {SDAS_ACQ_TYPE_MAIN, SDAS_ACQ_TYPE_AUX,
                        SDAS_ACQ_TYPE_MAIN_NO_MRB_UPDATE,
                        SDAS_ACQ_TYPE_AUX_NO_MRB_UPDATE };

/* this structure defines the header that is used by drivers to
 * access the ring-buffer writing routine send_to_writer() */
struct Acq_to_ring_buffer
{
  enum Channels_acq_type acq_type;	/* is this a 'main' or 'aux' channel */
  int acq_chan;						/* the acquisition program channel number */
  int year, month, day;				/* date for the data */
  int hour, min, sec;				/* time for the data */
  int n_samples;					/* number of samples following this header */
  enum Sample_time_base time_base;	/* is this the number of samples per
  									 * 'second', 'minute' or 'hour' */
};

/* this structure defines the header and data that are sent through the
 * message queue by send_to_writer() to pass the data to the sdas_writer
 * process */
struct Sdas_writer_msg
{
  int sdas_chan;                    /* the sdas channel number */
  int year, month, day;				/* date for the data */
  int hour, min, sec;				/* time for the data */
  int n_samples;					/* number of samples following this header */
  enum Sample_time_base time_base;	/* is this the number of samples per
  									 * 'second', 'minute' or 'hour' */
  int update_mrb;                   /* if TRUE update the most_recent_block file */
  long data [MAX_SAMPLE_RATE];
};

/* name of the message queue used by sdas_writer */
#define SDAS_WRITER_MQ_NAME         "sdas_writer"


/************************************************************************
 * section 3 - definitions for routines that handle configuration files
 ************************************************************************/

/* structure to hold the general.cfg database */
struct General_cfg
{
  char name [100];
  char value [100];
};

/* structure to hold the acquire.cfg database */
struct Acquire_cfg
{
  int id;					/* id (for join with config_cfg) */
  char prog_name [50];		/* name of the acquisition driver */
  char dev_name [50];		/* name of the controlling device */
  int buff_size;            /* size of buffer between device and program */
  char stty_params [200];   /* parameters for stty */
  char mode [200];			/* mode in which the driver should operate
                             * this field is optional and driver specific */
  int sample_rate;          /* the sample rate for the driver OR -1
                             * for no sample rate specified */
  enum Sample_time_base time_base; /* the time base for the sample rate */
  char address [200];       /* the address for the driver
                             * this field is optional and driver specific */
  int clock_reqd;			/* if TRUE the SDAS clock program is required
                             * for this driver */
  int debug;                /* debug (verbose) flag */
};

/* structure to hold the channels.cfg database */
struct Channels_cfg
{
  int sdas_chan;							/* unique SDAS channel number
  											 * as used in ring-buffer
											 * file names */
  char chan_name [50];
  char chan_type [50];						/* code for the type of data */
  char units [50];                          /* name of physical units used
                                             * to record data (e.g. count,
                                             * volts, nT, etc. */
  float offset; 							/* subtract from data samples */
  float scale;								/* divide into data samples */
  int acquire_id;							/* id field from the corresponding
  											 * Acquire_cfg record (relational
											 * join field) */
  enum Channels_acq_type acq_type;			/* main or aux */
  int acq_chan;								/* channel number used by the
  											 * digitiser driver */
};

/* structure to hold the detect.cfg database */
struct Detect_cfg
{
  char prog_name [50];		/* name of the detection program */
  char outp_name [50];		/* name of the output pipe to the detection
  							 * manager */	
  char cfg_name [50];		/* name of the configuration file for this
							 * detection program */
};

/* structure the hold the ltasta.cfg database */
struct LTASTA_cfg
{
  int sdas_chan;			/* unique SDAS channel number
  							 * as used in channels_cfg */
  int filt_type;            /* the filter type (see filter.c) */
  int n_poles;              /* number of filter poles (1-10) */
  double low_f, high_f;     /* filter low and high frequency points (Hz) */
  int pre_process;          /* code for pre-processing data */
  double ds_const;          /* constant used in pre-processing if using PP_DIFF_SQUARE */
  int lta_dur, sta_dur;     /* duration of LTA and STA (seconds) */
  double trig_lev;          /* level at which trigger starts */
  double detrig_lev;        /* level at which trigger ends */
  int lta_in_trig;          /* if TRUE continue calculating LTA during a
                             * trigger, if FALSE freeze LTA in a trigger */
  int group;                /* the group for this channel */
  int trig_time_win;        /* length of time window to check for triggers 
                             * on other channels in this group */
  int p_chans;              /* the percentag of channels (in a group) which
                             * must trigger before an event is declared */
};

/* codes used for pre-processing data before calculating LTA/STA */
#define PP_NONE                     0
#define PP_SQUARE                   1
#define PP_DIFF_SQUARE              2


/************************************************************************
 * section 4 - definitions for detection
 ************************************************************************/

/* these are the actions in the Trigger.action field */
enum Trigger_action
{
  TRIGGER_ON, TRIGGER_OFF
};

/* this structure defines the triggers that are passed between the
 * individual detection programs and the detection buffer manager */
struct Trigger
{
  int year, month, day;				/* date for the trigger */
  int hour, min, sec;				/* time for the trigger */
  int sdas_chan;					/* sdas channel number */
  enum Trigger_action action;		/* TRIGGER_ON or TRIGGER_OFF */
};

/* details on the sync chars used before and after a data block */
#define DET_N_START_SYNC_CHARS		6
#define DET_START_SYNC_CHAR			0x79u
#define DET_N_END_SYNC_CHARS		6
#define DET_END_SYNC_CHAR			0x80u

/* this structure defines an event */
struct SDAS_Event
{
  int year, month, day;             /* start date */
  int hour, min, sec, milli;        /* start time */
  double duration;                  /* length of the event */
  char *algorithm;                  /* name of the algorithm that created the event */
  char *algorithm_data;             /* data specific to the algorithm */
};

/* return codes for get_sdas_event() */
#define EVENT_OK			0
#define EVENT_EOF			1
#define EVENT_ERROR			2
#define EVENT_NOMEM			3


/************************************************************************
 * section 5 - declarations for the clock
 ************************************************************************/

/* a structure that we can hold a clock in */
struct SDAS_Clock
{
  int year, month, day;			/* the date */
  int hour, min, sec, milli;	/* the time */
  int valid;					/* if FALSE date/time is not valid */
};

/* codes for SDAS_Clock_Diags.external_clock_type - must be +ve or 0
 * as -ve values are used for error indication */
#define EXTERNAL_CLOCK_NONE		0
#define EXTERNAL_CLOCK_RCC		1
#define EXTERNAL_CLOCK_NMEA		2

/* a structure holding diagnostic information on the clock program */
struct SDAS_Clock_Diags
{
  /* the pid of the clock program */
  pid_t pid;

  /* the type of external clock hardware */
  int external_clock_type;

  /* error flags holds bit-mapped error codes - see below for the codes */
  long error_flags, last_error_flags;

  /* ext_clk_consecutive_count holds a count of the number of times there
   * has been a difference of 1 second between the last_external_clock and
   * the current external_clock */
  int ext_clk_consecutive_count;

  /* milli_secs_between_1pps holds the number of 1KHz ticks since the
   * last 1pps interrupt, milli_secs_between_serial_rx holds the number
   * of 1KHz ticks since the last character was received on the
   * serial port */
  long milli_secs_between_1pps, milli_secs_between_serial_rx;

  /* max_main_clock_error holds the maximum difference between the main
   * clock and true time - after the main clock is this much in error it will
   * set directly to the external clock rather than being slowly drifted */
  long max_main_clock_error;

  /* max_error_for_drift holds a similar value - after the main clock
   * is this much in error, drift rate calculations will not be performed.
   * using the two parameters allows drift rate calculations to be made
   * on large errors, but also allows them to be corrected straight away */
  long max_error_for_drift;

  /* main_clock_sync_rate holds the time (in mS) between attempts to
   * synchronise the main clock to the raw clock, milli_secs_since_sync
   * holds the time since the last successful sync */
  long main_clock_sync_rate;
  long milli_secs_since_sync;

  /* main_clock_err holds the number of milliseconds by which the main
   * clock was in error at the time of the last sync */
  long main_clock_err;

  /* at synchrnoisation, the difference between the raw and main clocks
   * is calculate (main_clock_err) - mc_correction_count is then
   * loaded with the absolute value of main_clock_err and
   * mc_correction_amount is set to the amount to correct by (-1, 0 or +1),
   * then once a second, until mc_correction_count reaches zero, 
   * mc_correction_amount is added to the main clock and mc_correction_count
   * is decremented */
  int mc_correction_count;
  int mc_correction_amount;

  /* mc_drift_period holds the period (in mS) between a single
   * milli-second being added to or subtracted from the main clock to
   * drift it, mc_drift_timer holds the count until the next correction
   * will take place, mc_drift_amount holds the amount to correct by
   * (-1, 0 or +1) */
  long mc_drift_period;
  long mc_drift_timer;
  int mc_drift_amount;

  /* total_correction holds the amount that the main
   * clock has been drifted and corrected by (in mS) since the last sync */
  int total_correction;

  /* correction_applied is used to prevent more than one correction
   * being applied each second */
  int correction_applied;

  /* details on the fake 1pps - fake_1pps_gap holds the gap in millieconds
   * between the end of a serial message data and the fake 1pps signal -
   * if it is -ve then use the hardware 1pps */
  int fake_1pps;
};

/* bit-mapped error flags for the diagnostics structure above */
#define ERR_UNKNOWN_PROXY		0x0001	/* message received from an unknown source */
#define ERR_BAD_SERIAL_DATA		0x0002	/* data on serial port could not be recognised */
#define ERR_NON_CONSECUTIVE		0x0004	/* serial data is not consecutive */
#define ERR_CLK_OUT_OF_LOCK     0x0008	/* clock is not in lock with its time source */
#define ERR_NO_1PPS				0x0010	/* missing 1Hz from exernal clock */
#define ERR_NO_SERIAL			0x0020	/* missing serial data from external clock */
#define ERR_NO_MAIN_SYNC		0x0040	/* there has not been a sync. for a long time */
#define ERR_CORR_TOO_LARGE		0x0080	/* the required clock correction is too large */
#define ERR_CLOCK_MQUEUE        0x0100  /* error with the clock's message queue */

/* the name of the shared memory area associated with the SDAS clock */
#define SDAS_CLOCK_SHM_NAME		"sdas_clock_shm_name"

/* the name of the fifo used to communicate with the clock -
 * this will be created in the pipes directory */
#define SDAS_CLOCK_FIFO_NAME	"sdas_clock_fifo"

/* codes for the different types of clock */
#define SDAS_RAW_CLOCK				1
#define SDAS_EXTERNAL_CLOCK			2
#define SDAS_LAST_EXTERNAL_CLOCK	3
#define SDAS_MAIN_CLOCK				4
#define SDAS_RAW_CLOCK_AT_SYNC		5
#define SDAS_MAIN_CLOCK_AT_SYNC		6

/* return codes for connect_to_sdas_clock */
#define CTSC_OK				0
#define CTSC_NO_SMEM		1
#define CTSC_NO_MQ  		2
#define CTSC_BAD_PARAMS		3
#define CTSC_SYS_FAULT		4
#define CTSC_NO_CLOCK		5

/* a structure used by clients to send messages to the clock program -
 * not used directly by clients (they use connect_to_sdas_clock()) */
struct SDAS_Clock_Client
{
  pid_t pid;        /* pid of the client sending the message */
  int time_sig;     /* signal number to be sent for time ticks */
  int exit_sig;     /* signal number to be sent if clock pogram exits */
  int frequency;    /* frequency of time ticks in Hz */
  int operation;    /* codes - see below */
};

/* codes for SDAS_Clock_Client.operation */
#define SCC_OPERATION_NEW_CLIENT        1
#define SCC_OPERATION_STOP_SIGS         2
#define SCC_OPERATION_RESTART_SIGS      3
#define SCC_OPERATION_CLIENT_EXIT       4

/* name of the message queue used by sdas_clock */
#define SDAS_CLOCK_MQ_NAME         "sdas_clock"


/************************************************************************
 * section 6 - high level time series handling and statistics
 ************************************************************************/

/* definition of time_type parameter to get_sdas_time_series() */
#define TS_TIME_START       1
#define TS_TIME_END         2
#define TS_TIME_MRB         3
#define TS_TIME_MRB_CACHE   4

/* definition of dur_units parameter to get_sdas_time_series() */
#define TS_SAMPLES          1
#define TS_MILLI_SECONDS    2
#define TS_SECONDS          3
#define TS_MINUTES          4
#define TS_HOURS            5

/* definition of mem_type parameter to get_sdas_time_series() and
 * Sdas_time_series.mem_type */
#define TS_MEM_STATIC       1
#define TS_MEM_DYNAMIC      2

/* definition of the Sdas_time_series structure */
struct Sdas_time_series
{

  int channel_count;            /* 0 .. n_channels */
  int channel_number;           /* SDAS channel number */

  int year, month, day;         /* date and time for the data */
  int hour, min, sec, milli;

  float sample_rate, sample_period; /* sample rate (in Hz) and period (in Secs) */
  float duration;               /* length of the data (in seconds) */
  long n_samples;               /* number of samples in the data array */
  
  long *data;                   /* array of time series data */

  int mem_type;                 /* see above - if mem_type is TS_MEM_DYNAMIC
                                 * then the caller is responsible for
                                 * freeing Sdas_time_series.data */

  struct Channels_cfg *channel_details; /* channel description */
  
  char *errmsg;                 /* if get_sdas_time_series() fails, this
                                 * member holds a descriptive error message */
};

/* a structure to hold a singel set of statistics for a single channel */
struct SDAS_data_stats
{
  int n_points;             /* number of points used to calculate the statistics */
  int n_missing;            /* the number of missing points in the data block -
                             * if these are hourly statistics then:
                             * n_points + n_missing = sample rate (in samples / hour) */
  double average;           /* the average value */
  double standard_dev;      /* the standard deviation */
  long min_val, max_val;    /* the minimum and maximum values */
};



/************************************************************************
 * section 7 - things to do with the SDAS monitor sub-system
 * removed on WINDOWS
 ************************************************************************/

/************************************************************************
 * section 8 - forward declarations for all routines - these are
 * split into two sections, the first for normal routines, the
 * second for those that must be excluded when compiling extract
 * under SOLARIS
 * second section removed on WINDOWS
 ************************************************************************/

int add_to_sdas_pid_file (char *log_dir, int pid_file_type,
                          pid_t pid, char *description,
                          int argc, char *argv[]);
int add_to_sdas_pid_file2 (char *log_dir, int pid_file_type,
                           pid_t pid, char *description,
                           char *command_line);
void addtime(struct timespec *,struct timespec *);
int calc_n_samples (int sample_rate, long duration);
long calc_sdas_average (int channel, int year, int month, int day, int hour,
                        int min, int sec, int milli, long duration, double *data,
                        struct Channels_cfg **chan_details);
void change_missing_data_value (long old_val, long new_val,
                                long *data, int n_points);
int clear_error_flag (char *shmem_addr, int flag);
int cmptime(struct timespec *,struct timespec *);
int connect_to_writer (int id, char *errmsg);
void convtime(struct timespec *,int *,int *,int *,int *,int *,int *,int *,int );
int copy_remote_file (char *remote_file, char *local_file);
int create_buffer (char *log_dir, int buffer_size, char *device, int *pid,
                   char *command_line);
char *create_error_flags (int n_err_msgs, ...);
int delete_from_sdas_pid_file (char *log_dir, int pid_file_type,
                               char *description);
struct Errlog_store *errlog_get_errors (int *n_recs);
int errlog_load_data (char *dir, int day, int month, int year);
int errlog_load_data2 (char *filename);
void errlog_order_data (int order);
void fill_with_missing_data (long miss_val, long *data, int n_points);
struct Acquire_cfg *find_acq_record (int channel);
int find_general_string (char *name, char **value);
int find_general_int (char *name, long *value);
int find_general_real (char *name, double *value);
struct Acquire_cfg *find_local_acq_record (int channel);
int find_local_sdas_gap (int new, int channel, int year, int month, int day,
                         int n_days,
                         int *out_year, int *out_month, int *out_day,
                         int *out_hour, int *out_min, int *out_sec,
                         int *out_milli);
struct Acquire_cfg *find_remote_acq_record (int channel);
int find_sdas_config_file (char *config_file);
int find_sdas_gap (int new, int channel, int year, int month, int day,
                   int n_days,
                   int *out_year, int *out_month, int *out_day,
                   int *out_hour, int *out_min, int *out_sec,
                   int *out_milli);
void force_database_reload (int flag);
int get_channel (int chan_index);
struct Channels_cfg *get_channel_details (int channel,
									      int year, int month, int day,
									      int hour, int min, int sec);
struct Channel_group *get_channel_group (int group_index);
struct Channels_cfg *get_config_history (int history_no, int *n_records);
char *get_data_font_name (void);
time_t get_errlog_mod_time (char *dir, int day, int month, int year);
time_t get_errlog_mod_time2 (char *filename);
int get_local_channel (int chan_index);
struct Channels_cfg *get_local_channel_details (int channel,
							   			        int year, int month, int day,
										        int hour, int min, int sec);
struct Channel_group *get_local_channel_group (int group_index);
struct Channels_cfg *get_local_config_history (int history_no, int *n_records);
int get_local_most_recent_block_time (int channel, int re_read,
                                      int *year, int *month, int *day,
                                      int *hour, int *min, int *sec);
int get_local_n_channels (void);
int get_local_n_channel_groups (void);
int get_local_n_hist_records (void);
int get_local_sample_rate (int channel, int year, int month, int day, int hour,
                           int min, int sec, long duration);
long get_local_sdas_data (int channel, int year, int month, int day, int hour,
                          int min, int sec, int milli, long duration, long *data,
                          struct Channels_cfg **chan_details);
int get_local_sdas_event (int day, int month, int year, int get_ag_data,
                          struct SDAS_Event *event);
int get_most_recent_block_time (int channel, int re_read,
                                int *year, int *month, int *day,
                                int *hour, int *min, int *sec);
int get_n_channels (void);
int get_n_channel_groups (void);
char *get_next_channel_directory (char *wave_dir, int *channel);
char *get_next_data_file (char *channel_dir, int *year, int *month,
                          int *day, int *hour);
char *get_next_stats_file (char *channel_dir, int *year, int *month,
                           int *day, int *hour);
int get_remote_channel (int chan_index);
struct Channels_cfg *get_remote_channel_details (int channel,
							    		         int year, int month, int day,
										         int hour, int min, int sec);
struct Channel_group *get_remote_channel_group (int group_index);
int get_remote_most_recent_block_time (int channel, int re_read,
                                       int *year, int *month, int *day,
                                       int *hour, int *min, int *sec);
int get_remote_n_channels (void);
int get_remote_n_channel_groups (void);
int get_remote_sample_rate (int channel, int year, int month, int day, int hour,
                            int min, int sec, long duration);
long get_remote_sdas_data (int channel, int year, int month, int day, int hour,
                           int min, int sec, int milli, long duration, long *data,
                           struct Channels_cfg **chan_details);
int get_remote_sdas_event (int day, int month, int year, int get_ag_data,
                           struct SDAS_Event *event);
int get_sample_rate (int channel, int year, int month, int day, int hour,
                     int min, int sec, long duration);
long get_sdas_data (int channel, int year, int month, int day, int hour,
                    int min, int sec, int milli, long duration, long *data,
                    struct Channels_cfg **chan_details);
int get_sdas_event (int day, int month, int year, int get_ag_data,
                    struct SDAS_Event *event);
void get_sdas_pid_record (char *store, int record_no, int *pid,
                          char *description, char *command_line);
int get_sdas_time_series (int channel, int year, int month, int day,
                          int hour, int min, int sec, int milli,
                          int time_type, long duration, int dur_units, 
                          int mem_type, struct Sdas_time_series *ts);
int get_uptime_log_length (char *log_dir);
time_t get_uptime_mod_time (char *log_dir);
char *get_window_font_name (void);
char *init_daemon (int argc, char *argv[], char *description,
                   void (*term_sig) (int), int prio, int options);
void init_fonts (void);
int init_local_sdas_reading (void);
void init_ltasta (long lta_duration, long sta_duration,
                   float trigger_threshold, float detrigger_threshold,
                   int lta_off_in_trigger, float sample_rate,
                   int type, struct LTASTA_Data *ltasta);
int init_remote_sdas_reading (char *ip_addr, int ip_port, int timeout);
int init_sdas_reading (char *ip_addr, int ip_port, int timeout);
int init_write_event (char *errmsg);
int is_error_flag_set (char *shmem_addr, int flag);
int is_monitor_running (char *log_dir);
int is_sdas_remote (void);
int list_remote_directory (char *remote_dir, char *pattern, struct File_info **list);
int load_local_sdas_pid_file (char *log_dir, int pid_file_type,
                              char **store, int *n_records);
int load_remote_sdas_pid_file (char *log_dir, int pid_file_type,
                               char **store, int *n_records);
int load_sdas_pid_file (char *log_dir, int pid_file_type, 
                        char **store, int *n_records);
char *make_acq_stderr_log_filename (char *log_dir);
char *make_full_debug_path (char *filename);
void make_sdas_data_filename (char *dir, int channel, int year, int month,
				   			  int day, int hour, char *filename);
void make_sdas_event_filename (char *dir, int year, int month,
				   	 		   int day, char *filename);
void make_sdas_pid_filename (char *log_dir, int pid_file_type,
                             char *pid_filename);
void make_sdas_stats_filename (char *dir, int channel, int year, int month,
				   	 		   int day, int hour, char *filename);
char *make_state_filename (char *utility, int use_net);
char *make_uptime_log_filename (char *log_dir);
#ifndef _LINUX
mqd_t open_clock_message_queue (int write_access, int read_access,
                                int queue_len);
mqd_t open_writer_message_queue (int write_access, int read_access,
                                 int queue_len);
#endif
char *process_acq_cl (int argc, char *argv[],
                      void (*usage) (void), struct Acquire_cfg *details);
struct Acquire_cfg *read_acquire_cfg (int *nr, char *errmsg);
struct Channel_group *read_channel_groups (int *nr, char *errmsg);
struct Channels_cfg *read_channels_cfg (int *nr, char *errmsg);
struct Channels_cfg *read_channels_cfg_by_name (char *filename, int *nr,
                                                char *errmsg);
int read_data_points (int handle, int n_points, long *data);
struct Detect_cfg *read_detect_cfg (int *nr, char *errmsg);
int read_error_flag (pid_t pid, struct Err_flag_info *err_flag_info);
struct General_cfg *read_general_cfg (int *nr, char *errmsg);
struct LTASTA_cfg *read_ltasta_cfg (int *nr, char *errmsg);
int read_local_sdas_stats (int channel, int year, int month,
                           int day, int hour, struct SDAS_data_stats *stats);
int read_remote_sdas_stats (int channel, int year, int month,
                            int day, int hour, struct SDAS_data_stats *stats);
struct SDAS_Monitor_cfg *read_sdas_monitor_cfg (char *errmsg);
int read_sdas_stats (int channel, int year, int month,
                     int day, int hour, struct SDAS_data_stats *stats);
int read_uptime_message (char *log_dir, int reopen,
                         int msg_no, int *year, int *month,
                         int *day, int *hour, int *min, int *sec, char *msg);
void remove_error_flags (pid_t pid);
void remove_monitor_file (char *log_dir);
int remove_sdas_pid_file (char *log_dir, int pid_file_type);
char *sdas_error_msg (void);
char *sdas_local_error_msg (void);
char *sdas_remote_error_msg (void);
int select_new_data_font (void);
void send_errors_to_logger (char *shmem_addr);
int send_to_writer (struct Acq_to_ring_buffer *header, long *data);
void set_def_acq_details (char *program_name, int buff_size, char *mode,
                          int sample_rate, enum Sample_time_base time_base,
                          char *address, struct Acquire_cfg *details);
int set_error_flag (char *shmem_addr, int flag);
void shutdown_local_sdas_reading (void);
void shutdown_remote_sdas_reading (void);
void shutdown_sdas_reading (void);
void subtime(struct timespec *,struct timespec *);
void swap_byte_order (long *data, int n_points);
void update_ltasta (int n_points, long *new_data, struct LTASTA_Data *ltasta);
char *verify_acquisition_configuration (void);
int write_event (struct SDAS_Event *event);
char *write_uptime_message (char *log_dir, int operation, time_t log_time);

#endif
