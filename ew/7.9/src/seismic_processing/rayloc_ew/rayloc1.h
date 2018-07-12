/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 ***************************************************************************/

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc1.h 1669 2004-08-05 04:15:11Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2004/08/05 04:15:11  friberg
 *     First commit of rayloc_ew in EW-CENTRAL CVS
 *
 *     Revision 1.2  2004/08/04 19:27:54  ilya
 *     Towards version 1.0
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:07  ilya
 *     initial import into CVS
 *
 */

/***************************************************************************
                          rayloc1.h  -  description
                             -------------------
    begin                : Tue Jun 1 2004
    copyright            : (C) 2004 by Ilya Dricker, ISTI
    email                : i.dricker@isti.com
 ***************************************************************************/

 
#ifndef RAYLOC1_H
#define RAYLOC1_H

#define RAYLOC_MAX_PHASE_NAME_LEN 6

#define RAYLOC_SUCCESS 1
#define RAYLOC_FAILED -1
#define RAYLOC_TRUE 1
#define RAYLOC_FALSE 0
#define RAYLOC_MODEL_HEAD_NOT_EXIST -14101
#define RAYLOC_MODEL_TABLE_NOT_EXIST -14102
#define RAYLOC_TAU_TABLE_NOT_EXIST -14103
#define RAYLOC_NOT_DIRECTORY -14104
#define RAYLOC_DIR_NOT_WRITABLE -14105
#define RAYLOC_INPUT_NOT_EXIST -14106
#define RAYLOC_FAILED_TO_UNLINK -14107
#define RAYLOC_FAILED_TO_CREATE_INPUT -14108
#define RAYLOC_FAILED_TIME_CONVERTION -14109
#define RAYLOC_FAILED_TO_GET_FLAGS -14110
#define RAYLOC_UNUSED_PHASE -14111
#define RAYLOC_FAILED_TO_OPEN_FILE -14112
#define RAYLOC_ALLOCATION_FAILED -14113
#define RAYLOC_NOT_STATION_STRING -14114
#define RAYLOC_NO_MORE_STATION_SLOTS -14115
#define REAYLOC_FAILED_TO_FIND_STATION -14116
/*
 Flags:
    H      Hold latitude, longitude, depth, and origin time flag (T to constrain,
           F otherwise)
    F      Fix depth (T to constrain, F otherwise)
    K      Use PKP phases (T to use, F otherwise)
    D      Use depth phases (T to use, F otherwise)
    S      Use S phases (T to use, F otherwise)
  Other commands:
    R      Pick weight residual interval flag (T/F)
    Rmin   Lower bound of residual interval in which weights can be non-zero
    Rmax   Upper bound of residual interval in which weights can be non-zero
    D1-5   Pick weight distance interval flag(s) (T/F)
    Dmin1-5  Lower bound of distance interval(s) in which weights are zero
    Dmax2-5  Upper bound of distance interval(s) in which weights are zero
*/


typedef struct {
	short hold_params;             /*Hold latitude, longitude, depth, and origin time flag */
	short fix_depth;               /* Fix depth */
	short use_PKP;                 /* Use PKP phases */
	short use_depth_ph;            /* Use depth phases */
	short use_S_ph;                /*   Use S phases */
	short pick_weight_interval;    /*Pick weight residual interval flag (T/F)*/
	double Rmin;                   /* Lower bound of residual interval in which weights can be non-zero */
	double Rmax;                   /* Upper bound of residual interval in which weights can be non-zero */

	short D1;                      /* Pick weight distance interval flag(s) (T/F)*/
	double Dmin1;                  /* Lower bound of distance interval(s) in which weights are zero*/
	double Dmax1;                  /* */

	short D2;                      /* Pick weight distance interval flag(s) (T/F)*/
	double Dmin2;                  /* Lower bound of distance interval(s) in which weights are zero*/
	double Dmax2;                  /* Upper bound of distance interval(s) in which weights are zero*/

	short D3;                      /* Pick weight distance interval flag(s) (T/F)*/
	double Dmin3;                  /* Lower bound of distance interval(s) in which weights are zero*/
	double Dmax3;                  /* Upper bound of distance interval(s) in which weights are zero*/

	short D4;                      /* Pick weight distance interval flag(s) (T/F)*/
	double Dmin4;                  /* Lower bound of distance interval(s) in which weights are zero*/
	double Dmax4;                  /* Upper bound of distance interval(s) in which weights are zero*/

	short D5;                      /* Pick weight distance interval flag(s) (T/F)*/
	double Dmin5;                  /* Lower bound of distance interval(s) in which weights are zero*/
	double Dmax5;                  /* Upper bound of distance interval(s) in which weights are zero*/
} RAYLOC_PROC_FLAGS;


typedef struct {
	double lat;
	double lon;
	double elev;
	char net[4];
	char sta[7];
	long long base36_net;
	long long base36_sta;	
} RAYLOC_ONE_STATION;

typedef struct {
	int num;
	int max_num;
	int use_36_base_digest;
	RAYLOC_ONE_STATION *sta[5000];
} RAYLOC_STATIONS;

typedef struct {
	int num;
	int max_num;
	char phases[100][RAYLOC_MAX_PHASE_NAME_LEN + 1];
} RAYLOC_PHASES;


int
	lib_rayloc(const char *in_message,
	           int in_mesLen,
	           RAYLOC_PHASES *unused_list,
	           RAYLOC_PROC_FLAGS *flags,
	           const char *token,
	           const char *dirname,
	           unsigned char headerType,
	           unsigned char headerMod,
	           unsigned char headerInst,
	           unsigned char picksType,
	           unsigned char picksMod,
	           unsigned char picksInst,
	           char **out_message,
	           char **err_message,
	           RAYLOC_STATIONS *stns);


int
	lib_rayloc_stub(const char *token, const char *dirname);

int
	file2string(const char *fname, char **str);

int rayloc_epochsec17( double *sec, char *tstr );




RAYLOC_PHASES *
	rayloc_new_phase_list(void);

int
	rayloc_add_phase(RAYLOC_PHASES *phase_list, const char *phase);

void
	rayloc_destroy_phase_list(RAYLOC_PHASES *phase_list);

int
	rayloc_if_in_the_phase_list(RAYLOC_PHASES *phase_list, const char *phase);


RAYLOC_STATIONS *
	rayloc_stations_from_file(const char *fname, int *errNO, int *errNum);

void
	rayloc_destroy_stations(RAYLOC_STATIONS *stations);

int
	rayloc_if_in_the_station_list(RAYLOC_STATIONS *stations, const char *net, const char *sta);

double
	rayloc_get_station_lat(RAYLOC_STATIONS *stations, int num_in_list);

double
	rayloc_get_station_lon(RAYLOC_STATIONS *stations, int num_in_list);


double
	rayloc_get_station_elev(RAYLOC_STATIONS *stations, int num_in_list);



#endif
