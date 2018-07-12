
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: eqfilter.h 6447 2015-11-12 19:55:48Z scott $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2002/12/10 19:12:59  dietz
 *     Added new test on total number of phases with weight > 0.0
 *
 *     Revision 1.1  2000/02/14 17:08:44  lucky
 *     Initial revision
 *
 *
 */

/* eqfilter's include file */

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

typedef struct _partest3
{
  unsigned char instid;
  float var1;
  float var2;
  float var3;
} PARTEST3;

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

static PARTEST1 GapTest[MAX_INST];
static int NGapTest;

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

static PARTEST1 MinMagTest[MAX_INST];
static int NMinMagTest;

static PARTEST3 *MaxDistTest;
static int NMaxDistTest;
static int NAllocMaxDistTest;
static int MaxOriginAgeSecs;

/*
typedef struct _ncodatest
{
  unsigned char instid;
  int num_rel;
  int NCoda[MAX_NCODATEST];
  float Mag[MAX_NCODATEST];
} NCODATEST;

static NCODATEST NcodaTest[MAX_INST];
static int NNcodaTest;
*/
static PARTEST2 NcodaTest[MAX_INST];
static int NNcodaTest;

