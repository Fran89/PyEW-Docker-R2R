/* read_sdas_data.c - routines for getting SDAS data and
 *                    acilliary info. from a remote SDAS system by IP
 *
 * S. Flower, Aug 2000 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _SOLARIS
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/uio.h>
#endif

#include "zlib.h"

#include "geolib.h"
#include "sdas.h"

/* codes for the type of compression */
#define COMPRESS_ASCII      1
#define COMPRESS_BINARY     2
#define COMPRESS_ZLIB       3

/* bit-mapped flags for process_command() */
#define EXPECT_CLOS_CODE		0x01

/* return codes for process_command() */
#define PC_OK					0
#define PC_CLOS					1
#define PC_REMOTE				2
#define PC_ERROR				3

/* private global variables */
static int socket;      /* socket handle for connection to server */
static int compression; /* type of compression used for data transfer */
static int verbose = GEO_FALSE; /* use for debugging */
static int n_channels = -1;     /* number of channels on remote SDAS */
static char error_msg [200];

/* private forward declarations */
static int process_command (int socket, char *command, int flags,
                            int *err_code, char *err_msg);
static struct Acquire_cfg *add_to_acq_list (struct Acquire_cfg *new_acq_rec);
static struct Channels_cfg *add_to_chan_list (struct Channels_cfg *new_chan);
static struct Channel_group *download_channel_groups (int socket, int *nr, char *errmsg);
static struct Channel_group *free_channel_groups (struct Channel_group *channel_groups, int *n_recs);

/***************************************************************************
 * init_sdas_reading
 *
 * Description: initialise the routines used to read SDAS data
 *
 * Input paramters: ip_addr, ip_port - IP connection details 
 *                  timeout - inactivity timer for the IP connection (in secs)
 *                            (-ve == no timeout testing)
 * Output parameters: none
 * Returns: ISR_OK - the initialisation completed OK
 *          ISR_NO_CONN - unable to connect to server
 *          ISR_SERR - unable to exchange information with server
 *
 * Comments: This routine must be called before calling ANY other routine
 *           in this module
 *
 **************************************************************************/
int init_sdas_reading (char *ip_addr, int ip_port, int timeout)

{

  int error_code;
  long bint_value [2];
  char command [100];
  

  /* initialise */
  n_channels = -1;
  strcpy (error_msg, "");
  if (getenv ("SDAS_DEBUG")) verbose = GEO_TRUE;

#ifdef WIN32
  /* initialise Windows sockets */
  if (! init_winsock ())
  {
    strcpy (error_msg, "Windows Sockets library not available or incorrect version");
    return ISR_NO_CONN;
  }
#endif

  /* attempt a connection to the server */
  if (! connect_to_server (ip_addr, ip_port, -1, timeout, GEO_TRUE, &socket))
  {
    sprintf (error_msg, "Unable to connect to %s on port %d", ip_addr, ip_port);
    return ISR_NO_CONN;
  }

  /* on program exit shutdown the connection */
  atexit (shutdown_sdas_reading);

  /* check whether bytes need to be swapped (at the server end) */
  strcpy (command, "BINT ");
  bint_value [0] = 0x41424344l;
  bint_value [1] = 0;
  memcpy (&command [strlen (command)], bint_value, sizeof (bint_value));
  if (process_command (socket, command, 0, &error_code, error_msg) != PC_OK)
    return ISR_SERR;

  /* set the compression type to zlib */
  sprintf (command, "CMPR ZLIB %s", ZLIB_VERSION);
  if (process_command (socket, command, 0, &error_code, error_msg) != PC_OK)
  {
    sprintf (command, "CMPR BINARY", ZLIB_VERSION);
    if (process_command (socket, command, 0, &error_code, error_msg) != PC_OK)
    {
      sprintf (command, "CMPR ASCII", ZLIB_VERSION);
      if (process_command (socket, command, 0, &error_code, error_msg) != PC_OK)
        return ISR_SERR;
      else compression = COMPRESS_ASCII;
    }
    else compression = COMPRESS_BINARY;
  }
  else compression = COMPRESS_ZLIB;

  return ISR_OK;

}

/**************************************************************************
 * shutdown_sdas_reading
 *
 * Description: close the connection
 *
 * Input parameters: none
 * Output parameters: none
 * Returns: none
 *
 * Comments:
 *
 **************************************************************************/
void shutdown_sdas_reading (void)

{

  int error_code;
  

  process_command (socket, "CLOS", EXPECT_CLOS_CODE,
                   &error_code, error_msg);
  close_socket (socket);

}

/**************************************************************************
 * get_n_channels
 *
 * Description: get the number of channels with logged data
 *
 * Input parameters: none
 * Output parameters: none
 * Returns: the number of channels OR -1 for an error
 *
 * Comments:
 *
 **************************************************************************/
int get_n_channels (void)

{

  int status, length, error_code;
  char string [100];
  

  strcpy (error_msg, "");

  /* do we have the information cached */
  if (n_channels >= 0) return n_channels;

  /* write the command to get the number of channels */
  status = process_command (socket, "NCHA", 0, &error_code, error_msg);
  if (status != PC_OK) return -1;

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return -1;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* parse the information */
  if (! check_number (string, GEO_FALSE, GEO_FALSE))
  {
    strcpy (error_msg, "bad response from server");
    return -1;
  }
  n_channels = atoi (string);
  return n_channels;

}

/**************************************************************************
 * get_channel
 *
 * Description: get SDAS channel number
 *
 * Input parameters: chan_index - channel index (0 -> n_channels)
 * Output parameters: none
 * Returns: the SDAS channel number OR
 *          -1 if the channel is not available
 *          -2 for another error
 *
 * Comments:
 *
 **************************************************************************/
int get_channel (int chan_index)

{

  int status, length, limit, error_code;
  char string [100];
  

  strcpy (error_msg, "");

  /* check the channel index is within range */
  limit = get_n_channels ();
  if (limit < 0) return -2;
  if ((chan_index < 0) || (chan_index >= limit))
  {
    strcpy (error_msg, "channel index out of range");
    return -1;
  }

  /* write the command to get the number of channels */
  sprintf (string, "CHAN %d", chan_index);
  status = process_command (socket, string, 0, &error_code, error_msg);
  if (status != PC_OK) return -1;

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return -1;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* parse the information */
  if (! check_number (string, GEO_FALSE, GEO_FALSE))
  {
    strcpy (error_msg, "bad response from server");
    return -1;
  }
  return atoi (string);

}

/**************************************************************************
 * get_channel_details
 *
 * Description: get details for the given SDAS channel number
 *
 * Input parameters: channel - the SDAS channel
 *                   year, month, day - the date
 *                   hour, min, sec - the time
 * Output parameters: none
 * Returns: the SDAS channel details OR
 *          NULL if the channel is not available at the given date/time
 *               OR there is not enough memory or a comms. error
 *
 * Comments:
 *
 **************************************************************************/
struct Channels_cfg *get_channel_details (int channel,
							    			     int year, int month, int day,
								 		         int hour, int min, int sec)

{

  int status, count, length, error_code;
  char string [200], *ptr;
  struct Channels_cfg new_chan, *chan_list_ptr;


  strcpy (error_msg, "");

  /* write the command to get the details */
  sprintf (string, "INFO %d %02d-%02d-%04d %02d:%02d:%02d",
           channel, day, month, year, hour, min, sec);
  status = process_command (socket, string, 0, &error_code, error_msg);
  if (status != PC_OK) return 0;

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return 0;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* extract the information */
  memset (&new_chan, 0, sizeof (new_chan));
  new_chan.sdas_chan = channel;
  for (ptr = strtok2 (string, "'"), count = 0; ptr;
       ptr = strtok2 (0, "'"), count ++)
  {
    switch (count)
    {
    case 0:
      strcpy (new_chan.chan_name, ptr);
      break;
    case 1:
      strcpy (new_chan.chan_type, ptr);
      break;
    case 2:
      strcpy (new_chan.units, ptr);
      break;
    default:
      strcpy (error_msg, "bad response from server");
      return 0;
    }    
  }

  /* add a new element to the list */
  chan_list_ptr = add_to_chan_list (&new_chan);
  if (! chan_list_ptr) strcpy (error_msg, "not enough memory");
  return chan_list_ptr;

}

/**************************************************************************
 * get_sample_rate
 *
 * Description: get the sample rate for a given channel at a given date/time
 *
 * Input parameters: channel - the channel
 *                   year, month, day - the date
 *                   hour, min, sec - the time
 *                   duration - the duration (in milli-seconds)
 * Output parameters: none
 * Returns: the sample rate (in samples / hour) OR
 *          -1 if no data has been recorded for this channel/date/time/duration
 *          -2 for an error
 *
 * Comments: returns the sample rate for the first hour file for which data
 *           has been recorded - does NOT check that the sample rate is
 *           the same for all periods in the given duration
 *
 **************************************************************************/
int get_sample_rate (int channel, int year, int month, int day, int hour,
                            int min, int sec, long duration)

{

  int status, length, sample_rate, error_code;
  long data_len;
  char string [100];
  

  strcpy (error_msg, "");

  /* write the command to get the sample rate */
  sprintf (string, "SRAT %d %02d-%02d-%04d %02d:%02d:%02d %ld",
           channel, day, month, year, hour, min, sec, duration);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    switch (error_code)
    {
    case 19: return -1;
    case 49: return -2;
    }
    return -2;
  default:
    return -2;
  }

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return -2;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* parse the information */
  if (sscanf (string, "%d %ld", &sample_rate, &data_len) != 2)
  {
    strcpy (error_msg, "bad response from server");
    return -2;
  }
  /* this should never happen, but may be possible if a v0.3+ client
   * is reading from a v0.2 server */
  if (sample_rate < 1)
  {
    if (sample_rate == -1)
      strcpy (error_msg, "No data recorded for this date/time");
    else
      strcpy (error_msg, "Unexpected system fault on server");
  }
  return sample_rate;

}

/**************************************************************************
 * get_sdas_data
 *
 * Description: get the data for a given channel at a given date/time
 *
 * Input parameters: channel - the channel
 *                   year, month, day - the date
 *                   hour, min, sec, milli - the time
 *                   duration - the duration of the data to retrieve
 *                              (in milli-seconds)
 * Output parameters: data - an array to hold the data
 *                    chan_details - if NOT null copy the channel config.
 *                                   details to this structure
 * Returns: the number of samples retrieved OR
 *          GSD_BAD_SR - the sample rate changes within the given duration
 *          GSD_BAD_CD - the channel details change within the given duration
 *                       or no channel details could be found
 *          GSD_IO_ERR - there was an IO error
 *          GSD_REM_ERR - there was an error with the remote server
 *          GSD_NO_MEM - not enough memory (either on server or locally)
 *
 * Comments:
 *
 **************************************************************************/
long get_sdas_data (int channel, int year, int month, int day, int hour,
                           int min, int sec, int milli, long duration, long *data,
                           struct Channels_cfg **chan_details)


{

  int status, length, sample_rate, error_code, found;
  long data_len, count, *data_ptr;
  char string [200], buffer [1024], *ptr;
  struct Channels_cfg new_chan;
  z_stream d_stream;
  

  strcpy (error_msg, "");

  /* write the command to get data */
  sprintf (string, "DATA %d %02d-%02d-%04d %02d:%02d:%02d.%03d %ld",
           channel, day, month, year, hour, min, sec, milli, duration);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    switch (error_code)
    {
    case 5:  return GSD_NO_MEM;
    case 24: return 0;
    case 25: return GSD_BAD_SR;
    case 26: return GSD_BAD_CD;
    case 27: return GSD_IO_ERR;
    }
    return GSD_REM_ERR;
  default:
    return GSD_REM_ERR;
  }

  /* get the information about the data */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return GSD_REM_ERR;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* parse the information about the data */
  memset (&new_chan, 0, sizeof (new_chan));
  new_chan.sdas_chan = channel;
  found = GEO_FALSE;
  for (ptr = strtok2 (string, "'"), count = 0; ptr;
       ptr = strtok2 (0, "'"), count ++)
  {
    switch (count)
    {
    case 0:
      sample_rate = atoi (ptr);
      break;
    case 1:
      data_len = atol (ptr);
      break;
    case 2:
      strcpy (new_chan.chan_name, ptr);
      break;
    case 3:
      strcpy (new_chan.chan_type, ptr);
      break;
    case 4:
      found = GEO_TRUE;
      strcpy (new_chan.units, ptr);
      break;
    default:
      strcpy (error_msg, "bad response from server");
      return GSD_REM_ERR;
    }    
  }
  if (! found)
  {
    strcpy (error_msg, "bad response from server");
    return GSD_REM_ERR;
  }
  if (sample_rate < 1)
  {
    strcpy (error_msg, "bad sample rate received from server");
    return GSD_REM_ERR;
  }
  if (data_len < 0)
  {
    strcpy (error_msg, "bad data length received from server");
    return GSD_REM_ERR;
  }

  /* sort out the channel details */
  if (chan_details)
  {
    *chan_details = add_to_chan_list (&new_chan);
    if (! *chan_details)
    {
      strcpy (error_msg, "not enough memory");
      return GSD_NO_MEM;
    }
  }
  
  /* what type of compression are we using ?? */
  switch (compression)
  {
  case COMPRESS_ASCII:
    /* retrieve and convert ascii data */
    for (count=0, data_ptr = data; count<data_len; count++, data_ptr ++)
    {
      length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
      if (length <= 0)
      {
        strcpy (error_msg, "no response from server");
        return GSD_REM_ERR;
      }
      string [length] = '\0';
      if (sscanf (string, "%ld", data_ptr) != 1)
      {
        strcpy (error_msg, "bad response from server");
        return GSD_REM_ERR;
      }
    }
    break;
    
  case COMPRESS_BINARY:
    /* retrieve binary data */
    for (count=0, data_ptr=data; count<data_len; count++, data_ptr++)
    {
      length = net_read (socket, (char *) data_ptr, sizeof (long));
      if (length != sizeof (long))
      {
        strcpy (error_msg, "no response from server");
        return GSD_REM_ERR;
      }
    }
    break;
    
  case COMPRESS_ZLIB:
    /* initialise zlib */
    d_stream.zalloc = (alloc_func) 0;
    d_stream.zfree = (free_func) 0;
    d_stream.opaque = (voidpf) 0;
    d_stream.avail_out = data_len * sizeof (long);
    d_stream.next_out = (unsigned char *) data;
    status = inflateInit (&d_stream);
    switch (status)
    {
    case Z_OK:
      break;
    case Z_MEM_ERROR:
      strcpy (error_msg, "not enough memory");
      return GSD_NO_MEM;
    default:
      sprintf (error_msg, "zlib returned status code %d", status);
      return GSD_IO_ERR;
    }

    /* until the end of the data ... */
    do
    {
      /* read chunks of data */
      length = net_partial_read (socket, buffer, sizeof (buffer));
      if (length < 0)
      {
        strcpy (error_msg, "no response from server");
        return GSD_REM_ERR;
      }

      /* pass the chunks to zlib */
      d_stream.avail_in = length;
      d_stream.next_in = (unsigned char *) buffer;
      status = inflate (&d_stream, Z_NO_FLUSH);
      switch (status)
      {
      case Z_STREAM_END:
      case Z_OK:
        break;
      default:
        sprintf (error_msg, "zlib returned status code %d", status);
        inflateEnd (&d_stream);
        return GSD_IO_ERR;
      }
    } while (status == Z_OK);    

    /* tidy up */
    inflateEnd (&d_stream);
    break;
  }

  return data_len;

}

/**************************************************************************
 * find_sdas_gap
 *
 * Description: find the next data gap for a given channel at a given date
 *
 * Input parameters: new - if TRUE use chanel, date, time, duration parameters
 *                         if FALSE continue with previous search
 *                   channel - the channel
 *                   year, month, day - the date
 *                   n_days - the number of days to search
 * Output parameters: out_year, out_month, out_day - the gap date
 *                    out_hour, out_min, out_sec, out_milli - the gap time
 * Returns: FSG_GAP - found the start of a gap (details in out_* variables)
 *          FSG_DATA - found the start of non-gap data (details in out_* variables)
 *          FSG_DONE - the end of the given duration has been reached
 *          FSG_IO_ERR - there was an IO error
 *
 * Comments:
 *
 **************************************************************************/
int find_sdas_gap (int new, int channel, int year, int month, int day,
                          int n_days,
                          int *out_year, int *out_month, int *out_day,
                          int *out_hour, int *out_min, int *out_sec,
                          int *out_milli)
                   
{

  int status, error_code, length, count, ret_val;
  char string [200], *ptr;
  

  /* send the command to retrieve the gap information */
  sprintf (string, "FGAP %d %d %02d-%02d-%04d %d",
           new, channel, day, month, year, n_days);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    if (error_code == 47) return FSG_DONE;
    return FSG_IO_ERR;
  default:
    return FSG_IO_ERR;
  }

  /* read the gap information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return FSG_IO_ERR;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* extract the gap information */
  count = 0;
  for (ptr = strtok2 (string, "'"); ptr; ptr = strtok2 (0, "'"))
  {
    switch (count ++)
    {
    case 0:
      if (! strcmp (ptr, "gap")) ret_val = FSG_GAP;
      else if (! strcmp (ptr, "data")) ret_val = FSG_DATA;
      else
      {
        strcpy (error_msg, "bad response from server");
        return FSG_IO_ERR;
      }
      break;
    case 1:
      if (sscanf (ptr, "%d-%d-%d", out_day, out_month, out_year) != 3)
      {
        strcpy (error_msg, "bad response from server");
        return FSG_IO_ERR;
      }
      break;
    case 2:
      if (sscanf (ptr, "%d:%d:%d.%d", out_hour, out_min, out_sec, out_milli) != 4)
      {
        strcpy (error_msg, "bad response from server");
        return FSG_IO_ERR;
      }
      break;
    }    
  }

  /* check that the correct number of fields were returned */
  if (count != 3)
  {
    strcpy (error_msg, "bad response from server");
    return FSG_IO_ERR;
  }
  return ret_val;
                              
}

/***************************************************************************
 * sdas_error_msg
 *
 * Description: get the most recent error description
 *
 * Input parameters: none
 * Output parameters: none
 * Returns: the message
 *
 * Comments:
 *
 ***************************************************************************/
char *sdas_error_msg (void)

{

  return error_msg;

}

/***************************************************************************
 * get_most_recent_block_time
 *
 * Description: Get the date/time for the most recent block
 *
 * Input parameters: channel - the channel that you want the date/time for
 *                             set this -ve to get the most recent date/time
 *                             across all channels
 *                   re_read - if TRUE read the most recent block file
 *                             otherwise extract data from the previous read
 * Output parameters: year, month, day - the date
 *                    hour, min, sec - the time
 * Returns: GMRBT_OK - the information was retrieved OK
 *          GMRBT_FILE_ERR - error accessing the most recent block file
 *          GMRBT_NO_CHAN - the specified channel doesn't exist
 *          GMRBT_NO_MEM - there is not enough memory
 *          GMRBT_REM_ERR - remote system error
 * 
 * Comments:
 *
 ***************************************************************************/
int get_most_recent_block_time (int channel, int re_read,
                                       int *year, int *month, int *day,
                                       int *hour, int *min, int *sec)

{

  int status, length, error_code;
  char string [100];
  

  strcpy (error_msg, "");

  /* write the command to get the most recent block time */
  sprintf (string, "GMRB %d %d", channel, re_read);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    switch (error_code)
    {
    case 3: return GMRBT_FILE_ERR;
    case 4: return GMRBT_NO_CHAN;
    case 5: return GMRBT_NO_MEM;
    }
    return GMRBT_REM_ERR;
  default:
    return GMRBT_REM_ERR;
  }

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return GMRBT_REM_ERR;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* parse the information */
  if (sscanf (string, "%d-%d-%d %d:%d:%d",
              year, month, day, hour, min, sec) != 6)
  {
    strcpy (error_msg, "bad response from server");
    return GMRBT_REM_ERR;
  }
  if (! check_clock (*sec, *min, *hour, *day, *month, *year))
  {
    strcpy (error_msg, "bad date/time received from server");
    return GMRBT_REM_ERR;
  }
  return GMRBT_OK;

}

/***********************************************************************
 * load_sdas_pid_file
 *
 * Description: load the pid file into memory
 *
 * Input parameters: log_dir - the log directory
 *                   pid_file_type - SDAS_PID_FILE or SDAS_DAEMON_PID_FILE
 * Output parameters: store - the loaded pid file
 *                    n_records - the number of records in the store
 * Returns: LPF_NO_FILE - the file does not exist
 *
 * Comments: This operation is not supported remotely
 *
 ***********************************************************************/
int load_sdas_pid_file (char *log_dir, int pid_file_type,
                               char **store, int *n_records)

{

  strcpy (error_msg, "SDAS pid file cannot be loaded from remote server");
  return LPF_NO_FILE;

}

/***************************************************************************
 * find_acq_record
 *
 * Description: find the acquisition driver record (from acquire.cfg)
 *              for the given channel
 *
 * Input parameters: channel - the SDAS channel
 * Output parameters: none
 * Returns: the SDAS acquisition details OR
 *          NULL if the information could not be found
 *
 * Comments:
 *
 **************************************************************************/
struct Acquire_cfg *find_acq_record (int channel)

{

  int status, length, error_code, count;
  char string [300], *ptr;
  struct Acquire_cfg new_acq_rec, *acq_list_ptr;


  strcpy (error_msg, "");

  /* write the command to find an acquisition record */
  sprintf (string, "FACQ %d", channel);
  status = process_command (socket, string, 0, &error_code, error_msg);
  if (status != PC_OK) return 0;

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return 0;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* extract the information */
  memset (&new_acq_rec, 0, sizeof (new_acq_rec));
  for (ptr = strtok2 (string, "'"), count = 0; ptr;
       ptr = strtok2 (0, "'"), count ++)
  {
    switch (count)
    {
    case 0: new_acq_rec.id = atoi (ptr); break;
    case 1: strcpy (new_acq_rec.prog_name, ptr); break;
    case 2: strcpy (new_acq_rec.dev_name, ptr); break;
    case 3: new_acq_rec.buff_size = atoi (ptr); break;
    case 4: strcpy (new_acq_rec.stty_params, ptr); break;
    case 5: strcpy (new_acq_rec.mode, ptr); break;
    case 6: new_acq_rec.sample_rate = atoi (ptr); break;
    case 7: new_acq_rec.time_base = (enum Sample_time_base) atoi (ptr); break;
    case 8: strcpy (new_acq_rec.address, ptr); break;
    case 9: new_acq_rec.clock_reqd = atoi (ptr); break;
    case 10: new_acq_rec.debug = atoi (ptr); break;
    default: strcpy (error_msg, "bad response from server"); return 0;
    }    
  }

  /* add a new element to the list */
  acq_list_ptr = add_to_acq_list (&new_acq_rec);
  if (! acq_list_ptr) strcpy (error_msg, "not enough memory");
  return acq_list_ptr;

} 
 
/***************************************************************************
 * copy_sdas_file
 *
 * Description: copy a file from the remote SDAS system
 *
 * Input parameters: remote_file - name of the file to copy from
 *                   local_file - name of the destination location
 * Output parameters: none
 * Returns: CRF_OK - file copied OK
 *          CRF_NO_FILE - remote file does not exist
 *          CRF_REMOTE_ERR - error on SDAS server
 *          CRF_LOCAL_ERR - error writing local file
 *
 * Comments: both local and remote file names may include environment
 *           variables
 *
 ***************************************************************************/
int copy_sdas_file (char *remote_file, char *local_file)

{

  int length, status, ret_val, length_read, error_code;
  long file_size, file_count;
  char string [300];
  FILE *fp;
  

  /* write the command to send a file */
  sprintf (string, "FILE %s", remote_file);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    if (error_code == 38) return CRF_NO_FILE;
    return CRF_REMOTE_ERR;
  default:
    return CRF_REMOTE_ERR;
  }
  
  /* get the length of the file */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return CRF_REMOTE_ERR;
  }
  string [length] = '\0';
  if (! check_number (string, GEO_FALSE, GEO_FALSE))
  {
    sprintf (error_msg, "bad file size returned [%s]", string);
    return CRF_REMOTE_ERR;
  }
  file_size = atol (string);

  /* open the local file - continue retrieving the remote file even
   * if this fails so that the server's output is flushed */
  if (substitute_variables (local_file, GEO_TRUE, string) != SV_OK)
  {
    strcpy (error_msg, "error substituting environment variables");
    ret_val = CRF_LOCAL_ERR;
  }
  else if (! (fp = fopen (string, "wb")))
  {
    strcpy (error_msg, "unable to create local file");
    ret_val = CRF_LOCAL_ERR;
  }
  else ret_val = CRF_OK;

  /* get the file contents */
  for (file_count=0; file_count < file_size; file_count += length_read)
  {
    length = file_size - file_count;
    if (length > sizeof (string)) length = sizeof (string);
    length_read = net_read (socket, string, length);
    if (length_read <= 0)
    {
      strcpy (error_msg, "unable to read file data from remote server");
      ret_val = CRF_REMOTE_ERR;
      break;
    }
    if (ret_val == CRF_OK)
    {
      if (fwrite (string, 1, length_read, fp) != (unsigned) length_read)
      {
        fclose (fp);
        strcpy (error_msg, "error writing to local file");
        ret_val = CRF_LOCAL_ERR;
      }
    }
  }

  /* tidy up */
  if (ret_val == CRF_OK) fclose (fp);
  return ret_val;

}

/***************************************************************************
 * list_sdas_directory
 *
 * Description: list a directory on the remote SDAS system
 *
 * Input parameters: remote_dir - name of the directory to list
 *                   pattern - pattern to match against which may
 *                             include wildcards OR
 *                             emptry string to match all files
 * Output parameters: list - the directory list which should
 *                           be free() when no longer needed -
 *                           the stat information in this list is not valid 
 * Returns: the number of directory entries OR
 *          LRD_NO_DIR - remote file does not exist
 *          LRD_REMOTE_ERR - error on SDAS server
 *          LRD_NO_MEM - not enough memory on local machine
 *
 * Comments: the remote directory name may include environment
 *           variables
 *
 ***************************************************************************/
int list_sdas_directory (char *remote_dir, char *pattern, struct File_info **list)

{

  int n_dir_entries, count, status, error_code, length;
  char string [300], *ptr, *ptr2, *msg;
  struct File_info *list_ptr;
  

  /* write the command to list a directory */
  if (*pattern)
    sprintf (string, "DIRE %s %s", remote_dir, pattern);
  else
    sprintf (string, "DIRE %s", remote_dir);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    if (error_code == 38) return LRD_NO_DIR;
    return CRF_REMOTE_ERR;
  default:
    return CRF_REMOTE_ERR;
  }
  
  /* get the number of entries */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return LRD_REMOTE_ERR;
  }
  string [length] = '\0';
  if (! check_number (string, GEO_FALSE, GEO_FALSE))
  {
    sprintf (error_msg, "bad number of directory entries returned [%s]", string);
    return LRD_REMOTE_ERR;
  }
  n_dir_entries = atoi (string);

  /* check there are some entries */
  if (n_dir_entries <= 0) return 0;

  /* allocate space for the directories - continue to retrieve directories
   * from the server even if the allocation fails, so that the server
   * output is flushed */
  *list = malloc (sizeof (struct File_info) * n_dir_entries);

  /* retrieve the directory entries */
  for (count=0, list_ptr=*list; count<n_dir_entries; count++, list_ptr ++)
  {
    msg = 0;
    length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
    if (length <= 0) msg = "no response from server";
    else if (! (ptr = strtok2 (string, "'"))) msg = "bad response from server";
    else if (! (ptr2 = strtok2 (0, "'"))) msg = "bad response from server";
    else if (strtok2 (0, "'")) msg = "bad response from server";
    else
    {
/* File_info is a different structure on WINDOWS and UNIX */
/*      strcpy (list_ptr->name, ptr); */
/*      strcpy (list_ptr->full_path, ptr2); */
      strcpy (list_ptr->name, ptr2);
    }
    if (msg)
    {
      strcpy (error_msg, msg);
      if (*list) free (*list);
      return LRD_REMOTE_ERR;
    }

  }

  /* work out what to return */
  if (*list) return n_dir_entries;
  return LRD_NO_MEM;

}

/***************************************************************************
 * get_n_channel_groups
 * get_channel_group
 *
 * Description: get information on the way SDAS channels are grouped together
 *
 * Input parameters: group_index - the number of the group (0..n_groups-1)
 * Output parameters: none
 * Returns: the number of channel groups OR -1 for an error
 *          details on the specified group OR NULL for an error
 *
 * Comments:
 *
 ***************************************************************************/
int get_n_channel_groups (void)

{

  int n_recs;
  


  if (! download_channel_groups (socket, &n_recs, error_msg))
    return -1;
  return n_recs;

}


struct Channel_group *get_channel_group (int group_index)

{

  int n_recs;
  struct Channel_group *channel_groups;


  channel_groups = download_channel_groups (socket, &n_recs, error_msg);
  if (! channel_groups) return 0;

  if (group_index < 0 || group_index >= n_recs)
  {
    strcpy (error_msg, "Index for channel group out of valid range");
    return 0;
  }

  return channel_groups + group_index;

}

/***************************************************************************
 * get_sdas_event
 *
 * Description: get an event
 *
 * Input parameters: day, month, year - set these to the date to read the
 *                                      first event of the day, set them
 *                                      to 0 to read subsequent events
 *                   get_ag_data - TRUE to retrieve the event.algorithm_data field
 *                                 FALSE to leave it blank
 * Output parameters: event - an event structure
 * Returns: EVENT_OK - the event was retrieved OK
 *          EVENT_EOF - no more events
 *          EVENT_ERROR - error retrieving event data
 *          EVENT_NOMEM - not enough memory
 *
 * Comments: the algorithm_data field is allocated from free memory (when
 *           it is used), so must be free()d when no longer needed - if
 *           there is no algorithm data associated with the event, this
 *           field will contain NULL
 *
 ***************************************************************************/
int get_sdas_event (int day, int month, int year, int get_ag_data,
                           struct SDAS_Event *event)

{

  int status, error_code, algorithm_data_length, ret_val, found, length;
  int count;
  char string [200], *ptr, algorithm_name [200];

  static int name_list_length;
  static char *algorithm_name_list = 0;


  strcpy (error_msg, "");

  /* send the command to retrieve the next event */
  sprintf (string, "EVNT %02d-%02d-%04d %d", day, month, year, get_ag_data);
  status = process_command (socket, string, 0, &error_code, error_msg);
  switch (status)
  {
  case PC_OK:
    break;
  case PC_REMOTE:
    if (error_code == 44) return EVENT_EOF;
    return EVENT_ERROR;
  default:
    return EVENT_ERROR;
  }

  /* read the first line of information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return EVENT_ERROR;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* extract the information from the first line - don't return on an error
   * because we want to try to pick up the second line of information so
   * that the comms don't get confused */
  ret_val = EVENT_OK;
  count = 0;
  for (ptr = strtok2 (string, "'"); ptr; ptr = strtok2 (0, "'"))
  {
    switch (count ++)
    {
    case 0:
      if (sscanf (ptr, "%d-%d-%d", &(event->year), &(event->month),
                                   &(event->day)) != 3)
      {
        strcpy (error_msg, "bad response from server");
        ret_val = EVENT_ERROR;
      }
      break;
    case 1:
      if (sscanf (ptr, "%d:%d:%d.%d", &(event->hour), &(event->min),
                                      &(event->sec), &(event->milli)) != 4)
      {
        strcpy (error_msg, "bad response from server");
        ret_val = EVENT_ERROR;
      }
      break;
    case 2:
      if (sscanf (ptr, "%lf", &(event->duration)) != 1)
      {
        strcpy (error_msg, "bad response from server");
        ret_val = EVENT_ERROR;
      }
      break;
    case 3:
      strcpy (algorithm_name, ptr);
      break;
    case 4:
      if (sscanf (ptr, "%d", &algorithm_data_length) != 1)
      {
        strcpy (error_msg, "bad response from server");
        ret_val = EVENT_ERROR;
      }
      break;
    }    
  }

  /* check that the correct number of fields were returned */
  if (count != 5) strcpy (error_msg, "bad response from server");

  /* allocate space for the algorithm data */
  if (algorithm_data_length <= 0) event->algorithm_data = 0;
  else
  {
    algorithm_data_length += 10;
    event->algorithm_data = malloc (algorithm_data_length);
    if (! event->algorithm_data)
    {
      strcpy (error_msg, "not enough memory");
      ret_val = EVENT_NOMEM;
    }
  }

  /* read the algorithm data */
  if (! event->algorithm_data)
  {
    length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
    if (length < 0)
    {
      strcpy (error_msg, "no response from server");
      ret_val = EVENT_ERROR;
    }
  }
  else
  {
    length = net_read_line (socket, event->algorithm_data, algorithm_data_length, "\r\n", 3);
    if (length <= 0)
    {
      strcpy (error_msg, "no response from server");
      ret_val = EVENT_ERROR;
    }
    *(event->algorithm_data + length) = '\0';
  }

  /* return if there were any errors */
  if (ret_val != EVENT_OK) return ret_val;

  /* sort out the algorithm name - we store the name in a static list of
   * unique names and point the event algorithm name field at the correct
   * element in the list - the name list looks like this:
   *     name\0name\0name\0....name\0\0 */
  found = GEO_FALSE;
  if (! algorithm_name_list)
  {
    name_list_length = 0;
    length = strlen (algorithm_name) +2;
    ptr = malloc (length);
  }
  else
  {
    /* try to find the name in the list */
    for (ptr=algorithm_name_list; *ptr; ptr += strlen (ptr) +1)
    {
      if (! strcmp (ptr, algorithm_name))
      {
        event->algorithm = ptr;
        found = GEO_TRUE;
      }
    }
    if (! found)
    {
      length += strlen (algorithm_name) +1;
      ptr = realloc (algorithm_name_list, length);
    }
  }
  if (! found)
  {
    if (! ptr)
    {
      strcpy (error_msg, "not enough memory");
      return EVENT_NOMEM;
    }
    algorithm_name_list = ptr;
    if (name_list_length <= 0) ptr = algorithm_name_list;
    else ptr = algorithm_name_list + name_list_length -1;
    strcpy (ptr, algorithm_name);
    event->algorithm = ptr;
    name_list_length += length;
    *(algorithm_name_list + name_list_length -1) = '\0';
  }

  return EVENT_OK;

}

/**************************************************************************
 * read_sdas_stats
 *
 * Description: read an SDAS stats file
 *
 * Input parameters: channel - the channel
 *                   year, month, day - the date
 *                   hour - the time for an hourly file, or -1 for a daily file
 * Output parameters: stats - the statistics
 * Returns: TRUE if stats read OK, FALSE otherwise
 *
 * Comments:
 *
 **************************************************************************/
int read_sdas_stats (int channel, int year, int month,
                            int day, int hour, struct SDAS_data_stats *stats)
                      
{

  int status, count, length, error_code;
  char string [200];


  strcpy (error_msg, "");

  /* write the command to get the stats */
  sprintf (string, "STAT %d %02d-%02d-%04d %d",
           channel, day, month, year, hour);
  status = process_command (socket, string, 0, &error_code, error_msg);
  if (status != PC_OK) return GEO_FALSE;

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (error_msg, "no response from server");
    return GEO_FALSE;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* extract the information */
  count = 0;
  if (sscanf (string, "%d'%d'%d'%d'%lf'%lf",
              &(stats->n_points), &(stats->n_missing), &(stats->min_val),
              &(stats->max_val), &(stats->average), &(stats->standard_dev)) != 6) return GEO_FALSE;

  return GEO_TRUE;
  
}

/* ------------------------------------------------------------------------
 * ------------------------------------------------------------------------
 * private code from here on
 * ------------------------------------------------------------------------
 * ------------------------------------------------------------------------ */

/***************************************************************************
 * process_command
 *
 * Description: send a command, get and process the response
 *
 * Input parameters: socket - the socket that to the server
 *                   command - the command to send
 *                   flags - processing options
 * Output parameters: err_code - a code passed on from the server
 *                    err_msg - a message for the user if there was an error
 * Returns: PC_OK - if command was processed OK
 *          PC_CLOS - if CLOS was received and flags included EXPECT_CLOS_CODE
 *          PC_REMOTE - the remote server sent an error - the code is
 *                      in err_code
 *          PC_ERROR - for all other cases
 *
 * Comments:
 *
 **************************************************************************/
static int process_command (int socket, char *command, int flags,
                            int *err_code, char *err_msg)

{

  int status, length;
  char response [200], err_command [100], local_command [100];

  static int recurse_count = 0;


  /* initialise */
  strcpy (err_msg, "");
  *err_code = 0;

  /* write the command */
  if (verbose)
    fprintf (stderr, "sdas_ds: sending message [%s]\n", command);
  sprintf (local_command, "%s\n", command);
  status = net_write (socket, local_command, strlen (local_command));

  /* check for any errors */
  switch (status)
  {
  case NET_ERROR:
    strcpy (err_msg, "error writing to server");
    return PC_ERROR;
  case NET_DISCONNECT:
    strcpy (err_msg, "unexpected disconnection from server");
    return PC_ERROR;
  case NET_TIMEOUT:
    strcpy (err_msg, "timeout waiting for server");
    return PC_ERROR;
  }

  /* read the response */
  length = net_read_line (socket, response, sizeof (response), "\r\n", 2);
  switch (length)
  {
  case NET_ERROR:
    strcpy (err_msg, "error reading from server");
    return PC_ERROR;
  case NET_DISCONNECT:
    strcpy (err_msg, "unexpected disconnection from server");
    return PC_ERROR;
  case NET_TIMEOUT:
    strcpy (err_msg, "timeout waiting for server");
    return PC_ERROR;
  }
  response [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: server responded [%s]\n", response);
  
  /* check the response string */
  if (! strcmp (response, "OKOK"))
    return PC_OK;
  else if (! strncmp (response, "SERR", 4))
  {
    if (length < 6)
    {
      sprintf (err_msg, "bad server response [%s]", response);
      return PC_ERROR;
    }
    *err_code = atoi (&response [5]);
    if (*err_code <= 0)
    {
      sprintf (err_msg, "bad server SERR code [%s]", response);
      return PC_ERROR;
    }
  }
  else if (! strcmp (response, "CLOS"))
  {
    if (flags & EXPECT_CLOS_CODE) return PC_CLOS;
    strcpy (err_msg, "unexpected CLOS from server");
    return PC_ERROR;
  }
  else
  {
    sprintf (err_msg, "unknown server response [%s]", response);
    return PC_ERROR;
  }
    
  /* if you get here there was an error, so get the description
   * from the server */
  if (recurse_count <= 0)
  {
    recurse_count ++;
    sprintf (err_command, "DERR %d", *err_code);
    if (process_command (socket, err_command, 0, &status, response) != PC_OK)
    {
      recurse_count --;
      sprintf (err_msg, "unable to retrieve error message for code %d\n", *err_code);
      return PC_ERROR;
    }
    recurse_count --;

    /* get the description */
    length = net_read_line (socket, response, sizeof (response), "\r\n", 3);
    if (length <= 0)
    {
      sprintf (err_msg, "unable to retrieve error message for code %d\n", *err_code);
      return PC_ERROR;
    }
    response [length] = '\0';
    if (verbose)
      fprintf (stderr, "sdas_ds: extra response [%s]\n", response);

    sprintf (err_msg, "server error - %s", response);
    return PC_REMOTE;
  }

  /* if you get here, then this is a recusive call to get an error
   * description that has failed, so simply return with an error */
  return PC_ERROR;

}

/**************************************************************************
 * add_to_acq_list
 *
 * Description: add an acquisition record to an (internal) list so that
 *              a memory reference to it is permanent
 *
 * Input parameters: new_acq_rec - the data to add
 * Output parameters: none
 * Returns: the address of the stored description OR NULL for no memory
 *
 * Comments: We need to make sure that once a record is stored in memory
 * it is not moved, so we can't use realloc() to make a resizeable array.
 * Instead we use realloc to make an array of pointers to each individual
 * structure.
 *
 **************************************************************************/
static struct Acquire_cfg *add_to_acq_list (struct Acquire_cfg *new_acq_rec)

{

  int count;
  struct Acquire_cfg **acq_list_ptr, *acq_ptr;

  static struct Acquire_cfg **acq_list = 0;
  static int acq_list_len = 0;


  /* do we need to add a new element to the list ?? */
  for (count=0; count<acq_list_len; count++)
  {
    if (! memcmp (*(acq_list + count), new_acq_rec, sizeof (struct Acquire_cfg)))
      return *(acq_list + count);
  }
  
  /* allocate memory to extend the list and store the new data */
  acq_list_ptr = realloc (acq_list,
                          (acq_list_len +1) * sizeof (struct Acquire_cfg *));
  acq_ptr = malloc (sizeof (struct Acquire_cfg));
  if ((! acq_list_ptr) || (! acq_ptr)) return 0;

  /* add the new element to the list */
  acq_list = acq_list_ptr;
  acq_list_ptr = acq_list + acq_list_len;
  *acq_list_ptr = acq_ptr;
  memcpy (acq_ptr, new_acq_rec, sizeof (struct Acquire_cfg));
  acq_list_len ++;
  return acq_ptr;

}

/**************************************************************************
 * add_to_chan_list
 *
 * Description: add a channel description to an (internal) list so that
 *              a memory reference to it is permanent
 *
 * Input parameters: new_chan - the chan to add
 * Output parameters: none
 * Returns: the address of the stored description OR NULL for no memory
 *
 * Comments: We need to make sure that once a record is stored in memory
 * it is not moved, so we can't use realloc() to make a resizeable array.
 * Instead we use realloc to make an array of pointers to each individual
 * structure.
 *
 **************************************************************************/
static struct Channels_cfg *add_to_chan_list (struct Channels_cfg *new_chan)

{

  int count;
  struct Channels_cfg **chan_list_ptr, *chan_ptr;

  static struct Channels_cfg **chan_list = 0;
  static int chan_list_len = 0;


  /* do we need to add a new element to the list ?? */
  for (count=0; count<chan_list_len; count++)
  {
    if (! memcmp (*(chan_list + count), new_chan, sizeof (struct Channels_cfg)))
      return *(chan_list + count);
  }
  
  /* allocate memory to extend the list and store the new channel details */
  chan_list_ptr = realloc (chan_list,
                           (chan_list_len +1) * sizeof (struct Channels_cfg *));
  chan_ptr = malloc (sizeof (struct Channels_cfg));
  if ((! chan_list_ptr) || (! chan_ptr)) return 0;

  /* add the new element to the list */
  chan_list = chan_list_ptr;
  chan_list_ptr = chan_list + chan_list_len;
  *chan_list_ptr = chan_ptr;
  memcpy (chan_ptr, new_chan, sizeof (struct Channels_cfg));
  chan_list_len ++;
  return chan_ptr;

}

/**************************************************************************
 * download_channel_groups
 * free_channel_groups
 *
 * Description: download the channel groups list
 *              free the structure used to hold channel group information
 *
 * Input parameters: socket - the connection to the data server
 * Output parameters: nr - number of records read
 *                    errmsg - if there is an error than a 
 *                             descriptive message is put here
 * Returns: pointer to a Channel_group structure or NULL if there was
 *		    an error reading the group lists
 *
 * Comments:
 *
 **************************************************************************/
static struct Channel_group *download_channel_groups (int socket, int *nr, char *errmsg)

{

  int status, error_code, group_count, length, arg_count, channel_count;
  char string [300], *ptr;
  struct Channel_group *group_ptr;

  static int n_recs = 0;
  static struct Channel_group *channel_groups = (struct Channel_group *) 0, dummy_group;


  /* check if the groups have already been downloaded */
  if (channel_groups)
  {
    *nr = n_recs;
	return channel_groups;
  }

  /* send the command to get the number of groups */
  status = process_command (socket, "NGRP", 0, &error_code, errmsg);
  if (status != PC_OK) return 0;

  /* get the information */
  length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
  if (length <= 0)
  {
    strcpy (errmsg, "no response from server");
    return 0;
  }
  string [length] = '\0';
  if (verbose)
    fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

  /* parse the information */
  if (! check_number (string, GEO_FALSE, GEO_FALSE))
  {
    strcpy (errmsg, "bad response from server");
    return 0;
  }
  n_recs = atoi (string);
  
  /* check for zero groups - if there are no groups, channel_groups is
   * set to 0, so we need to set it to a dummy value */
  if (n_recs <= 0)
  {
    channel_groups = &dummy_group;
    *nr = 0;
    return channel_groups;
  }

  /* allocate space for the group records */
  channel_groups = malloc (n_recs * sizeof (struct Channel_group));
  if (! channel_groups)
  {
    n_recs = 0;
    strcpy (errmsg, "not enough memory");
    return 0;
  }

  /* initialise the group records, so that free_channel_groups() will work */
  for (group_count=0; group_count<n_recs; group_count ++)
  {
    group_ptr = channel_groups + group_count;
    group_ptr->n_channels = 0;
    group_ptr->channel_list = 0;
  }
  
  /* read the group records */
  for (group_count=0; group_count<n_recs; group_count ++)
  {
    group_ptr = channel_groups + group_count;

    /* send the command to get this group */
    sprintf (string, "GROP %d", group_count);
    status = process_command (socket, string, 0, &error_code, errmsg);
    if (status != PC_OK)
    {
      channel_groups = free_channel_groups (channel_groups, &n_recs);
      return 0;
    }

    /* get the information */
    length = net_read_line (socket, string, sizeof (string), "\r\n", 3);
    if (length <= 0)
    {
      strcpy (errmsg, "no response from server");
      channel_groups = free_channel_groups (channel_groups, &n_recs);
      return 0;
    }
    string [length] = '\0';
    if (verbose)
      fprintf (stderr, "sdas_ds: extra response [%s]\n", string);

    /* parse the information */
    arg_count = channel_count = 0;
    for (ptr = strtok (string, "'"); ptr; ptr = strtok (0, "'"))
    {
      switch (arg_count ++)
      {
      case 0:
        /* get the group name */
        strcpy (group_ptr->group_name, ptr);
        break;
        
      case 1:
        /* get the number of channels in the group */
        if (! check_number (ptr, GEO_FALSE, GEO_FALSE))
        {
          sprintf (errmsg, "number of channels in channel group corrupt [%s]", ptr);
          channel_groups = free_channel_groups (channel_groups, &n_recs);
          return 0;
        }
        group_ptr->n_channels = atoi (ptr);

        /* allocate space for the channels */
        group_ptr->channel_list = malloc (group_ptr->n_channels * sizeof (int));
        if (! group_ptr->channel_list)
        {
          channel_groups = free_channel_groups (channel_groups, &n_recs);
          strcpy (errmsg, "not enough memory");
          return 0;
        }
        break;
        
      default:
        /* get the next channel number */
        if (! check_number (ptr, GEO_FALSE, GEO_FALSE))
        {
          sprintf (errmsg, "channel number in channel group corrupt [%s]", ptr);
          channel_groups = free_channel_groups (channel_groups, &n_recs);
          return 0;
        }
        *(group_ptr->channel_list + channel_count) = atoi (ptr);
        channel_count ++;
        break;
      }
      
    }

    /* check that the details were received OK */
    if (arg_count < 2 || group_ptr->n_channels != channel_count)
    {
      channel_groups = free_channel_groups (channel_groups, &n_recs);
      strcpy (errmsg, "bad response from server");
      return 0;
    }
  }
  
  *nr = n_recs;  
  return channel_groups;


}

static struct Channel_group *free_channel_groups (struct Channel_group *channel_groups, int *n_recs)
{
  int count;
  
  for (count=0; count<*n_recs; count++)
    free ((channel_groups + count)->channel_list);
  if (channel_groups) free (channel_groups);
  *n_recs = 0;
  return 0;
}
