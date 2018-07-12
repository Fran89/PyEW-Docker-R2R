/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: chansubs.c 4530 2011-08-10 01:24:19Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:07:35  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <dcc_misc.h>
#include <stationidx.h>

_SUB void UnpackChanNames(char *ins,char *inl,char *inc,
			  char *outs,char *outl,char *outc)
{

  char *wrt,chr;
  int i;

  if (ins==NULL) ins="";
  if (inl==NULL) inl="";
  if (inc==NULL) inc="";

  if (outs!=NULL) {
    wrt = outs;
    for (i=0; i<5; i++) {
      chr = ins[i];
      if (chr==' ') break;
      *wrt++ = chr;
    }
    *wrt++ = '\0';	
  }

  if (outl!=NULL) {
    wrt = outl;
    for (i=0; i<2; i++) {
      chr = inl[i];
      if (chr==' ') break;
      *wrt++ = chr;
    }
    *wrt++ = '\0';	
  }

  if (outc!=NULL) {
    wrt = outc;
    for (i=0; i<3; i++) {
      chr = inc[i];
      if (chr==' ') break;
      *wrt++ = chr;
    }
    *wrt++ = '\0';	
  }

}

_SUB void PackChanNames(char *ins,char *inl,char *inc,
			char *outs,char *outl,char *outc)
{

  char *wrt,chr;
  int i;

  if (ins==NULL) ins="";
  if (inl==NULL) inl="";
  if (inc==NULL) inc="";

  if (outs!=NULL) {
    wrt = outs;
    for (i=0; i<5; i++) outs[i] = ' ';
    for (i=0; i<5; i++) {
      chr = ins[i];
      if (chr=='\0') break;
      *wrt++ = chr;
    }
  }

  if (outl!=NULL) {
    wrt = outl;
    for (i=0; i<2; i++) outl[i] = ' ';
    for (i=0; i<2; i++) {
      chr = inl[i];
      if (chr=='\0') break;
      *wrt++ = chr;
    }
  }

  if (outc!=NULL) {
    wrt = outc;
    for (i=0; i<3; i++) outc[i] = ' ';
    for (i=0; i<3; i++) {
      chr = inc[i];
      if (chr=='\0') break;
      *wrt++ = chr;
    }
  }

}

/* Take packed names, make nice printable channel name with location */

LOCAL char channame[10];

_SUB char *FullChanName(char *inl,char *inc)
{

  char outl[5],outc[10];

  UnpackChanNames(NULL,inl,inc,NULL,outl,outc);

  if (outl[0]=='\0')	strcpy(channame,outc);
  else			sprintf(channame,"%s/%s",outl,outc);

  return(channame);

}

/* Set of subroutines for user to set up channel tables and search them */

LOCAL struct channel_list {
  struct	channel_list	*next;
  VOID 	*userptr;
  char	seedkey[11];
} *chan_root=NULL,*chan_tail=NULL;

_SUB void InitChanTables()
{

  chan_root = chan_tail = NULL;
}

_SUB void *FindChannel(char *ins,char *inl,char *inc)
{

  char outs[10],outl[10],outc[10];
  char keypack[11];
  struct channel_list *looper;

  UnpackChanNames(ins,inl,inc,outs,outl,outc);

  PackChanNames(outs,outl,outc,&keypack[0],&keypack[5],&keypack[7]);
  keypack[10] = '\0';

  for (looper = chan_root; looper!=NULL; looper=looper->next) 

    if (streq(looper->seedkey,keypack)) return(looper->userptr);

  return(NULL);
}

_SUB void MakeChanKey(struct station_index *index,char *outkey)
{

  char outs[10],outl[10],outc[10];
  char keypack[11];

  UnpackChanNames(index->SI_station,
		  index->SI_location,
		  index->SI_channel,
		  outs,outl,outc);

  PackChanNames(outs,outl,outc,&keypack[0],&keypack[5],&keypack[7]);
  keypack[10] = '\0';

  strcpy(outkey,keypack);

}

_SUB BOOL ChanKeyEq(struct station_index *index,char *chkkey)
{

  char keybak[20],idxkey[20],typ;

  strcpy(keybak,chkkey);

  MakeChanKey(index,idxkey);

  if (index->SI_channel[2]==' ') { /* Is muxed station */
    typ = keybak[9];		/* If mux, make it match */
    if (strchr("ZNE",typ)!=NULL) keybak[9] = ' ';
  }

  return(streq(keybak,idxkey));

}

_SUB void AddChannel(char *ins,char *inl,char *inc,void *user)
{

  char outs[10],outl[10],outc[10];
  char keypack[11];
  struct channel_list *new;

  UnpackChanNames(ins,inl,inc,outs,outl,outc);

  PackChanNames(outs,outl,outc,&keypack[0],&keypack[5],&keypack[7]);
  keypack[10] = '\0';

  new = (struct channel_list *) malloc(sizeof (struct channel_list));
  if (new==NULL) bombout(EXIT_NOMEM,"AddChannel");

  new->next = NULL;
  new->userptr = user;
  strcpy(new->seedkey,keypack);

  if (chan_tail==NULL)	chan_root = new;

  else	chan_tail->next = new;

  chan_tail = new;

}

LOCAL struct channel_list *travloop=NULL;

_SUB void *StartChannelTrav()
{

  VOID *ptr;

  travloop = chan_root;

  ptr = NULL;
  if (travloop!=NULL) ptr = travloop->userptr;
  return(ptr);
}

_SUB void *TraverseChannels()
{

  VOID *ptr;

  if (travloop==NULL) return(NULL);

  travloop = travloop->next;

  ptr = NULL;
  if (travloop!=NULL) ptr = travloop->userptr;
  return(ptr);
}

_SUB void ParseChanString(char *InBuff,char *defstation,char *statname,
			  char *location,char *channel)
{

  char *ib,*sc;

  channel[0] = location[0] = statname[0] = '\0';

  Upcase(InBuff);

  sc = strrchr(InBuff,'/');
  if (sc==NULL) ib= InBuff;
  else ib = sc+1;
  strcpy(channel,ib);
	
  if (sc!=NULL) {
    *sc = '\0';

    sc = strrchr(InBuff,'/');
    if (sc==NULL) ib= InBuff;
    else ib = sc+1;
    strcpy(location,ib);

    if (sc!=NULL) {
      *sc = '\0';

      sc = strrchr(InBuff,'/');
      if (sc==NULL) ib= InBuff;
      else ib = sc+1;
      strcpy(statname,ib);
    }
  }

  if (statname[0]=='\0') strcpy(statname,defstation);

}

_SUB BOOL ChanNamesEq(struct station_index *idx1,
		      struct station_index *idx2)
{

  UDCC_BYTE chk1[20],chk2[20];

  MakeChanKey(idx1,(char *)chk1);
  MakeChanKey(idx2,(char *)chk2);

  return(streq((char *)chk1,(char *)chk2));
}
