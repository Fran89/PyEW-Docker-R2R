#ifndef _GCF_H
#define _GCF_H
#include <stdio.h>
#include "gcf_error.h"

/* 
	COPYRIGHT 1998. Paul Friberg and Sid Hellman, ISTI
	Instrumental Software Technologies, Inc
 */

/* 
   The following two tokens are passed to gcf_read() to distinguish 
   between serial reads or unix disk file reads
*/
#define GCFTP	 (002)		/* serial or internet read */
#define GCFDISK  (004)		/* 1024 byte packet from disk read */
#define GCFUDP   (010)		/* UDP packets from SCREAM */
#define GCFDISKH (020)		/* 1024 byte packet from disk read ONLY PROCESS HEADER not DATA[] */
#define GCF32BIT (040)		/* The data can have 32 bit values */
#define GCF24BIT (0100)		/* The data can have 24 bit overflow values */

#define GCF_MAX_SAMP (1000)	/* number of samples possible in a GCF packet */
#define GCF_ID_STR   (8)	/* number of chars in the id strings */

#define GCF_BLOCK	(1024)	/* size of a 1024 packet on disk or via SCREAM */
#define GCF_HDRSIZE	(24)	/* the size of the header info */

#define GCF_LIB_VER_NUMBER "2001.088"

typedef struct gcf_time_type {
        short   year;		/* year including century */
        short   day_of_year;	/* Julian ;) day */
        short   mo;		/* month */
        short   day;		/* day of month */
        short   hour;		/* hour (24hr clock */
        short   min;		/* minutes */
        short   sec;		/* seconds */
        short   msec;		/* milliseconds */
        short   usec;		/* microseconds, set if precision allows */
} GTime;

typedef struct gcf_epoch {
	long day_epoch; 	/* days since Nov 17, 1989 */
	long sec_per_day;	/* number of seconds into this day */
	} Gepoch;


typedef struct gcf_head {
	long disk_addr;		/* internal use only */
	long stream_num;	/* internal use only */
	char system_id[GCF_ID_STR];	/* 6 character SAM or digitizer id */
	char stream_id[GCF_ID_STR];	/* channel identifier */
	Gepoch epoch;		/* Guralp Epoch notation */
	long num_samps;		/* number of samples in data, chars if sps==0 */
	long num_records;	/* number of compressed records */
	long compress_format;	/* 1, 2, or 4 */
	long sample_rate;	/* in sps, 0 indicates SOH block in data */
	long reserved;
	long max;		/* maximum value in data array */
	long min;		/* minimum value in data array */
	long data[GCF_MAX_SAMP];	/* data array in native byte order */
} GCFhdr;


/* some function definitions */

#ifdef STDC

void determine_byte_order();
int  gcfhdr_read(char *buf, GCFhdr *hdr, int type);
int  gcfhdr_read_s(char *buf, GCFhdr *hdr, int type, int size);
void gcfhdr_print(GCFhdr *hdr, FILE *fptr);
void gcfhdr_printstatus(GCFhdr *hdr, FILE *fptr);
void gcfhdr_print_data_max_min(GCFhdr *hdr, FILE *fptr);
void gcfhdr_swap(GCFhdr *hdr);
void gcf_swap(char * buf);
void gcf_serial2disk(char * inbuf, char * outbuf);
void gcf_serial2disk_new(char * inbuf, char * outbuf, int size);
int  gcf_open_scsidisk(char * disk_dev);
void * rmemcpy(void *, const void *, unsigned int);
int  day_of_year(GTime *t);
int  gcf_isleap(int year);
int  gepoch2gmt (Gepoch *epoch, GTime *time);
long gepoch2uepoch (Gepoch *epoch);
void decode_b36(long num, char *str);
char * gcf_getread_error();

/* this next one should probably be in gcf_udp.h */
int  udp_open(char * hostname, char * service, int port, int dontconn);

#else

void determine_byte_order();
void * rmemcpy();
int  gcfhdr_read();
int  gcfhdr_read_s();
void gcfhdr_print();
void gcfhdr_printstatus();
void gcfhdr_swap();
void gcf_swap();
void gcf_serial2disk();
void gcf_serial2disk_new();
int  gcf_open_scsidisk();
int  day_of_year();
int  gcf_isleap();
int  gepoch2gmt();
long gepoch2uepoch();
void decode_b36();
char * gcf_getread_error();

int  udp_open();

#endif

#endif /*_GCF_H */
