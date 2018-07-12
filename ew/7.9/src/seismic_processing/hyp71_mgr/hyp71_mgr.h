#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <earthworm.h>
#include <kom.h>
#include <transport.h>
#include <site.h>
#include <read_arc.h>
#include <chron3.h>
#include <math.h>
#include <trace_buf.h>
#include <errno.h>

/* Some limits for tables ans strings.
 *************************************/

#define MAXLAY		20
#define MAXTESTS	15
#define MAXTRIAL	15
#define MAX_STR		255
#define SUMCARDLEN	200
#define MAX_BYTES_PER_PUN 10000

/* Error to send to statmgr.  Errors 100-149 are reserved for hyp71_mgr.
  (must not overlap with error numbers of other links in the sausage.)
 ************************************************************************/
#define ERR_TOOBIG        101   /* Got a msg from pipe too big for target  */
#define ERR_TMPFILE       102   /* Error writing hypoinverse input file    */
#define ERR_ARCFILE       103   /* Error reading hypoinverse archive file  */
#define ERR_ARC2RING      104   /* Trouble putting arcmsg in memory ring   */
#define ERR_SUM2RING      105   /* Trouble putting summsg in memory ring   */
#define ERR_MSG2RING      106   /* Trouble passing a msg from pipe to ring */
#define ERR_PIPEPUT       107   /* Trouble writing to pipe                 */

/* Define several variables and tables.
 **************************************/

static int nTests = 0;
static double Test_num[MAXTESTS];
static double Test_val[MAXTESTS];
static int nLay = 0;
static double zTop[2*MAXLAY];
static double vLay[2*MAXLAY];
static double PS_ratio = 1.78;
static double Xnear = 200;
static double Xfar = 450;
static int nTrialDepth = 0;
static double TrialDepth[MAXTRIAL];
static int UseLocalmag = 0;		/* default : don't use localmag	*/
static char LocalmagFile[MAX_STR];
static char MagOutputFile[MAX_STR];
static int archive_files = 0;		/* default : don't archive files */
static char ArchiveDir[MAX_STR];
static int TestMode = 0;		/* default : don't use test mode */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key to pick transport ring            */
static unsigned char InstId;        /* local installation id                 */
static unsigned char TypeError;
static unsigned char TypeKill;
static unsigned char TypeHyp2000Arc;
static unsigned char TypeH71Sum2K;

/* Structures for hyp2000 data handling.
 ***************************************/

struct Hsum Sum;               /* Hyp2000 summary data                   */
struct Hpck Pick;              /* Hyp2000 pick structure                 */

/* These strings contain file names needed for Hypo71.
 *****************************************************/

static char H71PRT[20];         /*  */
static char H71PUN[20];         /*  */
static char arcIn[20];          /* Archive file sent to hypo_ew            */
static char arcOut[20];         /* Archive file output by hypo_ew          */
static char sumName[20];        /* Summary file output by hypo_ew          */

/* Define structures used to store pick, coda, hypocenter info
 *************************************************************/
/* Structures for hyp2000 data */
struct Hsum Sum;               /* Hyp2000 summary data                   */
struct Hpck Pick;              /* Hyp2000 pick structure                 */

/* Define some simple "functions":
   *******************************/
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))        /* Larger value   */
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))        /* Smaller value  */
#endif
#define ABS(a) ((a) > 0 ? (a) : -(a))           /* Absolute value */

#define MAX_BYTES_PER_PUN 10000
#define ERR_TMPFILE       102   /* Error writing hypoinverse input file    */
#define MAX_STR                   255


int  hypoarc_2_hypo71inp( char * , char * , double *, double *, char *);
char *hypo71pun_2_hypoarc( char * , char * , char * );
int search_depth( char * , double *, double *, double *);
int check_depth( char * , double);
void run_hypo71( char * , char * , char * , char * , char * , char *);


/* found in hyp71_mgr.c  */
void ReportError( int );
