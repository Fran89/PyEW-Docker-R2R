/*! \file
 *
 * \brief Memory management for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_memory.c 4165 2011-01-24 15:19:28Z quintiliani $
 *
 */

#include "nmxp_memory.h"

#ifndef NMXP_MEM_DEBUG

int nmxp_mem_null_function() {
    return -1;
}

#else

#include "nmxp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "config.h"

/* Set debug_log_single to 1 for logging malloc(), strdup() and free() calls */
static int debug_log_single = 0;

#define MAX_LEN_SOURCE_FILE_LINE 100

typedef struct {
    void *p;
    int size;
    char source_file_line[MAX_LEN_SOURCE_FILE_LINE];
    struct timeval tv;
} NMXP_MEM_STRUCT;

typedef struct {
    char source_file_line[MAX_LEN_SOURCE_FILE_LINE];
    long int times;
} NMXP_MEM_SOURCE_FILE_LINE_STAT;

#define MAX_MEM_STRUCTS (4096 * 16)

static NMXP_MEM_STRUCT nms[MAX_MEM_STRUCTS];
static int i_nms = 0;

#define MAX_MEM_SFS (1024)
static NMXP_MEM_SOURCE_FILE_LINE_STAT sfs[MAX_MEM_SFS];
static int i_sfs = 0;


inline long int nmxp_mem_add_sfs(char *source_file_line, int t) {
    long int cur_times;
    int i;
    i=0;
    while(i < i_sfs
	    && i < MAX_MEM_SFS
	    &&  strcmp(sfs[i].source_file_line, source_file_line)!=0) {
	i++;
    }
    if(i >= i_sfs) {
	if(i_sfs < MAX_MEM_SFS) {
	    strncpy(sfs[i_sfs].source_file_line, source_file_line, MAX_LEN_SOURCE_FILE_LINE);
	    sfs[i_sfs].times=t;
	    i_sfs++;
	    cur_times = t;
	} else {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_mem_add_sfs i_sfs > MAX_MEM_SFS %d > %d\n", i_sfs, MAX_MEM_SFS);
	    cur_times = -1000000;
	}
    } else {
	sfs[i].times+=t;
	cur_times = sfs[i].times;
    }
    return cur_times;
}

inline void nmxp_mem_print_sfs() {
    int i;
    i=0;
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "\n");
    while(i < i_sfs) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "%4d: %10ld %s\n",
		i+1, sfs[i].times, sfs[i].source_file_line
		);
	i++;
    }
}

inline int nmxp_mem_add_ptr(void *ptr, size_t size, char *source_file_line, struct timeval *tv) {
    int ret = -1;
    if(i_nms < MAX_MEM_STRUCTS) {
	nmxp_mem_add_sfs(source_file_line, 1);
	nms[i_nms].p = ptr;
	nms[i_nms].size = size;
	strncpy(nms[i_nms].source_file_line, source_file_line, MAX_LEN_SOURCE_FILE_LINE);
	nms[i_nms].tv.tv_sec = tv->tv_sec;
	nms[i_nms].tv.tv_usec = tv->tv_usec;
	ret = i_nms;
	i_nms++;
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_mem_add_ptr i_nms > MAX_MEM_STRUCTS %d > %d\n", i_nms, MAX_MEM_STRUCTS);
	ret = -1;
    }
    return ret;
}


inline int nmxp_mem_rem_ptr(void *ptr, struct timeval *tv, int *size) {
    int i, j;

    tv->tv_sec = 0;
    tv->tv_usec = 0;
    *size = 0;

    i = 0;
    while(i < i_nms && nms[i].p != ptr) {
	i++;
    }

    if(i >= i_nms  ||  i > MAX_MEM_STRUCTS) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_mem_rem_ptr %010p not found i=%d\n", ptr, i);
	i = -1;
    } else {
	nmxp_mem_add_sfs(nms[i].source_file_line, -1);
	/* shift */
	tv->tv_sec = nms[i].tv.tv_sec;
	tv->tv_usec = nms[i].tv.tv_usec;
	*size = nms[i].size;
	j = i;
	while(j < i_nms-1) {
	    nms[j].p = nms[j+1].p;
	    nms[j].size = nms[j+1].size;
	    strncpy(nms[j].source_file_line, nms[j+1].source_file_line, MAX_LEN_SOURCE_FILE_LINE);;
	    nms[j].tv.tv_sec = nms[j+1].tv.tv_sec;
	    nms[j].tv.tv_usec = nms[j+1].tv.tv_usec;
	    j++;
	}
	i_nms--;
    }
    return i;
}


inline int nmxp_mem_print_ptr(int print_items, int print_sfs, char *source_file, int line) {
    int i;
    static int old_tot_size = 0;
    int tot_size;

    tot_size = 0;
    for(i=0; i < i_nms  &&  i_nms < MAX_MEM_STRUCTS; i++) {
	tot_size += nms[i].size;
    }

    if(tot_size != old_tot_size) {
	if(print_items) {
	    i=0;
	    while(i<i_nms  && i_nms < MAX_MEM_STRUCTS) {
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "%d %d.%d %010p %d %s\n",
			i,
			nms[i].tv.tv_sec,
			nms[i].tv.tv_usec,
			nms[i].p,
			nms[i].size,
			nms[i].source_file_line
			);
		i++;
	    }
	}
	old_tot_size = tot_size;
    }

    if(print_sfs) {
	nmxp_mem_print_sfs();
    }

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "nmxp_mem_print_ptr() tot %d  %s:%d\n", tot_size, source_file, line);

    return tot_size;
}


inline char *nmxp_mem_source_file_line(char *source_file, int line) {
    static char source_file_line[MAX_LEN_SOURCE_FILE_LINE];
    snprintf(source_file_line, MAX_LEN_SOURCE_FILE_LINE, "%s:%d", source_file, line);
    return source_file_line;
}


#define NMXP_SEG_MALLOC 1024
inline void *nmxp_mem_malloc(size_t size, char *source_file, int line) {
    void *ret = NULL;
    struct timeval tv;
    int i;
    char source_file_line[MAX_LEN_SOURCE_FILE_LINE];
    size_t old_size;

    old_size = size;
    size = ( ( ( (old_size - 1) / NMXP_SEG_MALLOC ) ) + 1) * NMXP_SEG_MALLOC;

    gettimeofday(&tv, NULL);
    ret = malloc(size);
    strncpy(source_file_line, nmxp_mem_source_file_line(source_file, line), MAX_LEN_SOURCE_FILE_LINE);
    i = nmxp_mem_add_ptr(ret, size, source_file_line, &tv);
    if(debug_log_single  ||  i == -1) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "nmxp_mem_malloc %d.%d %010p+%d %s_%d\n", tv.tv_sec, tv.tv_usec, ret, size, source_file_line, i);
    }
    return ret;
}


inline char *nmxp_mem_strdup(const char *str, char *source_file, int line) {
    char *ret = NULL;
    int size;
    struct timeval tv;
    int i;
    char source_file_line[MAX_LEN_SOURCE_FILE_LINE];
    size_t old_size;

    gettimeofday(&tv, NULL);

    if(str) {
	old_size = strlen(str) + 1;
	size = ( ( ( (old_size - 1) / NMXP_SEG_MALLOC ) ) + 1) * NMXP_SEG_MALLOC;

	ret = (char *) malloc(size);
	strcpy(ret, str);

	strncpy(source_file_line, nmxp_mem_source_file_line(source_file, line), MAX_LEN_SOURCE_FILE_LINE);
	i = nmxp_mem_add_ptr(ret, size, source_file_line, &tv);
	if(debug_log_single  ||  i == -1) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "nmxp_mem_strdup %d.%d %010p+%d %s_%d\n", tv.tv_sec, tv.tv_usec, ret, size, source_file_line, i);
	}
    }

    return ret;
}


inline void nmxp_mem_free(void *ptr, char *source_file, int line) {
    int i;
    struct timeval tv;
    int size;
    char source_file_line[MAX_LEN_SOURCE_FILE_LINE];

    if(ptr) {
	i = nmxp_mem_rem_ptr(ptr, &tv, &size);
	strncpy(source_file_line, nmxp_mem_source_file_line(source_file, line), MAX_LEN_SOURCE_FILE_LINE);
	if(debug_log_single  ||  i == -1) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "nmxp_mem_free   %d.%d %010p+%d %s_%d\n", tv.tv_sec, tv.tv_usec, ptr, size, source_file_line, i);
	}
	free(ptr);
	/* prevents double frees, added 2010-07-26, RR */
	ptr = NULL;
    }
}

#endif

