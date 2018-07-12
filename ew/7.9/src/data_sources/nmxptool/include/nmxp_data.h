/*! \file
 *
 * \brief Data for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_data.h 4965 2012-07-22 07:12:34Z quintiliani $
 *
 */

#ifndef NMXP_DATA_H
#define NMXP_DATA_H 1

#include <stdint.h>
#include <stdio.h>
#include <time.h>


/*! \brief struct tm plus ten thousandth second field */
typedef struct {
    struct tm t;
    uint32_t d;
} NMXP_TM_T;



/*! First 4 bytes of all messages. */
#define NMX_SIGNATURE 0x7abcde0f

/*! */
#define NMXP_DATA_IS_LEAP(yr)     ( yr%400==0 || (yr%4==0 && yr%100!=0) )


/*! \brief Defines the type for reason of shutdown */
typedef enum {
    NMXP_SHUTDOWN_NORMAL		= 1,
    NMXP_SHUTDOWN_ERROR			= 2,
    NMXP_SHUTDOWN_TIMEOUT		= 3
} NMXP_SHUTDOWN_REASON;

/*! \brief Defines the client message types */
typedef enum {
   NMXP_MSG_CONNECT			= 100,
   NMXP_MSG_REQUESTPENDING		= 110,
   NMXP_MSG_CANCELREQUEST		= 205,
   NMXP_MSG_TERMINATESUBSCRIPTION	= 200,

   NMXP_MSG_ADDTIMESERIESCHANNELS	= 120,
   NMXP_MSG_ADDSOHCHANNELS		= 121,
   NMXP_MSG_ADDSERIALCHANNELS		= 124,
   NMXP_MSG_ADDTRIGGERCHANNELS		= 122,
   NMXP_MSG_ADDEVENTS			= 123,

   NMXP_MSG_REMOVETIMESERIESCHANNELS	= 130,
   NMXP_MSG_REMOVESOHCHANNELS		= 131,
   NMXP_MSG_REMOVESERIALCHANNELS	= 134,
   NMXP_MSG_REMOVETRIGGERCHANNELS	= 132,
   NMXP_MSG_REMOVEEVENTS		= 133,

   NMXP_MSG_CONNECTREQUEST		= 206,
   NMXP_MSG_CHANNELLISTREQUEST		= 209,
   NMXP_MSG_PRECISLISTREQUEST		= 203,
   NMXP_MSG_CHANNELINFOREQUEST		= 226,
   NMXP_MSG_DATASIZEREQUEST		= 229,
   NMXP_MSG_DATAREQUEST			= 227,
   NMXP_MSG_TRIGGERREQUEST		= 231,
   NMXP_MSG_EVENTREQUEST		= 232
   
} NMXP_MSG_CLIENT;

/*! \brief Defines the server message types. */
typedef enum {
    NMXP_MSG_CHANNELLIST		= 150,
    NMXP_MSG_ERROR			= 190,
    NMXP_MSG_COMPRESSED			= 1,
    NMXP_MSG_DECOMPRESSED		= 4,
    NMXP_MSG_TRIGGER			= 5,
    NMXP_MSG_EVENT			= 6,

    NMXP_MSG_READY			= 208,
    NMXP_MSG_PRECISLIST			= 253,
    NMXP_MSG_CHANNELHEADER		= 256,
    NMXP_MSG_DATASIZE			= 257,
    NMXP_MSG_NAQSEVENT			= 260,
    NMXP_MSG_NAQSTRIGGER		= 259

} NMXP_MSG_SERVER;


/*! \brief Header for all messages. */
typedef struct {
    int32_t signature;
    int32_t type;
    int32_t length;
} NMXP_MESSAGE_HEADER;

#define NMXP_MAX_LENGTH_DATA_BUFFER (4092 * 4)

#define NMXP_DATA_MAX_SIZE_DATE 200

/*! \brief Length in bytes of channel strings */
#define NMXP_DATA_NETWORK_LENGTH 10

/*! \brief Length in bytes of station strings */
#define NMXP_DATA_STATION_LENGTH 10

/*! \brief Length in bytes of channel strings */
#define NMXP_DATA_CHANNEL_LENGTH 10
#define NMXP_DATA_LOCATION_LENGTH 3

/*! Time-out for keeping the DataServer connection alive  */
#define NMXP_DAP_TIMEOUT_KEEPALIVE 15

/*! \brief Parameter structure for functions that process data */
typedef struct {
    int32_t key;			/*!< \brief Channel Key */
    char network[NMXP_DATA_NETWORK_LENGTH];	/*!< \brief Network code */
    char station[NMXP_DATA_STATION_LENGTH];	/*!< \brief Station code */
    char channel[NMXP_DATA_CHANNEL_LENGTH];	/*!< \brief Channel code */
    char location[NMXP_DATA_LOCATION_LENGTH];	/*!< \brief Location code */
    int32_t packet_type;			/*!< \brief Packet type */
    int32_t x0;				/*!< \brief First sample. It is significant only if x0n_significant != 0 */
    int32_t xn;				/*!< \brief Last sample. It is significant only if x0n_significant != 0 */
    int32_t x0n_significant;			/*!< \brief Declare if xn significant */
    int32_t oldest_seq_no;			/*!< \brief Oldest Sequence number */
    int32_t seq_no;				/*!< \brief Sequence number */
    double time;			/*!< \brief Time first sample. Epochs. */
    int *pDataPtr;			/*!< \brief Array of samples */
    int32_t nSamp;				/*!< \brief Number or samples */
    int32_t sampRate;			/*!< \brief Sample rate */
    int timing_quality;			/*!< \brief Timing quality for functions send_raw*() */
    char quality_indicator;             /*!< \brief Quality indicator D, R, Q or M, new in Seed 2.4 */
} NMXP_DATA_PROCESS;


/*! \brief For SDS or BUD directory structure */
typedef enum {
    NMXP_TYPE_WRITESEED_SDS = 0,
    NMXP_TYPE_WRITESEED_BUD
} NMXP_DATA_SEED_TYPEWRITE;

#define NMXP_DATA_MAX_SIZE_FILENAME 1024
#define NMXP_DATA_MAX_NUM_OPENED_FILE 200
/*! \brief Parameter structure for functions that handle mini-seed records */
typedef struct {
    int n_open_files;
    int last_open_file;
    int cur_open_file;
    int err_general;
    int err_outfile_mseed[NMXP_DATA_MAX_NUM_OPENED_FILE];
    FILE *outfile_mseed[NMXP_DATA_MAX_NUM_OPENED_FILE];
    char filename_mseed[NMXP_DATA_MAX_NUM_OPENED_FILE][NMXP_DATA_MAX_SIZE_FILENAME];
    char outdirseed[NMXP_DATA_MAX_SIZE_FILENAME];
    char default_network[5];
    NMXP_DATA_SEED_TYPEWRITE type_writeseed;
    /* pmsr is used like (void *) but it has to be a pointer to MSRecord !!! */
    void *pmsr;
} NMXP_DATA_SEED;

/*! \brief Initialize a structure NMXP_DATA_PROCESS
 *
 *  \param pd Pointer to a NMXP_DATA_PROCESS structure.
 *
 */
int nmxp_data_init(NMXP_DATA_PROCESS *pd);


/*! \brief Unpack a 17-byte Nanometrics compressed data bundle.           
 *
 * \param[out] outdata
 * \param indata
 * \param prev
 *
 * \return Number of unpacked data samples, -1 if null bundle. 
 *
 * Author:  Doug Neuhauser
 *          UC Berkeley Seismological Laboratory
 *          doug@seismo.berkeley.edu
 *
 */
int nmxp_data_unpack_bundle (int32_t *outdata, unsigned char *indata, int32_t *prev);


/* \brief Value for parameter exclude_bitmap in the function nmxp_data_trim() */
#define NMXP_DATA_TRIM_EXCLUDE_FIRST 2

/* \brief Value for parameter exclude_bitmap in the function nmxp_data_trim() */
#define NMXP_DATA_TRIM_EXCLUDE_LAST  4

/*! \brief Convert epoch in string
 */
int nmxp_data_to_str(char *out_str, double time_d);


/*! \brief Return year from epoch
 */
int nmxp_data_year_from_epoch(double time_d);


/*! \brief Return julian day from epoch
 */
int nmxp_data_yday_from_epoch(double time_d);


/*! \brief Trim data within a time interval
 *
 * \param pd Pointer to struct NMXP_DATA_PROCESS
 * \param trim_start_time Start time.
 * \param trim_end_time End time.
 * \param exclude_bitmap Bitmap for excluding or not the first and/or the last sample.
 *
 * \retval 2 On success, data has not been trimmed.
 * \retval 1 On success, data has been trimmed.
 * \retval 0 On error.
 *
 */
int nmxp_data_trim(NMXP_DATA_PROCESS *pd, double trim_start_time, double trim_end_time, unsigned char exclude_bitmap);


/*! \brief Return number of epochs in GMT 
 *
 */
time_t nmxp_data_gmtime_now();


/*! \brief Compute latency from current time and struct NMXP_DATA_PROCESS
 *
 * \param pd Pointer to struct NMXP_DATA_PROCESS
 *
 */
double nmxp_data_latency(NMXP_DATA_PROCESS *pd);


/*! \brief Print info about struct NMXP_DATA_PROCESS
 *
 * \param pd Pointer to struct NMXP_DATA_PROCESS
 * \param flag_sample If it is not equal to zero sample values will be printed
 *
 */
int nmxp_data_log(NMXP_DATA_PROCESS *pd, int flag_sample);


/*! \brief Parse string and set value in ret_tm
 *
 *
 */
int nmxp_data_parse_date(const char *pstr_date, NMXP_TM_T *ret_tmt);


/*! \brief Wrapper for timegm
 *
 *
 */
double nmxp_data_tm_to_time(NMXP_TM_T *tmt);


/*! \brief Return path of the current directory
 *
 * Return value need to be freed!
 */
char *nmxp_data_gnu_getcwd ();


/*! \brief Check if the directory exists
 *
 * \retval 1 exists, 0 otherwise.
 */
int nmxp_data_dir_exists (char *dirname);


/*! \brief Check if the directory exists
 *
 * Return value need to be freed!
 */
char *nmxp_data_dir_abspath (char *dirname);


/*! \brief Create the directory.
 * Wrapper for mkdir() over different systems.
 */
int nmxp_data_mkdir(const char *dirname);


/*! \brief Create the directory and subdirectories if is needed
 */
int nmxp_data_mkdirp(const char *filename);


/*! \brief Initialize a structure NMXP_DATA_SEED
 *
 *  \param data_seed Pointer to a NMXP_DATA_SEED structure.
 *  \param default_network String containing default network code.
 *  \param outdirseed Root output directory for SDS or BUD structure.
 *  \param type_writeseed Declare SDS or BUD structure.
 *
 */
int nmxp_data_seed_init(NMXP_DATA_SEED *data_seed, char *default_network, char *outdirseed, NMXP_DATA_SEED_TYPEWRITE type_writeseed);

/*! \brief Open file in a structure NMXP_DATA_SEED, in case close file before.
 *
 *  \param data_seed Pointer to a NMXP_DATA_SEED structure.
 *  
 *  N.B. nmxp_data_seed_fopen() reads information from data_seed->pd.
 *
 */
int nmxp_data_seed_fopen(NMXP_DATA_SEED *data_seed);

/*! \brief Close file in a structure NMXP_DATA_SEED
 *
 *  \param data_seed Pointer to a NMXP_DATA_SEED structure.
 *  \param i Index of the file descriptor to delete.
 *
 */
int nmxp_data_seed_fclose(NMXP_DATA_SEED *data_seed, int i);

/*! \brief Close all file in a structure NMXP_DATA_SEED
 *
 *  \param data_seed Pointer to a NMXP_DATA_SEED structure.
 *
 */
int nmxp_data_seed_fclose_all(NMXP_DATA_SEED *data_seed);


/*! \brief
 *
 */
int nmxp_data_get_filename_ms(NMXP_DATA_SEED *data_seed, char *dirseedchan, char *filenameseed);


/*! \brief Write mini-seed records from a NMXP_DATA_PROCESS structure.
 *
 * \param pd Pointer to struct NMXP_DATA_PROCESS. If it is NULL then flush all data into mini-SEED file.
 * \param data_seed Pointer to struct NMXP_DATA_SEED.
 * \param pmsr Pointer to mini-SEED record.
 *
 * \warning pmsr is used like (void *) but it has to be a pointer to MSRecord !!!
 *
 * \return Returns the number records created on success and -1 on error. Return value of msr_pack().
 *
 */
int nmxp_data_msr_pack(NMXP_DATA_PROCESS *pd, NMXP_DATA_SEED *data_seed, void *pmsr);


/*! \brief Swap 2 bytes. 
 *
 * \param in Variable length 2 bytes.
 *
 */
void nmxp_data_swap_2b (int16_t *in);


/*! \brief Swap 3 bytes. 
 *
 * \param in Variable length 3 bytes.
 *
 */
void nmxp_data_swap_3b (unsigned char *in);


/*! \brief Swap 4 bytes. 
 *
 * \param in Variable length 4 bytes.
 *
 */
void nmxp_data_swap_4b (int32_t *in);


/*! \brief Swap 8 bytes. 
 *
 * \param in Variable length 8 bytes.
 *
 */
void nmxp_data_swap_8b (double *in);


/*! \brief Determine the byte order of the host machine. 
 *  Due to the lack of portable defines to determine host byte order this
 *  run-time test is provided.  The code below actually tests for
 *  little-endianess, the only other alternative is assumed to be big endian.
 *
 *  \retval 0 if the host is little endian.
 *  \retval 1 otherwise.
 */
int nmxp_data_bigendianhost ();


#endif

