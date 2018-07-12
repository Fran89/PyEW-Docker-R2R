/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: st_timepar.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2008/09/26 22:25:00  kress
 *     Fix numerous compile warnings and some tab-related fortran errors for linux compile
 *
 *     Revision 1.2  2006/11/29 21:21:56  stefan
 *     added missing type specifier for timparse and parstart
 *
 *     Revision 1.1  2000/03/13 23:48:35  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include "st_parsetime.h"

int temp=0,temp2=0;
char *valback;

#define timwrap() parwrap()
# define WHITE 257
# define AM 258
# define PM 259
# define NUMBER 260
# define JANUARY 261
# define FEBRUARY 262
# define MARCH 263
# define APRIL 264
# define MAY 265
# define JUNE 266
# define JULY 267
# define AUGUST 268
# define SEPTEMBER 269
# define OCTOBER 270
# define NOVEMBER 271
# define DECEMBER 272
#define timclearin timchar = -1
#define timerrok timerrflag = 0
extern int timchar;
extern short timerrflag;
#ifndef TIMMAXDEPTH
#define TIMMAXDEPTH 150
#endif
#ifndef TIMSTYPE
#define TIMSTYPE int
#endif
TIMSTYPE timlval, timval;
# define TIMERRCODE 256



# define U(x) x
# define NLSTATE timprevious=TIMNEWLINE
# define BEGIN timbgin = timsvec + 1 +
# define INITIAL 0
# define TIMLERR timsvec
# define TIMSTATE (timestate-timsvec-1)
# define TIMOPTIM 1
# define TIMLMAX 200
# define timmore() (timmorfg=1)
# define output(c) putc(c,stderr)
# define ECHO fprintf(stderr, "%s",timtext)
/* # define REJECT { nstr = timreject(); goto timfussy;} */
int timleng; extern char timtext[];
int timmorfg;
extern char *timsptr, timsbuf[];
int timtchar;
extern int timlineno;
struct timsvf {
	struct timwork *timstoff;
	struct timsvf *timother;
	int *timstops;};
struct timsvf *timestate;
extern struct timsvf timsvec[], *timbgin;
# define TIMNEWLINE 10

int timlook(void);
int parwrap(void);
int timback(int *p, int m);

int timlex(void) {
  int nstr; extern int timprevious;
  while((nstr = timlook()) >= 0)
  /* timfussy: */
    switch(nstr){
    case 0:
      if(timwrap()) return(0); break;
    case 1:
      { return(0); }
      break;
    case 2:
      { valback = timtext;
	return(NUMBER); }
      break;
    case 3:
      { return(WHITE); }
      break;
    case 4:
    case 5:
      { timlval=1; return(JANUARY); }
      break;
    case 6:
    case 7:
      { timlval=2; return(FEBRUARY); }
      break;
    case 8:
    case 9:
      { timlval=3; return(MARCH); }
      break;
    case 10:
    case 11:
      { timlval=4; return(APRIL); }
      break;
    case 12:
      { timlval=5; return(MAY); }
      break;
    case 13:
    case 14:
      { timlval=6; return(JUNE); }
      break;
    case 15:
    case 16:
      { timlval=7; return(JULY); }
      break;
    case 17:
    case 18:
      { timlval=8; return(AUGUST); }
      break;
    case 19:
    case 20:
      { timlval=9; return(SEPTEMBER); }
      break;
    case 21:
    case 22:
      { timlval=10; return(OCTOBER); }
      break;
    case 23:
    case 24:
      { timlval=11; return(NOVEMBER); }
      break;
    case 25:
    case 26:
      { timlval=12; return(DECEMBER); }
      break;
    case 27:
      { return(timtext[0]); }
      break;
    case -1:
      break;
    default:
      fprintf(stderr,"bad switch timlook %d",nstr);
    } return(0); }
/* end of timlex */

#define input() pickstr()
#define unput(a) unpicks(a)

char *indats,lbkstr[100];
int sofar,somax,curlbk,streof;

void parstart(char *string, int stomax)
{

	indats=string;
	sofar=0;
	somax = stomax;
	curlbk =0;
	streof=0;
}

int parwrap(void)
{
	return(streof);
}

char pickstr(void)
{
	if (curlbk>0) return(lbkstr[--curlbk]);
	if (indats[sofar]=='\n'||indats[sofar]=='\0'||sofar>=somax) {
		streof=1;
		return(0);
	}

	return(indats[sofar++]);
}

void unpicks(char a)
{
	if (curlbk>=100) printf("ungetted over buffer");
	lbkstr[curlbk++] = a;
	return;
}


int timvstop[] ={
0,

27,
0,

3,
27,
0,

3,
0,

2,
27,
0,

27,
0,

27,
0,

27,
0,

27,
0,

27,
0,

27,
0,

27,
0,

27,
0,

2,
0,

10,
0,

17,
0,

25,
0,

6,
0,

4,
0,

15,
0,

13,
0,

8,
0,

12,
0,

23,
0,

21,
0,

19,
0,

16,
0,

14,
0,

11,
0,

9,
0,

18,
0,

5,
0,

22,
0,

26,
0,

7,
0,

24,
0,

20,
0,
0};
# define TIMTYPE char
struct timwork { TIMTYPE verify, advance; } timcrank[] ={
0,0,	0,0,	1,3,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	1,4,	4,5,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	1,5,	4,5,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	4,5,	0,0,
0,0,	1,6,	6,15,	6,15,
6,15,	6,15,	6,15,	6,15,
6,15,	6,15,	6,15,	6,15,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	0,0,	0,0,
0,0,	0,0,	1,7,	11,22,
19,29,	1,8,	10,20,	1,9,
8,18,	9,19,	13,24,	1,10,
14,25,	17,27,	1,11,	1,12,
1,13,	12,23,	2,7,	7,16,
1,14,	2,8,	16,26,	2,9,
7,17,	18,28,	10,21,	2,10,
20,30,	23,35,	2,11,	2,12,
2,13,	21,31,	22,33,	21,32,
2,14,	24,36,	25,37,	26,38,
27,39,	22,34,	28,40,	29,41,
30,42,	31,43,	32,44,	33,45,
35,46,	36,47,	37,48,	38,49,
39,50,	40,51,	41,52,	42,53,
45,54,	46,55,	47,56,	48,57,
50,58,	51,59,	52,60,	53,61,
55,62,	56,63,	57,64,	59,65,
60,66,	61,67,	62,68,	63,69,
64,70,	65,71,	66,72,	68,73,
70,74,	74,75,	0,0,	0,0,
0,0};
struct timsvf timsvec[] ={
0,	0,	0,
timcrank+-1,	0,		0,
timcrank+-17,	timsvec+1,	0,
timcrank+0,	0,		timvstop+1,
timcrank+2,	0,		timvstop+3,
timcrank+0,	timsvec+4,	timvstop+6,
timcrank+2,	0,		timvstop+8,
timcrank+3,	0,		timvstop+11,
timcrank+3,	0,		timvstop+13,
timcrank+4,	0,		timvstop+15,
timcrank+5,	0,		timvstop+17,
timcrank+2,	0,		timvstop+19,
timcrank+2,	0,		timvstop+21,
timcrank+7,	0,		timvstop+23,
timcrank+7,	0,		timvstop+25,
timcrank+0,	timsvec+6,	timvstop+27,
timcrank+4,	0,		0,
timcrank+6,	0,		0,
timcrank+22,	0,		0,
timcrank+2,	0,		0,
timcrank+14,	0,		0,
timcrank+21,	0,		0,
timcrank+16,	0,		0,
timcrank+7,	0,		0,
timcrank+17,	0,		0,
timcrank+22,	0,		0,
timcrank+30,	0,		timvstop+29,
timcrank+19,	0,		timvstop+31,
timcrank+37,	0,		timvstop+33,
timcrank+25,	0,		timvstop+35,
timcrank+23,	0,		timvstop+37,
timcrank+20,	0,		timvstop+39,
timcrank+41,	0,		timvstop+41,
timcrank+44,	0,		timvstop+43,
timcrank+0,	0,		timvstop+45,
timcrank+43,	0,		timvstop+47,
timcrank+34,	0,		timvstop+49,
timcrank+30,	0,		timvstop+51,
timcrank+39,	0,		0,
timcrank+33,	0,		0,
timcrank+40,	0,		0,
timcrank+33,	0,		0,
timcrank+54,	0,		0,
timcrank+0,	0,		timvstop+53,
timcrank+0,	0,		timvstop+55,
timcrank+48,	0,		0,
timcrank+44,	0,		0,
timcrank+56,	0,		0,
timcrank+54,	0,		0,
timcrank+0,	0,		timvstop+57,
timcrank+40,	0,		0,
timcrank+59,	0,		0,
timcrank+61,	0,		0,
timcrank+45,	0,		0,
timcrank+0,	0,		timvstop+59,
timcrank+62,	0,		0,
timcrank+60,	0,		0,
timcrank+53,	0,		0,
timcrank+0,	0,		timvstop+61,
timcrank+62,	0,		0,
timcrank+50,	0,		0,
timcrank+44,	0,		0,
timcrank+65,	0,		0,
timcrank+53,	0,		0,
timcrank+70,	0,		0,
timcrank+55,	0,		0,
timcrank+49,	0,		0,
timcrank+0,	0,		timvstop+63,
timcrank+57,	0,		0,
timcrank+0,	0,		timvstop+65,
timcrank+71,	0,		0,
timcrank+0,	0,		timvstop+67,
timcrank+0,	0,		timvstop+69,
timcrank+0,	0,		timvstop+71,
timcrank+59,	0,		0,
timcrank+0,	0,		timvstop+73,
0,	0,	0};
struct timwork *timtop = timcrank+173;
struct timsvf *timbgin = timsvec+1;
char timmatch[] ={
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
040 ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,011 ,01  ,01  ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
0};
char timextra[] ={
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
int timlineno =1;
# define TIMU(x) x
# define NLSTATE timprevious=TIMNEWLINE
char timtext[TIMLMAX];
struct timsvf *timlstate [TIMLMAX], **timlsp, **timolsp;
char timsbuf[TIMLMAX];
char *timsptr = timsbuf;
int *timfnd;
extern struct timsvf *timestate;
int timprevious = TIMNEWLINE;
int timlook(void) {
	register struct timsvf *timstate, **lsp;
	register struct timwork *timt;
	struct timsvf *timz;
	int timch;
	struct timwork *timr;
# ifdef LEXDEBUG
	int debug;
# endif
	char *timlastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	if (!timmorfg)
		timlastch = timtext;
	else {
		timmorfg=0;
		timlastch = timtext+timleng;
		}
	for(;;){
		lsp = timlstate;
		timestate = timstate = timbgin;
		if (timprevious==TIMNEWLINE) timstate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(stderr,"state %d\n",timstate-timsvec-1);
# endif
			timt = timstate->timstoff;
			if(timt == timcrank){		/* may not be any transitions */
				timz = timstate->timother;
				if(timz == 0)break;
				if(timz->timstoff == timcrank)break;
				}
			*timlastch++ = (char)(timch = input());
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(stderr,"char ");
				allprint(timch);
				putchar('\n');
				}
# endif
			timr = timt;
			if ( (size_t)timt > (size_t)timcrank){
				timt = timr + timch;
				if (timt <= timtop && timt->verify+timsvec == timstate){
					if(timt->advance+timsvec == TIMLERR)	/* error transitions */
						{unput(*--timlastch);break;}
					*lsp++ = timstate = timt->advance+timsvec;
					goto contin;
					}
				}
# ifdef TIMOPTIM
			else if((size_t)timt < (size_t)timcrank) {		/* r < timcrank */
				timt = timr = timcrank+(timcrank-timt);
# ifdef LEXDEBUG
				if(debug)fprintf(stderr,"compressed state\n");
# endif
				timt = timt + timch;
				if(timt <= timtop && timt->verify+timsvec == timstate){
					if(timt->advance+timsvec == TIMLERR)	/* error transitions */
						{unput(*--timlastch);break;}
					*lsp++ = timstate = timt->advance+timsvec;
					goto contin;
					}
				timt = timr + TIMU(timmatch[timch]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(stderr,"try fall back character ");
					allprint(TIMU(timmatch[timch]));
					putchar('\n');
					}
# endif
				if(timt <= timtop && timt->verify+timsvec == timstate){
					if(timt->advance+timsvec == TIMLERR)	/* error transition */
						{unput(*--timlastch);break;}
					*lsp++ = timstate = timt->advance+timsvec;
					goto contin;
					}
				}
			if ((timstate = timstate->timother) != NULL && (timt= timstate->timstoff) != timcrank){
# ifdef LEXDEBUG
				if(debug)fprintf(stderr,"fall back to state %d\n",timstate-timsvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--timlastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(stderr,"state %d char ",timstate-timsvec-1);
				allprint(timch);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(stderr,"stopped at %d with ",*(lsp-1)-timsvec-1);
			allprint(timch);
			putchar('\n');
			}
# endif
		while (lsp-- > timlstate){
			*timlastch-- = 0;
			if (*lsp != 0 && (timfnd= (*lsp)->timstops) != NULL && *timfnd > 0){
				timolsp = lsp;
				if(timextra[*timfnd]){		/* must backup */
					while(timback((*lsp)->timstops,-*timfnd) != 1 && lsp > timlstate){
						lsp--;
						unput(*timlastch--);
						}
					}
				timprevious = TIMU(*timlastch);
				timlsp = lsp;
				timleng = (int)(timlastch-timtext+1);
				timtext[timleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(stderr,"\nmatch ");
					sprint(timtext);
					fprintf(stderr," action %d\n",*timfnd);
					}
# endif
				return(*timfnd++);
				}
			unput(*timlastch);
			}
		if (timtext[0] == 0  /* && feof(timin) */)
			{
			timsptr=timsbuf;
			return(0);
			}
		timprevious = timtext[0] = input();
		if (timprevious>0)
			output(timprevious);
		timlastch=timtext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}
int timback(int *p, int m)
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}

char inbuffer[100];

void timerror(char *s)
{
	fprintf(stderr,"timerror %s\n",s);
	return;
}

short timexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define TIMNPROD 40
# define TIMLAST 246
short timact[]={

  10,  12,  13,  14,  15,  16,  17,  18,  19,  20,
  21,  22,  23,  12,  13,  14,  15,  16,  17,  18,
  19,  20,  21,  22,  23,  34,  33,  32,  60,  10,
  34,  33,  32,  34,  25,   4,  35,  29,  35,  32,
  33,  30,  11,   2,  27,  59,  31,  24,  26,   3,
  28,   9,   8,   7,   6,   5,  36,   1,   0,   0,
  38,   0,  40,  41,  42,  43,   0,  45,   0,   0,
  39,   0,  46,  44,  37,   0,   0,   0,   0,   0,
   0,  48,   0,  53,  54,  55,  49,  56,  57,  50,
   0,  52,  51,  47,  58,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  25,   0,   0,
   0,   0,  25,   0,   0,  25 };
short timpact[]={

-260,-1000,-223,-223, -20,-1000,-1000,-1000,-1000,-1000,
-1000,-223,-1000,-1000,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-260,-1000,-231,-231,-231,-231,
-248,-231,-1000,-1000,-1000,-1000,-231,-1000, -15,-1000,
 -22,-1000,  -8,  -6, -12, -22,-223,-231,-231,-231,
-1000,-231,-231,-1000,-1000,-1000,  -6,-1000,-232,-1000,
-1000 };
short timpgo[]={

   0,  57,  43,  49,  44,  55,  54,  53,  52,  51,
  35,  50,  37,  41,  42,  46,  45 };
short timr1[]={

   0,   1,   1,   1,   1,   1,   3,   3,   3,   3,
   3,   9,   5,  11,   6,  12,   7,  13,  13,   8,
   2,   2,   2,   2,  15,   4,  14,  14,  14,  14,
  14,  14,  14,  14,  14,  14,  14,  14,  10,  16 };
short timr2[]={

   0,   1,   1,   3,   0,   3,   1,   1,   1,   1,
   1,   3,   5,   1,   5,   1,   5,   1,   1,   5,
   7,   5,   3,   1,   1,   1,   1,   1,   1,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   1 };
short timchk[]={

-1000,  -1,  -2,  -3, -10,  -5,  -6,  -7,  -8,  -9,
 260, -14, 261, 262, 263, 264, 265, 266, 267, 268,
 269, 270, 271, 272,  -4, 257,  -4,  -4, -11, -12,
 -13, -15,  47,  46,  45,  58,  -4,  -3, -10,  -2,
 -10, -10, -10, -10, -14, -10, -10, -11, -12, -13,
  -4, -15,  -4, -10, -10, -10, -10, -10, -12, -16,
 260 };
short timdef[]={

   4,  -2,   1,   2,  23,   6,   7,   8,   9,  10,
  38,   0,  26,  27,  28,  29,  30,  31,  32,  33,
  34,  35,  36,  37,   0,  25,   0,  17,   0,   0,
   0,   0,  13,  15,  18,  24,   0,   5,   0,   3,
  23,  11,   0,   0,   0,  22,   0,   0,   0,   0,
  17,   0,   0,  12,  14,  16,  21,  19,   0,  20,
  39 };
#
# define TIMFLAG -1000
/* # define TIMERROR goto timerrlab */
# define TIMACCEPT return(0)
# define TIMABORT return(1)

/*	parser for yacc output	*/

int timdebug = 0; /* 1 for debugging */
TIMSTYPE timv[TIMMAXDEPTH]; /* where the values are stored */
int timchar = -1; /* current input token number */
int timnerrs = 0;  /* number of errors */
short timerrflag = 0;  /* error recovery flag */

int timparse(void) {

	short tims[TIMMAXDEPTH];
	short timj, timm;
	register TIMSTYPE *timpvt;
	register short timstate, *timps, timn;
	register TIMSTYPE *timpv;
	register short *timxi;

	timstate = 0;
	timchar = -1;
	timnerrs = 0;
	timerrflag = 0;
	timps= &tims[-1];
	timpv= &timv[-1];

 timstack:    /* put a state and value onto the stack */

	if( timdebug  ) printf( "state %d, char 0%o\n", timstate, timchar );
	if( ++timps> &tims[TIMMAXDEPTH] ) {
	   timerror( "yacc stack overflow" );
	   return(1);
	}
	*timps = timstate;
	++timpv;
	*timpv = timval;

 timnewstate:

	timn = timpact[timstate];

	if( timn<= TIMFLAG ) goto timdefault; /* simple state */

	if( timchar<0 ) if( (timchar=timlex())<0 ) timchar=0;
	if( (timn += (short)timchar)<0 || timn >= TIMLAST ) goto timdefault;

	if( timchk[ timn=timact[ timn ] ] == timchar ){ /* valid shift */
		timchar = -1;
		timval = timlval;
		timstate = timn;
		if( timerrflag > 0 ) --timerrflag;
		goto timstack;
		}

 timdefault:
	/* default state action */

	if( (timn=timdef[timstate]) == -2 ) {
		if( timchar<0 ) if( (timchar=timlex())<0 ) timchar = 0;
		/* look through exception table */

		for( timxi=timexca; (*timxi!= (-1)) || (timxi[1]!=timstate) ; timxi += 2 ) ; /* VOID */

		while( *(timxi+=2) >= 0 ){
			if( *timxi == timchar ) break;
			}
		if( (timn = timxi[1]) < 0 ) return(0);   /* accept */
		}

	if( timn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( timerrflag ){

		case 0:   /* brand new error */

			timerror( "syntax error" );
		/* timerrlab: */
			++timnerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			timerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( timps >= tims ) {
			   timn = timpact[*timps] + TIMERRCODE;
			   if( timn>= 0 && timn < TIMLAST && timchk[timact[timn]] == TIMERRCODE ){
			      timstate = timact[timn];  /* simulate a shift of "error" */
			      goto timstack;
			      }
			   timn = timpact[*timps];

			   /* the current timps has no shift onn "error", pop stack */

			   if( timdebug ) printf( "error recovery pops state %d, uncovers %d\n", *timps, timps[-1] );
			   --timps;
			   --timpv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	timabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

			if( timdebug ) printf( "error recovery discards char %d\n", timchar );

			if( timchar == 0 ) goto timabort; /* don't discard EOF, quit */
			timchar = -1;
			goto timnewstate;   /* try again in the same state */

			}

		}

	/* reduction by production timn */

		if( timdebug ) printf("reduce %d\n",timn);
		timps -= timr2[timn];
		timpvt = timpv;
		timpv -= timr2[timn];
		timval = timpv[1];
		timm=timn;
			/* consult goto table to find next state */
		timn = timr1[timn];
		timj = timpgo[timn] + *timps + 1;
		if( timj>=TIMLAST || timchk[ timstate = timact[timj] ] != -timn ) timstate = timact[timpgo[timn]];
		switch(timm){

case 11:

{ juldateset(timpvt[-2], timpvt[-0]); } break;
case 12:

{ nordate(timpvt[-4],timpvt[-2],timpvt[-0]); } break;
case 14:

{ nordate(timpvt[-2],timpvt[-0],timpvt[-4]); } break;
case 16:

{ nordate(timpvt[-2],timpvt[-4],timpvt[-0]); } break;
case 19:

{ nordate(timpvt[-4],timpvt[-2],timpvt[-0]); } break;
case 20:

{
					  timeload(timpvt[-6],timpvt[-4],timpvt[-2],timpvt[-0]); } break;
case 21:

{
					  timeload(timpvt[-4],timpvt[-2],timpvt[-0],0); } break;
case 22:

{ timeload(timpvt[-2],timpvt[-0],0,0); } break;
case 23:

{ timeload(timpvt[-0],0,0,0); } break;
case 38:

 {
			  sscanf(valback,"%d",&temp);
			  timval = temp;
			} break;
case 39:

 {
			  temp = (int)strlen(valback);
			  for(temp2=temp;temp2<3;temp2++) valback[temp2]='0';
			  valback[3]='\0';
			  sscanf(valback,"%d",&temp);
			  timval = temp;
			} break;
		}
		goto timstack;  /* stack new state and value */

	}
