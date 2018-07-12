
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: server_thread.h 5705 2013-08-05 00:48:39Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2005/04/21 22:52:36  davidk
 *     Updated code to support SCNL menu protocol adjustment (v5.1.24).
 *     Modified code to handle menu requests from SCN and SCNL clients differently.
 *     The server only deals(well) with SCNL clients.  SCN clients are handed an
 *     empty menu and sent on their way.
 *     SCNL clients are handed a proper menu.
 *     The two client types are differentiated by an "SCNL" at the end of the menu
 *     request.
 *     Updated the version timestamp.
 *
 *     Revision 1.3  2004/05/18 22:30:19  lombard
 *     Modified for location code
 *
 *     Revision 1.2  2001/01/18 02:27:53  davidk
 *     Changed ERR_XXX constants defined in server_thread.h
 *     to RET_ERR_XXX because they were overlapping with the
 *     ERR_XXX constants in wave_serverV.c/wave_serverV.h
 *     which are used for issuing status messages to statmgr.
 *     Commented out unused ERR_XXX constants.
 *
 *     Revision 1.1  2000/02/14 19:58:27  lucky
 *     Initial revision
 *
 *
 */

/*
 *
 *	server_thread.h: This is the interface file for the
 *                       wave_serverV main server thread.
 *
 */

#ifndef _SERVER_THREAD_
#define _SERVER_THREAD_

#include <trace_buf.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif /* !TRUE */

#define R_FAIL  (-2)
#define R_SKIP    1
#define R_DONE    0

#define RET_ERR_SOCKET    -3       /* error with socket connect, send, recv */
#define RET_ERR_NODATA    -5       /* data requested is not in tank         */

#define kMAX_CLIENT_MSG_LEN     255     /* Largest size that a client request
					 * message may be before overflowing
					 * rcv buffer: */
#define kMAX_CLIENT_FIELD_LEN   64      /* Largest size that a field in a
					 * client request message may be
					 * before overflowing field buffer: */
#define kMAX_CLIENT_FIELDS      9       /* Largest number of fields present in
					 * a client request message in order
					 * for it to be parsed correctly: */
#define kBLOCK_SIZE             1024    /* Number of samples to be written at
					 * one time when writing trace data
					 * out to the client: */

/* Describes the format of data being handled: */
typedef enum
{
    kI2,                    /*  ..INTEL 2-byte integer. */
    kI4,                    /*  ..INTEL 4-byte integer. */
    kS2,                    /*  ..SPARC 2-byte integer. */
    kS4                     /*  ..SPARC 4-byte integer. */
} DATATYPE;

/* Different types of requests from a client that we can recognise: */
typedef enum
    {
	kMSG_UNKNOWN,
	kMENU,
  kSCNMENU,
	kMENUPIN,
	kMENUSCNL,
	kGETPIN,
	kGETSCNL,
	kGETSCNLRAW
    } CLIENT_MSG_TYPE;

#define kMENU_TOKEN       "MENU:"
#define kMENU_SCNL_TOKEN  "SCNL"
#define kMENUPIN_TOKEN    "MENUPIN:"
#define kMENUSCNL_TOKEN   "MENUSCNL:"
#define kGETPIN_TOKEN     "GETPIN:"
#define kGETSCNL_TOKEN    "GETSCNL:"
#define kGETSCNLRAW_TOKEN "GETSCNLRAW:"

/* This record is filled in by ParseMsg(). It contains the data derived from
 * the incoming socket stream broken up into fields and converted to the
 * correct data type for easier access.  */
#define kREQID_LEN      12
#define kFILLVALUE_LEN  30

typedef struct
{
    CLIENT_MSG_TYPE         fMsg;
    char    reqId[kREQID_LEN];
    int     pin;
    char    site[TRACE2_STA_LEN];
    char    channel[TRACE2_CHAN_LEN];
    char    network[TRACE2_NET_LEN];
    char    location[TRACE2_LOC_LEN];
    double  starttime;
    double  endtime;
    char    fillvalue[kFILLVALUE_LEN];
} CLIENT_MSG;

#endif /* _SERVER_THREAD_ */

