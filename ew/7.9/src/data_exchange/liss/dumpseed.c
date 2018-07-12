/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dumpseed.c 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:07:59  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:43:46  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:42:20  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>
#include <seed_data.h>
#include <seed_data_proto.h>
#include <dcc_misc.h>
#include <seed_select.h>

#include "dumpseed_proto.h"
#ifdef _WINNT
#include "getopt.h"
#endif
#define MAX_FILES 8192
#define SEED_RECORD_SIZE 4096

int decode = 0;
int format = 0;
int export=0;
int quiet=0;

int blocksize;
int bufsize;

#define FORM_SRO 10
#define FORM_CDSN 20
#define FORM_16 30
#define FORM_32 40
#define FORM_SEED_S1 51
#define FORM_SEED_S2 52
#define FORM_SEED_S3 53
#define FORM_ASCII 60

#define FORM_UNSUPPORTED 999


#define LW(a) LocGM68_WORD(a)
#define LL(a) LocGM68_LONG(a)

UDCC_BYTE tape_read_buff[65536];
UDCC_BYTE *seed_rec_buf;
SEED_DATA *seed_rec;

DCC_LONG decomp_samples[SEED_RECORD_SIZE*2];

char *pidfile=NULL;

char datfmt[20];
char decfmt[20];
int form;
char chanpick[20];
int chanpickp=0;
int telemetry=0;
int ignore_blockettes=0;
int tel_port=4000;
char *select_file = NULL;

struct sel3_root dumproot;

FILE *seediov;

struct blkhdr {
  UDCC_WORD	blktype;
  UDCC_WORD	blkxref;
};

VOID Blk_DO(UDCC_BYTE *prec, int type) 
{ 
  struct  Data_only *dat;

  dat = (struct Data_only *) prec;

  if (!export) {
    printf("1000-DATAONLY: ");
    switch(dat->Encoding) {
    case 1: printf("16-WORD"); break;
    case 2: printf("24-WORD"); break;
    case 3: printf("32-WORD"); break;
    case 4: printf("IEEE-SP"); break;
    case 5: printf("IEEE-DP"); break;
    case 10: printf("STEIM-1"); break;
    case 11: printf("STEIM-2"); break;
    case 12: printf("GEOSCOPE-MPX24"); break;
    case 13: printf("GEOSCOPE-16-3"); break;
    case 14: printf("GEOSCOPE-16-4"); break;
    case 15: printf("USNSN"); break;
    case 16: printf("CDSN"); break;
    case 17: printf("GRAEF"); break;
    case 18: printf("IPG"); break;
    case 30: printf("SRO/ASRO"); break;
    case 31: printf("HGLP"); break;
    case 32: printf("DWWSSN"); break;
    case 33: printf("RSTN"); break;
    default: printf("Endoding %d",dat->Encoding);
    }
  }

  format = dat->Encoding;

  if (!export) {
    printf(" Swap=%s",dat->Order?"BE":"LE");

    printf(" Len=%d\n",dat->Length);
  }
}

VOID Blk_QE(UDCC_BYTE *prec, int type) 
{ 
  struct  Data_ext *dat;

  dat = (struct Data_ext *) prec;

  if (!export) {

    printf("1001-QTADEXT: ");
    printf("Quality=%d%% ",dat->Timing);
    printf("Usec=%d ",dat->Usec);
    printf("Frames=%d\n",dat->Frames);
  } else {

    printf("QTAD%d, %d, %d\n",
	   dat->Timing,
	   dat->Usec,
	   dat->Frames);

  }

}

VOID Blk_GE(UDCC_BYTE *prec, int type) { printf("Blockette %d seen\n",type); }
     
     VOID Blk_ME(UDCC_BYTE *prec, int type) 
{ 
  struct murdock_detect *mur;
  int i;

  mur = (struct murdock_detect *) prec;

  if (!export) {
    printf("201-MURDET: %s %d ",
	   mur->Event_Flags&MEVENT_DILAT?"d":"c",
	   mur->Lookback);
		
    for (i=0; i<5; i++) putchar('0'+mur->SNR_Qual[i]);

    printf(" %s",ST_PrintDate(SH_Cvt_Time(mur->Onset_Time),TRUE));
    printf(" %g",SH_Cvt_Float(mur->Signal_Amplitude));
    printf(" %g",SH_Cvt_Float(mur->Signal_Period));
    printf(" %g",SH_Cvt_Float(mur->Background_Estimate));

    printf(" %c\n",'A'+mur->Pick);
  } else {
    printf("PICK%s, %d, ",
	   mur->Event_Flags&MEVENT_DILAT?"d":"c",
	   mur->Lookback);
		
    for (i=0; i<5; i++) putchar('0'+mur->SNR_Qual[i]);

    printf(", %s",ST_PrintDate(SH_Cvt_Time(mur->Onset_Time),TRUE));
    printf(", %g",SH_Cvt_Float(mur->Signal_Amplitude));
    printf(", %g",SH_Cvt_Float(mur->Signal_Period));
    printf(", %g",SH_Cvt_Float(mur->Background_Estimate));

    printf(" %c\n",'A'+mur->Pick);
  }

}


VOID Blk_STC(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}

VOID Blk_SIC(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}


VOID Blk_PRC(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}


VOID Blk_GC(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}


VOID Blk_CA(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}


VOID Blk_BB(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}


VOID Blk_BD(UDCC_BYTE *prec, int type) 
{
  if (!export) {
    printf("Blockette %d seen\n",type); 
  }
}


void dumpseed()
{

  UDCC_BYTE 	buff[100];
  int a,b,c;
  STDTIME btim;
  STDFLT d;
  UDCC_BYTE *membf;
  struct blkhdr *newblk;
  char chan[20];
  int form;
  ITEMLIST *taglist=NULL,*tagloop=NULL;
  int anyact=0;

  format = 0;

  membf = (UDCC_BYTE *) seed_rec;

  memcpy(buff,seed_rec->Channel_ID,3);
  buff[3] = '\0';
  TrimString(buff);

  if (chanpickp)
    if (!WildMatch(chanpick, buff)) return;

  if (select_file) {
    int interesting;
    char n[10],s[10],l[10],c[10];

    SH_Get_Idents(seed_rec,n,s,l,c);
    TrimString(n);
    Upcase(n);
    TrimString(s);
    Upcase(s);
    TrimString(l);
    Upcase(l);
    TrimString(c);
    Upcase(c);

    interesting = SelectDateInteresting(&dumproot,
					SH_Start_Time(seed_rec),
					SH_End_Time(seed_rec),
					n,s,l,c,
					&taglist);

    if (!interesting) return;

  }

  memcpy(buff,seed_rec->Seq_ID,6);
  buff[6] = '\0';

  if (!export) {

    printf("Record %s Type %c ",buff,seed_rec->Record_Type);
    if (seed_rec->Record_Type!='D') {
      printf("\n");
      return;
    }

    memcpy(buff,seed_rec->Network_ID,2);
    buff[2] = '\0';
    TrimString(buff);

    if (buff[0]!='\0')  printf("Network %s ",buff);

    memcpy(buff,seed_rec->Station_ID,5);
    buff[5] = '\0';
    TrimString(buff);

    printf("Station %s ",buff);

    memcpy(buff,seed_rec->Location_ID,2);
    buff[2] = '\0';
    TrimString(buff);

    if (buff[0]!='\0') printf("Location %s ",buff);

    memcpy(buff,seed_rec->Channel_ID,3);
    buff[3] = '\0';
    TrimString(buff);

    printf("Channel %s\n",buff);
    strcpy(chan,buff);

    btim.year = LW(seed_rec->Start_Time.year);
    btim.day = LW(seed_rec->Start_Time.day);
    btim.hour = seed_rec->Start_Time.hour;
    btim.minute = seed_rec->Start_Time.minute;
    btim.second = seed_rec->Start_Time.seconds;
    btim.msec = LW(seed_rec->Start_Time.fracs);

    printf("Time %04d,%03d,%02d:%02d:%02d.%04d ",
	   (int) btim.year,
	   (int) btim.day,
	   (int) btim.hour,
	   (int) btim.minute,
	   (int) btim.second,
	   (int) btim.msec);

    printf("Samples %d ",LW(seed_rec->Number_Samps));

    a = LW(seed_rec->Rate_Factor);
    b = LW(seed_rec->Rate_Mult);

    printf("Factor %d Mult %d",a,b);

    if (b==0) printf(" (Rate Illegal!)\n");
    else {
      if (a<0) d = 1.0/(-((STDFLT) a));
      else d = (STDFLT) a;
      if (b<0) d /= -((STDFLT) b);
      else d *= (STDFLT) b;

      printf(" (Rate %g)\n",d);

    }

    anyact = 0;

    if (seed_rec->Activity_Flags) {

      printf("Activity ");

      if (seed_rec->Activity_Flags&ACTFLAG_CALSIG) printf("CALSIG ");
      if (seed_rec->Activity_Flags&ACTFLAG_CLKFIX) printf("CLKFIX ");
      if (seed_rec->Activity_Flags&ACTFLAG_BEGEVT) printf("BEGEVT ");
      if (seed_rec->Activity_Flags&ACTFLAG_ENDEVT) printf("ENDEVT ");

      printf("(0x%02x) ",seed_rec->Activity_Flags);
      anyact = 1;
    }

    if (seed_rec->IO_Flags) {
    
      printf("IO ");

      if (seed_rec->IO_Flags&IOFLAG_ORGPAR) printf("ORGPAR ");
      if (seed_rec->IO_Flags&IOFLAG_LONGRC) printf("LONGRC ");
      if (seed_rec->IO_Flags&IOFLAG_SHORTR) printf("SHORTR ");

      printf("(0x%02x) ",seed_rec->IO_Flags);
      anyact = 1;
    }

    if (seed_rec->Qual_Flags) {

      printf("Qual ");

      if (seed_rec->Qual_Flags&QULFLAG_AMPSAT) printf("AMPSAT ");
      if (seed_rec->Qual_Flags&QULFLAG_SIGCLP) printf("SIGCLP ");
      if (seed_rec->Qual_Flags&QULFLAG_SPIKES) printf("SPIKES ");
      if (seed_rec->Qual_Flags&QULFLAG_GLITCH) printf("GLITCH ");
      if (seed_rec->Qual_Flags&QULFLAG_PADDED) printf("PADDED ");
      if (seed_rec->Qual_Flags&QULFLAG_TLMSNC) printf("TLMSNC ");

      printf("(0x%02x) ",seed_rec->IO_Flags);
      anyact = 1;
    }

    if (anyact) 
      printf("\n");

    anyact = 0;
    for (tagloop=taglist; tagloop!=NULL; tagloop=tagloop->next) {
      if (!anyact) 
	printf("Select Tags:");
      anyact = 1;
      printf(" %s",
	     tagloop->item);
    }
    if (anyact) 
      printf("\n");
  } else {

    /* Write the data as BDF */
  
    if (seed_rec->Record_Type!='D') {
      return;
    }

    memcpy(buff,seed_rec->Station_ID,5);
    buff[5] = '\0';
    TrimString(buff);

    printf("STA %s\n",buff);

    memcpy(buff,seed_rec->Network_ID,2);
    buff[2] = '\0';
    TrimString(buff);

    if (buff[0]!='\0')  printf("NET %s\n",buff);

    memcpy(buff,seed_rec->Location_ID,2);
    buff[2] = '\0';
    TrimString(buff);

    if (buff[0]!='\0') printf("LOC %s\n",buff);

    memcpy(buff,seed_rec->Channel_ID,3);
    buff[3] = '\0';
    TrimString(buff);

    printf("COMP%s\n",buff);
    strcpy(chan,buff);

    a = LW(seed_rec->Rate_Factor);
    b = LW(seed_rec->Rate_Mult);

    if (b==0) printf("RATE0.0\n");
    else {
      if (a<0) d = 1.0/(-((STDFLT) a));
      else d = (STDFLT) a;
      if (b<0) d /= -((STDFLT) b);
      else d *= (STDFLT) b;

      printf("RATE%.4g\n",d);

    }

    btim.year = LW(seed_rec->Start_Time.year);
    btim.day = LW(seed_rec->Start_Time.day);
    btim.hour = seed_rec->Start_Time.hour;
    btim.minute = seed_rec->Start_Time.minute;
    btim.second = seed_rec->Start_Time.seconds;
    btim.msec = LW(seed_rec->Start_Time.fracs);

    printf("TIME%04d,%03d,%02d:%02d:%02d.%04d\n",
	   (int) btim.year,
	   (int) btim.day,
	   (int) btim.hour,
	   (int) btim.minute,
	   (int) btim.second,
	   (int) btim.msec);

    printf("NSAM%d\n",LW(seed_rec->Number_Samps));

  }
  
  a = LW(seed_rec->First_Blockette);

  if (!export) {
    printf("Blockettes %d Correction %d Data Start %d First Block %d\n",
	   seed_rec->Total_Blockettes,
	   LL(seed_rec->Time_Correction),
	   LW(seed_rec->Data_Start),a);
  }

  if (!ignore_blockettes) {
    FOREVER {
      if (a<=0 || a>4095) break;
      c = a;
      newblk = (struct blkhdr *) &membf[a];
      a = LW(newblk->blkxref);
      b = LW(newblk->blktype);
      switch (b) {
      case 200:	Blk_GE(&membf[c],b);	break;
      case 201:	Blk_ME(&membf[c],b);	break;
      case 300:	Blk_STC(&membf[c],b);	break;
      case 310:	Blk_SIC(&membf[c],b);	break;
      case 320:	Blk_PRC(&membf[c],b);	break;
      case 390:	Blk_GC(&membf[c],b);	break;
      case 395:	Blk_CA(&membf[c],b);	break;
      case 400:	Blk_BB(&membf[c],b);	break;
      case 405:	Blk_BD(&membf[c],b);	break;
      case 1000:	Blk_DO(&membf[c],b);	break;
      case 1001:	Blk_QE(&membf[c],b);	break;
      default:
	if (!export)
	  printf("%%Unknown Blockette %d Seen\n",b);
      }
    }
  }

  if (!export) printf("\n");

  form=0;

  if (streq(datfmt,"ASRO")) form=30;
  if (streq(datfmt,"SRO")) form=30;

  if (streq(datfmt,"RSTN")) form=33;
  if (streq(datfmt,"CDSN")) form=33;

  if (streq(datfmt,"DWWSSN")) form=1;
  if (streq(datfmt,"16-BIT")) form=1;

  if (streq(datfmt,"32-BIT")) form=3;

  if (streq(datfmt,"HARV")) form=10;
  if (streq(datfmt,"SEED")) form=10;
  if (streq(datfmt,"SEEDS1")) form=10;
  if (streq(datfmt,"STEIM")) form=10;
  if (streq(datfmt,"STEIM1")) form=10;

  if (streq(datfmt,"SEEDS2")) form=11;
  if (streq(datfmt,"STEIM2")) form=11;

  if (streq(datfmt,"SEEDS3")) form=20;
  if (streq(datfmt,"STEIM3")) form=20;

  if (streq(datfmt,"LOG")) form=50;

  if (format==0) format=form;

  if (streq(chan,"LOG")) 
    format=50;

  switch(format) {
  case 1:  strcpy(decfmt,"DWWSSN"); break;
  case 3:  strcpy(decfmt,"32-BIT"); break;
  case 10:   strcpy(decfmt,"STEIM1"); break;
  case 11:   strcpy(decfmt,"STEIM2"); break;
  case 16:   strcpy(decfmt,"CDSN"); break;
  case 20:   strcpy(decfmt,"STEIM3"); break;
  case 30:   strcpy(decfmt,"SRO"); break;
  case 32:   strcpy(decfmt,"DWWSSN"); break;
  case 33:   strcpy(decfmt,"CDSN"); break;
  case 50:  strcpy(decfmt,"LOG"); break;
  default:
    strcpy(decfmt,"UNKNOWN");
    printf("Don't know how to decode format %d\n",format);
  }

  if (decode) {
    if (export) {
      printf("FORM%s\n",decfmt);
      for (tagloop=taglist; tagloop!=NULL; tagloop=tagloop->next)
	printf("TAG %s\n",
	       tagloop->item);
      printf("DATA\n");
    }

    if (streq(decfmt,"LOG")) {
      for (a=0; a<LW(seed_rec->Number_Samps); a++) {
	char *dc;
	dc = (char *) &seed_rec_buf[LW(seed_rec->Data_Start)];
	dc += a;
	if (export) 
	  printf("%d\n",*dc);
	else
	  putchar(*dc);
      }
      printf("\n");
    } else 
      if (!streq(decfmt,"UNKNOWN")) {
	int actual,numsamps;
	numsamps = LW(seed_rec->Number_Samps);
	actual = SH_Data_Decode(decfmt, (long *) decomp_samples, seed_rec, 0);
	for (a=0; a<min(numsamps,actual); a++) 
	  if (export) 
	    printf("%d\n",decomp_samples[a]);
	  else
	    printf("%d ",decomp_samples[a]);
	if (!export) 
	  printf("\n");
	if (actual<numsamps) {
	  if (export)
	    printf("ERR Record had fewer samples (%d) than header (%d)\n",
		   actual,numsamps);
	  else
	    fprintf(stderr,
		    "%%%%Record had fewer samples (%d) than header (%d)\n",
		   actual,numsamps);
	}
      }
  }
}

BOOL get_nexrec()
{

  int i,b;

  if (feof(seediov)) return(FALSE);

  i=read(fileno(seediov), tape_read_buff, bufsize);
  if (i<=0) return(FALSE);

  for (b=0; b<i; b+=blocksize) {
    if (b+blocksize>i) continue;
    seed_rec_buf = &tape_read_buff[b];
    seed_rec = (SEED_DATA *) seed_rec_buf;
    dumpseed();
    fflush(stdout);
    fflush(stderr);
#ifndef _WINNT
    if (pidfile)
      watchdog(pidfile);
#endif
  }

  return(TRUE);

}

void process_record(char *inseed)
{


  seed_rec_buf = (UDCC_BYTE *) inseed;
  seed_rec = (SEED_DATA *) seed_rec_buf;
  dumpseed();
  fflush(stdout);
  fflush(stderr);
#ifndef _WINNT
  if (pidfile)
    watchdog(pidfile);
#endif
}

void usage(char *name)
{

  fprintf(stderr,
	  "usage: %s {-d} {-r buffersize} {-b blksiz} {-f def-format} {-e} {-c chanwild} {-t} {-p port} {-P pidname} {-s selectfile} hostname or files (wildcards ok)\n",name);
  fprintf(stderr,"\n\t-d Decode Records\n\t-e Export as BDF Files\n\t-i Ignore Blockettes\n\t-t Connect to Telemetry instead of read file\n\n");
  exit(2);
}

char *ifil[MAX_FILES];
int num_files=0;

int main(int argc,char **argv)
{

  int c;
  extern char *optarg;
  extern int optind;
  int errflg=0;
  int i;

  blocksize = 0;
  bufsize = 0;
  strcpy(datfmt,"STEIM1");
  strcpy(chanpick,"*");

  SelectInit(&dumproot);

  while ((c = getopt(argc, argv, "qedtib:f:c:r:p:P:s:")) != EOF)
    switch (c) {
    case 'd':
      decode++;
      break;
    case 'e':
      export++;
      break;
    case 'q':
      quiet++;
      break;
    case 'i':
      ignore_blockettes ++;
      break;
    case 'b':
      blocksize = atoi(optarg);
      if (bufsize==0) bufsize = blocksize;
      break;
    case 'r':
      bufsize = atoi(optarg);
      break;
    case 'f':
      strcpy(datfmt,optarg);
      Upcase(datfmt);
      break;
    case 'c':
      strcpy(chanpick,optarg);
      Upcase(chanpick);
      chanpickp=1;
      break;
    case 't':
      telemetry ++;
      break;
    case 'p':
      tel_port = atoi(optarg);
      break;
    case 's':
      select_file = strdup(optarg);
      break;
    case 'P':
      pidfile = strdup(optarg);
      break;
    case '?':
      errflg++;
    }

  if (errflg) usage(argv[0]);

  if (telemetry) {
    if (blocksize==0) blocksize=512;
    if (bufsize==0) bufsize=512;
  }
  
  if (blocksize==0) blocksize=4096;
  if (bufsize==0) bufsize=4096;

  if (!telemetry && pidfile)
    printf("Pidfile not recommended without telemetry\n");

  if (select_file && chanpickp) 
    bombout(EXIT_ABORT,"Select file not compatible with channel select");

  if (select_file) {
    SelectLoadup(&dumproot, select_file);
    if (!quiet) 
      SelectList(&dumproot);
  }

#ifndef _WINNT
  if (pidfile)
    watchdog(pidfile);
#endif

  for (; optind < argc; optind++) {
    ifil[num_files++] = strdup(argv[optind]);
    if (num_files+1>MAX_FILES) 
      bombout(EXIT_ABORT,"Too many files - only supports %d",MAX_FILES);
  }
  
  if (num_files<=0) usage(argv[0]);

  if (!telemetry) {

    for (i=0; i<num_files; i++) {

      int dash=0;

      if (strcmp(ifil[i], "-")==0) dash=1;

      if (dash) 
	seediov = stdin;
      else {
	seediov = fopen(ifil[i],"r");
	if (seediov==NULL) {
	  fprintf(stderr,"Couldn't open seed file %s: %s\n",
		  ifil[i],
		  strerror(errno));
	  continue;
	}
      }

      if (!quiet) {
	if (!dash) 
	  fprintf(stderr,"[Process seed file %s]\n",ifil[i]);
	else
	  fprintf(stderr,"[Process standard input]\n");
      }

      while (get_nexrec());

      if (!dash)
	fclose(seediov);
    }

    exit(EXIT_NORMAL);
  }

  if (num_files>1)
    bombout(EXIT_ABORT,"Can't open more than one host");

  process_telemetry(ifil[0], tel_port, blocksize);

  exit(EXIT_NORMAL);

}	
