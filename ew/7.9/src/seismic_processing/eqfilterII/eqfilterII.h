
/* glfilter's include file */

/* added tOrigin to reflect changes in global_loc_rw, withers 20051205 */

/* Region structs
 *******************/
 
#define     MAX_SIDES                           20
#define     MAX_REGIONS_PER_INST     5   
#define     MAX_INST                            20
/*  maximium number of relationships for Ncoda test */
/*  no more than NCODATEST relations per inst_id */
#define MAX_NCODATEST 10
 
typedef struct _region
{
 
    int     num_sides;
    float   x[MAX_SIDES + 1];
    float   y[MAX_SIDES + 1];
 
} REGION;
 
typedef struct _authreg
{
 
        unsigned char           instid;
        int                                     numIncReg;
        REGION                          IncRegion[MAX_REGIONS_PER_INST];
        int                                     numExcReg;
        REGION                          ExcRegion[MAX_REGIONS_PER_INST];
 
} AUTHREG;
 
 
static          AUTHREG         AuthReg[MAX_INST];
static          int                     numInst;

typedef struct _eqsum
{
   char                   origin_time_char[22];
   double                 tOrigin;
   double                 elat;
   double                 elon;
   double                 edepth;
   double                 gap;
   int                    nsta;
   int                    npha;
   int                    nphtot;
   double                 gdmin;
   double                 dmin;
   double                 deperr;
   double                 rms;
   double                 e0;
   double                 errh;
   double                 errz;
   double                 avh;
   double                 Md;
   int                    ncoda;
} EQSUM;

#define EQSUM_NULL -12345

typedef struct _partest2
{
  unsigned char instid;
  float var1;
  float var2;
} PARTEST2;

typedef struct _partest1
{
  unsigned char instid;
  float var;
} PARTEST1;

static PARTEST2 DepthTest[MAX_INST];
static int NDepthTest;

static PARTEST1 nphTest[MAX_INST];
static int NnphTest;

static PARTEST1 nphtotTest[MAX_INST];
static int NnphtotTest;

static PARTEST1 nstaTest[MAX_INST];
static int NnstaTest;

static PARTEST1 GapTest[MAX_INST];
static int NGapTest;

/* dmin in degrees for global_loc rayloc */
static PARTEST1 GDminTest[MAX_INST];
static int NGDminTest;

/* dmin in km for hyp2000 */
static PARTEST1 DminTest[MAX_INST];
static int NDminTest;

static PARTEST1 RMSTest[MAX_INST];
static int NRMSTest;

static PARTEST1 MaxE0Test[MAX_INST];
static int NMaxE0Test;

static PARTEST1 MaxERHTest[MAX_INST];
static int NMaxERHTest;

static PARTEST1 MaxERZTest[MAX_INST];
static int NMaxERZTest;

static PARTEST1 MaxAVHTest[MAX_INST];
static int NMaxAVHTest;

static PARTEST1 MinMagTest[MAX_INST];
static int NMinMagTest;

static PARTEST2 NcodaTest[MAX_INST];
static int NNcodaTest;

