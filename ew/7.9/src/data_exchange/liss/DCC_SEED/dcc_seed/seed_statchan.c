/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_statchan.c 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:06:20  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <string.h>
#include <dcc_time.h>
#include <dcc_misc.h>
#include <dcc_seed.h>

/*
   
   Malloc a STATION_LIST entry and place in linked list 
   ----STATION_LIST *CreateStation(Network,Station)
   
   Malloc a STATION_ENTRY entry, initialize values and return
   ----STATION_ENTRY *InitStationEntry()
   
   Breaks all STATION_TIMES entry for this station at the date (if necessary)
   ----VOID BreakStationTimes(STATION_LIST *instation, STDTIME timetobreak)
   
   Malloc a STATION_TIMES entry, link into list by date, return status 
   ----STATION_TIMES *LinkInStation(STATION_LIST *instation,
				    STDTIME start, STDTIME end)
				    
   Find the STATION_LIST record for your station
   ----STATION_LIST *SearchStation(char *net, char *stat)
			    
   Find the STATION_TIMES record which includes your time
   ----STATION_TIMES *SearchStationTime(STATION_LIST *instat, STDTIME find)
				    
   Malloc a CHANNNEL_LIST entry and place in linked list 
   ----CHANNEL_LIST *CreateChannel(STATION_LIST *stat, char *loc, char *chan)
				    
   Malloc a CHANNEL_ENTRY, initialize values and return
   ----CHANNEL_ENTRY *InitChannelEntry()
				    
   Break all CHANNEL_TIMES entry at given time (if necessary)
   ----VOID BreakChannelTimes(CHANNEL_LIST *inchannel, STDTIME timetobreak)
				    
   Malloc a CHANNEL_TIMES entry, link into list by date, and return status
   ----CHANNEL_TIMES *LinkInChannel(CHANNEL_LIST *chan, 
			     STDTIME start, STDTIME end)
							     
   Find the CHANNEL_LIST record for your channel
   ----CHANNEL_LIST *SearchChannel(STATION_LIST *stat, char *loc, char *chan)
								     
   Find the CHANNEL_TIMES record for your time
   ----CHANNEL_TIMES *SearchChannelTime(CHANNEL_LIST *chan, STDTIME find)

   Set the Modified dates for CHANNEL_TIMES entries within the range
   ---VOID ChannelModifiedDate(CHANNEL_LIST *chan, STDTIME mod, 
                               STDTIME start, STDTIME end)
								     
*/


/**************************Station Table Support*****************************/

/*
 *	
 *	Search the station table for the given key
 *	
 */

_SUB STATION_LIST *SearchStation(char *innet,char *instat)
{

  STATION_LIST *looper;

  for (looper=VI->Root_Station; looper!=NULL; looper=looper->Next)
    if (streq(looper->Network,FixKey(innet))&&
	streq(looper->Station,FixKey(instat))) return(looper);

  return(NULL);			/* Not Found */
}

/*
 *
 *	Search the time table for a given time
 *
 */

_SUB STATION_TIMES *SearchStationTime(STATION_LIST *instation, 
				      STDTIME findtime)
{

  STATION_TIMES *looper;

  /* Skip down list till time is bracketed by times in current entry */

  for (looper=instation->Root_Time; looper!=NULL; looper=looper->Next) {
    if (ST_TimeComp(findtime, looper->Effective_End) > 0)
      continue;			/* Time is later, skip down to it */
    if (ST_TimeComp(findtime, looper->Effective_Start) < 0)
      return(NULL);		/* Time was before list or was
				   skipped over by list */
    return(looper);
  }

  return(NULL);			/* Time past end of last entry */

}

/*
 *	
 *	Malloc a new station structure and initialize it 
 *	
 */

_SUB STATION_ENTRY *InitStationEntry()
{

  STATION_ENTRY *Station;

  Station = (STATION_ENTRY *) SafeAlloc(sizeof (STATION_ENTRY));

  Station->Site_Name = NULL;
  Station->Station_Owner = NULL;

  Station->Latitude = 0;
  Station->Lat_Prec = 4;
  Station->Longitude = 0;
  Station->Long_Prec = 4;
  Station->Coord_Type = NULL;
  Station->Coord_Map = NULL;
  Station->Elevation = 0;
  Station->Elev_Prec = 0;
  Station->Elev_Type = NULL;
  Station->Elev_Map = NULL;

  Station->Established = ST_Zero();
  Station->Long_Order = 0;
  Station->Word_Order = 0;

  Station->Clock_Type = NULL;
  Station->Station_Type = NULL;
  Station->Successor_Net = NULL;
  Station->Successor_Station = NULL;

  Station->Software_Revs = NULL;
  Station->Vault_Cond = NULL;

  Station->Desc_Type = NULL;
  Station->Description = NULL;

  return(Station);
}

/*
 *	
 *	Add a new station to the link list in alphabetical order
 *      Add Networking code 2-Jun-94/SH
 *	
 */

_SUB STATION_LIST *CreateStation(char *net, char *stat)
{

  int cm;
  STATION_LIST *instation,*looper,*last;

  /* Create the new STATION_LIST entry */

  instation = (STATION_LIST *)
    SafeAlloc(sizeof (STATION_LIST));

  /* Populate new entry (insure uppercase values) */

  instation->Network = SafeAllocString(net);
  Upcase(instation->Network);
  instation->Station = SafeAllocString(stat);
  Upcase(instation->Station);

  instation->Root_Channel =
    instation->Tail_Channel = NULL;

  instation->Root_Time =
    instation->Tail_Time = NULL;

  instation->Root_Comment = 
    instation->Tail_Comment = NULL;
	
  instation->Next = NULL;

  /* Search down to find location where entry will be inserted */

  last = NULL;
  for (looper=VI->Root_Station; looper!=NULL; 
       last=looper,looper=looper->Next) {

    cm = strcmp(looper->Network,instation->Network);
    if (cm==0)			/* Same - so compare stations */
      cm = strcmp(looper->Station,instation->Station);
    if (cm==0) return(NULL);	/* Key already here! */
    if (cm<0) continue;		/* Key greater... */
    break;			/* Key is less */
  }

  /* Insert into list */

  if (looper==NULL) {		/* Add to end of list */
    if (VI->Tail_Station==NULL) 
      VI->Root_Station = VI->Tail_Station = instation;
    else {
      VI->Tail_Station->Next = instation;
      VI->Tail_Station = instation;
    }
    instation->Next = NULL;
  } else if (last==NULL) {	/* Insert to top of list */
    instation->Next = VI->Root_Station;
    VI->Root_Station = instation;
  } else {			/* Link into middle of list */
    last->Next = instation;
    instation->Next = looper;
  }		

  /* Return success */

  return(instation);
}


/*
   Breaks all STATION_TIMES entry for this station at the date (if necessary)
   
   Search through given STATION_TIMES list looking for times which
   contain 'timetobreak'.  LinkInStation will assure that there are no
   overlapping times, so we will return after splitting the first entry;

*/

_SUB VOID BreakStationTimes(STATION_LIST *instation, STDTIME timetobreak)
{

  STATION_TIMES *stattim1,*stattim2;
  int t;

  /* Skip down list till time is bracketed by times in current entry */

  for (stattim1=instation->Root_Time; 
       stattim1!=NULL; stattim1=stattim1->Next) {

    t = ST_TimeComp(timetobreak, stattim1->Effective_End);

    if (t == 0) 
      return;			/* matched end time, no break needed */
    if (t > 0) 
      continue;			/* Time is later, skip down to it */

    t = ST_TimeComp(timetobreak, stattim1->Effective_Start);

    if (t <= 0) 
      return;			/* match start time, or not in list */

    break;
  }

  if (stattim1==NULL) return;	/* Time past end of last entry */

  /* Allocate a new entry and copy the old into it */

  stattim2 = (STATION_TIMES *) SafeAlloc(sizeof (STATION_TIMES));

  *stattim2 = *stattim1;

  /* Fix the effective time brackets */

  stattim1->Effective_End = timetobreak;
  stattim2->Effective_Start = timetobreak;

  /* Insert 'stattim2' into the linked list after 'stattim1' */

  stattim2->Next = NULL;

  if (stattim1->Next==NULL) {
    stattim1->Next = stattim2;
    instation->Tail_Time = stattim2;
  } else {
    stattim2->Next = stattim1->Next;
    stattim1->Next = stattim2;
  }
}

/*
 *   Malloc a STATION_TIMES entry, link into list by date, return status 
 */

_SUB STATION_TIMES *LinkInStation(STATION_LIST *instation,
				  STDTIME start, STDTIME end)
{

  STATION_TIMES *newrec,*statloop,*statloopnext,*statloopprev;

  /* Create a new STATION_TIMES record and pre-initialize it */

  newrec = (STATION_TIMES *) SafeAlloc(sizeof(STATION_TIMES));

  newrec->Effective_Start = start;
  newrec->Effective_End = end;
  newrec->Last_Modified = ST_Zero();
  newrec->Update_Flag = 'N';

  newrec->Station = NULL;
  
  newrec->Root_Data = 
    newrec->Tail_Data = NULL;

  newrec->Next = NULL;

  /* Break all stations at our new start and ending times -- this will
     assure that we can simply eliminate and copy below, all splitting 
     will already be done for us */

  BreakStationTimes(instation, start);
  BreakStationTimes(instation, end);
  
  /* Go again and destroy all of the STATION_TIMES and their CHANNEL_LISTs */

  for (statloopprev=NULL,statloop=instation->Root_Time; statloop!=NULL;
       statloopprev=statloop,statloop=statloopnext) {

    statloopnext = statloop->Next;

    if (ST_TimeSpan(start,
		    end,
		    statloop->Effective_Start,
		    statloop->Effective_End,
		    NULL,
		    NULL)!=ST_SPAN_NIL) {

      /* Kill the STATION_TIMES and unlink it */

      if (statloop==instation->Root_Time) {
	instation->Root_Time = statloopnext;
	if (!statloopnext)
	  instation->Tail_Time = NULL;
      } else
	if (!statloopnext) {
	  /* statloopprev should never be null here */
	  statloopprev->Next = NULL;
	  instation->Tail_Time = statloopprev;
	} else
	  statloopprev->Next = statloop->Next;

      SafeFree(statloop);

    }
  }
  
  /* Link in our new STATION_TIMES record */

  for (statloopprev=NULL,statloop=instation->Root_Time; statloop!=NULL;
       statloopprev=statloop,statloop=statloop->Next)  {

    if (ST_TimeComp(start,statloop->Effective_Start) < 0) break;

  }
   
  /* Attach to end */

  if (statloop==NULL) {
    if (!instation->Root_Time) 
      instation->Root_Time = 
	instation->Tail_Time = newrec;
    else {
      instation->Tail_Time->Next = newrec;
      instation->Tail_Time = newrec;
    }
  } else
    /* Attach inline */
    if (!statloopprev) {
      newrec->Next = instation->Root_Time;
      instation->Root_Time = newrec;
    } else {
      statloopprev->Next = newrec;
      newrec->Next = statloop;
    }

  /* return sucessful outcome */

  return(newrec);
}
	

/**********************Process Channel Lists*************************/

/*
 *	
 *	Search the channel table for the given key
 *	
 */

_SUB CHANNEL_LIST *SearchChannel(STATION_LIST *station,char *loc,char *chan)
{

  CHANNEL_LIST *looper;
  char *tloc;

  if (chan==NULL) return(NULL);
  if (station==NULL) return(NULL);
  
  if (loc) {
    if (streq(loc,"  ")) loc = "";
    if (streq(loc,"__")) loc = "";
  } else loc = "";

  for (looper=station->Root_Channel; looper!=NULL; looper=looper->Next) {
    tloc = looper->Location;
    if (!tloc) tloc = "";

    if (!streq(tloc,FixKey(loc))) continue;
    if (streq(looper->Identifier,FixKey(chan))) return(looper);
  }

  return(NULL);			/* Not Found */
}

/*
 * SearchChannelTime - Search channel for time
 */

_SUB CHANNEL_TIMES *SearchChannelTime(CHANNEL_LIST *inchannel, 
				      STDTIME findtime)
{

  CHANNEL_TIMES *looper;

  /* Skip down list till time is bracketed by times in current entry */

  for (looper=inchannel->Root_Time; looper!=NULL; looper=looper->Next) {
    if (ST_TimeComp(findtime, looper->Effective_End) > 0)
      continue;			/* Time is later, skip down to it */
    if (ST_TimeComp(findtime, looper->Effective_Start) < 0)
      return(NULL);		/* Time was before list or was
				   skipped over by list */
    return(looper);
  }

  return(NULL);			/* Time past end of last entry */

}

/*
 *
 *   Malloc a CHANNNEL_LIST entry and place in linked list 
 * 
 */

_SUB CHANNEL_LIST *CreateChannel(STATION_LIST *stat, char *loc, char *chan)
{

  CHANNEL_LIST *newchan,*loopchan,*lastchan;
  int t;

  /* Create new channel record */

  newchan = (CHANNEL_LIST *) SafeAlloc(sizeof(CHANNEL_LIST));
  
  if (loc&&streq(loc,"__")) loc = NULL;
  if (loc&&streq(loc,"  ")) loc = NULL;
  if (loc&&streq(loc," ")) loc = NULL;
  if (loc&&loc[0]=='\0') loc = NULL;

  if (loc) {
    newchan->Location = SafeAllocString(loc);
    Upcase(newchan->Location);
  }
  else 
    newchan->Location = NULL;

  newchan->Identifier = SafeAllocString(chan);
  Upcase(newchan->Identifier);

  newchan->SubChannel = 0;

  newchan->Root_Time = 
    newchan->Tail_Time = NULL;

  newchan->Root_Comment = 
    newchan->Tail_Comment = NULL;

  newchan->Root_Data = 
    newchan->Tail_Data = NULL;

  newchan->Use_Count = 0;
  newchan->Next = NULL;

  /* Find out where the channel would appear in the list */

  for (lastchan=NULL, loopchan=stat->Root_Channel; 
       loopchan!=NULL; 
       lastchan=loopchan, loopchan=loopchan->Next) {

    t = ChanComp(loopchan, newchan);
    if (t == 0) 
      return(NULL);      /* Channel already here!! */
    if (t<0) 
      continue;          /* Skip down */
    break;
  }

  /* Add Channel to linked list */

  if (!loopchan) {  /* At End? */
    if (stat->Tail_Channel)
      stat->Tail_Channel->Next = newchan;
    else
      stat->Root_Channel = newchan;
    stat->Tail_Channel = newchan;
  } 
  else /* In Middle? */
    if (lastchan) {
      lastchan->Next = newchan;
      newchan->Next = loopchan;
    }
  else {  /* At Top? */
    newchan->Next = stat->Root_Channel;
    stat->Root_Channel = newchan;
  }

  return(newchan);

}
				    

/*
 *	
 *	Malloc a new channel structure and initialize it 
 *	
 */

_SUB CHANNEL_ENTRY *InitChannelEntry()
{

  CHANNEL_ENTRY *Channel;

  Channel = (CHANNEL_ENTRY *) SafeAlloc(sizeof (CHANNEL_ENTRY));

  Channel->Instrument = NULL;
  Channel->Optional = NULL;
  Channel->Signal_Response = NULL;
  Channel->Calibration_Input = NULL;

  Channel->Coord_Set = 0;
  Channel->Latitude = 0;
  Channel->Lat_Prec = 4;
  Channel->Longitude = 0;
  Channel->Long_Prec = 4;
  Channel->Coord_Type = NULL;
  Channel->Coord_Map = NULL;
  Channel->Elevation = 0;
  Channel->Elev_Prec = 0;
  Channel->Elev_Type = NULL;
  Channel->Elev_Map = NULL;

  Channel->Local_Depth = 0;
  Channel->Azimuth = 0;
  Channel->Dip = 0;
  Channel->Format_Type = 0;
  Channel->Data_Exp = 0;
  Channel->Sample_Rate = 0;
  Channel->Max_Drift = 0;
  Channel->Channel_Flags = 0;

  Channel->Clock_Type = NULL;
  Channel->Digitizer_Type = NULL;

  Channel->Derived_Location = NULL;
  Channel->Derived_Channel = NULL;

  Channel->Calibration_Location = NULL;
  Channel->Calibration_Channel = NULL;

  Channel->Desc_Type = NULL;
  Channel->Description = NULL;

  return(Channel);
}

/*
 *	
 *	Format nice name from channel 
 *	
 */

_SUB VOID ChanText(CHANNEL_LIST *chan,char *buf)
{

  if (chan->Location!=NULL) 
    sprintf(buf,"%s/%s",chan->Location,chan->Identifier);
  else	sprintf(buf,"%s",chan->Identifier);

}

/*
 *	
 *	Channel name comparisons (with ZNE order)
 *	
 */

VOID _putdes(char *buf,int pos,char *search,char ch)
{

  char *new;

  buf[pos] = '_';

  new = strchr(search,ch);
  if (new==NULL) {
    buf[pos] = '*';
    buf[pos+1] = ch;
    return;
  }

  buf[pos+1] = 'A' + (new - search);

}

char *_sortkey[3] = { "HBESMLVURAX","HLGTABSMCX","ZNEABCTRI123X"};
VOID _ugname(CHANNEL_LIST *chan,char *buf)
{

  char newident[20];
  int i;

  for (i=0; i<8; i++) newident[i] = '\0';	
  for (i=0; i<6; i+=2) 
    if (chan->Identifier[i>>1]=='\0') break;
    else	_putdes(newident,i,_sortkey[i>>1],
			chan->Identifier[i>>1]);

  if (chan->Location!=NULL) 
    sprintf(buf,"%s/%s",chan->Location,newident);
  else	sprintf(buf,"%s",newident);

}

_SUB BOOL ChanComp(CHANNEL_LIST *chan1,CHANNEL_LIST *chan2)
{

  char name1[20],name2[20];

  if (chan1->Location==NULL&&chan2->Location!=NULL) return(-1);
  if (chan1->Location!=NULL&&chan2->Location==NULL) return(1);

  _ugname(chan1,name1);
  _ugname(chan2,name2);

  return(strcmp(name1,name2));
}

/* 
 *
 *  BreakChannel - Duplicate any time spans seed with break time
 *
 */

_SUB void BreakChannelTimes(CHANNEL_LIST *inchannel, STDTIME timetobreak)
{

  CHANNEL_TIMES *chantim1,*chantim2;
  int t;

  /* Skip down list till time is bracketed by times in current entry */

  for (chantim1=inchannel->Root_Time; 
       chantim1!=NULL; chantim1=chantim1->Next) {

    t = ST_TimeComp(timetobreak, chantim1->Effective_End);

    if (t == 0) 
      return;			/* matched end time, no break needed */
    if (t > 0) 
      continue;			/* Time is later, skip down to it */

    t = ST_TimeComp(timetobreak, chantim1->Effective_Start);

    if (t <= 0) 
      return;			/* match start time, or not in list */

    break;
  }

  if (chantim1==NULL) return;	/* Time past end of last entry */

  /* Allocate a new entry and copy the old into it */

  chantim2 = (CHANNEL_TIMES *) SafeAlloc(sizeof (CHANNEL_TIMES));

  *chantim2 = *chantim1;

  /* Fix the effective time brackets */

  chantim1->Effective_End = timetobreak;
  chantim2->Effective_Start = timetobreak;

  /* Copy the responses from from chantim1 to chantim2 */

  chantim2->Root_Response = 
    chantim2->Tail_Response = NULL;

  CopyResponses(chantim1->Root_Response,
		&chantim2->Root_Response,
		&chantim2->Tail_Response);

  /* Insert 'chantim2' into the linked list after 'chantim1' */

  chantim2->Next = NULL;

  if (chantim1->Next==NULL) {
    chantim1->Next = chantim2;
    inchannel->Tail_Time = chantim2;
  } else {
    chantim2->Next = chantim1->Next;
    chantim1->Next = chantim2;
  }
}

/*
 *	
 *  Malloc a CHANNEL_TIMES entry, link into list by date, and return status
 *	
 */


_SUB CHANNEL_TIMES *LinkInChannel(CHANNEL_LIST *inchannel,
				  STDTIME start, STDTIME end)
{

  CHANNEL_TIMES *newrec,*chanloop,*chanloopnext,*chanloopprev;

  /* Split channels at the times specified */

  BreakChannelTimes(inchannel, start);
  BreakChannelTimes(inchannel, end);
  
  /* Create the new structure and populate it */

  newrec = (CHANNEL_TIMES *) SafeAlloc(sizeof(CHANNEL_TIMES));

  newrec->Effective_Start = start;
  newrec->Effective_End = end;
  newrec->Last_Modified = ST_Zero();
  newrec->Update_Flag = 'N';
  
  newrec->Channel = NULL;

  newrec->Root_Response = 
    newrec->Tail_Response = NULL;

  newrec->Next = NULL;

  /* Search and destroy all conflicting CHANNEL_TIMES */

  for (chanloopprev=NULL,chanloop=inchannel->Root_Time; chanloop!=NULL;
       chanloopprev=chanloop,chanloop=chanloopnext) {

    chanloopnext = chanloop->Next;

    if (ST_TimeSpan(start,
		    end,
		    chanloop->Effective_Start,
		    chanloop->Effective_End,
		    NULL,
		    NULL)!=ST_SPAN_NIL) {

      /* Kill the CHANNEL_TIMES and unlink it */

      if (chanloop==inchannel->Root_Time) {
	inchannel->Root_Time = chanloopnext;
	if (!chanloopnext)
	  inchannel->Tail_Time = NULL;
      } else
	if (!chanloopnext) {
	  /* chanloopprev should never be null here */
	  chanloopprev->Next = NULL;
	  inchannel->Tail_Time = chanloopprev;
	} else
	  chanloopprev->Next = chanloop->Next;

      SafeFree(chanloop);

    }
  }
  
  /* Link in our new CHANNEL_TIMES record */

  for (chanloopprev=NULL,chanloop=inchannel->Root_Time; chanloop!=NULL;
       chanloopprev=chanloop,chanloop=chanloop->Next)  {

    if (ST_TimeComp(start,chanloop->Effective_Start) < 0) break;

  }
   
  /* Attach to end */

  if (chanloop==NULL) {
    if (!inchannel->Root_Time) 
      inchannel->Root_Time = 
	inchannel->Tail_Time = newrec;
    else {
      inchannel->Tail_Time->Next = newrec;
      inchannel->Tail_Time = newrec;
    }
  } else
    /* Attach inline */
    if (!chanloopprev) {
      newrec->Next = inchannel->Root_Time;
      inchannel->Root_Time = newrec;
    } else {
      chanloopprev->Next = newrec;
      newrec->Next = chanloop;
    }

  /* return sucessful outcome */

  return(newrec);
}


/*
 *	
 *	Convert from ascii flag representation to bits
 *	
 */

_SUB UDCC_LONG CnvChanFlags(char *inflags)
{

  UDCC_LONG flags;
  int i;

  Upcase(inflags);

  flags = 0;

  for (i=0; i<strlen(inflags); i++)
    switch(inflags[i]) {
    case 'T':	flags |= CFG_TRIGGERED;		break;
    case 'C':	flags |= CFG_CONTINUOUS;	break;
    case 'H':	flags |= CFG_HEALTHCHAN;	break;
    case 'G':	flags |= CFG_GEODATA;		break;
    case 'W':	flags |= CFG_ENVIRON;		break;
    case 'F':	flags |= CFG_FLAGS;		break;
    case 'S':	flags |= CFG_SYNTH;		break;
    case 'I':	flags |= CFG_CALIN;		break;
    case 'E':	flags |= CFG_EXPERIMENT;	break;
    case 'M':	flags |= CFG_MAINTENANCE;	break;
    case 'B':	flags |= CFG_BEAM;		break;
    default:
      fprintf(stderr,"Illegal Channel Type Flag: %c (%s)\n",
	      inflags[i],
	      inflags);
    }

  return(flags);
}


/*
 *
 * Set the Modified dates for CHANNEL_TIMES entries within the range
 *
 */

_SUB VOID ChannelModifiedDate(CHANNEL_LIST *chan, STDTIME mod, 
			 STDTIME start, STDTIME end)
{
								     
  CHANNEL_TIMES *looper;

  /* Skip down list till time is bracketed by times in current entry */

  if (mod.year<1900 ||
      mod.year>3000) return;   /* Bogus dates */

  for (looper=chan->Root_Time; looper!=NULL; looper=looper->Next) 
    if (ST_TimeSpan(start,
		    end,
		    looper->Effective_Start,
		    looper->Effective_End,
		    NULL,
		    NULL)!=ST_SPAN_NIL) 
      if (ST_TimeComp(mod,looper->Last_Modified)>0)
	looper->Last_Modified = mod;
}
