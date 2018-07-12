/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: pick_params.h 6114 2014-06-11 17:36:04Z baker $
 *
 *    Revision history:
 *     $Log: pick_params.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.2  2005/12/29 23:02:28  cjbryan
 *     revised comments
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:49  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 */
/*
 * The FORTRAN picker, from which raypicker was derived,
 * had two fixed types of 'filters', which are used to create
 * the STA series for each channel.  There is code that is
 * specific to each filter.
 * 
 * As requested, these filters are now called picker series.
 * Essentially, all of of the picking-related activities and
 * data which much be persisted between message arrivals are
 * managed by these pickers.
 * 
 * One goal of the port was to make the code entirely generic,
 * and allow for simpler alterations or addition to the picker
 * series types.
 * 
 * ALL PICK SERIES-SPECIFIC PARAMETERS ARE DECLARED HEREIN.
 * 
 * This current file will support up to eight of these pickers 
 * per channel -- which is a result of the bit-width of the type
 * PICKER_AGGR_STATE (typedef below).  If the number of pickers
 * is to change, then most of the other items in this file must
 * also change -- but that's the extent of it.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 : August 2003, dbh
 */
 
#ifndef PICK_PARAMS_H
#define PICK_PARAMS_H

/*
 * CAUTION: There must be a constistency among
 *          - PICKER_TYPE_COUNT
 *          - PICKER_PARAM_DATA's first element size (PICKER_PARAM_DATA defined in info_channel.c)
 *          - the number of bits in PICKER_AGGR_STATE
 *          - the number of elements in PICKER_ON (defined in info_channel.c)
 */
 
#define PICKER_TYPE_COUNT  2

#define PICKER_ANNOINTED_TYPE 0 /* the picker series type that is the most desired 
                                 * for determining the triggering status.  If the
                                 * annointed one has been confirmed, then the trigger 
								 * will not shut down until the annointed one shuts down
								 * regardless of the  states of any other pickers. */

/* indices into PICKER_PARAM_DATA[][] */
#define PICKER_CORNER_HI   0
#define PICKER_CORNER_LO   1
#define PICKER_CONFFACT    2    /* confirm duration calc. factor, formerly BFAC */
#define PICKER_DURATIONLO  3
#define PICKER_DURATIONHI  4
#define PICKER_SENSITIVITY 5    /* formerly TRGLM */
#define PICKER_VALUECOUNT  6    /* next highest value, used for array size declarations */

typedef unsigned char PICKER_AGGR_STATE; /* WARNING: THIS VARIABLE TYPE MUST CONTAIN AT LEAST AS
                                          * MANY BITS AS THERE ARE PICKERS IN A CHANNEL. i.e.,
                                          * AT LEAST PICKER_TYPE_COUNT BITS. */

#endif /* PICK_PARAMS_H */
