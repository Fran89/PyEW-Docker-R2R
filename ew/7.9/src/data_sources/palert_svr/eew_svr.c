#include "eew.h"

//===================== SHM
struct PEEW *ptr; 
struct PEEWp *ptrp; 
//===================== Sanlien Added
int table_count;
char name_table[1000][8];
char station_table[1000][8];
double latitude_table[1000];
double longitude_table[1000];
double altitude_table[1000];
char s1[10];
uchar quit_char[] = {0,3,0,5,1,5,0,1};
//===================== else
//int NTP_counter[SOCKETS];
//char cmd[100];

//=====================================Earthworm

MSG_LOGO  GetLogo[MAXLOGO];    /* array for requesting module,type,instid */
short     nLogo;
pid_t     myPid;               /* for restarts by startstop               */

static char Buffer[BUF_SIZE];   /* character string to hold event message */
static      SHM_INFO  Region;      /* shared memory region to use for i/o    */
     

MSG_LOGO  Logo2;    /* array for requesting module,type,instid */
short     nLogo2;
static char Buffer2[BUF_SIZE];   /* character string to hold event message */
static      SHM_INFO  Region2;      /* shared memory region to use for i/o    */

   
/* Things to read or derive from configuration file
 **************************************************/
static char    RingName[MAX_RING_STR];        /* name of transport ring for i/o    */
static char    MyModName[MAX_MOD_STR];       /* speak as this module name/id      */
static int     LogSwitch;           /* 0 if no logfile should be written */
static long    HeartBeatInterval;   /* seconds between heartbeats        */

static char    RingName2[MAX_RING_STR];        /* name of transport ring for i/o    */

/* Things to look up in the earthworm.h tables with getutil.c functions
 **********************************************************************/
static long          RingKey;       /* key of transport ring for i/o     */
static unsigned char InstId;        /* local installation id             */
static unsigned char MyModId;       /* Module Id for this program        */
static unsigned char TypeHeartBeat; 
static unsigned char TypeError;
static unsigned char TypeHinvArc;
static unsigned char TypeH71Sum;

static long          RingKey2;       /* key of transport ring for i/o     */

/* Error messages used by template 
 *********************************/
static char  Text[150];        /* string for log/error messages          */
   
   time_t      timeNow;          /* current time                  */       
   time_t      timeLastBeat;     /* time last heartbeat was sent  */

   long      recsize;          /* size of retrieved message     */
   MSG_LOGO  reclogo;          /* logo of retrieved message     */
   int       res;
 
   long      recsize2;          /* size of retrieved message     */
   MSG_LOGO  reclogo2;          /* logo of retrieved message     */
   int       res2;

   char * lockfile;
   int lockfile_fd;


//char      outmsg_P[200], ccmd[300];

int check_packet_type();
int check_wave_packet();
int do_echo_none_wave( int skt );


void main ( int argc, char **argv )
{
	int err;
	int KEY = 22;
	int KEYp = 43;
	int shmidp;
	int shmid;
	int sizep;
	int size;
	int i;

	//for(i=0;i<SOCKETS;i++) NTP_counter[i]=0;

//-------------------------------------------------------------------Earthworm
/* Check command line arguments 
 ******************************/
   if ( argc != 2 )
   {
        fprintf( stderr, "Usage: template <configfile>\n" );
        exit( 0 );
   }
/* Initialize name of log-file & open it 
 ***************************************/
   logit_init( argv[1], 0, 256, 1 );
   
/* Read the configuration file(s)
 ********************************/
   template_config( argv[1] );
   logit( "" , "%s: Read command file <%s>\n", argv[0], argv[1] );

/* Look up important info from earthworm.h tables
 ************************************************/
   template_lookup();
 
printf("RingName: %s \n", RingName);
printf("MyModName: %s \n", MyModName);
printf("LogSwitch: %d \n", LogSwitch);
printf("HeartBeatInterval: %d \n", HeartBeatInterval);

printf("RingKey: %d \n", RingKey);

printf("RingName2: %s \n", RingName2);
printf("RingKey2: %d \n", RingKey2);

/* Reinitialize logit to desired logging level 
 **********************************************/
   logit_init( argv[1], 0, 256, LogSwitch );


   lockfile = ew_lockfile_path(argv[1]); 
   if ( (lockfile_fd = ew_lockfile(lockfile) ) == -1) {
	fprintf(stderr, "one  instance of %s is already running, exiting\n", argv[0]);
	exit(-1);
   }
/*
   fprintf(stderr, "DEBUG: for %s, fd=%d for %s, LOCKED\n", argv[0], lockfile_fd, lockfile);
*/

/* Get process ID for heartbeat messages */
   myPid = getpid();
   if( myPid == -1 )
   {
     logit("e","template: Cannot get pid. Exiting.\n");
     exit (-1);
   }

printf("======= myPid: %d \n", myPid);


   Logo2.type   = 114;
   Logo2.mod    = 1;
   Logo2.instid = 52;


/* Attach to Input/Output shared memory ring 
 *******************************************/
   tport_attach( &Region, RingKey );
   logit( "", "template: Attached to public memory region %s: %d\n", 
          RingName, RingKey );

/* Flush the transport ring */
   while( tport_getmsg( &Region, GetLogo, nLogo, &reclogo, &recsize, Buffer, 
			sizeof(Buffer)-1 ) != GET_NONE );

/* Attach to Input/Output shared memory ring2 
 *******************************************/
 //  tport_attach( &Region2, RingKey2 );
 //  logit( "", "template: Attached to public memory region %s: %d\n", 
 //         RingName2, RingKey2 );


/* Force a heartbeat to be issued in first pass thru main loop
 *************************************************************/
   timeLastBeat = time(&timeNow) - HeartBeatInterval - 1;

//---------------------------------------------------------------------

  	get_table();
	printf("\r\n\nEEW Server start.\r\n");

  
  //--- for initializing share memory
  size = 1000*sizeof(struct PEEW);
  shmid = shmget(KEY, size,  IPC_CREAT | 0600);
  ptr = (struct PEEW *)shmat(shmid, NULL, 0);
  printf("Share Memory   No.   %d \n", shmid);
  //--- end initializing share memory

  //--- for initializing share memory P arrival
  sizep = 1000*sizeof(struct PEEWp);
  shmidp = shmget(KEYp, sizep,  IPC_CREAT | 0600);
  ptrp = (struct PEEWp *)shmat(shmidp, NULL, 0);
  printf("Share Memory P No.   %d \n\n\n", shmidp);
  //--- for socket initial and accept client connection
  serv(502);

  //killsockets();
  printf("\n\nProgram end normally. \n\n");
}

void get_table()
{
	char s[60];
	FILE *fp;
	
	fp=fopen("location","r");
	if(fp==NULL) return;
  table_count=0;
  do{
  	fgets(s,60,fp);
    memset(name_table[table_count],0,8);
    strncpy(s1,s,7);
   	sprintf(name_table[table_count],"%i",atoi(s1));
    memset(station_table[table_count],0,8);
    strncpy(station_table[table_count],s+9,7);
    strncpy(s1,s+18,10);
    latitude_table[table_count]=atof(s1);
    strncpy(s1,s+29,10);
    longitude_table[table_count]=atof(s1);
    strncpy(s1,s+40,8);
    altitude_table[table_count]=atof(s1); 
    //printf("%s %s %3.6f %3.6f %3.2f  \n",name_table[table_count],station_table[table_count],latitude_table[table_count],longitude_table[table_count],altitude_table[table_count]);
    table_count++;
  }while(feof(fp)==0);
  if(strcmp(name_table[table_count-1],name_table[table_count-2])==0) table_count--;
  fclose(fp);

}


/*************************************************************************
 * TCP/IP
 * do_echo
 *
 * when client is connected, do echo here
 *************************************************************************/
int do_echo( int skt )
{
	int cc, err, i, pt;
	float wkf1, wkf2, wkf3, wkfp, wkftc;
  	double t_now,sec,shift_time;
  	int iy,im,id,ih,mi,jj,k;
	time_t timep;
	struct tm *p;
	struct timeval tv;
	struct timezone tz;
	int nt;
	char ss[8];

	char stname[20];
	//----- Earthworm	
	DDate 	 Date1;
	TRACE2_HEADER wfhead;
	int data[1000];
	//----- OUT MESSAGE to RING2
	int       lineLen;

	

	err =1;

	
	if(err <= 0)
	{

	}
	else 
	{

		wkf1 = buf_tcp.i[53]; wkf1 = wkf1 * 1960 / 32768;       //pa
		wkf2 = buf_tcp.i[52]; wkf2 /= 1000;			//pv
		wkf3 = buf_tcp.i[51]; wkf3 /= 1000;			//pd
    		time(&timep);
	  	p=localtime(&timep);
    		gettimeofday(&tv,NULL);	// get current time


    		shift_time = tv.tv_sec-make_mstime(1900+p->tm_year,1+p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
   		if(buf_tcp.i[2]==(1900+p->tm_year) && buf_tcp.i[3]==(1+p->tm_mon) && buf_tcp.i[4]==(p->tm_mday)) 
   		{
		    	//if(buf_tcp.i[48]%2!=1) NTP_counter[skt]++;
		    	//else                   NTP_counter[skt]=0;
		  


		  	if(buf_tcp.i[0]==1)
      		  	{
        	  		iy=buf_tcp.i[2];
        	  		im=buf_tcp.i[3];
        	  		id=buf_tcp.i[4];
        	  		ih=buf_tcp.i[5];
        	 		 mi=buf_tcp.i[6];
        	  		sec=buf_tcp.b[15];
        	  		t_now=buf_tcp.b[14];
        	  		t_now/=1000;
        	  		t_now+=make_mstime(iy,im,id,ih,mi,sec);
        	  		t_now+=shift_time;
        	  		for(jj=0;jj<1000;jj++) if(ptr[jj].flag == 0) break;
        	  		if(jj==1000) jj=0;
        	  		memset(ptr[jj].stn_name,0,8);
        	  		ptr[jj].flag     = -1;
      		  		sprintf(ss,"%i",buf_tcp.i[14]);
        	  		nt=mappingtable(ss);

        	  		if(nt!=-1) 
        	  		{
        	    			for(i=0;i<8;i++)
        	      				ptr[jj].stn_name[i]=station_table[nt][i];
        	    			ptr[jj].report_time  = t_now;
        	    			ptr[jj].Pga           = wkf1;
        	    			ptr[jj].Pgv           = wkf2;
        	    			ptr[jj].Pgd           = wkf3;
        	    			ptr[jj].flag         = 1; 
        	    			if(jj!=999) ptr[jj+1].flag       = 0; 

//-------------------------------------------------------------------------------------------------------------------------  Earthworm
					/* Store Waveform data
   					*******************/
       	           			 Date1.yr  = iy;
       	            			Date1.mon = im;
       	            			Date1.dy  = id;
       	            			Date1.hr  = ih;
       	            			Date1.min = mi;
       	            			Date1.sec = buf_tcp.b[15]; 
       	           			Date1.yr -= 1900;
       	            
       	           			wfhead.pinno = 0;
       	            			wfhead.nsamp=100;
       	            			wfhead.samprate=100;       
       	            			wfhead.starttime = ttime(Date1);
       	            			wfhead.endtime   = ttime(Date1)+1.0;
                    
	            			for(i=0;i<5;i++) stname[i]  =  ptr[jj].stn_name[i];                   
		    			stname[4]='\0';
                    
if(buf_tcp.i[48]%2!=1) printf("---- 6 -- socket: %d , %s \n", skt, stname);

//					if ( NTP_counter[skt] > 5 ) 
//					{
//if (NTP_counter[skt]>50000) NTP_counter[skt]=0;
//printf("---- 6 -- socket: %d , NTP_counter[skt]: %d , %s \n", skt, NTP_counter[skt], stname);
						//sprintf(cmd,"echo %s %d >> NTP_failed.txt", stname, NTP_counter[skt]);
						//system(cmd);
//					}
					//-------------Z
	            			sprintf(wfhead.sta      ,stname);
	            			sprintf(wfhead.chan     ,"HHZ");
	            			sprintf(wfhead.net      ,"TW");
	            			sprintf(wfhead.loc      ,"--");

                    			for(i=0;i<100;i++)
                    			{      
                     				k=buf_tcp.b[201+i*10];
                      				if(k >127)
						{
                      					k=k^255;
                      					k = k * 256 + (buf_tcp.b[200+i*10] ^ 255) + 1;
                      					k=k*-1;
                      				}else   k=k * 256 + buf_tcp.b[200+i*10];	
                     				 data[i] = k;
                   			}	                    
	            			if (put_msg( RingName, &wfhead, data  )==-1) printf("put_msg error ! \n");
					//-------------N 
	            			sprintf(wfhead.sta      ,stname);
	            			sprintf(wfhead.chan     ,"HHN");
	            			sprintf(wfhead.net      ,"TW");
	            			sprintf(wfhead.loc      ,"--");
	            
                    			for(i=0;i<100;i++)
                    			{      
                     				k=buf_tcp.b[203+i*10];
                      				if(k >127)
						{
                      					k=k^255;
                      					k = k * 256 + (buf_tcp.b[202+i*10] ^ 255) + 1;
                      					k=k*-1;
                      				}else   k=k * 256 + buf_tcp.b[202+i*10];	
                      				data[i] = k;
                    			}
		    			put_msg( RingName, &wfhead, data  );
					//-------------E 
	            			sprintf(wfhead.sta      ,stname);
	            			sprintf(wfhead.chan     ,"HHE");
	            			sprintf(wfhead.net      ,"TW");
	            			sprintf(wfhead.loc      ,"--");
	            
                    			for(i=0;i<100;i++)
                    			{      
                     				k=buf_tcp.b[205+i*10];
                      				if(k >127)
						{
                      					k=k^255;
                      					k = k * 256 + (buf_tcp.b[204+i*10] ^ 255) + 1;
                      					k=k*-1;
                      				}else   k=k * 256 + buf_tcp.b[204+i*10];	
                      				data[i] = k;
                    			}
		    			put_msg( RingName, &wfhead, data  );
//-------------------------------------------------------------------------------------------------------------------------                     
                  }else ptr[jj].flag     = 0;
                }

					//if ( NTP_counter[skt] > 5 ) 
					//	printf("---- 61 -- socket: %d \n", skt);


        if((buf_tcp.i[0]==119 || buf_tcp.i[0]==300) && buf_tcp.i[48]%2==1)
        {
           iy=buf_tcp.i[8];
           im=buf_tcp.i[9];
           id=buf_tcp.i[10];
           ih=buf_tcp.i[11];
           mi=buf_tcp.i[12];
           sec=buf_tcp.b[27];
           t_now=buf_tcp.b[26];
           t_now/=1000;
           t_now+=make_mstime(iy,im,id,ih,mi,sec);
           t_now+=shift_time;
           
    	   wkftc=buf_tcp.i[30];
    	   wkftc /= 1000; 
           pt=buf_tcp.i[23];  //pd flag
           pt>>=5;
           for(jj=0;jj<1000;jj++) if(ptrp[jj].flag == 0) break;	      								
           if(jj==1000) jj=0;
           memset(ptrp[jj].stn_name,0,8);
           ptrp[jj].flag     = -1;
           ptrp[jj].Ptype    = -1;
      	   sprintf(ss,"%i",buf_tcp.i[14]);
           nt=mappingtable(ss);
           if(nt!=-1)
	   { 
             	for(i=0;i<8;i++)
               		ptrp[jj].stn_name[i]=station_table[nt][i];
           	if(pt==1) ptrp[jj].Ptype        = 0;
           	if(pt==2 | pt==3) ptrp[jj].Ptype        = 1;
           	if(pt>=4) ptrp[jj].Ptype        = 2;
           	if(buf_tcp.i[0]==300) ptrp[jj].Ptype        = 3;

       	   	ptrp[jj].latitude=latitude_table[nt];
       	   	ptrp[jj].longitude=longitude_table[nt];
       	  	ptrp[jj].altitude=altitude_table[nt];
           	ptrp[jj].Parrival     = t_now;

           	if(buf_tcp.i[0]==119)
           	{
             		ptrp[jj].Pa           = 0;
             		ptrp[jj].Pv           = 0;
             		ptrp[jj].Pd           = 0;
           	}else
           	{
             		ptrp[jj].Pa           = wkf1;
             		ptrp[jj].Pv           = wkf2;
             		ptrp[jj].Pd           = wkf3;
           	}
           	ptrp[jj].Tc           = 0;
           	ptrp[jj].report_time  = tv.tv_sec+tv.tv_usec/1.0e6; 
           	if(ptrp[jj].Ptype==0 | ptrp[jj].Ptype==3)
           	{
            		ptrp[jj].flag         = 1; 
            		if(jj!=999) ptrp[jj+1].flag       = 0; 

           	}  
           	else
            		ptrp[jj].flag         = 0; 
		
		// ------ OUTPUT MESSAGE to RING2
/*		
		sprintf(outmsg_P,"%s %s %s %s %f %f %f %f %f %f %11.5f"
                                ,ptrp[jj].stn_name 
                                ,"HHZ"
                                ,"TW"
                                ,"--"
                                ,ptrp[jj].longitude 
                                ,ptrp[jj].latitude 
				,ptrp[jj].Pa
                                ,ptrp[jj].Pv  
				,ptrp[jj].Pd
                                ,ptrp[jj].Tc  
				,ptrp[jj].Parrival);	
		lineLen = strlen(outmsg_P);
*/
		//if ( tport_putmsg( &Region2, &Logo2, lineLen, outmsg_P ) != PUT_OK )
		//{
		//	sprintf(ccmd,"echo --outmsg_P error--- %s >> outmsg_P_err.txt ",outmsg_P);
		//	system(ccmd);			
		//}
		//else
		//{
			//sprintf(ccmd,"echo --outmsg_P --- %s >> outmsg_P.txt ",outmsg_P);
			//system(ccmd);				
		//}
		
						//if ( NTP_counter[skt] > 5 ) 
						printf("---- 62 -- socket: %d \n", skt);
		// ---------- Store in a text file
           	FILE * Event_File;
           	char   eventfile[100];
           	memset(eventfile,0,100);
           	sprintf(eventfile,"%d_P3evts.txt",( (1900+p->tm_year)*100+(1+p->tm_mon) ) );
           	Event_File = fopen(eventfile,"a"); 
           	fprintf(Event_File,"%04d/%02d/%02d %02d:%02d:%2.3f P %i %3i %4s %12.6f %6.03f %6.03f %6.03f %6.03f %12.6f %3.1f %3.1f %3.1f\n"
             ,iy,im,id,ih,mi,sec,jj,ptrp[jj].Ptype,ptrp[jj].stn_name,ptrp[jj].Parrival,ptrp[jj].Pa,ptrp[jj].Pv,ptrp[jj].Pd,ptrp[jj].Tc,ptrp[jj].report_time,ptrp[jj].latitude,ptrp[jj].longitude,ptrp[jj].altitude);      
           	fclose(Event_File);
           	
           	
           						//if ( NTP_counter[skt] > 5 ) 
						//printf("---- 63 -- socket: %d \n", skt);
           	
        }else ptrp[jj].flag     = 0;
      } 		
    }

					//if ( NTP_counter[skt] > 5 ) 
					//	printf("---- 64 \n");


		for(i = 0; i < sizeof(quit_char); i ++)
		{
//if ( NTP_counter[skt] > 5 ) 
						//printf("---- 65 \n");
			if(buf_tcp.b[i] != quit_char[i]) return 1;
		}
//if ( NTP_counter[skt] > 5 ) 
						printf("---- 66 \n");
		return -9;
	}
}
 





int mappingtable(char *name_t)
{
	int i;
	
	for(i=0;i<table_count;i++)
    if(strcmp(name_t,name_table[i])==0) break;
	if(i==table_count)
	  return -1;
	else
	  return i;  
}

/*-----------------------------------------* 
 * Definition of Date                      *
 *-----------------------------------------*/
 
time_t ttime(DDate dd)
{
	struct tm tt2= {0}; 
	time_t tt2_t;
	//char s[64];	
	
	tt2.tm_year = dd.yr;
	tt2.tm_mon  = dd.mon-1;
	tt2.tm_mday = dd.dy;
	tt2.tm_hour = dd.hr;
	tt2.tm_min  = dd.min;
	tt2.tm_sec  = dd.sec;
	
	tt2_t = mktime(&tt2);
   	//strftime (s, sizeof s, "%Y/%m/%d", &tt2);
   	//printf("%s \n", s);
   	
   	//return tt2_t+28800;
   	return tt2_t;
}


int put_msg( char *ring, TRACE2_HEADER *trh, int *data )
{
   FILE *     fp;
   char       ringname[50];
   char       line[MAX_CHAR];
   SHM_INFO   region;	
   MSG_LOGO   logo;
   long       RingKey;         /* Key to the transport ring to write to */
   int        pause;
   double     tlast, dt;
   int        firstpick = 1;
   
/* For Waveform output
   *******************/   
   static char   msg[MAX_BUFSIZ];
   TRACE2_HEADER *wfhead;
   int            *long_data; 
   int 		 i;
   DDate 	 Date1;
   size_t        size;

   wfhead =  ( TRACE2_HEADER *)&msg[0];
   long_data  =  (int *)( msg+ sizeof(TRACE2_HEADER) );   

/* Store Waveform data
   *******************/
       wfhead->pinno     = trh->pinno;
       wfhead->nsamp     = trh->nsamp;
       wfhead->samprate  = trh->samprate;       
       wfhead->starttime = trh->starttime;
       wfhead->endtime   = trh->endtime;

	 wfhead->version[0]='2';
	 wfhead->version[1]='0';
	 sprintf(wfhead->quality  ," ");
	 sprintf(wfhead->pad      ," ");
	  sprintf(wfhead->sta      ,"%s",trh->sta);
	  sprintf(wfhead->chan     ,"%s",trh->chan);
	  sprintf(wfhead->net      ,"%s",trh->net);
	  sprintf(wfhead->loc      ,"%s",trh->loc);
	 sprintf(wfhead->datatype      ,"i4");
	 
        for(i=0;i<wfhead->nsamp;i+=1)
    				long_data[i]=data[i];
  	logo.instid = 2;
  	logo.mod    = 2;
  	logo.type   = 19;

        size = 4 * wfhead->nsamp;
        size += sizeof( TRACE2_HEADER );

/* Attach to transport ring
   ************************/
   strcpy( ringname, ring );

   if ( ( RingKey = GetKey(ringname) ) == -1 )
   {
      printf("eew_svr: Invalid RingName <%s>; exiting!\n", ringname );
      return -1;
   }
   else
   {
   	//printf("Correct: %s %d\n", ringname, RingKey);
   }
   
   tport_attach( &region, RingKey );
  
/*----------------------main loop----------------------*/

/* Read from pick file, put to transport ring
   ******************************************/

      	if ( tport_getflag( &region ) == TERMINATE ) 
	{		
		//printf("Termination \n");
	} 
	else
	{
      		if ( tport_putmsg( &region, &logo, size, msg ) != PUT_OK )
      		{
         		//printf(" Error putting message in region %d\n", region.key );
      		}
      		else
      		{
      			//printf( "Put msg into %s \n", ringname);
      		}
	}
/*----------------------end of main loop----------------------*/

   tport_detach( &region );

   return 0;
}


//Time Function======================================================================
// Recognize leap years -----------------------------------------------
int isleap( int yr, int cal) {
   int l;

   if( yr < 0 )
   yr++;
   l = ( yr%4 == 0 );
   if( cal == GREGORIAN )
      l = l && ( yr%100 != 0 || yr%400 == 0 );
   return( l );
}
// Compute Julian Day number from calendar date -----------------------
long jdn( int yr, int mo, int day, int cal ) {
   long ret;
	int  eom[2][15] = {
   { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 396, 424 },
   { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366, 397, 425 },
	};   

   if( yr < 0 )
      yr++;

   // Move Jan. & Feb. to end of previous year
   if( mo <= 2 ) {
      --yr;
      mo += 12;
   }

   ret = qfloor( (long)(4*365+1) * (yr+4712), 4 ) + eom[0][mo-1] + day +
      ( cal==GREGORIAN ? -qfloor(yr, 100) + qfloor(yr, 400) + 2 : 0 );

   return( ret );
}
// Build ms_time from components --------------------------------------
double make_mstime( int year, int month, int day, int hour,
                    int min, double second) 
{
	int  eom[2][15] = {
   { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365, 396, 424 },
   { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366, 397, 425 },
	};	
   int *et = eom[isleap(year,GREGORIAN)];
   return( (double) DAY*(jdn(year,month,day,GREGORIAN)-BASEJDN) +
      hour*HOUR + min*MINUTE + second*SECOND );
}
/* month_day */
void month_day(int year, int yearday, int *pmonth, int *pday)
{
  static char daytab[2][13] = {
  	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
  };	
  int  i,leap;
  leap = year%4 == 0 && year%100 != 0 || year%400 == 0;
  for (i = 0; yearday > daytab[leap][i]; i++)
    yearday -= daytab[leap][i];
  *pmonth = i;
  *pday = yearday;
}

void template_config( char *configfile )
{
   int      ncommand;     /* # of required commands you expect to process   */ 
   char     init[10];     /* init flags, one byte for each required command */
   int      nmiss;        /* number of required commands that were missed   */
   char    *com;
   char    *str;
   int      nfiles;
   int      success;
   int      i;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   ncommand = 5;
   for( i=0; i<ncommand; i++ )  init[i] = 0;
   nLogo = 0;

/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
        logit( "e",
                "template: Error opening command file <%s>; exiting!\n", 
                 configfile );
        exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {  
            com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file 
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  logit( "e", 
                          "template: Error opening command file <%s>; exiting!\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("LogFile") ) {
                LogSwitch = k_int();
                init[0] = 1;
            }
  /*1*/     else if( k_its("MyModuleId") ) {
                str = k_str();
                if(str) strcpy( MyModName, str );
                init[1] = 1;
            }
  /*2*/     else if( k_its("RingName") ) {
                str = k_str();
                if(str) strcpy( RingName, str );
                init[2] = 1;
            }
  /*22*/     else if( k_its("RingName2") ) {
                str = k_str();
                if(str) strcpy( RingName2, str );
                init[2] = 1;
            }
  /*3*/     else if( k_its("HeartBeatInterval") ) {
                HeartBeatInterval = k_long();
                init[3] = 1;
            }


         /* Enter installation & module to get event messages from
          ********************************************************/
  /*4*/     else if( k_its("GetEventsFrom") ) {
                if ( nLogo+1 >= MAXLOGO ) {
                    logit( "e", 
                            "template: Too many <GetEventsFrom> commands in <%s>", 
                             configfile );
                    logit( "e", "; max=%d; exiting!\n", (int) MAXLOGO/2 );
                    exit( -1 );
                }
                if( ( str=k_str() ) ) {
                   if( GetInst( str, &GetLogo[nLogo].instid ) != 0 ) {
                       logit( "e", 
                               "template: Invalid installation name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].instid = GetLogo[nLogo].instid;
                }
                if( ( str=k_str() ) ) {
                   if( GetModId( str, &GetLogo[nLogo].mod ) != 0 ) {
                       logit( "e", 
                               "template: Invalid module name <%s>", str ); 
                       logit( "e", " in <GetEventsFrom> cmd; exiting!\n" );
                       exit( -1 );
                   }
                   GetLogo[nLogo+1].mod = GetLogo[nLogo].mod;
                }
                if( GetType( "TYPE_HYP2000ARC", &GetLogo[nLogo].type ) != 0 ) {
                    logit( "e", 
                               "template: Invalid message type <TYPE_HYP2000ARC>" ); 
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                if( GetType( "TYPE_H71SUM2K", &GetLogo[nLogo+1].type ) != 0 ) {
                    logit( "e", 
                               "template: Invalid message type <TYPE_H71SUM2K>" ); 
                    logit( "e", "; exiting!\n" );
                    exit( -1 );
                }
                nLogo  += 2;
                init[4] = 1;
            }

         /* Unknown command
          *****************/ 
            else {
                logit( "e", "template: <%s> Unknown command in <%s>.\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               logit( "e", 
                       "template: Bad <%s> command in <%s>; exiting!\n",
                        com, configfile );
               exit( -1 );
            }
        }
        nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       logit( "e", "template: ERROR, no " );
       if ( !init[0] )  logit( "e", "<LogFile> "           );
       if ( !init[1] )  logit( "e", "<MyModuleId> "        );
       if ( !init[2] )  logit( "e", "<RingName> "          );
       if ( !init[3] )  logit( "e", "<HeartBeatInterval> " );
       if ( !init[4] )  logit( "e", "<GetEventsFrom> "     );
       logit( "e", "command(s) in <%s>; exiting!\n", configfile );
       exit( -1 );
   }

   return;
}

/******************************************************************************
 *  template_lookup( )   Look up important info from earthworm.h tables       *
 ******************************************************************************/
void template_lookup( void )
{
/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey = GetKey(RingName) ) == -1 ) {
        fprintf( stderr,
                "template:  Invalid ring name <%s>; exiting!\n", RingName);
        exit( -1 );
   }

/* Look up keys to shared memory regions
   *************************************/
   if( ( RingKey2 = GetKey(RingName2) ) == -1 ) {
        fprintf( stderr,
                "template:  Invalid ring name <%s>; exiting!\n", RingName2);
        exit( -1 );
   }

/* Look up installations of interest
   *********************************/
   if ( GetLocalInst( &InstId ) != 0 ) {
      fprintf( stderr, 
              "template: error getting local installation id; exiting!\n" );
      exit( -1 );
   }

/* Look up modules of interest
   ***************************/
   if ( GetModId( MyModName, &MyModId ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid module name <%s>; exiting!\n", MyModName );
      exit( -1 );
   }

/* Look up message types of interest
   *********************************/
   if ( GetType( "TYPE_HEARTBEAT", &TypeHeartBeat ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_HEARTBEAT>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_ERROR", &TypeError ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_ERROR>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_HYP2000ARC", &TypeHinvArc ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_HYP2000ARC>; exiting!\n" );
      exit( -1 );
   }
   if ( GetType( "TYPE_H71SUM2K", &TypeH71Sum ) != 0 ) {
      fprintf( stderr, 
              "template: Invalid message type <TYPE_H71SUM2K>; exiting!\n" );
      exit( -1 );
   }
   return;
} 

void template_status( unsigned char type, short ierr, char *note )
{
   MSG_LOGO    logo;
   char        msg[256];
   long        size;
   time_t        t;
 
/* Build the message
 *******************/ 
   logo.instid = InstId;
   logo.mod    = MyModId;
   logo.type   = type;

   time( &t );

   if( type == TypeHeartBeat )
   {
        sprintf( msg, "%ld %ld\n\0", (long) t, (long) myPid);
   }
   else if( type == TypeError )
   {
        sprintf( msg, "%ld %hd %s\n\0", (long) t, ierr, note);
        logit( "et", "---template: %s\n", note );
   }

   size = strlen( msg );   /* don't include the null byte in the message */     

/* Write the message to shared memory
 ************************************/
   if( tport_putmsg( &Region, &logo, size, msg ) != PUT_OK )
   {
        if( type == TypeHeartBeat ) {
           logit("et","template:  Error sending heartbeat.\n" );
        }
        else if( type == TypeError ) {
           logit("et","template:  Error sending error:%d.\n", ierr );
        }
   }

   return;
}



/* include signal */


Sigfunc *
signal(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}
/* end signal */

Sigfunc *
Signal(int signo, Sigfunc *func)	/* for our signal() function */
{
	Sigfunc	*sigfunc;

	if ( (sigfunc = signal(signo, func)) == SIG_ERR)
		perror("signal error");
	return(sigfunc);
}


void cleanExit(int sig)
{
	printf("Catch signal:%d", sig);
        exit(0);
}

int serv(int port)
{
	int					listenfd, connfd;
	int					fd_connect[FD_SETSIZE];
	char					client_ip[FD_SETSIZE][20];
	ssize_t				n;
	fd_set				rset;
	char				buf[100];
	char				store_buf[1500][100];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	static uchar Message1[] = {1,2,0,0,0,6,1,6,0,0xc0,0,1};
	static uchar Message1_len = 12;

        int reuseaddr=1;
	int err;
	int r_doecho;

	char ccmd[300];
        int a_nt, jj, kk, i;

	unsigned char buf_in[TCP_BUF_LENGTH], buf_out[TCP_BUF_LENGTH];
	int num_in;
	Squeue S[FD_SETSIZE];

	int act_index=0;
	
	// for palert_sta_ip.txt
	FILE *fk;
	//sta_ip pa_ip[1000];
	int num_pa;
	
	// Set up palert list
	//fk = fopen("palert_sta_ip.txt","rt");
	//if(fk==NULL)
	//{
	//	printf("palert_sta_ip.txt does not exist ! \n");
	//	return -2;
	//}
	//num_pa=0;
	//while( fscanf(fk,"%s %s", pa_ip[num_pa].ip, pa_ip[num_pa].sta)==2 )	
	//{
	//	printf("%d %s %s \n", num_pa, pa_ip[num_pa].ip, pa_ip[num_pa].sta);
	//	num_pa ++;
	//}
	//fclose(fk);
	//printf("Total %d palerts in the Network ! \n", num_pa);

	



	// Initialize
	clilen = sizeof(cliaddr);

	bzero(&cliaddr, sizeof(cliaddr));
	bzero(&buf_in, sizeof(buf_in));
	bzero(&buf_out, sizeof(buf_out));

	for (i = 0; i < FD_SETSIZE; i++)
	{
		fd_connect[i] = -1;             /* -1 indicates available entry */
		iniQueue(&S[i]);	
		sprintf(client_ip[i],"empty");
        }



	// Open master socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
        // set master socket
	fd_connect[0] = listenfd;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);
	// Reuse master socket
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));

	err=bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if (err < 0)
	{
		perror("bind--");
		printf("Bind error :%i \n", err);
		return -2;
	}

	err=listen(listenfd, LISTENQ);
	if(err < 0)
	{
		perror("listen");
		printf("Make listening error: %i \n", err);
		return -3;
	}



	while(1) 
        {
//============================================================================================================================= Earthworm
          	if  ( time(&timeNow) - timeLastBeat  >=  HeartBeatInterval ) 
          	{
            		timeLastBeat = timeNow;
            		template_status( TypeHeartBeat, 0, "" ); 
//printf("---- 32 \n");
          	}
          	/* see if a termination has been requested 
           	*****************************************/
          	if ( tport_getflag( &Region ) == TERMINATE || tport_getflag( &Region ) == myPid )
          	{
           	/* detach from shared memory */
                	tport_detach( &Region ); 
           	/* write a termination msg to log file */
                	logit( "t", "template: Termination requested; exiting!\n" );
			printf( "template: Termination requested; exiting!\n" );
                	fflush( stdout );
	   	/* should check the return of these if we really care */
   			ew_unlockfile(lockfile_fd);
   			ew_unlink_lockfile(lockfile);
	   	// kill all sockets
//printf("---- 3 \n");
			for (i = 0; i <= FD_SETSIZE; i++)
		    		if (fd_connect[i] > 0) 
					close(fd_connect[i]);
			return 1;
               }
//============================================================================================================================= Socket
		// Clean contents in rset
       	 	FD_ZERO(&rset);
                // set master socket
		fd_connect[0] = listenfd;
		// put connected fds into rset list
		for (i = 0; i < FD_SETSIZE-10; i++)
			if (fd_connect[i] > 0)  FD_SET(fd_connect[i], &rset);             /* -1 indicates available entry */
		
		if( !select(FD_SETSIZE-9, &rset, NULL, NULL, NULL) ) {printf("---- QQ \n"); continue;}
		
		// search for all fd_connect
		for (i = 0; i < FD_SETSIZE-10; i++)
		{
			if(fd_connect[i]>0)
				if (FD_ISSET(fd_connect[i], &rset))
				{
					//check if fd is master socket
					if( fd_connect[i]==listenfd )
					{
						if( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen) ) <0 )
						{        
							printf("Accept error ! \n");                    			
						 	perror("accept error:");	
						}
						else
						{
							bzero( &buf, sizeof(buf) );
							inet_ntop(AF_INET, &cliaddr.sin_addr, buf, TCP_BUF_LENGTH);
							printf("socket: %d , new client: %s, port: %d  \n",
									connfd,
									buf,
									ntohs(cliaddr.sin_port) );

							// Check if the connected IP is legal.
							
							//for(jj=0;jj<num_pa;jj++)
							//	if( !strncmp( buf,pa_ip[num_pa].ip,strlen(pa_ip[num_pa].ip) ) ) break;

							//if(jj==num_pa) 
							//{
							//	sprintf(ccmd,"echo --Illegal IP--- %s >> ill_ip.txt ",buf);
							//	printf("--Illegal IP--- %s \n ",buf);
							//	system(ccmd);
							//	continue;
							//}

							// Check if the connected IP is already exist.
							for (jj = 0; jj < FD_SETSIZE-10; jj++)
								if( client_ip[jj][0]!='e' && client_ip[jj][1]!='m' )
									if( !strncmp(buf,client_ip[jj],strlen(client_ip[jj]) ) )
									{
					printf("Repeat Connection , close -- slave_socket: %d ,  %s \n", fd_connect[jj], buf);

					sprintf(ccmd,"echo Repeat Connection :  %s >> repeat_ip.txt ",buf);
										system(ccmd);

										close(fd_connect[jj]);
										fd_connect[jj] = -1;
				  					  	sprintf(client_ip[jj],"empty");
										break;
 									}


							// put the new fd into fd_connect list
			   				for (jj = 0; jj < FD_SETSIZE-10; jj++)
								if (fd_connect[jj] < 0) 
                                				{
									fd_connect[jj] = connfd;	/* save descriptor */
				        				sprintf(client_ip[jj],"%s",buf);
									printf("Accept new----- client_ip[%d] : %s \n", jj, client_ip[jj]);
									break;
								}
							// check if the upper bound of FD_SIZE has been reached
			  				if (jj == FD_SETSIZE-10)
							{
								printf("too many clients \n");
								close(connfd);
								continue;
                           				}
						}  // end of accept					
					}
					else	// if fd are not master socket, it is slave socket
					{
						bzero( &buf_tcp.b[0], sizeof(buf_tcp.b[0]) );
						if ( (n = read(fd_connect[i], &buf_tcp.b[0], TCP_BUF_LENGTH)) <= 0) 
						{
							perror("Read error --> ");

							//if(n==0) continue;


			printf(" read error close -- slave_socket: %d ,  recv = %d , %s \n", fd_connect[i], n, client_ip[i]);



					sprintf(ccmd,"echo read_error :  %s >> read_error.txt ",client_ip[i]);
										system(ccmd);

							
							close(fd_connect[i]);
							fd_connect[i] = -1;
							sprintf(client_ip[i],"empty");
						} 
						else
						{

/*-------------------------------------------
int check_packet_type()
return 1 : It's not a wave packet.
return 2 : It's a wave packet.
-------------------------------------------*/

//----------------------------------------------------- Check if the packet is wave or not
							if ( check_packet_type()==1 && n==200  ) 
							{
								do_echo_none_wave( fd_connect[i] );
							       continue;
							}

							// store input buf
							memcpy(buf_in,&buf_tcp.b[0],n);
					
							act_index = -1;
							for(jj=0;jj<FD_SETSIZE;jj++)
							{
								if( !strcmp(client_ip[i],S[jj].ip) )
								{
									act_index = jj;
									break;
								}
							}

							if(act_index==-1)
								for(jj=0;jj<FD_SETSIZE;jj++)
							if(S[jj].flag==0)
							{
								S[jj].flag = 1;
								sprintf(S[jj].ip,"%s",client_ip[i]);
								act_index = jj;
								break;
							}

							EnQueue(&S[act_index], buf_in, n);
							if(S[act_index].rear >= 1200)
							{
								DeQueue(&S[act_index], buf_out);

								// store output buf
								memcpy(&buf_tcp.b[0],buf_out,1200);	

/*-------------------------------------------
int check_wave_packet()
return 1 : It's not a correct wave packet.
return 2 : It's a correct wave packet.
-------------------------------------------*/
								// check if yr < 2000
								if(buf_tcp.i[2]<2000)
								//if( check_wave_packet()!=2  )
								{
	sprintf(ccmd,"echo --Palert Reset---Fd: %d, bytes: %d, IP: %s >> ttt.txt ",fd_connect[i],n,client_ip[i]);
         printf("--Palert Reset---Fd: %d, bytes: %d, IP: %s \n ",fd_connect[i],n,client_ip[i]);
									system(ccmd);
									// reset queue
							  		iniQueue(&S[act_index]);
								        // close socket
	 printf("palert reset close -- slave_socket: %d ,  recv = %d , %s \n", fd_connect[i], n, buf);
									close(fd_connect[i]);
									fd_connect[i] = -1;
									sprintf(client_ip[i],"empty");

							         }
//---------  If the wave packet is correct, put wave packet into do_echo
								r_doecho=do_echo( fd_connect[i] );

                                                                if(r_doecho==-9) printf("---- -9 \n");


							}  //  if(S.rear >= 1200)
							if(S[act_index].rear > 6000)
							{
			sprintf(ccmd,"echo ------S.rear = %d, >> error.txt ", S[act_index].rear);
								system(ccmd);
								// reset queue
						  		iniQueue(&S[act_index]);
								// close socket
			printf("rear over 6000 close -- slave_socket: %d ,  recv = %d , %s \n", fd_connect[i], n, buf);
								close(fd_connect[i]);
								fd_connect[i] = -1;
								sprintf(client_ip[i],"empty");
							}
						} // End of read
					} // End of if ( listenfd )
				} // 	End of if ( FD_ISSET )
		} // 	End of for ( FD_SETSIZE )
	} // End of while 
}// End of serv()


void iniQueue(Squeue *S)
{
	S->rear=0;
	memset(S->data,0,TCP_BUF_LENGTH*5);
	S->flag = 0;
	memset(S->ip,0,20);	
}
void EnQueue(Squeue *S, unsigned char *buf_in, int num_in)
{
	memcpy(&S->data[S->rear],buf_in,num_in*sizeof(char));
	S->rear += num_in;
}
void DeQueue(Squeue *S, unsigned char *buf_out)
{
	memcpy(buf_out,&S->data[0],1200*sizeof(char));
	S->rear -= 1200;
	memcpy(&S->data[0],&S->data[1200],S->rear*sizeof(char));
}




/*-------------------------------------------
int check_packet_type()
return 1 : It's not a wave packet.
return 2 : It's a wave packet.
-------------------------------------------*/
int check_packet_type()
{
	if( buf_tcp.i[0]!=1 && buf_tcp.i[74]==200 
						&& buf_tcp.b[140]==30
						&& buf_tcp.b[141]==33
						&& buf_tcp.b[142]==30
						&& buf_tcp.b[143]==35						
						&& buf_tcp.b[144]==31
						&& buf_tcp.b[145]==35
						&& buf_tcp.b[146]==30
						&& buf_tcp.b[147]==31						
						) return 1;
	else
						  return 2;
}
/*-------------------------------------------
int check_wave_packet()
return 1 : It's not a complete wave packet.
return 2 : It's a complete wave packet.
-------------------------------------------*/
int check_wave_packet()
{
	if( buf_tcp.i[0]==1 && buf_tcp.i[74]==1200 
						&& buf_tcp.b[140]==30
						&& buf_tcp.b[141]==33
						&& buf_tcp.b[142]==30
						&& buf_tcp.b[143]==35						
						&& buf_tcp.b[144]==31
						&& buf_tcp.b[145]==35
						&& buf_tcp.b[146]==30
						&& buf_tcp.b[147]==31						
						) return 2;
	else
						  return 1;
}


int do_echo_none_wave( int skt )
{
	int cc, err, i, pt;
	float wkf1, wkf2, wkf3, wkfp, wkftc;
  	double t_now,sec,shift_time;
  	int iy,im,id,ih,mi,jj,k;
	time_t timep;
	struct tm *p;
	struct timeval tv;
	struct timezone tz;
	int nt;
	char ss[8];

	char stname[20];
//----- Earthworm	
	DDate 	 Date1;
	TRACE2_HEADER wfhead;
	int data[1000];
	

        if((buf_tcp.i[0]==119 || buf_tcp.i[0]==300) && buf_tcp.i[48]%2==1)
        {
           iy=buf_tcp.i[8];
           im=buf_tcp.i[9];
           id=buf_tcp.i[10];
           ih=buf_tcp.i[11];
           mi=buf_tcp.i[12];
           sec=buf_tcp.b[27];
           t_now=buf_tcp.b[26];
           t_now/=1000;
           t_now+=make_mstime(iy,im,id,ih,mi,sec);
           t_now+=shift_time;
           
    	     wkftc=buf_tcp.i[30];
    	     wkftc /= 1000; 
           pt=buf_tcp.i[23];  //pd flag
           pt>>=5;
           for(jj=0;jj<1000;jj++) if(ptrp[jj].flag == 0) break;	      								
           if(jj==1000) jj=0;
           memset(ptrp[jj].stn_name,0,8);
           ptrp[jj].flag     = -1;
           ptrp[jj].Ptype    = -1;
      	   sprintf(ss,"%i",buf_tcp.i[14]);
           nt=mappingtable(ss);
           if(nt!=-1)
            { 
             for(i=0;i<8;i++)
               ptrp[jj].stn_name[i]=station_table[nt][i];
             if(pt==1) ptrp[jj].Ptype        = 0;
             if(pt==2 | pt==3) ptrp[jj].Ptype        = 1;
             if(pt>=4) ptrp[jj].Ptype        = 2;
             if(buf_tcp.i[0]==300) ptrp[jj].Ptype        = 3;
       	     //strcpy(ptrp[jj].stn_name,station_table[nt]);
       	     ptrp[jj].latitude=latitude_table[nt];
       	     ptrp[jj].longitude=longitude_table[nt];
       	     ptrp[jj].altitude=altitude_table[nt];
             ptrp[jj].Parrival     = t_now;
             if(buf_tcp.i[0]==119)
             {
               ptrp[jj].Pa           = 0;
               ptrp[jj].Pv           = 0;
               ptrp[jj].Pd           = 0;
             }else
             {
               ptrp[jj].Pa           = wkf1;
               ptrp[jj].Pv           = wkf2;
               ptrp[jj].Pd           = wkf3;
             }
             //ptrp[jj].Tc           = wkftc;
             ptrp[jj].Tc           = 0;
             ptrp[jj].report_time  = tv.tv_sec+tv.tv_usec/1.0e6; //make_mstime(1900+p->tm_year,1+p->tm_mon,p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
             if(ptrp[jj].Ptype==0 | ptrp[jj].Ptype==3)
             {
              ptrp[jj].flag         = 1; 
              if(jj!=999) ptrp[jj+1].flag       = 0; 
              //printf("P %i %3i %4s %12.6f %6.03f %6.03f %6.03f %6.03f %12.6f %3.1f %3.1f %3.1f\n",jj,ptrp[jj].Ptype,ptrp[jj].stn_name,ptrp[jj].Parrival,ptrp[jj].Pa,ptrp[jj].Pv,ptrp[jj].Pd,ptrp[jj].Tc,ptrp[jj].report_time,ptrp[jj].latitude,ptrp[jj].longitude,ptrp[jj].altitude);
             }  
             else
				ptrp[jj].flag         = 0; 

				FILE * Event_File;
				char   eventfile[100];
				memset(eventfile,0,100);
				sprintf(eventfile,"%d_P3evts.txt",( (1900+p->tm_year)*100+(1+p->tm_mon) ) );
				Event_File = fopen(eventfile,"a"); 
				fprintf(Event_File,"%4d/%2d/%2d %2d:%2d:%2.3f P %i %3i %4s %12.6f %6.03f %6.03f %6.03f %6.03f %12.6f %3.1f %3.1f %3.1f\n"
					,iy,im,id,ih,mi,sec,jj,ptrp[jj].Ptype,ptrp[jj].stn_name,ptrp[jj].Parrival,ptrp[jj].Pa,ptrp[jj].Pv,ptrp[jj].Pd,ptrp[jj].Tc,ptrp[jj].report_time,ptrp[jj].latitude,ptrp[jj].longitude,ptrp[jj].altitude);      
				fclose(Event_File);
            }else ptrp[jj].flag     = 0;
        } 		
    

		for(i = 0; i < sizeof(quit_char); i ++)
		{
			if(buf_tcp.b[i] != quit_char[i]) return 1;
		}
		return -9;
	
}





