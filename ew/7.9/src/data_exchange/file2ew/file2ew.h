
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: file2ew.h 5453 2013-05-09 06:56:48Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.4  2002/12/06 00:37:26  dietz
 *     Added support for new optional instid argument of SuffixType cmd
 *
 *     Revision 1.3  2002/06/05 16:19:44  lucky
 *     I don't remember
 *
 *     Revision 1.2  2001/03/27 01:13:06  dietz
 *     Added support for reading heartbeat file contents.
 *
 *     Revision 1.1  2001/02/08 16:36:02  dietz
 *     Initial revision
 *
 *     Revision 1.2  2000/09/26 22:59:05  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 19:19:05  lucky
 *     Initial revision
 *
 *
 */

/* 
 * file2ew.h:
 *
 * LDD February2001
 */

/* String length definitions
 ***************************/
#define GRP_LEN 25            /* length of page group name              */
#define NAM_LEN 100	      /* length of full directory name          */
#define TEXT_LEN NAM_LEN*3
#define SM_BOX_LEN       25   /* maximum length of a box name */

/* Error messages used by file2ew 
 ***********************************/
#define ERR_CONVERT       0   /* trouble converting a file  */
#define ERR_PEER_LOST     1   /* no peer heartbeat file for a while */
#define ERR_PEER_ALIVE    2   /* got a peer heartbeat file again    */
#define ERR_PEER_HBEAT    3   /* contents of heartbeat file look weird */

/* Function prototypes
 *********************/
int  file2ew_ship( unsigned char type, unsigned char iid, 
                   char *msg, size_t msglen );
void file2ew_status( unsigned char, short, char * );
int  file2ewfilter_com( void );
int  file2ewfilter_init( void );
int  file2ewfilter_hbeat( FILE *fp, char *fname, char *sysname );
int  file2ewfilter( FILE *fp, char *fname );
void file2ewfilter_shutdown( void );




