/*
 * vtabulardata_buf.h
 *
 * Developed for ssam data, cheryl
 *
 * April, 2000
 *
 * copied from ssam_buf.h and modified.
 *
 *
 *
 */

#ifndef VTABULAR_BUF_H
#define VTABULAR_BUF_H

#define NETWORK_NULL_STRING "-"

/* NOTE:
 * 
 * 
 *
 */

#define	VTABULAR_STA_LEN	6
#define	VTABULAR_CHAN_LEN	8
#define	VTABULAR_NET_LEN	6
#define MAX_VTABULARDATA_SIZ 4096   /* define maximum size of trace message buffer */

#pragma pack(1)
typedef struct {
        int   row_size;         /* length in bytes of row information plus 
                                         data. */
        double  endtime;        /* time of last sample in epoch seconds 	
                                      (seconds since midnight 1/1/1970) */
        short     ncol;           /* column number to start   
                                       (0 for ssam data) */
        long     opt;            /* optional */
        char    rowpad[6];      /* padding May want to put earthworm-format
                                 data value here (sun, vax, intel, etc.)  */
        double samprate;        /* Sample rate; nominal */
} VTABULAR_ROW;

typedef struct {
        char    sta[VTABULAR_STA_LEN];         /* Site name */
        char    chan[VTABULAR_CHAN_LEN];         /* Network name */
        char    net[VTABULAR_NET_LEN];        /* Component/channel code */
        char    tbl_code[2];     /* lookup code for decoding colomn format. Should be SS for ssam  */ 
        double  msgtime;         /* Time message is put into the ring. */
        short   nrows;           /* Number of rows in message (1 for ssam
                                   data.) */
       char    datatype[2];    /* Data format code */
       char  padding[6];         /* padding */

        VTABULAR_ROW   RowHeader;

} VTABULAR_HEADER;




typedef union {
        char    msg[MAX_VTABULARDATA_SIZ];
        VTABULAR_HEADER trh;
        int     i;
} VtabularPacket;

#pragma pack()


/* Byte 0 of data quality flags, as in SEED format
   ***********************************************/
#define AMPLIFIER_SATURATED    0x01
#define DIGITIZER_CLIPPED      0x02
#define SPIKES_DETECTED        0x04
#define GLITCHES_DETECTED      0x08
#define MISSING_DATA_PRESENT   0x10
#define TELEMETRY_SYNCH_ERROR  0x20
#define FILTER_CHARGING        0x40
#define TIME_TAG_QUESTIONABLE  0x80

/* CSS datatype codes
   ******************/
/*
        t4      SUN IEEE single precision real
        t8      SUN IEEE double precision real
        s4      SUN IEEE integer
        s2      SUN IEEE short integer
        f4      VAX/Intel IEEE single precision real
        f8      VAX/Intel IEEE double precision real
        i4      VAX/Intel IEEE integer
        i2      VAX/Intel IEEE short integer
        g2      NORESS gain-ranged
*/
#endif
