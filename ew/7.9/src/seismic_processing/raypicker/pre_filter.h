/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pre_filter.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: pre_filter.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.7  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.6  2004/07/13 19:23:02  cjbryan
 *     added RECURSIVE_FILTER struct and cleaned up filter code
 *
 *     Revision 1.5  2004/06/10 20:22:35  cjbryan
 *     re-engineered array initialization
 *
 *     Revision 1.4  2004/04/23 17:33:27  cjbryan
 *     changed bool to int
 *
 *     Revision 1.3  2004/04/21 20:30:19  cjbryan
 *     *** empty log message ***
 *
 *
 *
 */
 
/*
 * This file declares types and functions used to
 * handle broadband and short-period pre-filters.
 * And the shared processing to transform each into
 * a similar pseudo short-period time series.
 * 
 * @author Ray Buland (original FORTRAN)
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh
 */

#ifndef PRE_FILTER_H
#define PRE_FILTER_H

#include <limits.h>          /* LONG_MIN, LONG_MAX */
#include <time.h>            /* time_t  */
#include <ioc_filter.h>


/* CHANNEL_PRE_FILTER is persistence data that is used for
 *  pre-filtering a specific time-series channel. */
typedef struct _CHANNEL_PRE_FILTER
{
    double    mean;                           /* originally iirmn (channel mean)           */
    FILTER   *filter;                         /* pointer to shared filter for this channel */
} CHANNEL_PRE_FILTER;


/* function prototypes */
int InitTransferFn();
void FreeTransferFn();
int InitAllPreFilters(int maxPreFilters);
static int InitPreFilter(FILTER *filter);
static int SetupBroadbandFilter(const double sampleInterval, 
                                FILTER *filter, int debug);
static int SetupShortPeriodFilter(const double sampleInterval, 
                                  FILTER *filter, int debug);
int InitChannelPreFilter(const double sampleRate, const double mean, 
                         const int isBroadband, RECURSIVE_FILTER *channelData, 
                         int maxPreFilters, int debug); 
int PreFilterSamplePoint(const double sampleValue, 
                         RECURSIVE_FILTER *prefilter, double *filteredValue);
double PreFilterCompensate(const double filteredValue, const RECURSIVE_FILTER *prefilter);


#endif /* PRE_FILTER_H */
