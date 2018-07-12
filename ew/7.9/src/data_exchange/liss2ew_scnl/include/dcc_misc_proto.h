/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dcc_misc_proto.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:55  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:26  mark
 *     Initial checkin
 *
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

/* Processing file bombopen.c */

 FILE *bombopen(char *infile, char *inmode);

/* Found 1 functions */

/* Processing file bombout.c */


/* Found 0 functions */

/* Processing file calctime.c */

 void timeon()
;
 void timeof()
;

/* Found 2 functions */

/* Processing file chansubs.c */

 void UnpackChanNames(char *ins,char *inl,char *inc,
			  char *outs,char *outl,char *outc);

 void PackChanNames(char *ins,char *inl,char *inc,
			char *outs,char *outl,char *outc);

 char *FullChanName(char *inl,char *inc);

 void InitChanTables();

 void *FindChannel(char *ins,char *inl,char *inc);

 void MakeChanKey(struct station_index *index,char *outkey);

 BOOL ChanKeyEq(struct station_index *index,char *chkkey);

 void AddChannel(char *ins,char *inl,char *inc,void *user);

 void *StartChannelTrav();

 void *TraverseChannels();

 void ParseChanString(char *InBuff,char *defstation,char *statname,
			  char *location,char *channel);

 BOOL ChanNamesEq(struct station_index *idx1,
		      struct station_index *idx2);

/* Found 12 functions */

/* Processing file compat_strerror.c */


/* Found 0 functions */

/* Processing file ctx_file.c */

 FILE *open_ctx_file();


/* Found 1 functions */

/* Processing file dataswaps.c */

 DCC_LONG TwidLONG(DCC_LONG inlong);	/* Twiddle a long word from one to another */

 DCC_WORD TwidWORD(DCC_WORD inword);	/* Twiddle a word around */

 DCC_LONG LocGVAX_LONG(DCC_LONG inlong);/* Local gets vax long */

 DCC_LONG LocGM68_LONG(DCC_LONG inlong);/* Local gets M68000 long */

 DCC_WORD LocGVAX_WORD(DCC_WORD inword);/* Local gets vax DCC_WORD */

 DCC_WORD LocGM68_WORD(DCC_WORD inword);/* Local gets M68000 DCC_WORD */

 DCC_LONG VAXGLoc_LONG(DCC_LONG inlong);/* Vax gets local long */

 DCC_LONG M68GLoc_LONG(DCC_LONG inlong);/* 68000 gets local long */

 DCC_WORD VAXGLoc_WORD(DCC_WORD inword);/* vax gets local word */

 DCC_WORD M68GLoc_WORD(DCC_WORD inword);/* 68000 gets local word */


/* Found 10 functions */

/* Processing file dcc_env.c */

 char *dcc_env(char *inenv);


/* Found 1 functions */

/* Processing file envfile.c */

 void envfile(char *envvar, char *fname, char *outname);

 void catfile(char *envvar, char *fname, char *outname);


/* Found 2 functions */

/* Processing file fixopen.c */

 FILE *fix_create(char *filename,int rsize,int alloc,int extend);

 FILE *fix_open(char *filename);

 int fix_readr(void *ptr, int size, int nitems, FILE *stream);


/* Found 3 functions */

/* Processing file getmyname.c */

 char *getmyname();


/* Found 1 functions */

/* Processing file itemlist.c */

 ITEMLIST *MakeItemList(char *inputstring, char separator, BOOL killwhitespace, BOOL permitnullitems);

 	VOID  FreeItemList(ITEMLIST *root);

	VOID	AppendItemList(ITEMLIST *originallist, ITEMLIST *newlist);


/* Found 3 functions */

/* Processing file log.c */


/* Found 0 functions */

/* Processing file readenv.c */

 char *readenv(char *environ);


/* Found 1 functions */

/* Processing file readline.c */

 BOOL ReadLine(FILE *ifile,char *buffer,int leng);


/* Found 1 functions */

/* Processing file safemem.c */

 void *SafeAlloc(int siz);

 char *SafeAllocString(char *str);

 void SafeFree(void *inptr);

 void exit_nomem();

/* Found 4 functions */

/* Processing file spawncommand.c */

 int	spawnjob(char *commstr,char *outputname,UDCC_LONG *completion);


/* Found 1 functions */

/* Processing file storeenv.c */

 char *storeenv(char *environ,char *content);


/* Found 1 functions */

/* Processing file strfuns.c */

 VOID TrimString(char *inst);

 char *NonWhite(char *inst);

 VOID Upcase(char *instr);

 VOID Locase(char *instr);

 char *alstr(char *instr);

 BOOL KeyFound(char *in1,char *in2);


/* Found 6 functions */

/* Processing file tapfio_posix.c */

 int mag_openr(char *name);

 int mag_openw(char *name);

 int mag_close(int chan);

 int mag_read(int chan,char *buffer,unsigned maxm,int *rlen);

 int mag_readsts(int chan,char *buffer,unsigned maxm,	int *rlen,
		     unsigned *rstat,unsigned *rerr);

 int mag_rd(int chan,char *buff,unsigned maxm);

 int mag_write(int chan,char *buffer,unsigned maxm);

 int mag_write_eof(int chan);

 int mag_eof(int chan);

 int mag_write_eoi(int chan);

 void mag_flap(int chan);

 void mag_rewind(int chan);

 void mag_rew(int chan);

 void mag_skipf(int chan,int files);

 void mag_skipr(int chan,int files);

 char *mag_error(int errn);

/* Found 16 functions */

/* Processing file watchdog.c */

 void watchdog(char *pidfile);

/* Found 1 functions */

/* Processing file wildmatch.c */

 BOOL WildMatch(char *pattern, char *target);

/* Found 1 functions */

