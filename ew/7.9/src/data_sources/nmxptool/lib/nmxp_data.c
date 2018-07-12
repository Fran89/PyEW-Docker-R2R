/*! \file
 *
 * \brief Data for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_data.c 6803 2016-09-09 06:06:39Z et $
 *
 */

#include "nmxp_data.h"
#include "nmxp_log.h"
#include "nmxp_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif


#include "config.h"

#ifdef HAVE_LIBMSEED
#include <libmseed.h>
#endif

/*
For a portable version of timegm(), set the TZ environment variable  to
UTC, call mktime() and restore the value of TZ.  Something like
*/
#ifndef HAVE_TIMEGM

time_t my_timegm (struct tm *tm) {
    time_t ret;
    /*TODO stefano avoid static*/
    static char first_time = 1;
    char *tz;

#ifndef HAVE_SETENV
#ifndef HAVE_UNDERSCORE_TIMEZONE
#warning Computation of packet latencies could be wrong if local time is not equal to UTC.
    if(first_time) {
	    first_time = 0;
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "Computation of packet latencies could be wrong if local time is not equal to UTC.\n");
    }
#endif
#endif

    ret = mktime(tm);

#ifdef HAVE_SETENV
    if(first_time) {
	first_time = 0;
	tz = getenv("TZ");
	setenv("TZ", tz, 1);
	tzset();
    }
#endif

#ifdef HAVE_UNDERSCORE_TIMEZONE
    ret -= _timezone;
#endif

    return ret;
}

#endif


int nmxp_data_init(NMXP_DATA_PROCESS *pd) {
    pd->key = -1;
    pd->network[0] = 0;
    pd->station[0] = 0;
    pd->channel[0] = 0;
    pd->location[0] = 0;
    pd->packet_type = -1;
    pd->x0 = -1;
    pd->xn = -1;
    pd->x0n_significant = 0;
    pd->oldest_seq_no = -1;
    pd->seq_no = -1;
    pd->time = -1.0;
    pd->pDataPtr = NULL;
    pd->nSamp = 0;
    pd->sampRate = -1;
    pd->timing_quality = -1;
    return 0;
}


int nmxp_data_unpack_bundle (int32_t *outdata, unsigned char *indata, int32_t *prev)
{         
	int32_t nsamples = 0;
	int32_t d4[4];
	int16_t d2[2];
	int32_t cb[4];  
	int32_t i, j, k=0;
	unsigned char cbits;
	/* TOREMOVE int my_order = get_my_wordorder(); */
	int my_host_is_bigendian = nmxp_data_bigendianhost();

	cbits = (unsigned char)indata[0];
	if (cbits == 9) return (-1);
	++indata;

	/* Get the compression bits for the bundle. */
	for (i=0,j=6; j>=0; i++,j-=2) {
		cb[i] = (cbits>>j) & 3;
	}       

	for (j=0; j<4; j++) {
		/*
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "cb[%d]=%d\n", j, cb[j]);
		*/
		switch (cb[j]) 
		{   
			case 0:       /* not used     */
				k=0;
				break;       
			case 1:       /* 4 byte diffs */
				d4[0] = (signed char)indata[0];
				d4[1] = (signed char)indata[1];
				d4[2] = (signed char)indata[2];
				d4[3] = (signed char)indata[3];
				k=4;
				break;
			case 2:       /* 2 16-bit diffs */
				memcpy (&d2[0],indata,2);
				memcpy (&d2[1],indata+2,2);
				/* TOREMOVE if (my_order != SEED_LITTLE_ENDIAN) { */
				if (my_host_is_bigendian) {
					nmxp_data_swap_2b (&d2[0]);
					nmxp_data_swap_2b (&d2[1]);
				}
				d4[0] = d2[0];
				d4[1] = d2[1];
				k=2;
				break;
			case 3:       /* 1 32-bit diff */
				memcpy (&d4[0],indata,4);
				/* TOREMOVE if (my_order != SEED_LITTLE_ENDIAN) { */
				if (my_host_is_bigendian) {
					nmxp_data_swap_4b (&d4[0]);
				}
				k=1;
				break;
		}
		indata += 4;

		for (i=0; i<k; i++) {
			*outdata = *prev + d4[i];
			/* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "val = %d, diff[%d] = %d, *prev = %d\n",
				*outdata, i, d4[i], *prev);
				*/
			*prev = *outdata;

			outdata++;
			++nsamples;
		}
	}
	return (nsamples);
}


int nmxp_data_to_str(char *out_str, double time_d) {
    time_t time_t_start_time;
    struct tm tm_start_time;

    if(time_d > 0.0) {
	    time_t_start_time = (time_t) time_d;
    } else {
	    time_t_start_time = 0;
    }

    gmtime_r(&time_t_start_time, &tm_start_time);

    snprintf(out_str, NMXP_DATA_MAX_SIZE_DATE, "%04d.%03d,%02d:%02d:%02d.%04d",
	    tm_start_time.tm_year + 1900,
	    /*
	    tm_start_time.tm_mon + 1,
	    tm_start_time.tm_mday,
	    */
	    tm_start_time.tm_yday + 1,
	    tm_start_time.tm_hour,
	    tm_start_time.tm_min,
	    tm_start_time.tm_sec,
	    (time_t_start_time == 0)? 0 : (int) (  ((time_d - (double) time_t_start_time)) * 10000.0 )
	   );
    
    return 0;
}

int nmxp_data_year_from_epoch(double time_d) {
    time_t time_t_start_time;
    struct tm tm_start_time;

    if(time_d > 0.0) {
	    time_t_start_time = (time_t) time_d;
    } else {
	    time_t_start_time = 0;
    }
    /*Use reentrant to be trhead safe*/
    gmtime_r(&time_t_start_time,&tm_start_time);    
    return tm_start_time.tm_year + 1900;
}


int nmxp_data_yday_from_epoch(double time_d) {
    time_t time_t_start_time;
    struct tm tm_start_time;

    if(time_d > 0.0) {
	    time_t_start_time = (time_t) time_d;
    } else {
	    time_t_start_time = 0;
    } 
    /*Use reentrant to be trhead safe*/
    gmtime_r(&time_t_start_time,&tm_start_time);    
    return tm_start_time.tm_yday + 1;
}

int nmxp_data_trim(NMXP_DATA_PROCESS *pd, double trim_start_time, double trim_end_time, unsigned char exclude_bitmap) {
    int ret = 0;
    double first_time, last_time;
    int first_nsamples_to_remove = 0;
    int last_nsamples_to_remove = 0;
    int32_t new_nSamp = 0;
    int32_t i;


    if(pd) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmxp_data_trim(pd, %.4f, %.4f, %d)\n", trim_start_time, trim_end_time, exclude_bitmap);
	first_time = pd->time;
	last_time = pd->time + ((double) pd->nSamp / (double) pd->sampRate);
	if(first_time <= trim_start_time &&  trim_start_time <= last_time) {
	    first_nsamples_to_remove = (int) ( ((trim_start_time - first_time) * (double) pd->sampRate) + 0.5 );
	    if((exclude_bitmap & NMXP_DATA_TRIM_EXCLUDE_FIRST)) {
		first_nsamples_to_remove++;
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Excluded the first sample!\n");
	    }
	}
	if(first_time <= trim_end_time  &&  trim_end_time <= last_time) {
	    last_nsamples_to_remove = (int) ( ((last_time - trim_end_time) * (double) pd->sampRate) + 0.5 );
	    if((exclude_bitmap & NMXP_DATA_TRIM_EXCLUDE_LAST)) {
		last_nsamples_to_remove++;
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Excluded the last sample!\n");
	    }
	}

	if( (first_time < trim_start_time  &&  last_time < trim_start_time) ||
		(first_time > trim_end_time  &&  last_time > trim_end_time) ) {
	    first_nsamples_to_remove = pd->nSamp;
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Excluded all samples!\n");
	}

	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "first_time=%.2f last_time=%.2f trim_start_time=%.2f trim_end_time=%.2f\n",
		first_time, last_time, trim_start_time, trim_end_time);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "first_nsamples_to_remove=%d last_nsamples_to_remove=%d pd->nSamp=%d\n",
		first_nsamples_to_remove,
		last_nsamples_to_remove,
		pd->nSamp);

	if(first_nsamples_to_remove > 0 || last_nsamples_to_remove > 0) {

	    new_nSamp = pd->nSamp - (first_nsamples_to_remove + last_nsamples_to_remove);

	    if(new_nSamp > 0) {

		if(first_nsamples_to_remove > 0) {
		    pd->x0 = pd->pDataPtr[first_nsamples_to_remove];
		}

		if(last_nsamples_to_remove > 0) {
		    pd->xn = pd->pDataPtr[pd->nSamp - last_nsamples_to_remove];
		}

		for(i=0; i < pd->nSamp - first_nsamples_to_remove; i++) {
		    pd->pDataPtr[i] = pd->pDataPtr[first_nsamples_to_remove + i];
		}
		pd->nSamp = new_nSamp;
		pd->time += ((double) first_nsamples_to_remove / (double) pd->sampRate);

		ret = 1;


	    } else if(new_nSamp == 0) {
		if(pd->pDataPtr) {
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmxp_data_trim() nSamp = %d for %s.%s.%s.\n",
			    new_nSamp, NMXP_LOG_STR(pd->network), NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel));
		}
		pd->nSamp = 0;
		pd->x0 = -1;
		pd->xn = -1;
		ret = 1;
	    } else {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Error in nmxp_data_trim() nSamp = %d for %s.%s.%s.\n",
			    new_nSamp, NMXP_LOG_STR(pd->network), NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel));
	    }

	} else {
	    ret = 2;
	}
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "nmxp_data_trim() is called with pd = NULL\n");
    }

    if(ret == 1) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmxp_data_trim() trimmed data! (Output %d samples for %s.%s.%s)\n",
		pd->nSamp, NMXP_LOG_STR(pd->network), NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel));
    }

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "nmxp_data_trim() %s.%s.%s exit ret=%d\n",
	    NMXP_LOG_STR(pd->network), NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel), ret);

    return ret;
}

time_t nmxp_data_gmtime_now() {
    time_t time_now;
    time(&time_now);
    return time_now;
}

double nmxp_data_latency(NMXP_DATA_PROCESS *pd) {
    double latency = 0.0;
    time_t time_now = nmxp_data_gmtime_now();
    
    if(pd) {
	latency = ((double) time_now) - (pd->time + ((double) pd->nSamp / (double) pd->sampRate));
    }

    return latency;
}


int nmxp_data_log(NMXP_DATA_PROCESS *pd, int flag_sample) {

    char str_start[NMXP_DATA_MAX_SIZE_DATE], str_end[NMXP_DATA_MAX_SIZE_DATE];
    int i;

    str_start[0] = 0;
    str_end[0] = 0;
    
    if(pd) {
	nmxp_data_to_str(str_start, pd->time);
	nmxp_data_to_str(str_end, pd->time + ((double) pd->nSamp / (double) pd->sampRate));

	/* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "%12d %5s.%3s rate=%03d (%s - %s) [%d, %d] pts=%04d (%d, %d, %d, %d) lat=%.1f len=%d\n", */
	/* printf("%10d %5s.%3s 03dHz (%s - %s) lat=%.1fs [%d, %d] pts=%04d (%d, %d, %d, %d) len=%d\n", */
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%s.%s.%3s.%2s %3dHz (%s - %s) lat %.1fs [%d, %d] (%d) %4dpts (%d, %d, %d, %d, %d)\n",
		/* pd->key, */
		NMXP_LOG_STR(pd->network),
		(strlen(pd->station) == 0)? "XXXX" : NMXP_LOG_STR(pd->station),
		(strlen(pd->channel) == 0)? "XXX" : NMXP_LOG_STR(pd->channel),
		(strlen(pd->location) == 0)? "XX" : NMXP_LOG_STR(pd->location),
		pd->sampRate,
		NMXP_LOG_STR(str_start),
		NMXP_LOG_STR(str_end),
		nmxp_data_latency(pd),
		pd->packet_type,
		pd->seq_no,
		pd->oldest_seq_no,
		pd->nSamp,
		pd->x0,
		(pd->pDataPtr == NULL)? 0 : pd->pDataPtr[0],
		(pd->pDataPtr == NULL || pd->nSamp < 1)? 0 : pd->pDataPtr[pd->nSamp-1],
		pd->xn,
		pd->x0n_significant
	      );

	if(pd->pDataPtr  &&  flag_sample != 0  &&  pd->nSamp > 0) {
	    for(i=0; i < pd->nSamp; i++) {
		nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "%6d ", pd->pDataPtr[i]);
		if((i + 1) % 20 == 0) {
		    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\n");
		}
	    }
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_ANY, "\n");
	}

    } else {
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_PACKETMAN, "Pointer to NMXP_DATA_PROCESS is NULL!\n");
    }

    return 0;
}


int nmxp_data_parse_date(const char *pstr_date, NMXP_TM_T *ret_tmt) {
/* Input formats: 
 *     <date>,<time> | <date>
 *
 * where:
 *     <date> = yyyy/mm/dd | yyy.jjj
 *     <time> = hh:mm:ss | hh:mm
 *
 *     yyyy = year
 *     mm   = month       (1-12)
 *     dd   = day         (1-31)
 *     jjj  = day-of-year (1-365)
 *     hh   = hour        (0-23)
 *     mm   = minute      (0-59)
 *     ss   = second      (0-59)
 */

    int ret = 0;

    char str_tt[20];
    int k;

#define MAX_LENGTH_STR_MESSAGE 30
    char str_date[MAX_LENGTH_STR_MESSAGE] = "NO DATE";

#define MAX_LENGTH_ERR_MESSAGE 500
    char err_message[MAX_LENGTH_ERR_MESSAGE] = "NO MESSAGE";

    char *pEnd = NULL;
    int32_t app;
    int state;
    int flag_finished = 0;

    time_t time_now;
    struct tm tm_now;

    int month_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int m, d, day_sum, jday;


    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_DATE, "Date to validate '%s'\n", NMXP_LOG_STR(pstr_date));
	
    strncpy(str_date, pstr_date, MAX_LENGTH_STR_MESSAGE);
    pEnd = str_date;
    app = strtol(str_date, &pEnd, 10);
    state = 0;
    if(  errno == EINVAL ||  errno == ERANGE ) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_DATE, "%s\n", NMXP_LOG_STR(strerror(errno)));
	ret = -1;
    }

    if(pEnd[0] != 0  &&  ret != -1) {
	ret = 0;
    } else {
	strncpy(err_message, "Error parsing year!", MAX_LENGTH_ERR_MESSAGE);
	ret = -1;
    }

    /* initialize ret_tmt */
    time(&time_now);
    gmtime_r(&time_now,&tm_now);

    ret_tmt->t.tm_sec = 0 ;
    ret_tmt->t.tm_min = 0;
    ret_tmt->t.tm_hour = 0;
    ret_tmt->t.tm_mday = tm_now.tm_mday;
    ret_tmt->t.tm_mon = tm_now.tm_mon;
    ret_tmt->t.tm_year = tm_now.tm_year;
    ret_tmt->t.tm_wday = tm_now.tm_wday;
    ret_tmt->t.tm_yday = tm_now.tm_yday;
    ret_tmt->t.tm_isdst = tm_now.tm_isdst;
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    ret_tmt->t.tm_gmtoff = tm_now.tm_gmtoff;
#endif
    ret_tmt->d = 0;

    
    /* loop for parsing by a finite state machine */
    while( 
	    !flag_finished
	    && ret == 0
	    &&  errno != EINVAL
	    &&  errno != ERANGE
	    ) {

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_DATE, "state=%d value=%d flag_finished=%d ret=%d pEnd[0]=%c [%d]  (%s)\n",
	    state, app, flag_finished, ret, (pEnd[0]==0)? '_' : pEnd[0], pEnd[0], NMXP_LOG_STR(pEnd));

	/* switch on state */
	switch(state) {
	    case 0: /* Parse year */
		ret_tmt->t.tm_year = app - 1900;
		if(pEnd[0] == '/') {
		    state = 1; /* Month */
		} else if(pEnd[0] == '.') {
		    state = 3; /* Julian Day */
		} else {
		    strncpy(err_message, "Wrong separator after year!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    case 1: /* Parse month */
		ret_tmt->t.tm_mon = app - 1;
		if(pEnd[0] == '/')
		    state = 2; /* Day of month */
		else {
		    strncpy(err_message, "Wrong separator after month!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    case 2: /* Parse day of month */
		ret_tmt->t.tm_mday = app;
		if(pEnd[0] == 0) {
		    flag_finished = 1;
		} else if(pEnd[0] == ',') {
		    state = 4; /* Hour */
		} else {
			strncpy(err_message, "Wrong separator after day of month!", MAX_LENGTH_ERR_MESSAGE);
			ret = -1;
		    }
		break;

	    case 3: /* Parse Julian Day */
		ret_tmt->t.tm_yday = app - 1;

		jday=app;

		if(NMXP_DATA_IS_LEAP(ret_tmt->t.tm_year)) {
		    month_days[1]++;
		}

		m=0;
		day_sum = 0;
		while(month_days[m] < (jday - day_sum)) {
		    day_sum += month_days[m++];
		}
		d = jday-day_sum;

		ret_tmt->t.tm_mon = m;
		ret_tmt->t.tm_mday = d;

		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_DATE, "Month %d Day %d\n", m, d);

		if(pEnd[0] == 0) {
		    flag_finished = 1;
		} else if(pEnd[0] == ',') {
		    state = 4; /* Hour */
		} else {
		    strncpy(err_message, "Wrong separator after julian day!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    case 4: /* Parse hour */
		ret_tmt->t.tm_hour = app;
		if(pEnd[0] == ':') {
		    state = 5; /* Minute */
		} else {
		    strncpy(err_message, "Wrong separator after hour!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    case 5: /* Parse minute */
		ret_tmt->t.tm_min = app;
		if(pEnd[0] == 0) {
		    flag_finished = 1;
		} else if(pEnd[0] == ':') {
		    state = 6; /* Second */
		} else {
		    strncpy(err_message, "Wrong separator after minute!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    case 6: /* Parse second */
		ret_tmt->t.tm_sec = app;
		if(pEnd[0] == 0) {
		    flag_finished = 1;
		} else if(pEnd[0] == '.') {
		    state = 7; /* ten thousandth of second */
		} else {
		    strncpy(err_message, "Error parsing after second!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    case 7: /* Parse ten thousandth of second */
		ret_tmt->d = app;
		if(pEnd[0] == 0) {
		    flag_finished = 1;
		} else {
		    strncpy(err_message, "Error parsing after ten thousandth of second!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		}
		break;

	    default : /* NOT DEFINED */
		snprintf(err_message, MAX_LENGTH_ERR_MESSAGE, "State %d not defined!", state);
		ret = -1;
		break;
	}
	if(pEnd[0] != 0  && !flag_finished  &&  ret == 0) {
	    pEnd[0] = ' '; /* overwrite separator with space */
	    if(state == 7) {
		pEnd++;
		str_tt[0] = '1';
		str_tt[1] = 0;
		if(pEnd[0] == 0 || strlen(pEnd) > 4) {
		    strncpy(err_message, "Error parsing ten thousandth of second!", MAX_LENGTH_ERR_MESSAGE);
		    ret = -1;
		} else {
		    strncat(str_tt, pEnd, 20 - strlen(str_tt));
		    k=0;
		    while(k<5) {
			if(str_tt[k] == 0) {
			    str_tt[k] = '0';
			}
			k++;
		    }
		    str_tt[k] = 0;
		    pEnd = str_tt;
		}
	    }
	    app = strtol(pEnd, &pEnd, 10);
	    if(state == 7) {
		    app -= 10000;
	    }
	    if(  errno == EINVAL ||  errno == ERANGE ) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_DATE, "%s\n", NMXP_LOG_STR(strerror(errno)));
		ret = -1;
	    }
	}
    }

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_DATE, "FINAL: state=%d value=%d flag_finished=%d ret=%d pEnd[0]=%c [%d]  (%s)\n",
	    state, app, flag_finished, ret, (pEnd[0]==0)? '_' : pEnd[0], pEnd[0], NMXP_LOG_STR(pEnd));

    if(!flag_finished && (ret == 0)) {
	strncpy(err_message, "Date incomplete!", MAX_LENGTH_ERR_MESSAGE);
	ret = -1;
    }

    if(ret == -1) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_DATE, "in date '%s' %s\n",
		NMXP_LOG_STR(pstr_date), NMXP_LOG_STR(err_message));
    } else {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_DATE, "Date '%s' has been validate! %04d/%02d/%02d %02d:%02d:%02d.%04d\n",
		NMXP_LOG_STR(pstr_date),
		ret_tmt->t.tm_year,
		ret_tmt->t.tm_mon,
		ret_tmt->t.tm_mday,
		ret_tmt->t.tm_hour,
		ret_tmt->t.tm_min,
		ret_tmt->t.tm_sec,
		ret_tmt->d
		);
    }

    return ret;
}


double nmxp_data_tm_to_time(NMXP_TM_T *tmt) {
    double ret_d = 0.0;
    
#ifdef HAVE_TIMEGM
    ret_d = timegm(&(tmt->t));
#else
    ret_d = my_timegm(&(tmt->t));
#endif

    ret_d += ((double) tmt->d / 10000.0 );

    return ret_d;
}


#ifdef HAVE_GETCWD
/* TODO */
#endif

char *nmxp_data_gnu_getcwd () {
    size_t size = NMXP_DATA_MAX_SIZE_FILENAME;
    while (1)
    {
	char *buffer = (char *) NMXP_MEM_MALLOC(size);
	if (getcwd (buffer, size) == buffer)
	    return buffer;
	NMXP_MEM_FREE (buffer);
	if (errno != ERANGE)
	    return NULL;
	size *= 2;
    }
}


int nmxp_data_dir_exists (char *dirname) {
    int ret = 0;
    char *cur_dir = NULL;

    if(dirname) {
	cur_dir = nmxp_data_gnu_getcwd();
	if(chdir(dirname) == -1) {
	    /* ERROR */
	} else {
	    ret = 1;
	}
	if(cur_dir) {
	    if(chdir(cur_dir) == -1) {
		/* ERROR */
	    }
	    NMXP_MEM_FREE(cur_dir);
	}
    }

    return ret;
}


char *nmxp_data_dir_abspath (char *dirname) {
    char *ret = NULL;
    char *cur_dir = NULL;

    if(dirname) {
	cur_dir = nmxp_data_gnu_getcwd();
	if(chdir(dirname) == -1) {
	    /* ERROR */
	} else {
	    ret = nmxp_data_gnu_getcwd();
	}
	if(cur_dir) {
	    if(chdir(cur_dir) == -1) {
		/* ERROR */
	    }
	    NMXP_MEM_FREE(cur_dir);
	}
    }

    return ret;
}

#ifdef HAVE_MKDIR
/* TODO */
#endif

#ifdef HAVE_WINDOWS_H
const char nmxp_data_sepdir = '\\';
#else
const char nmxp_data_sepdir = '/';
#endif


int nmxp_data_mkdir(const char *dirname) {
    int ret = 0;
#ifndef HAVE_WINDOWS_H
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
#endif
#ifndef HAVE_WINDOWS_H
    ret=mkdir(dirname, mode);
#else
    ret=mkdir(dirname);
#endif
    return ret;
}


int nmxp_data_mkdirp(const char *filename) {
    char *cur_dir = NULL;
    char *dir = NULL;
    int i, l;
    int	error=0;

    if(!filename)
	return -1;
    dir = NMXP_MEM_STRDUP(filename);
    if(!dir)
	return -1;

    cur_dir = nmxp_data_gnu_getcwd();

    l = strlen(dir);
    i = 0;
    while(i < l  &&  error != -1) {
	if(dir[i] == nmxp_data_sepdir  &&  i > 0) {
	    dir[i] = 0;
	    /* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "trying to create %s...\n", dir); */
	    if(chdir(dir) == -1) {
		error=nmxp_data_mkdir(dir);
	    }
	    dir[i] = nmxp_data_sepdir;
	}
	i++;
    }
    if(error != -1) {
	error=nmxp_data_mkdir(dir);
    }

    NMXP_MEM_FREE(dir);

    if(cur_dir) {
	if(chdir(cur_dir) == -1) {
	    /* ERROR */
	}
	NMXP_MEM_FREE(cur_dir);
    }
    return error;
}


#ifdef HAVE_LIBMSEED

int nmxp_data_seed_init(NMXP_DATA_SEED *data_seed, char *outdirseed, char *default_network, NMXP_DATA_SEED_TYPEWRITE type_writeseed) {
    int i;
    char *dirname = NULL;

    if(outdirseed) {
	dirname = nmxp_data_dir_abspath(outdirseed);
	strncpy(data_seed->outdirseed, dirname, NMXP_DATA_MAX_SIZE_FILENAME);
    } else {
	dirname = nmxp_data_gnu_getcwd();
	strncpy(data_seed->outdirseed, dirname, NMXP_DATA_MAX_SIZE_FILENAME);
    }
    if(dirname) {
	NMXP_MEM_FREE(dirname);
	dirname = NULL;
    }
    strncpy(data_seed->default_network, default_network, 5);
    data_seed->type_writeseed = type_writeseed;

    data_seed->n_open_files = 0;
    data_seed->last_open_file = -1;
    data_seed->cur_open_file = -1;
    data_seed->err_general = 0;
    for(i=0; i < NMXP_DATA_MAX_NUM_OPENED_FILE; i++) {
	data_seed->err_outfile_mseed[i] = 0;
	data_seed->outfile_mseed[i] = NULL;
	data_seed->filename_mseed[i][0] = 0;
    }

    data_seed->pmsr = NULL;

    return 0;
}


int nmxp_data_seed_fopen(NMXP_DATA_SEED *data_seed) {
    int i;
    int found;
    int err = 0;
    char dirseedchan[NMXP_DATA_MAX_SIZE_FILENAME];
    char filename_mseed[NMXP_DATA_MAX_SIZE_FILENAME];
    char filename_mseed_fullpath[NMXP_DATA_MAX_SIZE_FILENAME];

    filename_mseed[0] = 0;
    nmxp_data_get_filename_ms(data_seed, dirseedchan, filename_mseed);
    if(strlen(filename_mseed)) {
	found=0;
	i=0;
	while(i < data_seed->n_open_files  &&  !found) {
	    if( strcmp(filename_mseed, data_seed->filename_mseed[i]) == 0) {
		found = 1;
	    } else {
		i++;
	    }
	}

	if(found) {
	    data_seed->cur_open_file = i;
	    /* nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Curr  [%3d/%3d] %s\n", data_seed->cur_open_file, data_seed->n_open_files,
		    data_seed->filename_mseed[data_seed->cur_open_file]); */
	} else {

	    if(!nmxp_data_dir_exists(dirseedchan)) {
		/* nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Directory %s does not exist!\n", dirseedchan); */
		if(nmxp_data_mkdirp(dirseedchan) == -1) {
		    err++;
		    data_seed->err_general++;
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Directory %s has not been created!\n", dirseedchan);
		} else {
		    /* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "Directory %s created!\n", dirseedchan); */
		    if(!nmxp_data_dir_exists(dirseedchan)) {
			err++;
			data_seed->err_general++;
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Directory %s should be created but it does not exist!\n", dirseedchan);
		    }
		}
	    } else {
		/* nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "Directory %s exists!\n", dirseedchan); */
	    }

	    if(err==0) {
		int app;
		/* data_seed->last_open_file = (data_seed->last_open_file + 1) % NMXP_DATA_MAX_NUM_OPENED_FILE; */
		app = (data_seed->last_open_file + 1) % NMXP_DATA_MAX_NUM_OPENED_FILE;
		nmxp_data_seed_fclose(data_seed, app);
		strncpy(data_seed->filename_mseed[app], filename_mseed, NMXP_DATA_MAX_SIZE_FILENAME);
		snprintf(filename_mseed_fullpath, NMXP_DATA_MAX_SIZE_FILENAME, "%s%c%s",
			dirseedchan, nmxp_data_sepdir,
			data_seed->filename_mseed[app]);

		data_seed->outfile_mseed[app] = fopen(filename_mseed_fullpath, "a+");

		if(data_seed->outfile_mseed[app] == NULL) {
		    err++;
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Error opening file %s\n", filename_mseed_fullpath);
		} else {

		    if(data_seed->n_open_files < NMXP_DATA_MAX_NUM_OPENED_FILE) {
			data_seed->n_open_files++;
		    }

		    data_seed->last_open_file = app;
		    data_seed->cur_open_file = data_seed->last_open_file;
		}
	    }

	    /* nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Open  [%3d/%3d] %s\n", data_seed->cur_open_file, data_seed->n_open_files,
	       data_seed->filename_mseed[data_seed->cur_open_file]); */
	}

    }

    return err;
}


int nmxp_data_seed_fclose(NMXP_DATA_SEED *data_seed, int i) {

    if(i >= 0  &&  i < NMXP_DATA_MAX_NUM_OPENED_FILE) {
	if(data_seed->outfile_mseed[i]) {
	    /* nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "Close [%3d/%3d] %s\n", i, data_seed->n_open_files, data_seed->filename_mseed[i]); */
	    fclose(data_seed->outfile_mseed[i]);
	    data_seed->outfile_mseed[i] = NULL;
	    data_seed->filename_mseed[i][0] = 0;
	}
    }

    return 0;
}

int nmxp_data_seed_fclose_all(NMXP_DATA_SEED *data_seed) {
    int i;
    for(i=0; i < NMXP_DATA_MAX_NUM_OPENED_FILE; i++) {
	nmxp_data_seed_fclose(data_seed, i);
    }
    data_seed->n_open_files = 0;
    data_seed->last_open_file = -1;
    data_seed->cur_open_file = -1;
    return 0;
}

#define NMXP_DATA_NETCODE_OR_DEFAULT_NETWORK ( ( (msr) && (msr->network[0] != 0) )? msr->network : data_seed->default_network )
int nmxp_data_get_filename_ms(NMXP_DATA_SEED *data_seed, char *dirseedchan, char *filenameseed) {
    int ret = 0;
    MSRecord *msr = data_seed->pmsr;
    
    dirseedchan[0] = 0;
    filenameseed[0] = 0;
    if(data_seed->type_writeseed == NMXP_TYPE_WRITESEED_SDS) {
	snprintf(dirseedchan, NMXP_DATA_MAX_SIZE_FILENAME, "%s%c%d%c%s%c%s%c%s.D", data_seed->outdirseed, nmxp_data_sepdir,
		nmxp_data_year_from_epoch(MS_HPTIME2EPOCH(msr->starttime)),
		nmxp_data_sepdir,
		NMXP_DATA_NETCODE_OR_DEFAULT_NETWORK,
		nmxp_data_sepdir,
		msr->station,
		nmxp_data_sepdir,
		msr->channel);
	snprintf(filenameseed, NMXP_DATA_MAX_SIZE_FILENAME, "%s.%s.%s.%s.D.%d.%03d",
		NMXP_DATA_NETCODE_OR_DEFAULT_NETWORK,
		msr->station,
		(msr->location[0] == 0)? "" : msr->location,
		msr->channel,
		nmxp_data_year_from_epoch(MS_HPTIME2EPOCH(msr->starttime)),
		nmxp_data_yday_from_epoch(MS_HPTIME2EPOCH(msr->starttime)));
    } else if(data_seed->type_writeseed == NMXP_TYPE_WRITESEED_BUD) {
	snprintf(dirseedchan, NMXP_DATA_MAX_SIZE_FILENAME, "%s%c%s%c%s", data_seed->outdirseed,
		nmxp_data_sepdir,
		NMXP_DATA_NETCODE_OR_DEFAULT_NETWORK,
		nmxp_data_sepdir,
		msr->station);
	snprintf(filenameseed, NMXP_DATA_MAX_SIZE_FILENAME, "%s.%s.%s.%s.%d.%03d",
		msr->station,
		NMXP_DATA_NETCODE_OR_DEFAULT_NETWORK,
		(msr->location[0] == 0)? "" : msr->location,
		msr->channel,
		nmxp_data_year_from_epoch(MS_HPTIME2EPOCH(msr->starttime)),
		nmxp_data_yday_from_epoch(MS_HPTIME2EPOCH(msr->starttime)));
    }

    return ret;
}


/* Private function for writing mini-seed records */
static void nmxp_data_msr_write_handler (char *record, int reclen, void *pdata_seed) {
    int err = 0;
    NMXP_DATA_SEED *data_seed = pdata_seed;
    MSRecord *msr = data_seed->pmsr;

    if(msr == NULL) {
	err++;
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_EXTRA, "msr is NULL in nmxp_data_msr_write_handler()!\n");
    }

    if(err==0) {

	err = nmxp_data_seed_fopen(data_seed);

	if(err==0) {
	    if( data_seed->outfile_mseed[data_seed->cur_open_file] ) {
		if ( fwrite(record, reclen, 1, data_seed->outfile_mseed[data_seed->cur_open_file]) != 1 ) {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN,
			    "Error writing %s to output file\n", data_seed->filename_mseed[data_seed->cur_open_file]);
		}
	    }
	}
    }
}


int nmxp_data_msr_pack(NMXP_DATA_PROCESS *pd, NMXP_DATA_SEED *data_seed, void *pmsr) {
    int ret =0;

    MSRecord *msr = pmsr;
    int64_t psamples;
    int precords;
    flag verbose = 0;
    int i;
    int *newdatasamples = NULL;
    int *ptrdatasamples = NULL;
    double gap_overlap;
    double expected_next_time;
    char str_time1[200];
    char str_time2[200];

    /* Set pointer to the current miniseed record buffer */
    data_seed->pmsr = pmsr;

    /* Populate MSRecord values */
    /* SEED utilizes the Big Endian word order as its standard.
     * In 2003, the FDSN adopted the format rule that Steim1 and
     * Steim2 data records are to be written with the big-endian
     * encoding only. */
    msr->byteorder = 1;         /* big endian byte order */
    msr->sampletype = 'i';      /* declare type to be 32-bit integers */

    if(pd) {

	msr->dataquality = pd->quality_indicator;
	msr->samprate = pd->sampRate;
	/* msr->sequence_number = pd->seq_no % 1000000; */

	/* Set starttime,  datasamples and numsamples */
	if(msr->datasamples == NULL) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
		    "New datasamples for %s.%s.%s (%d)\n",
		    msr->network, msr->station, msr->channel, pd->nSamp);
	    msr->starttime = MS_EPOCH2HPTIME(pd->time);
	    msr->datasamples = NMXP_MEM_MALLOC (sizeof(int) * (pd->nSamp)); 
	    memcpy(msr->datasamples, pd->pDataPtr, sizeof(int) * pd->nSamp); /* pointer to 32-bit integer data samples */
	    msr->numsamples = pd->nSamp;

	} else {

	    expected_next_time = (double) MS_HPTIME2EPOCH(msr->starttime) + ( (double) msr->numsamples * ( 1.0 / (double) msr->samprate ) );
	    gap_overlap = pd->time - expected_next_time;

	    /* Check if data is contiguous */
	    if( fabs(gap_overlap) < fabs( 1.0 / (2.0 * (double) msr->samprate) ) ) {

		/* Add samples */
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
			"Add datasamples for %s.%s.%s (%d)\n",
			msr->network, msr->station, msr->channel, pd->nSamp);
		newdatasamples = NMXP_MEM_MALLOC (sizeof(int) * (msr->numsamples + pd->nSamp)); 
		memcpy(newdatasamples, msr->datasamples, sizeof(int) * msr->numsamples); /* pointer to 32-bit integer data samples */
		memcpy(newdatasamples + msr->numsamples, pd->pDataPtr, sizeof(int) * pd->nSamp); /* pointer to 32-bit integer data samples */
		msr->numsamples += pd->nSamp;
		NMXP_MEM_FREE(msr->datasamples);
		msr->datasamples = newdatasamples;
		newdatasamples = NULL;

	    } else {

		/* Gap or Overlap, then flush remaining samples and go on */

		nmxp_data_to_str(str_time1, expected_next_time);
		nmxp_data_to_str(str_time2, pd->time);

		nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY,
			"Gap %.2f sec. for %s.%s.%s from %s to %s saving mini-SEED records.\n",
			gap_overlap,
			msr->network, msr->station, msr->channel, NMXP_LOG_STR(str_time1), NMXP_LOG_STR(str_time2));

		/* Pack the record(s) flushing data */
		precords = msr_pack (msr, &nmxp_data_msr_write_handler, data_seed, &psamples, 1, verbose);

		if ( precords == -1 ) {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN,
			    "Cannot pack records %s.%s.%s\n", msr->network, msr->station, msr->channel);
		} else {
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
			    "Packed forced %d samples into %d records for %s.%s.%s\n",
			    psamples, precords, msr->network, msr->station, msr->channel);
		}

		if(msr->datasamples) {
		    NMXP_MEM_FREE(msr->datasamples);
		    msr->datasamples = NULL;
		}

		msr->starttime = MS_EPOCH2HPTIME(pd->time);
		msr->datasamples = NMXP_MEM_MALLOC (sizeof(int) * (pd->nSamp)); 
		memcpy(msr->datasamples, pd->pDataPtr, sizeof(int) * pd->nSamp); /* pointer to 32-bit integer data samples */
		msr->numsamples = pd->nSamp;

	    }

	}

	/* Pack the record(s) without flushing data if it is not necessary */
	precords = msr_pack (msr, &nmxp_data_msr_write_handler, data_seed, &psamples, 0, verbose);

	if ( precords == -1 ) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN,
		    "Cannot pack records %s.%s.%s\n", msr->network, msr->station, msr->channel);
	} else {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
		    "Packed %d samples into %d records for %s.%s.%s\n",
		    psamples, precords, msr->network, msr->station, msr->channel);

	    if(psamples > 0) {

		if(psamples == msr->numsamples) {
		    /* Remove all samples allocated */
		    NMXP_MEM_FREE(msr->datasamples);
		    msr->datasamples = NULL;
		    msr->numsamples = 0;
		} else if(psamples < msr->numsamples) {
		    /* Shift remaining samples not used yet */
		    ptrdatasamples = msr->datasamples;
		    for(i=0; i < msr->numsamples - psamples; i++) {
			ptrdatasamples[i] = ptrdatasamples[i + psamples];
		    }
		    ptrdatasamples = NULL;
		    msr->numsamples -= psamples;
		} else {
		    /* TODO impossible! */
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
			    "Impossible.\n");
		}

	    } else {
		/* Do nothing */
	    }

	}

    } else {

	/* Flush all remaining samples */
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
		"Flush all remaining samples for  %s.%s.%s\n", msr->network, msr->station, msr->channel);

	if(msr->datasamples) {

	    /* Pack the record(s) flushing data */
	    precords = msr_pack (msr, &nmxp_data_msr_write_handler, data_seed, &psamples, 1, verbose);

	    if ( precords == -1 ) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN,
			"Cannot pack records %s.%s.%s\n", msr->network, msr->station, msr->channel);
	    } else {
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN,
			"Packed forced %d samples into %d records for %s.%s.%s\n",
			psamples, precords, msr->network, msr->station, msr->channel);
	    }

	    NMXP_MEM_FREE(msr->datasamples);
	    msr->datasamples = NULL;
	    msr->numsamples = 0;

	}

    }

    /* Reset pointer to the current miniseed record buffer */
    data_seed->pmsr = NULL;

    return ret;
}

#endif



void nmxp_data_swap_2b (int16_t *in) {
    unsigned char *p = (unsigned char *)in;
    unsigned char tmp;
    tmp = *p;
    *p = *(p+1);    
    *(p+1) = tmp;
}   


void nmxp_data_swap_3b (unsigned char *in) {
    unsigned char *p = (unsigned char *)in;
    unsigned char tmp;
    tmp = *p;
    *p = *(p+2);    
    *(p+2) = tmp;
}   


void nmxp_data_swap_4b (int32_t *in) {
    unsigned char *p = (unsigned char *)in;
    unsigned char tmp;
    tmp = *p;
    *p = *(p+3);
    *(p+3) = tmp;
    tmp = *(p+1);
    *(p+1) = *(p+2);
    *(p+2) = tmp;
}


void nmxp_data_swap_8b (double *in) {
    unsigned char *p = (unsigned char *)in;
    unsigned char tmp;
    if(sizeof(double) != 8) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
		"nmxp_data_swap_8b() argument is not 8 bytes length!\n");
    }
    tmp = *p;
    *p = *(p+7);
    *(p+7) = tmp;
    tmp = *(p+1);
    *(p+1) = *(p+6);
    *(p+6) = tmp;
    tmp = *(p+2);
    *(p+2) = *(p+5);
    *(p+5) = tmp;
    tmp = *(p+3);
    *(p+3) = *(p+4);
    *(p+4) = tmp;
}


int nmxp_data_bigendianhost () {
    int16_t host = 1;
    return !(*((int8_t *)(&host)));
} 
