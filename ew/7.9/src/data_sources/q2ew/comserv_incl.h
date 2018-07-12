#ifndef _COMSERV_INCL_H
#define _COMSERV_INCL_H
#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"


#define PTR_STA_CLIENT	pclient_station
#define PTR_CLIENT	pclient_struc
#define PTR_DATA	pdata_user
#define STATIONS	tstations_struc

typedef char CHAR23[24] ;

/* some pointer type defs to make things easier */
typedef seed_fixed_data_record_header 	*PTR_SEED_FSDH_HDR;
typedef data_only_blockette 		*PTR_DATA_ONLY_BLOCKETTE;
typedef data_extension_blockette 	*PTR_DATA_EXT_BLOCKETTE;
typedef timing 				*PTR_TIMING_BLOCK;
typedef murdock_detect 			*PTR_MURDOCK_DETECT_BLOCK;
typedef threshold_detect 		*PTR_THREASHOLD_DETECT_BLOCK;
typedef cal2 				*PTR_CAL2_BLOCK;
typedef step_calibration 		*PTR_STEP_CAL_BLOCK;
typedef sine_calibration 		*PTR_SINE_CAL_BLOCK;
typedef random_calibration 		*PTR_RAND_CAL_BLOCK;
typedef opaque_hdr 			*PTR_OPAQUE_HDR;


/* some function COMSERV defs */
pchar seednamestring (seed_name_type *, location_type *);

/* some function defs for this code base */
int handle_cs_status (int status, long station);

#endif
