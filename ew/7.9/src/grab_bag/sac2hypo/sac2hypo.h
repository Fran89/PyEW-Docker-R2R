
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sac2hypo.h,v 1.1.1.1 2005/07/14 20:06:46 paulf Exp $
 *
 *    Revision history:
 *     $Log: sac2hypo.h,v $
 *     Revision 1.1.1.1  2005/07/14 20:06:46  paulf
 *     Local ISTI CVS copy of EW v6.3
 *
 *     Revision 1.1  2000/02/14 19:13:13  lucky
 *     Initial revision
 *
 *
 */

/*
 *   sac2hypo.h - define structs used by sac2hypo
 */
  

#ifndef SAC2HYPO_H      /*Only include this file once */
# define SAC2HYPO_H

typedef struct _ArrivalStruct
{    
	unsigned long pkid;
	unsigned long linkid;
	unsigned long originID;
	unsigned long SCNID;
	char sta[7];
	char chan[9];
	char net[9];
	double  t;         /* arrival-time as sec since 1970     */
	char    fm;
	short   qual;
	long    pamp;
	long    caav[6];
	long    ccntr[6];
	long    codalen;
	char    datasrc;
	char    ph[3];      /* phase remark                   */
	float   res;        /* travel-time residual (sec)     */
	float   dist;       /* epicentral distance (km)       */
	short   azm;        /* azimuth                        */
	short   takeoff;    /* emergence angle at source      */
	float   wt;         /* actual weight used in location */
	short   xtpcoda;    /* extrapolated coda duration     */
	float   Md;         /* duration mag from this station */
	short   Mdwt;       /* weight of duration mag         */
} ArrivalStruct;



typedef struct _OriginStruct
{
/* Structure of info read from database GetPreferedOrig() call
 **************************************************/
	unsigned long originID;
  unsigned long eventID;
	unsigned long externalID; /* event id from binder or CarlTrig */
	char author[50];      /*  author of the origin     */
	double  ot;           /* origin time as sec since 1970   */
	float   lat;          /* latitude (North=positive)       */
	float   lon;          /* longitude(East=positive)        */
	float   z;            /* depth (down=positive)           */
	short   nph;          /* number of phases w/ weight >0.1 */
	short   gap;          /* maximum azimuthal gap           */
	int     dmin;         /* distance (km) to nearest station*/
	float   rms;          /* RMS travel time residual        */
	short   e2az;         /* azimuth of largest principal error */
   	short   e2dp;         /* dip of largest principal error     */
	float   e2;           /* magnitude (km) of largest principal error */ 
	short   e1az;         /* azimuth of intermediate principal error */
   	short   e1dp;         /* dip of intermediate principal error  */
	float   e1;           /* magnitude (km) of intermed principal error */ 
	float   e0;           /* magnitude (km) of smallest principal error */ 
	float   erh;          /* horizontal error (km) */
	float   erz;          /* vertical error (km) */
	float   Md;           /* duration magnitude */
  int     loadDate;     /* date origin was loaded into DB */
  char velModel[3];     /* velocity model used to find origin */ 
  char comment[255];     /* added by DK 4/26/98 */
} OriginStruct;


#endif /*SAC2HYPO_H*/
