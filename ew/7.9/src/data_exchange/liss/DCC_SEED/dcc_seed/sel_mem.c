/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sel_mem.c 4531 2011-08-10 01:29:00Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_time.h>
#include <dcc_misc.h>
#include <seed/seed_select.h>

/* Modified 29-Feb-00/PNL

   Program would not match any records if no DATE was specified in select
   file.  Now if no DATE is specified, all times match, as it says in the
   man page selectfile.4. */

/* Modified 6-Jan-98/SH
   
   Program would always match a record if the incoming station or network 
   was null.  This was changed so that * could match blank, but otherwise 
   not */
   

_SUB	VOID SelectInit(struct sel3_root *root)
{

  root->root_chain = NULL;
  root->tail_chain = NULL;
  root->jdmax = 0;
  root->jdmin = 1<<30;

}

PRIVATE	int gline=0;
PRIVATE	char gsel[512];
PRIVATE	BOOL geof=FALSE;

PRIVATE int SelectRline(FILE *ifil)
{

  gline++;

  if (!ReadLine(ifil,gsel,511)) {
    geof = TRUE;
    return(-2);
  }

  TrimString(gsel);
  Upcase(gsel);

  if (streq(gsel,"/")) return(-1);
  if (streq(gsel,"//")) {
    geof = TRUE;
    return(-2);
  }

  if (gsel[0]=='#') return(0);	/* Comment */
  return(strlen(gsel));
}


_SUB VOID SelectList(struct sel3_root *root)
{

  struct sel3_chain *loopchain;
  struct sel3_stat *loopstat;
  struct sel3_chan *loopchan;
  struct sel3_date *loopdate;
  ITEMLIST *looptag;
  int chan=0;

  for (loopchain=root->root_chain; loopchain!=NULL; 
       loopchain=loopchain->next) {
    chan++;
    printf("Chain %d\n",chan);

    for (loopdate=loopchain->root_date; loopdate!=NULL; 
	 loopdate=loopdate->next) {

      printf("  Date Range %s->",
	     ST_PrintDate(loopdate->sdate,FALSE));
      printf("%s (%c)\n",
	     ST_PrintDate(loopdate->edate,FALSE),
	     loopdate->addnot?'+':'-');

    }

    for (loopstat=loopchain->root_stat; loopstat!=NULL;
	 loopstat=loopstat->next) {

      printf("  Station %s/%s (%c)\n",
	     loopstat->network,
	     loopstat->station,
	     loopstat->addnot?'+':'-');
    }

    for (loopchan=loopchain->root_chan; loopchan!=NULL;
	 loopchan=loopchan->next) {

      printf("  Channel %s/%s (%c)\n",
	     loopchan->location==NULL?"NONE":
	     loopchan->location,
	     loopchan->channel,
	     loopchan->addnot?'+':'-');

    }

    for (looptag=loopchain->taglist; looptag!=NULL;
	 looptag=looptag->next) {
      printf("  Tag: %s\n",looptag->item);
    }

  }
}


PRIVATE VOID	_proctag(struct sel3_chain *newchain, char *instr)
{

  ITEMLIST *newtags;

  newtags = MakeItemList(instr,',',TRUE,FALSE);
	
  if (newtags==NULL) return;

  if (newchain->taglist==NULL) {
    newchain->taglist = newtags;
    return;
  }

  AppendItemList(newchain->taglist,newtags);

}

PRIVATE VOID	_procstat(struct sel3_chain *newchain, char *instr)
{

  struct sel3_stat *newstat;
  ITEMLIST *statlist,*loop,*statsub;
  char *statname;
  BOOL itemok;
  int ofs=0;

  statlist = MakeItemList(instr,',',TRUE,FALSE);
	
  for (loop=statlist; loop!=NULL; loop=loop->next) {

    newstat = SafeAlloc(sizeof(struct sel3_stat));
    newstat->next = NULL;
    newstat->class = FALSE;
		
    ofs = 0;
    switch(loop->item[0]) {
    case '+':
      newstat->addnot = TRUE;
      ofs = 1;
      break;
    case '-':
      newstat->addnot = FALSE;
      ofs = 1;
      break;
    default:
      newstat->addnot = TRUE;
    }

    statname = SafeAllocString(&loop->item[1]);

    statsub = MakeItemList(statname,'/',TRUE,TRUE);
    itemok = FALSE;
    if (statsub) {
      if (statsub->next) {
	if (statsub->next->next==NULL)
	  itemok = TRUE;
	if (statsub->next->item==NULL)
	  itemok = FALSE;
      }
      if (statsub->item==NULL)
	itemok = FALSE;
    }
		

    if (!itemok) {
      printf("?Bad stationame name %s (%s line %d)\n",
	     statname,filename,gline);
      continue;
    }

    newstat->network = NULL;
    newstat->station = NULL;

    newstat->network = SafeAllocString(statsub->item);
    newstat->station = SafeAllocString(statsub->next->item);
	
    if (newchain->tail_stat==NULL) newchain->root_stat = newstat;
    else	newchain->tail_stat->next = newstat;
    newchain->tail_stat = newstat;
  }

  FreeItemList(statlist);

}

PRIVATE VOID	_procchan(struct sel3_chain *newchain, char *instr)
{

  struct sel3_chan *newchan;
  ITEMLIST *chanlist, *loop, *chansub;
  char *channame;
  BOOL itemok;

  chanlist = MakeItemList(instr,',',TRUE,FALSE);

  for (loop=chanlist; loop!=NULL; loop=loop->next) {

    newchan = SafeAlloc(sizeof(struct sel3_chan));
    newchan->next = NULL;
		
    switch(loop->item[0]) {
    case '+':
      newchan->addnot = TRUE;
      channame = &loop->item[1];
      break;
    case '-':
      newchan->addnot = FALSE;
      channame = &loop->item[1];
      break;
    default:
      newchan->addnot = TRUE;
      channame = &loop->item[0];
    }
		
    chansub = MakeItemList(channame,'/',TRUE,TRUE);
    itemok = FALSE;
    if (chansub)
      if (chansub->next) {
	if (chansub->next->next==NULL)
	  itemok = TRUE;
	if (chansub->next->item==NULL)
	  itemok = FALSE;
      }

    if (!itemok) {
      printf("?Bad channel name %s (%s line %d)\n",
	     channame,filename,gline);
      continue;
    }

    newchan->location = NULL;
    newchan->channel = NULL;

    if (chansub->item)
      newchan->location = SafeAllocString(chansub->item);
    newchan->channel = SafeAllocString(chansub->next->item);

    if (newchain->tail_chan==NULL) newchain->root_chan = newchan;
    else	newchain->tail_chan->next = newchan;
    newchain->tail_chan = newchan;

    FreeItemList(chansub);

  }

  FreeItemList(chanlist);
}

PRIVATE VOID	_procdate(struct sel3_chain *newchain, char *instr)
{

  struct sel3_date *newdate;
  ITEMLIST *dates;
  char *datestr;
  char *ztime = "1990,1,00:00:00";
  char *itime = "9999,1,00:00:00";
  char *stime,*etime;
  BOOL dateok;
	
  newdate = SafeAlloc(sizeof(struct sel3_date));
  newdate->next = NULL;
		
  switch(instr[0]) {
  case '+':
    newdate->addnot = TRUE;
    datestr = &instr[1];
    break;
  case '-':
    printf("?Date Excludes not permitted (%s line %d)\n",
	   filename,gline);
    return;
    /*newdate->addnot = FALSE;
      datestr = &instr[1];
      break;*/
  default:
    newdate->addnot = TRUE;
    datestr = &instr[0];
  }

  dates = MakeItemList(datestr,'/',TRUE,TRUE);
	
  dateok = FALSE;
  if (dates)
    if (dates->next)
      if (dates->next->next==NULL)
	dateok = TRUE;


  if (!dateok) {
    printf("?Bad date range %s (%s line %d)\n",
	   datestr,filename,gline);
    return;
  }

  stime = dates->item;
  if (stime==NULL) stime = ztime;
  etime = dates->next->item;	
  if (etime==NULL) etime = itime;

  newdate->sdate = ST_ParseTime((UDCC_BYTE *)stime);
  newdate->edate = ST_ParseTime((UDCC_BYTE *)etime);

  if (newdate->sdate.year==0||newdate->edate.year==0) {
    printf("?Bad date range %s (%s line %d)\n",
	   datestr,filename,gline);
    return;
  }

  newdate->sjday = _julday(newdate->sdate.year,1,newdate->sdate.day);
  newdate->ejday = _julday(newdate->edate.year,1,newdate->edate.day);

  if (newchain->tail_date==NULL) newchain->root_date = newdate;
  else	newchain->tail_date->next = newdate;
  newchain->tail_date = newdate;

  FreeItemList(dates);

}

_SUB	VOID SelectLoadup(struct sel3_root *root,char *infilename)
{
	
  int a;
  FILE *infile;
  struct sel3_chain *newchain;

  filename = infilename;

  infile = fopen(filename,"r");
  if (infile==NULL) 
    bombout(EXIT_ABORT,"Cannot find sel file %s",filename);

  fprintf(stderr,"[Processing select file %s]\n",filename);

  while (!geof) {		/* Main loop */
    newchain = SafeAlloc(sizeof(struct sel3_chain));
    if (newchain==NULL) bombout(EXIT_INSFMEM,"sel3_chain");
    newchain->next = NULL;
    newchain->root_stat=newchain->tail_stat = NULL;
    newchain->root_chan=newchain->tail_chan = NULL;
    newchain->root_date=newchain->tail_date = NULL;
    newchain->taglist = NULL;

    if (root->tail_chain==NULL)	root->root_chain = newchain;
    else root->tail_chain->next = newchain;
    root->tail_chain = newchain;

    while (!geof) {
      a = SelectRline(infile);
      if (a== 0) continue;
      if (a== -1) break;
      if (a== -2) break;

      if (strncmp(gsel,"STAT=",5)==0) 
	_procstat(newchain,&gsel[5]);
      else if (strncmp(gsel,"CHAN=",5)==0) 
	_procchan(newchain,&gsel[5]);
      else if (strncmp(gsel,"DATE=",5)==0) 
	_procdate(newchain,&gsel[5]);
      else if (strncmp(gsel,"TIME=",5)==0) 
	_procdate(newchain,&gsel[5]);
      else if (strncmp(gsel,"TAG=",4)==0) 
	_proctag(newchain,&gsel[4]);

    }
  }
	
  fclose(infile);

  /*listab();*/
}

PRIVATE	BOOL _chanmatch(struct sel3_chan *inchan, char *location,char *channel)
{

  if (inchan->location==NULL)
    if (location!=NULL) return(FALSE);

  if (location==NULL) location="";

  if (!WildMatch(inchan->location,location)) return(FALSE);

  return(WildMatch(inchan->channel,channel));
}

_SUB BOOL SelectStatInteresting(struct sel3_root *root,
				char *network,char *station,
				char *location,char *channel,
				ITEMLIST **taglist)
{

  struct sel3_chain *loopchain;
  struct sel3_stat *loopstat;
  struct sel3_chan *loopchan;
  char net[20];
  char loc[20];
  char chan[20];
  char stat[20];

  BOOL go=FALSE;

  /* Set tag to nulls */

  if (taglist!=NULL) *taglist = NULL;

  /* Clean up the incoming station/location/channel names */

  net[0] = '\0';
  loc[0] = '\0';
  chan[0] = '\0';
  stat[0] = '\0';

  if (network) strcpy(net,network);
  if (station) strcpy(stat,station);
  if (location) strcpy(loc,location);
  if (channel) strcpy(chan,channel);

  network = net;
  station = stat;
  channel = chan;
  location = loc;

  if (net[0]=='\0') network = NULL;
  if (stat[0]=='\0') station = NULL;
  if (chan[0]=='\0') channel = NULL;
  if (loc[0]=='\0') location = NULL;

  for (loopchain=root->root_chain; loopchain!=NULL; 
       loopchain=loopchain->next) {

    if (taglist!=NULL) *taglist = loopchain->taglist;

    go = FALSE;

    for (loopstat=loopchain->root_stat; loopstat!=NULL;
	 loopstat=loopstat->next) {

      if (WildMatch(loopstat->station, stat) &&
	  WildMatch(loopstat->network, net)) 
	go = loopstat->addnot;		

    }

    if (!go) continue;

    if (channel==NULL) return(TRUE); /* Don't care bout chans */

    go = FALSE;

    for (loopchan=loopchain->root_chan; loopchan!=NULL;
	 loopchan=loopchan->next) {

      if (_chanmatch(loopchan,location,channel)) 
	go = loopchan->addnot;

    }

    if (go) return(TRUE);

  }

  return(go);
}

_SUB BOOL SelectChainInteresting(struct sel3_chain *root,
				 char *network,char *station,
				 char *location,char *channel,
				 ITEMLIST **taglist)
{

  struct sel3_chain *loopchain;
  struct sel3_stat *loopstat;
  struct sel3_chan *loopchan;
  char net[20];
  char loc[20];
  char chan[20];
  char stat[20];

  BOOL go=FALSE;

  /* Set tag to nulls */

  if (taglist!=NULL) *taglist = NULL;

  /* Clean up the incoming station/location/channel names */

  net[0] = '\0';
  loc[0] = '\0';
  chan[0] = '\0';
  stat[0] = '\0';

  if (network) strcpy(net,network);
  if (station) strcpy(stat,station);
  if (location) strcpy(loc,location);
  if (channel) strcpy(chan,channel);

  network = net;
  station = stat;
  channel = chan;
  location = loc;

  if (net[0]=='\0') network = NULL;
  if (stat[0]=='\0') station = NULL;
  if (chan[0]=='\0') channel = NULL;
  if (loc[0]=='\0') location = NULL;

  loopchain=root;

  if (taglist!=NULL) *taglist = loopchain->taglist;

  go = FALSE;

  for (loopstat=loopchain->root_stat; loopstat!=NULL;
       loopstat=loopstat->next) {

    if (WildMatch(loopstat->station, stat) &&
	WildMatch(loopstat->network, net)) 
      go = loopstat->addnot;		
	  
  }

  if (!go) return(FALSE);

  if (channel==NULL) return(TRUE); /* Don't care bout chans */

  go = FALSE;

  for (loopchan=loopchain->root_chan; loopchan!=NULL;
       loopchan=loopchan->next) {
	  
    if (_chanmatch(loopchan,location,channel)) 
      go = loopchan->addnot;

  }

  return(go);
}

_SUB BOOL SelectDateInteresting(struct sel3_root *root,
				STDTIME btime,STDTIME etime,
				char *network,char *station,
				char *location,char *channel,
				ITEMLIST **taglist)
{

  struct sel3_chain *loopchain;
  struct sel3_stat *loopstat;
  struct sel3_chan *loopchan;
  struct sel3_date *loopdate;

  char net[20];
  char loc[20];
  char chan[20];
  char stat[20];

  BOOL go=FALSE;

  /* Set tag to nulls */

  if (taglist!=NULL) *taglist = NULL;

  /* Clean up the incoming station/location/channel names */

  net[0] = '\0';
  loc[0] = '\0';
  chan[0] = '\0';
  stat[0] = '\0';
	
  if (network) strcpy(net,network);
  if (station) strcpy(stat,station);
  if (location) strcpy(loc,location);
  if (channel) strcpy(chan,channel);

  network = net;
  station = stat;
  channel = chan;
  location = loc;

  if (net[0]=='\0') network = NULL;
  if (stat[0]=='\0') station = NULL;
  if (chan[0]=='\0') channel = NULL;
  if (loc[0]=='\0') location = NULL;

  for (loopchain=root->root_chain; loopchain!=NULL; 
       loopchain=loopchain->next) {

    if (taglist!=NULL) *taglist = loopchain->taglist;

    go = FALSE;

    if (loopchain->root_date == NULL)
      go = TRUE;
    else
    {
      for (loopdate=loopchain->root_date; loopdate!=NULL; 
           loopdate=loopdate->next) {
        
        if (ST_TimeComp(btime,loopdate->edate)<=0&&
            ST_TimeComp(etime,loopdate->sdate)>=0)
          go = loopdate->addnot;
      }
    }
    
    if (!go) continue;

    go = FALSE;

    for (loopstat=loopchain->root_stat; loopstat!=NULL;
	 loopstat=loopstat->next) {

      if (WildMatch(loopstat->station, stat)&&
	  WildMatch(loopstat->network, net)) 
	go = loopstat->addnot;		

    }

    if (!go) continue;

    if (channel==NULL) return(TRUE); /* Don't care bout chans */

    go = FALSE;

    for (loopchan=loopchain->root_chan; loopchan!=NULL;
	 loopchan=loopchan->next) {

      if (_chanmatch(loopchan,location,channel))
	go = loopchan->addnot;

    }

    if (go) return(TRUE);

  }

  return(go);
}
