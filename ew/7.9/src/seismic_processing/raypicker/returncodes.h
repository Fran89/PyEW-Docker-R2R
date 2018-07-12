/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: returncodes.h 4453 2011-07-27 21:10:40Z stefan $
 *
 *    Revision history:
 *     $Log: returncodes.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2005/02/28 20:34:55  davidk
 *     Added error code for NO_DATA( raypicker hasn't seen tracebuf packets for 30 seconds.
 *
 *     Revision 1.2  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */

/*
 * Return codes used throughout the raypicker code.
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */
 
#ifndef RETURN_CODES_H
#define RETURN_CODES_H

/* Error messages used by raypicker 
 **********************************
 *
 * Some functions may return positive numbers, so error codes should
 * be negative. */
#define RAYPICK_SUCCESS      400
#define RAYPICK_FAILURE      401
#define RAYPICK_BADPARAM     402   /* parameter address passed in NULL           */
#define RAYPICK_MEMALLOC     403   /* memory allocation error                    */
#define	RAYPICK_BADCHANNEL   404   /* channel sampling rate not allowed          */
 
#define ERR_MISSMSG          410   /* message missed in transport ring           */
#define ERR_TOOBIG           411   /* retreived msg too large for buffer         */
#define ERR_NOTRACK          412   /* msg retreived; tracking limit exceeded     */
#define ERR_MISS_LAPPED      413   /* msg retrieved, but some overwritten before 
                                    * processed                                  */
#define ERR_MISS_SEQGAP      414   /* msg retrieved, but gap in sequence #s      */
#define ERR_QUEUE            415   /* trouble with the MsgQueue operation        */
#define ERR_NEXTAMPPOINT     416   /* trouble with NextAmpPoint                  */
#define ERR_SAMPRATETOOBIG   417   /* sampling rate larger than configured max   */
#define	ERR_SAMPRATENOMATCH  418   /* sampling rate does not match previous rate 
                                    * for this channel                           */
#define ERR_DATATYPE         419   /* unknown data type                          */
#define	ERR_UNKNOWN          420   /* unknown error                              */
#define ERR_NODATA           421   /* no input waveform data in last 30 secs     */


#endif  /*  RETURN_CODES_H  */
