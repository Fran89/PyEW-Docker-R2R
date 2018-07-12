
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *
 */

/* 
 * pdl2ew.h:
 *
 * SBH August2015
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
void pdl2ew_status( unsigned char, short, char * );

/* Layout of HypoARC message (important bits)
 *********************************************/
typedef struct _ARCDATA {
    char originTime[16];    // YYYYMMDDHHMMSSSS
    char latitude[7];       // DDaMMmm
    char longitude[8];      // DDDaMMmm
    char depth[5];          // DDDdd
    char pad0[3];
    char numPhases[3];      // III
    char azimuthalGap[3];   // III
    char minDist[3];        // DDD
    char pad1[22];
    char Md[3];             // Mmm
    char pad2[12];
    char horzError[4];      // DDdd
    char vertError[4];      // DDdd
    char pad3[29];
    char magCode[1];        // C
    char pad4[13];
    char eventID[10];       // CCCCCCCCCC
    char pad5[1];
    char Mpref[3];           // Ddd
    char pad6[11];
    char version[1];        // C
    char pad_eventid[15];
    char break1;            // newline
    char pad7[10];
    char break2;            // newline
    char term;            // newline
} ARCDATA;






