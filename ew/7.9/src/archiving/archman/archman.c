/*****************************************************************
 * archman.c - this program has 3 tasks:
 *
 * create an archive of continuous data (using waveman2disk)
 * move the continuous archive to a new directory (and hence to
 *     a new machine if the directory is a network mount)
 * move the trigger data (from trig2disk) to a new directory
 *
 * the program peeks into a transport ring and uses the trace_buf
 * messages it finds to tell it the date/time of the most recent
 * data block
 *
 * S. Flower, Feb 2001
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#ifndef _WINNT
#include <strings.h>
#endif
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "transport.h"
#include "swap.h"
#include "trace_buf.h"
#include "earthworm.h"
#include "earthworm_defs.h"
#include "kom.h"
#include "socket_ew.h"
#include "imp_exp_gen.h"
#include "trace_buf.h"
#include "time_ew.h"

#include "geolib.h"

#ifndef _WIN32
#define stricmp(a,b) strcasecmp((a),(b))
#endif

/* this structure holds a pair of directories - files in the source
 * will be moved to the destination */
struct Dir_pair
{
  char source [MAX_DIR_LEN];
  char dest [MAX_DIR_LEN];
};

/* this strcuture holds Station.Component.Network.Location sets */
/* RL v7.0 update */
struct SCNL
{
  char station [20];
  char component [20];
  char network [20];
  char location [3];
};

/* this structure holds details of wave servers */
struct Waveserver
{
  char ip_address [100];
  int ip_port;
};

/* this structure holds various configuration details */
struct Config
{
  /* names for things */
  char executable_name [100];
  char module_name [MAX_MOD_STR];
  char in_ring_name [MAX_RING_STR];
  char out_ring_name [MAX_RING_STR];

  /* ids for things */
  long in_ring_key;
  long out_ring_key;
  unsigned char installation_id;
  unsigned char module_id;
  unsigned char type_heartbeat;
  unsigned char type_error;
  unsigned char type_tracebuf2;     /* RL v7.0 */
  pid_t pid;

  /* a list of source/destination directory pairs */
  int n_dirs;
  struct Dir_pair *dir_list;

  /* a file for persistence (state between invocations) */
  char archive_state_file_name [MAX_DIR_LEN];

  /* the time (in secs) between heart beats */
  int heartbeat_interval;

  /* flag for whether to create a log file and whether to log
   * data messages (makes log files big) */
  int log_file_flag;
  int log_data_msgs;

  /* flag for whether to be verbose */
  int verbose;

  /* shared memory region for io */
  SHM_INFO in_shared_mem_region;
  SHM_INFO out_shared_mem_region;

  /* the size (in seconds) of continuous data segments (if <= 0 don't
   * store continuous data) and the delay between data being recorded
   * and archived (to allow all channels to arrive) */
  int continuous_data_segment_size;
  int continuous_data_delay;


  /* the remaining parameters are needed to configure waveman2disk */
  /* a list of SCNLs */
  int n_scnls;
  struct SCNL *scnl_list;

  /* a list of WaveServers */
  int n_waveservers;
  struct Waveserver *waveserver_list;

  /* general waveman2disk stuff */
  int timeout_seconds;
  int travel_timeout;
  int max_traces;
  long trace_buffer_len;
  int max_trace_msg;
  int gap_thresh;
  int min_duration;
  
  /* output details */
  char data_format [20];
  char out_dir [MAX_DIR_LEN];
  char output_format [20];
  
  /* StartTime added for wfdisc support */
  char start_time[20]; //Example string 2009/05/20 12:15:00
  int continuous_output; // Flag whether the data should be continuously
                         // outputted or whether it should only be written out 
						 // once per interval
};

/* forward declarations */
int main (int argc, char *argv[]);
void initialise (char *cfg_filename, struct Config *config);
void write_ew_heartbeat (struct Config *config, time_t current_system_time);
void write_ew_error (struct Config *config, short errno, char *message);
int get_time_data (struct Config *config, double *msg_time);
int is_next_archive_needed (struct Config *config, time_t data_time,
                            time_t *next_block_start_time);
void archive_data (struct Config *config, time_t *start_time);
void archive_data_passive (struct Config *config, time_t *start_time);
void move_data (struct Config *config, char *source_dir, char *dest_dir);
#ifdef _WINNT
char * win_strptime(const char *buf, const char *fmt, struct tm *tm);
#endif


/******************************************************************
 * main
 *
 * Description: program entry point
 *
 * Input parameters: argv[1] - name of configuration file
 * Output parameters: none
 * Returns: 0 for success, 1 for failure
 *
 * Comments:
 *
 ******************************************************************/
#define PROG_VERSION "0.1.4 2016-05-27"
int main (int argc, char *argv[])

{

  int quit, count;
  double msg_time, newest_msg_time;
  time_t current_system_time, next_block_start_time;
  struct Config config;
  struct Dir_pair *dir_pair_ptr;


  /* set the program name */
  strcpy (config.executable_name, "archman");

  /* Check command line arguments */
  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s <configfile>\n", config.executable_name);
    fprintf (stderr, "Version: %s\n", PROG_VERSION);
    exit (1);
  }

  /* initialise the program */
  newest_msg_time = 0.0;
  initialise (argv[1], &config);

  /* attach to the input and output rings */
  tport_attach (&(config.in_shared_mem_region), config.in_ring_key);
  tport_attach (&(config.out_shared_mem_region), config.out_ring_key);

  /* main loop */
  for (quit = 0; ! quit; )
  {
    /* get the system time for this run through the loop */
    time (&current_system_time);

    /* get the next message - if there was none then sleep for a bit */
    if (! get_time_data (&config, &msg_time))
    {
      if (config.verbose) printf ("archman: No messages found in input ring\n");
      sleep_ew (1000);
    }

    /* check if the time is old - if so ignore it */
    else if (msg_time > newest_msg_time)
    {
      newest_msg_time = msg_time;
      if (config.verbose) printf ("archman: Newest message time %.3lf\n", newest_msg_time);

      // If we are not at the end of the chunk lets run waveman2disk without advancing the 
	  // date block
	  if( (!is_next_archive_needed (&config, (time_t) newest_msg_time,
	                                       &next_block_start_time)) && (! quit)
										   && config.continuous_output) 
	  {
        archive_data_passive (&config, &next_block_start_time);
      }

      /* do we want to extract a chunk of continuous data ?? */
      while (is_next_archive_needed (&config, (time_t) newest_msg_time,
                                     &next_block_start_time) && (! quit))
      {
        /* archive a block of data */
        archive_data (&config, &next_block_start_time);

        /* send a heartbeat */
        write_ew_heartbeat (&config, current_system_time);

        /* check for termination */
        if (tport_getflag (&(config.in_shared_mem_region)) == TERMINATE) quit = 1;
      }

      /* move data between source and destination archive directories */
      for (count=0; (count<config.n_dirs) && (! quit); count++)
      {
        /* move some data */
        dir_pair_ptr = config.dir_list + count;
        move_data (&config, dir_pair_ptr->source, dir_pair_ptr->dest);

        /* send a heartbeat */
        write_ew_heartbeat (&config, current_system_time);

        /* check for termination */
        if (tport_getflag (&(config.in_shared_mem_region)) == TERMINATE) quit = 1;
      }
    }

    /* send a heartbeat */
    write_ew_heartbeat (&config, current_system_time);

    /* check for termination */
    if (tport_getflag (&(config.in_shared_mem_region)) == TERMINATE) quit = 1;
  }

  exit (0);

}

/******************************************************************
 * initialise
 *
 * Description: initialise the program
 *
 * Input parameters: cfg_filename - name of the config file
 * Output parameters: config - the program configuration
 * Returns: none
 *
 * Comments: on error a message is logged and the program exits
 *
 ******************************************************************/
void initialise (char *cfg_filename, struct Config *config)

{

  int n_files, success, found, count, ip_port, multiplier;
  char *command, *ptr, *name=NULL, *from_dir, *to_dir=NULL, *ip_address=NULL;
  char *station=NULL, *component=NULL, *network=NULL, *location=NULL;
  char *hour_string, *minute_string, *second_string;
  unsigned char *data_ptr=NULL;
  FILE *err_stream;
  struct Dir_pair *dir_list_ptr;
  struct SCNL *scnl_list_ptr;
  struct Waveserver *waveserver_list_ptr;


  /* where to send errors (before we can use logit()) */
  err_stream = stderr;

  /* set defaults and flag some parameters so we can see if they were missed */
  strcpy (config->module_name, "");
  strcpy (config->in_ring_name, "");
  strcpy (config->out_ring_name, "");
  config->heartbeat_interval = -1;
  config->log_file_flag = -1;
  config->log_data_msgs = FALSE;
  config->verbose = FALSE;
  config->continuous_data_segment_size = 0l;
  config->continuous_data_delay = 0l;
  strcpy (config->archive_state_file_name, "");
  config->n_dirs = 0;
  config->dir_list = (struct Dir_pair *) 0;
  config->n_scnls = 0;
  config->scnl_list = (struct SCNL *) 0;
  config->n_waveservers = 0;
  config->waveserver_list = (struct Waveserver *) 0;
  config->timeout_seconds = 500;
  config->travel_timeout = 30;
  config->max_traces = 100;
  config->trace_buffer_len = 72000l;
  config->max_trace_msg = 1000;
  config->gap_thresh = 20;
  config->min_duration = 30;
  strcpy (config->data_format, "sac");;
  strcpy (config->out_dir, ".");
  config->continuous_output = FALSE;
#if defined (_SPARC)
  strcpy (config->output_format, "sparc");
#elif defined (_INTEL)
  strcpy (config->output_format, "intel");
#else
#error "_INTEL or _SPARC must be set before compiling"
#endif

  /* attempt to open the configuration file(s) */
  n_files = k_open (cfg_filename);
  if (n_files == 0)
  {
    fprintf (err_stream,
             "%s: Error opening command file <%s>; exiting!\n",
             config->executable_name, cfg_filename);
    exit (1);
  }

  /* while a configuration file is open ... */
  while (n_files > 0)
  {
    /* for each line in the file ... */
    while (k_rd ())
    {
      /* Get the first token from line, remove blank lines and comments */
      command = k_str();
      if (! command) continue;
      if (*command == '#') continue;

      /* process the command */
      if (*command == '@')
      {
        /* Open a nested configuration file */
        success = n_files+1;
        n_files = k_open (command +1);
        if (n_files != success)
        {
          fprintf (err_stream,
                   "%s: Error opening command file <%s>; exiting!\n",
                   config->executable_name, command +1);
          exit (1);
        }
      }

      /* configuration file parameter: MyModuleId - the moudle id for this program */
      else if (k_its ("MyModuleId"))
      {
        ptr = k_str ();
        if (ptr) strcpy (config->module_name, ptr);
      }

      /* configuration file parameter: InRingName - the transport ring to read from */
      else if (k_its ("InRingName"))
      {
        ptr = k_str ();
        if (ptr) strcpy (config->in_ring_name, ptr);
      }

      /* configuration file parameter: OutRingName - the transport ring to read from */
      else if (k_its ("OutRingName"))
      {
        ptr = k_str ();
        if (ptr) strcpy (config->out_ring_name, ptr);
      }

      /* configuration file parameter: ArchiveStore - file for storing program state */
      else if (k_its ("ArchmanStore"))
      {
        ptr = k_str ();
        if (ptr) strcpy (config->archive_state_file_name, ptr);
      }

      /* configuration file parameter: HeartBeatInterval - the rate to send heart beats */
      else if (k_its ("HeartBeatInterval"))
        config->heartbeat_interval = k_int ();

      /* configuration file parameter: LogFile - the log file code - see logit_init() */
      else if (k_its ("LogFile"))
        config->log_file_flag = k_int ();

      /* configuration file parameter: LogDataMessages - if present log data messages */
      else if (k_its ("LogDataMessages"))
        config->log_data_msgs = TRUE;

      /* configuration file parameter: Verbose - if present show debug info. on console */
      else if (k_its ("Verbose"))
        config->verbose = TRUE;

      /* configuration parameter: size of continuous data archive files (hours, mins, secs) */
      else if (k_its ("ContinuousDataSegmentSize"))
      {
        /* get the continuous data segment size and units */
        config->continuous_data_segment_size = k_int ();
        ptr = k_str ();
        if (! ptr) multiplier = 60;
        else if ((! stricmp (ptr, "s")) ||
                 (! stricmp (ptr, "sec")) ||
                 (! stricmp (ptr, "secs")) ||
                 (! stricmp (ptr, "second")) ||
                 (! stricmp (ptr, "seconds"))) multiplier = 1;
        else if ((! stricmp (ptr, "m")) ||
                 (! stricmp (ptr, "min")) ||
                 (! stricmp (ptr, "mins")) ||
                 (! stricmp (ptr, "minute")) ||
                 (! stricmp (ptr, "minutes"))) multiplier = 60;
        else if ((! stricmp (ptr, "h")) ||
                 (! stricmp (ptr, "hour")) ||
                 (! stricmp (ptr, "hours"))) multiplier = 3600;
	else multiplier = 0;

	/* check the segment size */
	switch (multiplier)
	{
	case 1:
	case 60:
	  if ((config->continuous_data_segment_size <= 0) ||
	      (config->continuous_data_segment_size > 30) ||
 	      (60 % config->continuous_data_segment_size))
	  {
	    fprintf (err_stream,
        	     "%s: Invalid continuous data segment size <%d>; exitting!\n",
	             config->executable_name, config->continuous_data_segment_size);
	    exit (1);
	  }
	  break;
	case 3600:
	  if ((config->continuous_data_segment_size <= 0) ||
	      (config->continuous_data_segment_size > 24) ||
 	      (24 % config->continuous_data_segment_size))
	  {
	    fprintf (err_stream,
        	     "%s: Invalid continuous data segment size <%d>; exitting!\n",
	             config->executable_name, config->continuous_data_segment_size);
	    exit (1);
	  }
	  break;
	default:
          fprintf (err_stream, "%s: Continuous data size must be in hours, minutes or seconds; exitting!\n",
                   config->executable_name);
          exit (1);
	}
	config->continuous_data_segment_size *= multiplier;
      }

      /* configuration parameter: delay between data being recorded and archived (mins) */
      else if (k_its ("ContinuousDataDelay"))
      {
        ptr = k_str ();
	hour_string = strtok (ptr, ":");
	minute_string = strtok ((char *) 0, ":");
	second_string = strtok ((char *) 0, ":");
	if (second_string)
	{
	  if (! check_number (hour_string, FALSE, FALSE) ||
	      ! check_number (minute_string, FALSE, FALSE) ||
	      ! check_number (second_string, FALSE, FALSE))
	  {
            fprintf (err_stream, "%s: Continuous data delay must use numeric hours, minutes and seconds; exitting!\n",
                   config->executable_name);
	    exit (1);
	  }
	  config->continuous_data_delay = (atoi (hour_string) * 3600) +
	                                  (atoi (minute_string) * 60) +
					   atoi (second_string);
	}
	else
	{
	  if (! check_number (ptr, FALSE, FALSE))
	  {
            fprintf (err_stream, "%s: Continuous data delay must use numeric delay; exitting!\n",
                   config->executable_name);
	    exit (1);
	  }
          config->continuous_data_delay = atoi (ptr);
          ptr = k_str ();
          if (! ptr) multiplier = 60;
          else if ((! stricmp (ptr, "s")) ||
                   (! stricmp (ptr, "sec")) ||
                   (! stricmp (ptr, "secs")) ||
                   (! stricmp (ptr, "second")) ||
                   (! stricmp (ptr, "seconds"))) multiplier = 1;
          else if ((! stricmp (ptr, "m")) ||
                   (! stricmp (ptr, "min")) ||
                   (! stricmp (ptr, "mins")) ||
                   (! stricmp (ptr, "minute")) ||
                   (! stricmp (ptr, "minutes"))) multiplier = 60;
          else if ((! stricmp (ptr, "h")) ||
                   (! stricmp (ptr, "hour")) ||
                   (! stricmp (ptr, "hours"))) multiplier = 3600;
	  else
	  { 
            fprintf (err_stream, "%s: Continuous data delay must be in hours, minutes or seconds; exitting!\n",
                   config->executable_name);
            exit (1);
	  }
	  config->continuous_data_delay *= multiplier;
	}
      }

      /* configuration parameter: MoveDir - a pair of
       * directories to move files from/to */
      else if (k_its ("MoveDir"))
      {
        found = TRUE;
        if ( (from_dir = k_str ()) == NULL ) found = FALSE;
        else if ( (to_dir = k_str ()) == NULL ) found = FALSE;
        if (! found)
        {
          fprintf (err_stream, "%s: Bad MoveDir command; exitting!\n",
                   config->executable_name);
          exit (1);
        }
        dir_list_ptr = realloc (config->dir_list, 
                                sizeof (struct Dir_pair) * (config->n_dirs +1));
        if (! dir_list_ptr)
        {
          fprintf (err_stream, "%s: Not enough memory; exitting!\n",
                   config->executable_name);
          exit (1);
        }
        config->dir_list = dir_list_ptr;
        dir_list_ptr += config->n_dirs;
        config->n_dirs += 1;
        strcpy (dir_list_ptr->source, from_dir);
        strcpy (dir_list_ptr->dest, to_dir);
      }

      /* configuration parameter: SaveSCNL - an s.c.n. to archive */
      else if (k_its ("SaveSCNL"))
      {
        found = TRUE;
        if ( (station = k_str ()) == NULL ) found = FALSE;
        else if ( (component = k_str ()) == NULL ) found = FALSE;
        else if ( (network = k_str ()) == NULL ) found = FALSE;
	/* RL v7.0 */
        else if ( (location = k_str ()) == NULL ) found = FALSE;
        if (! found)
        {
          fprintf (err_stream, "%s: Bad SaveSCNL command; exitting!\n",
                   config->executable_name);
          exit (1);
        }
        scnl_list_ptr = realloc (config->scnl_list, 
                                sizeof (struct SCNL) * (config->n_scnls +1));
        if (! scnl_list_ptr)
        {
          fprintf (err_stream, "%s: Not enough memory; exitting!\n",
                   config->executable_name);
          exit (1);
        }
        config->scnl_list = scnl_list_ptr;
        scnl_list_ptr += config->n_scnls;
        config->n_scnls += 1;
        strcpy (scnl_list_ptr->station, station);
        strcpy (scnl_list_ptr->component, component);
        strcpy (scnl_list_ptr->network, network);
        strcpy (scnl_list_ptr->location, location);
      }

      /* configuration parameter: WaveServer - contact details */
      else if (k_its ("WaveServer"))
      {
        found = TRUE;
        if ( (ip_address = k_str ()) == NULL ) found = FALSE;
        ip_port = k_int ();
        if (! found)
        {
          fprintf (err_stream, "%s: Bad WaveServer command; exitting!\n",
                   config->executable_name);
          exit (1);
        }
        waveserver_list_ptr = realloc (config->waveserver_list, 
                                       sizeof (struct Waveserver) * (config->n_waveservers +1));
        if (! waveserver_list_ptr)
        {
          fprintf (err_stream, "%s: Not enough memory; exitting!\n",
                   config->executable_name);
          exit (1);
        }
        config->waveserver_list = waveserver_list_ptr;
        waveserver_list_ptr += config->n_waveservers;
        config->n_waveservers += 1;
        strcpy (waveserver_list_ptr->ip_address, ip_address);
        waveserver_list_ptr->ip_port = ip_port;
      }

      /* a bunch of configuration parameters passed through to waveman2disk -
       * see waveman2disk.d for details */
      else if (k_its ("TimeoutSeconds")) config->timeout_seconds = k_int ();
      else if (k_its ("TravelTimeout")) config->travel_timeout = k_int ();
      else if (k_its ("MaxTraces")) config->max_traces = k_int ();
      else if (k_its ("TraceBufferLen")) config->trace_buffer_len = k_int ();
      else if (k_its ("MaxTraceMsg")) config->max_trace_msg = k_int ();
      else if (k_its ("GapThresh")) config->gap_thresh = k_int ();
      else if (k_its ("MinDuration")) config->min_duration = k_int ();

      /* the output format details for waveman2disk */
      else if (k_its ("DataFormat"))
      {
        ptr = k_str ();
        strcpy (config->data_format, ptr);
      }
      else if (k_its ("OutDir"))
      {
        ptr = k_str ();
        strcpy (config->out_dir, ptr);
      }
      else if (k_its ("OutputFormat"))
      {
        ptr = k_str ();
        strcpy (config->output_format, ptr);
      }
      else if (k_its ("StartTime"))
      {
        ptr = k_str ();
        strncpy (config->start_time, ptr, 20);
      }
      else if (k_its ("ContinuousOutput"))
      {
        config->continuous_output = TRUE;
      }

      /* catch-all for config. errors */
      else
        fprintf (err_stream, "<%s> Unknown command in <%s>.\n",
                 command, cfg_filename);

      /* See if there were any errors processing the command */
      if( k_err() )
      {
        fprintf (err_stream,
                 "Bad <%s> command in <%s>; exiting!\n",
                 command, cfg_filename);
        exit (1);
      }
    }

    /* close the currently open file */
    n_files = k_close();
  }

  /* check that compulsory parameters are present */
  if (! config->module_name [0])
  {
    fprintf (err_stream, 
            "%s: missing module name in configuration file; exitting!\n",
             config->executable_name);
    exit (1);
  }
  if (! config->in_ring_name [0])
  {
    fprintf (err_stream, 
            "%s: missing input ring name in configuration file; exitting!\n",
             config->executable_name);
    exit (1);
  }
  if (! config->out_ring_name [0])
  {
    fprintf (err_stream, 
            "%s: missing output ring name in configuration file; exitting!\n",
             config->executable_name);
    exit (1);
  }
  if (config->heartbeat_interval < 0)
  {
    fprintf (err_stream, 
            "%s: missing heartbeat interval in configuration file; exitting!\n",
             config->executable_name);
    exit (1);
  }
  if (config->log_file_flag < 0)
  {
    fprintf (err_stream, 
            "%s: missing log file flag in configuration file; exitting!\n",
             config->executable_name);
    exit (1);
  }

  /* check that there are wave servers to get continuous data from */
  if ((config->continuous_data_segment_size > 0) ||
      (config->n_waveservers > 0))
  {
    if (config->continuous_data_segment_size <= 0)
    {
      fprintf (err_stream,
               "%s: Missing ContinuousDataSegmentSize parameter; exitting!\n",
             config->executable_name);
      exit (1);
    }
    if (config->n_waveservers <= 0)
    {
      fprintf (err_stream,
               "%s: Missing WaveServer parameter; exitting!\n",
             config->executable_name);
      exit (1);
    }
  }

  /* check the list of 'MoveFrom/To' directories */
  for (count=0; count<config->n_dirs; count++)
  {
    dir_list_ptr = config->dir_list + count;
    if (! check_dir (dir_list_ptr->source))
    {
      fprintf (err_stream,
               "%s: Bad directory <%s>; exitting!\n",
               config->executable_name, dir_list_ptr->source);
      exit (1);    
    }
    if (! check_dir (dir_list_ptr->dest))
    {
      fprintf (err_stream,
               "%s: Bad directory <%s>; exitting!\n",
               config->executable_name, dir_list_ptr->dest);
      exit (1);    
    }
  }

  /* Look up keys to shared memory regions */
  config->in_ring_key = GetKey (config->in_ring_name);
  if (config->in_ring_key == -1)
  {
    fprintf (err_stream,
             "%s: Invalid ring name <%s>; exitting!\n",
             config->executable_name, config->in_ring_name);
    exit (1);
  }
  config->out_ring_key = GetKey (config->out_ring_name);
  if (config->out_ring_key == -1)
  {
    fprintf (err_stream,
             "%s: Invalid ring name <%s>; exitting!\n",
             config->executable_name, config->out_ring_name);
    exit (1);
  }

  /* Look up installations of interest */
  if (GetLocalInst (&(config->installation_id)) != 0)
  {
    fprintf( err_stream, 
            "%s: error getting local installation id; exitting!\n",
             config->executable_name);
    exit (1);
  }

  /* Look up modules of interest */
  if (GetModId (config->module_name, &(config->module_id)) != 0 )
  {
    fprintf( err_stream, 
            "%s: Invalid module name <%s>; exitting!\n",
             config->executable_name, config->module_name);
    exit (1);
  }

  /* Look up message types of interest */
  for (count=0; count<3; count++)
  {
    switch (count)
    {
    case 0: name = "TYPE_HEARTBEAT"; data_ptr = &(config->type_heartbeat); break;
    case 1: name = "TYPE_ERROR";     data_ptr = &(config->type_error); break;
    case 2: name = "TYPE_TRACEBUF2";  data_ptr = &(config->type_tracebuf2); break;      /* RL v7.0 */
    }
    if (GetType (name, data_ptr) != 0)
    {
      fprintf (err_stream, 
               "%s: Invalid message type <%s>; exitting!\n",
               config->executable_name, name);
      exit (1);
    }
  }

  /* Get our Process ID for restart purposes */
  config->pid = getpid();
  if (config->pid == -1)
  {
    fprintf( err_stream, 
            "%s: error getting process id (pid); exitting!\n",
             config->executable_name);
    exit (1);
  }

  /* initialise the log file */
  logit_init (cfg_filename, (short) config->module_id, 256, config->log_file_flag);
  logit ("t" , "%s(%s): Read command file <%s>\n", 
         config->executable_name, config->module_name, cfg_filename);

}

/******************************************************************
 * write_ew_heartbeat
 *
 * Description: write a heartbeat to a transport ring
 *
 * Input parameters: config - the program's configuration
 *                   current_system_time - CPU clock
 * Output parameters: none
 * Returns: none
 *
 * Comments:
 *
 ******************************************************************/
void write_ew_heartbeat (struct Config *config, time_t current_system_time)

{

  int do_heartbeat;
  long size;
  char buffer [256];
  time_t bin_time;
  MSG_LOGO logo;

  static int first = TRUE;
  static time_t last_heartbeat_time;



  /* check that we are sending heartbeats */
  if (config->heartbeat_interval <= 0) return;

  /* always send a heartbeat on the first call */
  do_heartbeat = FALSE;
  if (first)
  {
    do_heartbeat = TRUE;
    first = FALSE;
  }
  else
  {
    /* is it time to send the next heartbeat ?? */
    if ((current_system_time - last_heartbeat_time) > config->heartbeat_interval)
      do_heartbeat = TRUE;
  }

  /* send the heartbeat */
  if (do_heartbeat)
  {
    if (config->verbose) printf ("archman: Sending heartbeat\n");
    last_heartbeat_time = current_system_time;

    /* Build the message */
    logo.instid = config->installation_id;
    logo.mod = config->module_id;
    logo.type = config->type_heartbeat;
    time (&bin_time);
    sprintf (buffer, "%ld %d\n", (long)bin_time, config->pid);
    size = (long)strlen (buffer);

    /* Write the message to shared memory */
    if (tport_putmsg (&(config->out_shared_mem_region), &logo, size, buffer) != PUT_OK)
      logit ("et", "%s(%s):  Error sending heartbeat.\n",
             config->executable_name, config->module_name);
  }

}

/******************************************************************
 * write_ew_error
 *
 * Description: write a heartbeat to a transport ring
 *
 * Input parameters: config - the program's configuration
 *                   errno - error number
 *                   message - the error message
 * Output parameters: none
 * Returns: none
 *
 * Comments: The errno is the 'err:' parameter in the error message
 *           record of statmgr's description (.desc) files
 *
 ******************************************************************/
void write_ew_error (struct Config *config, short errno, char *message)

{

  long size;
  char buffer [256];
  time_t bin_time;
  MSG_LOGO logo;


  /* Build the message */
  logo.instid = config->installation_id;
  logo.mod = config->module_id;
  logo.type = config->type_heartbeat;
  time (&bin_time);
  sprintf (buffer, "%ld %hd %s\n", (long)bin_time, errno, message);
  size = (long)strlen (buffer);

  /* log it as well */
  logit ("t", "%s(%s): %s\n", config->executable_name,
         config->module_name, message);

  /* Write the message to shared memory */
  if (tport_putmsg (&(config->out_shared_mem_region), &logo, size, buffer) != PUT_OK)
    logit ("et", "%s(%s):  Error sending error:%d.\n",
           config->executable_name, config->module_name, errno);

}

/******************************************************************
 * get_time_data
 *
 * Description: get trace_buf messages and extract the time
 *
 * Input parameters: config - the program's configuration
 * Output parameters: msg_time - the date/time of the message
 * Returns: TRUE if data was available, FALSE otherwise
 *
 * Comments:
 * 
 ***************************************************************************/
int get_time_data (struct Config *config, double *msg_time)

{

  int status, ret_val, year, month, day, hour, min, sec, milli;
  long found_size;
  double duration;
  char msg [MAX_TRACEBUF_SIZ], last_msg [MAX_TRACEBUF_SIZ];
  MSG_LOGO found_logo;
  TRACE_HEADER *trace_header;
  time_t epoch_time;
  struct tm cal_time;

  static int first = TRUE;
  static MSG_LOGO request_logo;


  if (first)
  {
    /* Specify logos to get */
    request_logo.type = config->type_tracebuf2;      /* RL v7.0 */
    GetModId ("MOD_WILDCARD", &request_logo.mod);
    GetInst ("INST_WILDCARD", &request_logo.instid);

    first = FALSE;
  }

  /* flush the input ring, keeping the last message */
  ret_val = FALSE;
  do
  {
    status = tport_getmsg (&(config->in_shared_mem_region), &request_logo,
                           (short) 1, &found_logo, &found_size,
	  		   (char *) &msg, MAX_TRACEBUF_SIZ);
    switch (status)
    {
    case GET_NONE:
    case GET_TOOBIG:
    case GET_NOTRACK:
      break;
    default:
      memcpy (last_msg, msg, MAX_TRACEBUF_SIZ);
      ret_val = TRUE;
      break;
    }
  } while (status != GET_NONE);

  if (ret_val)
  {
    /* get the trace header */
    trace_header  = (TRACE_HEADER *) msg;
    WaveMsgMakeLocal (trace_header);
    
    /* write to the log file */
    if (config->log_data_msgs)
    {
      epoch_time = (time_t) trace_header->starttime;
      gmtime_ew (&epoch_time, &cal_time);
      year  = cal_time.tm_year + 1900;
      month = cal_time.tm_mon + 1;
      day   = cal_time.tm_mday;
      hour  = cal_time.tm_hour;
      min   = cal_time.tm_min;
      sec   = cal_time.tm_sec;
      milli = (int)((trace_header->starttime * 1000.0) - ((double) epoch_time * 1000.0));
      duration = trace_header->endtime - trace_header->starttime;
      logit ("t", "Data: %s.%s.%s %02d-%02d-%04d %02d:%02d:%02d.%03d %d %.3lf %.1lf\n",
	      trace_header->sta, trace_header->chan, trace_header->net,
	      day, month, year, hour, min, sec, milli,
	      trace_header->nsamp, duration,
	      trace_header->samprate);
    }
    
    /* extract the time */
    *msg_time = trace_header->endtime;
  }

  return ret_val;

}

/****************************************************************************
 * is_next_archive_needed
 *
 * Description: work out whether there is enough data to extract the next
 *              archive
 *
 * Input parameters: config - the program configuration
 *                   data_time - date/time of the newest data block from the
 *                               transport ring
 *                   next_block_start_time - date/time of the start of the next
 *                                           block of data to archive
 * Output parameters: next_block_start_time is set of the first call
 * Returns: TRUE if data should be archived
 *
 * Comments: next_block_start_time need not be initialised as it is filled in on
 *           the first call
 *
 ****************************************************************************/
int is_next_archive_needed (struct Config *config, time_t data_time,
                            time_t *next_block_start_time)

{

  int found, count, remainder;
  struct tm unix_time;
  FILE *fp;

  static int first = TRUE;


  /* check if we are configured to archive */
  if (config->continuous_data_segment_size <= 0) return FALSE;

  /* on the first run through, attempt to load the date/time of the next
   * data block to archive from the program's state file */
  if (first)
  {
    found = FALSE;
    if (config->archive_state_file_name [0])
    {
      fp = fopen (config->archive_state_file_name, "r");
      if (fp)
      {
        if (fscanf (fp, "Date: %d-%d-%d\nTime: %d:%d:%d\n",
                    &(unix_time.tm_mday), &(unix_time.tm_mon),
                    &(unix_time.tm_year), &(unix_time.tm_hour),
                    &(unix_time.tm_min), &(unix_time.tm_sec)) == 6)
        {
          if (check_clock (unix_time.tm_sec, unix_time.tm_min,
                           unix_time.tm_hour, unix_time.tm_mday,
                           unix_time.tm_mon, unix_time.tm_year))
          {
            unix_time.tm_year -= 1900;
            unix_time.tm_mon --;
            found = TRUE;
          }
        }
        fclose (fp);
      }
    }


    /* if the time wasn't stored, use the current data time as the
     * first data to archive */
    if (! found)
	{
	  //If there was a valid date passed in StartDate use that
#ifdef _WINNT
	  if(!win_strptime(config->start_time, "%Y/%m/%d %T", &unix_time)) {
#else
	  if(!strptime(config->start_time, "%Y/%m/%d %T", &unix_time)) {
#endif
        memcpy (&unix_time, gmtime (&data_time), sizeof (struct tm));
      }
	}

    /* move the start time down to the nearest integer multiple of the
     * duration of the archive files */
    if (config->continuous_data_segment_size >= 3600)
    {
      count = config->continuous_data_segment_size / 3600;
      remainder = unix_time.tm_hour % count;
      unix_time.tm_min = unix_time.tm_sec = 0;
      unix_time.tm_hour -= remainder;
    }
    else if (config->continuous_data_segment_size >= 60)
    {
      count = config->continuous_data_segment_size / 60;
      remainder = unix_time.tm_min % count;
      unix_time.tm_sec = 0;
      unix_time.tm_min -= remainder;
    }
    else
    {
      remainder = unix_time.tm_sec % config->continuous_data_segment_size;
      unix_time.tm_sec -= remainder;
    }

    /* convert to elapse time */
    unix_time.tm_isdst = 0;
    *next_block_start_time = timegm_ew (&unix_time);
    if (config->verbose) printf ("archman: Start archive date/time: %02d-%02d-%04d %02d:%02d:%02d (%ld)\n",
                                 unix_time.tm_mday, unix_time.tm_mon +1, unix_time.tm_year + 1900,
                                 unix_time.tm_hour, unix_time.tm_min, unix_time.tm_sec,
                                 (long) *next_block_start_time);

    first = FALSE;
  }

  /* is there enough data to archive the next block ?? */
  if ((data_time - *next_block_start_time) >
      (config->continuous_data_segment_size + config->continuous_data_delay))
  {
    if (config->verbose) printf ("archman: Found data to archive (%ld %ld)\n",
                                 (long) data_time, (long) (*next_block_start_time));
    return TRUE;
  }
  return FALSE;

}

/****************************************************************************
 * archive_data
 *
 * Description: work out when it is time or the next block of archive data,
 *              then use waveman2disk to write the data
 *
 * Input parameters: config - the program configuration
 *                   start_time - date/time of the start of the data block
 * Output parameters: start_time is modified
 * Returns: none
 *
 * Comments:
 *
 ****************************************************************************/
void archive_data (struct Config *config, time_t *start_time)

{

  int count;
  char tmp_filename [MAX_DIR_LEN], command [MAX_DIR_LEN + 50];
  struct tm unix_time;
  struct SCNL *scnl_ptr;
  struct Waveserver *waveserver_ptr;
  FILE *fp;
#ifndef _WINNT
  int fd;
#endif

  /* build the waveman2disk configuration file */
#ifndef _WINNT
  strcpy(tmp_filename, "tmp.XXXXXXXXX");
  if ((fd = mkstemp (tmp_filename)) == -1)
    logit ("e", "Unable to open temporary file in current dir %s\n", tmp_filename);
    /* convert fd to fp */
  else if (  (fp = fdopen(fd, "w")) == NULL)
    logit ("e", "Unable to write to temporary file using fdopen()\n");
#else
   /* the windows alternative, the original code using insecure tmpnam() */
	  /* build the waveman2disk configuration file */
   if ( tmpnam (tmp_filename) == NULL )
     logit ("e", "Unable to get a temporary file name\n");
   else if ((fp = fopen (tmp_filename, "w")) == NULL )
     logit ("e", "Unable to open temporary file %s\n", tmp_filename);
#endif
  else
  {
    fprintf (fp, "# Waveman2disk configuration file\n");
    fprintf (fp, "# automatically generated by %s\n", config->executable_name);
    fprintf (fp, "LogFile %d\n", config->log_file_flag);
    if (config->verbose) fprintf (fp, "Debug\n");
    fprintf (fp, "InputMethod interactive\n");
    if (config->n_scnls < 0) fprintf (fp, "SaveSCNL * * * *\n");
    else
    {
      for (count=0; count<config->n_scnls; count++)
      {
	/* RL v7.0 update */
        scnl_ptr = config->scnl_list + count;
        fprintf (fp, "SaveSCNL %s %s %s %s\n",
                 scnl_ptr->station,scnl_ptr->component,scnl_ptr->network,scnl_ptr->location);
      }
    }
    memcpy (&unix_time, gmtime (start_time), sizeof (struct tm));
    unix_time.tm_year += 1900;
    unix_time.tm_mon += 1;
    fprintf (fp, "StartTime %04d%02d%02d%02d%02d%02d\n",
               unix_time.tm_year, unix_time.tm_mon,
               unix_time.tm_mday, unix_time.tm_hour,
               unix_time.tm_min, unix_time.tm_sec);
    fprintf (fp, "Duration %d\n", config->continuous_data_segment_size);
    for (count=0; count<config->n_waveservers; count++)
    {
      waveserver_ptr = config->waveserver_list + count;
      fprintf (fp, "WaveServer %s %d\n",
               waveserver_ptr->ip_address, waveserver_ptr->ip_port);
    }
    fprintf (fp, "TimeoutSeconds %d\n", config->timeout_seconds);
    fprintf (fp, "MaxTraces %d\n", config->max_traces);
    fprintf (fp, "TraceBufferLen %ld\n", config->trace_buffer_len);
    fprintf (fp, "GapThresh %d\n", config->gap_thresh);
    fprintf (fp, "MinDuration %d\n", config->min_duration);
    fprintf (fp, "DataFormat %s\n", config->data_format);
    fprintf (fp, "OutDir %s\n", config->out_dir);
    fprintf (fp, "OutputFormat %s\n", config->output_format);

    /* check and close the config. file */
    if (ferror (fp))
    {
      fclose (fp);
      logit ("e", "Error writing to temporary file %s\n", tmp_filename);
    }
    else
    {
      fclose (fp);

      /* run waveman2disk */
      sprintf (command, "waveman2disk %s", tmp_filename);
      if (config->verbose) printf ("archman: Executing <%s>\n", command);
      system (command);

      /* tidy up */
      remove (tmp_filename);
  
      /* advance the date/time for the next block and store it */
      *start_time += config->continuous_data_segment_size;
      memcpy (&unix_time, gmtime (start_time), sizeof (struct tm));
      unix_time.tm_year += 1900;
      unix_time.tm_mon += 1;
      fp = fopen (config->archive_state_file_name, "w");
      if (fp)
      {
        fprintf (fp, "Date: %02d-%02d-%04d\nTime: %02d:%02d:%02d\n",
                 unix_time.tm_mday, unix_time.tm_mon,
                 unix_time.tm_year, unix_time.tm_hour,
                 unix_time.tm_min, unix_time.tm_sec);
        fclose (fp);
      }
    }
  }

}
/****************************************************************************
 * archive_data_passive
 *
 * Description: work out when it is time or the next block of archive data,
 *              then use waveman2disk to write the data
 *
 * Input parameters: config - the program configuration
 *                   start_time - date/time of the start of the data block
 * Output parameters: start_time is NOT modified
 * Returns: none
 *
 * Comments:
 *
 ****************************************************************************/
void archive_data_passive (struct Config *config, time_t *start_time)

{

  int count;
  char tmp_filename [MAX_DIR_LEN], command [MAX_DIR_LEN + 50];
  struct tm unix_time;
  struct SCNL *scnl_ptr;
  struct Waveserver *waveserver_ptr;
  FILE *fp;
#ifndef _WINNT
  int fd;
#endif

#ifndef _WINNT
  strcpy(tmp_filename, "tmp.XXXXXXXXXX");
  if ((fd = mkstemp (tmp_filename)) == -1)
    logit ("e", "Unable to open temporary file in current dir %s\n", tmp_filename);
    /* convert fd to fp */
  else if (  (fp = fdopen(fd, "w")) == NULL)
    logit ("e", "Unable to write to temporary file using fdopen()\n");
#else
  /* build the waveman2disk configuration file */
  if ( tmpnam (tmp_filename) == NULL )
    logit ("e", "Unable to get a temporary file name\n");
  else if ( (fp = fopen (tmp_filename, "w")) == NULL)
    logit ("e", "Unable to open temporary file %s\n", tmp_filename);
#endif
  else
  {
    fprintf (fp, "# Waveman2disk configuration file\n");
    fprintf (fp, "# automatically generated by %s\n", config->executable_name);
    fprintf (fp, "LogFile %d\n", config->log_file_flag);
    if (config->verbose) fprintf (fp, "Debug\n");
    fprintf (fp, "InputMethod interactive\n");
    if (config->n_scnls < 0) fprintf (fp, "SaveSCNL * * * *\n");
    else
    {
      for (count=0; count<config->n_scnls; count++)
      {
	/* RL v7.0 update */
        scnl_ptr = config->scnl_list + count;
        fprintf (fp, "SaveSCNL %s %s %s %s\n",
                 scnl_ptr->station,scnl_ptr->component,scnl_ptr->network,scnl_ptr->location);
      }
    }
    memcpy (&unix_time, gmtime (start_time), sizeof (struct tm));
    unix_time.tm_year += 1900;
    unix_time.tm_mon += 1;
    fprintf (fp, "StartTime %04d%02d%02d%02d%02d%02d\n",
               unix_time.tm_year, unix_time.tm_mon,
               unix_time.tm_mday, unix_time.tm_hour,
               unix_time.tm_min, unix_time.tm_sec);
    fprintf (fp, "Duration %d\n", config->continuous_data_segment_size);
    for (count=0; count<config->n_waveservers; count++)
    {
      waveserver_ptr = config->waveserver_list + count;
      fprintf (fp, "WaveServer %s %d\n",
               waveserver_ptr->ip_address, waveserver_ptr->ip_port);
    }
    fprintf (fp, "TimeoutSeconds %d\n", config->timeout_seconds);
    fprintf (fp, "MaxTraces %d\n", config->max_traces);
    fprintf (fp, "TraceBufferLen %ld\n", config->trace_buffer_len);
    fprintf (fp, "GapThresh %d\n", config->gap_thresh);
    fprintf (fp, "MinDuration %d\n", config->min_duration);
    fprintf (fp, "DataFormat %s\n", config->data_format);
    fprintf (fp, "OutDir %s\n", config->out_dir);
    fprintf (fp, "OutputFormat %s\n", config->output_format);

    /* check and close the config. file */
    if (ferror (fp))
    {
      fclose (fp);
      logit ("e", "Error writing to temporary file %s\n", tmp_filename);
    }
    else
    {
      fclose (fp);

      /* run waveman2disk */
      sprintf (command, "waveman2disk %s", tmp_filename);
      if (config->verbose) printf ("archman: Executing <%s>\n", command);
      system (command);

      /* tidy up */
      remove (tmp_filename);
  
      /* advance the date/time for the next block and store it */
      //*start_time += config->continuous_data_segment_size;
      //memcpy (&unix_time, gmtime (start_time), sizeof (struct tm));
      //unix_time.tm_year += 1900;
      //unix_time.tm_mon += 1;
      //fp = fopen (config->archive_state_file_name, "w");
      //if (fp)
      //{
      //  fprintf (fp, "Date: %02d-%02d-%04d\nTime: %02d:%02d:%02d\n",
      //           unix_time.tm_mday, unix_time.tm_mon,
      //           unix_time.tm_year, unix_time.tm_hour,
      //           unix_time.tm_min, unix_time.tm_sec);
      //  fclose (fp);
      //}
    }
  }

}

/****************************************************************************
 * move_data
 *
 * Description: move all files in the source dir. to the destination
 *
 * Input parameters: config - the program configuration
 *                   source_dir - the directory to copy from
 *                   dest_dir - the directory to copy to
 * Output parameters: none
 * Returns: none
 *
 * Comments:
 *
 ****************************************************************************/
void move_data (struct Config *config, char *source_dir, char *dest_dir)

{

  char source_filename [MAX_DIR_LEN], dest_filename [MAX_DIR_LEN];
  struct File_info *info;


  /* for each file in the source directory ... */
  for (info = get_next_file (source_dir); info; info = get_next_file (0))
  {
    if (info->file_type == FILE_NORMAL)
    {
      /* make full path names for source and destination */
      strcpy (source_filename, source_dir);
      terminate_path (source_filename, TRUE);
      strcat (source_filename, info->name);
      strcpy (dest_filename, dest_dir);
      terminate_path (dest_filename, TRUE);
      strcat (dest_filename, info->name);

      /* copy the file */
      if (config->verbose) printf ("archman: Copying %s to %s\n", source_filename, dest_filename);
      switch (copy_file (source_filename, dest_filename, TRUE))
      {
      case COPY_OK:
        if (remove (source_filename))
          logit ("e", "Error deleting file (%s)\n", source_filename);
        break;
      default:
        logit ("e", "Error copying file (%s to %s)\n", source_filename, dest_filename);
        break;
      }
    }
  }

}


