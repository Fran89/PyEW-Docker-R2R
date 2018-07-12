/* Interface for UW-style seismic data files */
/* Conventions used in this interface: */
/* - Names starting with UWDFdh.. are designated to pass data coming */
/*   from the intial header block (e.g., old style UW format) */
/* - Names starting with UWDFch.. are designated to pass data that */
/*   is channel dependent; e.g., channel length, bias, starting offset, etc. */

#define __LIBUWDFIF
#include "usbr.h"
#include "uwdfif.h"

int ck_read_usbr(struct masthead *mh, FILE *readfs);

#ifdef sun
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define MAXSTAS 2048 /* current max number of stations allowed */

static FILE *readfs = NULL; /* stream pointer for read */
static FILE *newfile = NULL; /* new trace file stream (for writing) */
static char save_new_filename[50], temp_new_filename[20];
static struct masthead *mh = NULL, *new_mhead = NULL;
static struct chhead1 *ch1 = NULL;
static struct chhead2 *ch2 = NULL, *new_chead = NULL;
static struct chhead2 new_chead_proto;
static struct expanindex exps1;
static struct sta_loc *sta_loc_table = NULL;

static int is_rev[MAXSTAS]; /* truth table for station reversal */
/* Note: the following table is non-permanent hack to load the full
    station table;  This should be replaced with sta table server */
static struct sta_table {
	char name[8];
	float lat;
	float lon;
	float elev;
	char plflg;
	char stcom[80];
} complete_sta_table[MAXSTAS];
static int n_sta_table;

/* Note: following table is also non-permanent hack to load the full
    station reversal table;  This should also be replaced by server */
#define MAXREVS 200
#define LNSIZ 120
static struct rev_table {
	char line[LNSIZ];
} complete_rev_table[MAXREVS];
static int n_rev_table = 0, rev_on = 0;

/* Global variables */
static int swab_on_read, swab_on_write;    /* swab binary data flg; set TRUE if swab */
static char dataByteOrder = '\0', machineByteOrder = '\0';	/* I = Ieee (4321), L = Little-endian (1234), D = DEC-endian (3412)*/
static char outDataByteOrder = '\0';
static int uw_fmt_flg; /* if 2, uw-2 format is used for reading */
static int nchannels; /* number of channels */
static int no_chans_written; /* number of new channels written */
static int no_expan_structs; /* number of expansion structures; currently 1 */
static struct sm {
	char old_name[8];
	char new_name[8];
	char comp_flg[4];
	char id[4];
	char src[4]; /* Source of data; e.g., HWK for HAWK online acq */
} *sta_map = NULL;
static int nmap, def_map = 0, sta_mapping = FALSE;
static int ch_data_clipped;

/* Prototypes for useful local utilities */
static void julian(int *, int *), gregor(int *, int *);
/* CKW - swab -> lswab to avoid conflict with std swab defined in unistd.h */
static void lswab(unsigned char *, int, int, char, char);
static void set_swab_on_read(void);
static void set_swab_on_write(void);
static int init_channel_map(char *); /* Initialize channel map from external file */
static int channel_map(char *); /* return channel mapping for UW-1 to UW-2 */
static void strib(char *); /* strib is available outside */
static void get_station_locs(void); /* loads station location values */
static void get_revs(void); /* get station reversal table */
static void azdist(float, float, float, float, float *, float *, float *); /* compute distance */
static void set_earliest_starttime(void); /* reset master header starttime */

/* EXTERNAL FUNCTIONS BELOW HERE */

#define UW1_FIRST_STA	132

/* CK_READ_USBR - check if we're reading USBR-type UW-1 data */
int read_usbr(struct masthead *mh, FILE *readfs)
{
	char buf[16], *s1, *s2, c1, c2;
	int current, ii, type = 0;
	struct usbrhead *uh;

	/* How its done (sigh):
		Look for position of first station name to decide
		variant of UW-1 */

	/* do we know where we are? */
	if ( (current = ftell(readfs)) < 0 ) {
		fprintf(stderr,"Can't get position of input file\n");
		free(mh);
		mh = NULL;
		return(-1);
	}
	/* ok, seek to apparent position of first station name in std UW-1 */
	if ( fseek(readfs, UW1_FIRST_STA, SEEK_SET) != 0 ) {
		fprintf(stderr,"Can't seek on input file\n");
		free(mh);
		mh = NULL;
		return(-1);
	}
	/* grab some bytes to see what we have */
	if ( fread((void *)buf, sizeof(buf), 1, readfs) != 1 ) {
		fprintf(stderr,"Can't read input file\n");
		free(mh);
		mh = NULL;
		return(-1);
	}

	s1 = buf;	/* Position of first station name for std UW-1 */
	c1 = *s1;
	s2 = buf + 8;	/* Position of first station name for USBR-UW-1 */
	c2 = *s2;
	*(buf + sizeof(buf) - 1) = '\0';	/* keep strlen in check */

	/* check for a null terminated string in the right place */
	type = 3;			/* assume UW-2 */
	if ( isalnum(s1[0]) && isalnum(s1[1]) ) {
		if ( strlen(s1) < 6 )
			type = 1;	/* UW-1 */
	} else if ( isalnum(s2[0]) && isalnum(s2[1]) ) {
		if ( strlen(s2) < 6 )
			type = 2;	/* USBR-UW1 */
	} else {
		type = 3;		/* UW-2 */
	}

	/* if its not USBR-UW1, then return */
	if ( type == 0 ) {
		fprintf(stderr,"Can't grok the header(buf = ");
		for ( ii = 0 ; ii < sizeof(buf) ; ii++ )
			if ( isalnum((int)buf[ii]) )
				fprintf(stderr,"%c",buf[ii]);
			else
				fprintf(stderr,"\\%.3o",buf[ii]);
		fprintf(stderr,")\n");
		free(mh);
		mh = NULL;
		return -1;
	} else if ( type != 2 ) {
		/* return to previous position */
		if ( fseek(readfs, (int)current, SEEK_SET) != 0 ) {
			fprintf(stderr,"Can't restore position of input file\n");
			free(mh);
			mh = NULL;
			return(-1);
		}
		return FALSE;
	}

	/* USBR-UW1 type.  Re-read mh and leave file positioned at
	   start of first station header */
	uh = calloc(1,sizeof(struct usbrhead));
	rewind(readfs);
	if ( fread(uh, sizeof(*uh), 1, readfs) != 1 ) {
		fprintf(stderr,"Can't read USBR header\n");
		free(mh);
		mh = NULL;
		return(-1);
	}
	uh->comment[79] = '\0';

	/* Look at extra[2] to see if this is UW-1 or UW-2 format */
	if (uh->extra[2] == ' ' || uh->extra[2] == '1' || uh->extra[2] == '\0' ) {
		uw_fmt_flg = 1;
	} else if (uh->extra[2] == '2') { /* explicitly set to UW-2 */
		uw_fmt_flg = 2;
	} else {
		uw_fmt_flg = 1; /* default fallback */
		fprintf(stderr,"Assumming UW-1 (extra[2] = %d)\n", uh->extra[2]);
	}

	memcpy((void *)&(mh->nchan),(void *)&(uh->nchan),4);
	memcpy((void *)mh->extra,(void *)uh->extra,10);
	memcpy((void *)mh->comment,(void *)uh->comment,80);
	set_swab_on_read();
	/* Swabbing is done in blocks within the masthead structure */

	if (swab_on_read) {
		lswab((unsigned char*)&uh->nchan,1,sizeof(uh->nchan),dataByteOrder,machineByteOrder);
		lswab((unsigned char*)&uh->lrate,6,sizeof(uh->lrate),dataByteOrder,machineByteOrder);
		lswab((unsigned char*)&uh->flg,10,sizeof(uh->flg[0]),dataByteOrder,machineByteOrder);
	}
	
	mh->nchan = uh->nchan;
	mh->lrate = uh->lrate;
	mh->lmin = uh->lmin;
	mh->lsec = uh->lsec;
	mh->length = uh->length;
	mh->tapenum = uh->tapenum % 10000;
	mh->eventnum = uh->eventnum / 10;
	memcpy((void *)mh->flg,(void *)uh->flg,20);

	return TRUE;
}

/* CK_WRITE_USBR - write USBR-type UW-1 data */
int ck_write_usbr(struct masthead *mh, FILE *writefs) {
	struct usbrhead *uh;

	uh = (struct usbrhead *)malloc(sizeof(struct usbrhead));
	uh->nchan = mh->nchan;
	uh->lrate = mh->lrate;
	uh->lmin = mh->lmin;
	uh->lsec = mh->lsec;
	uh->length = mh->length;
	uh->tapenum = mh->tapenum;
	uh->eventnum = mh->eventnum;
	memcpy((void *)uh->flg,(void *)mh->flg,20);
	memcpy((void *)uh->comment,(void *)mh->comment,80);

	if (swab_on_write) {
		lswab((unsigned char*)&uh->nchan,1,sizeof(mh->nchan),machineByteOrder,outDataByteOrder);
		lswab((unsigned char*)&uh->lrate,6,sizeof(mh->lrate),machineByteOrder,outDataByteOrder);
		lswab((unsigned char*)&uh->flg,10,sizeof(mh->flg[0]),machineByteOrder,outDataByteOrder);
	}

	rewind (writefs);
	if ( fwrite((void *)uh, sizeof(struct usbrhead), 1, writefs) != 1 ) {
		fprintf(stderr,"Can't write the header\n");
		return FALSE;
	}

	return TRUE;
}

/* UWDF FILE FUNCTIONS */

int UWDFopen_file(char *name) /* open and init for read of new data file */
{
    int i, j;
    char name_for_open[50];
    char str[80];
    
    strcpy(name_for_open, name);

	if ( readfs != NULL ) {
		perror("UWDFopen_file: warning: closing previously opened file");
		UWDFclose_file();
	}   

	if ((readfs = fopen(name_for_open,"rb")) == NULL) {
#if 0
		sprintf(str,"UWDFopen_file: can't open %s",name_for_open);
		perror(str);
#endif
		return FALSE;
	}
    
    /* Create space for master header structure (and free old headers) */
	if ( mh != NULL || ch1 != NULL || ch2 != NULL || sta_loc_table != NULL )
		perror("UWDFopen_file: warning: found programming error");

	mh = (struct masthead *)calloc(1,sizeof(struct masthead));
	ch1 = (struct chhead1 *)calloc(MAXSTAS,sizeof(struct chhead1));
	ch2 = (struct chhead2 *)calloc(MAXSTAS,sizeof(struct chhead2));
	sta_loc_table = (struct sta_loc *)calloc(MAXSTAS,sizeof(struct sta_loc));
        
	if ( mh == NULL || ch1 == NULL || ch2 == NULL || sta_loc_table == NULL ) {
		fprintf(stderr,"UWDFopen_file: can't allocate space for headers\n");
		return FALSE;
	}
    
	/* Read in first part of header file (muxhead structure) */
	/* 
		Note: Multiple reads of header structure accommodate
		possible word boundary alignment problems; e.g., Ridge does
		not allow alignment of longs across word (4 byte) boundaries */

	if ( read_usbr(mh,readfs) ) {
		if ( mh == NULL )
			return(FALSE);	/* error reading or identifying header */
	} else {
		/* 
			Note: Multiple reads of header structure accommodate
			possible word boundary alignment problems; e.g., Ridge does
			not allow alignment of longs across word (4 byte) boundaries */
		fread((void *)&mh->nchan, 2, 1, readfs);
		fread((void *)&mh->lrate, 1, 50, readfs);
		fread((void *)mh->comment, 1, 80, readfs);
		mh->comment[79] = '\0';

		/* Look at extra[2] to see if this is UW-1 or UW-2 format */
		if (mh->extra[2] == ' ' || mh->extra[2] == '1' || mh->extra[2] == '\0' ) {
			uw_fmt_flg = 1;
		} else if (mh->extra[2] == '2') { /* explicitly set to UW-2 */
			uw_fmt_flg = 2;
		} else {
			uw_fmt_flg = 1; /* default fallback */
			fprintf(stderr,"Assumming UW-1 (extra[2] = %d)\n", mh->extra[2]);
		}

		set_swab_on_read();
		/* Swabbing is done in blocks within the masthead structure */
		if (swab_on_read) {
			lswab((unsigned char*)&mh->nchan,1,sizeof(mh->nchan),dataByteOrder,machineByteOrder);
			lswab((unsigned char*)&mh->lrate,4,sizeof(mh->lrate),dataByteOrder,machineByteOrder);
			lswab((unsigned char*)&mh->tapenum,12,sizeof(mh->tapenum),dataByteOrder,machineByteOrder);
		}
	}

	/* Determine the number of channels; from master header if UW-1 */
	if (uw_fmt_flg == 1) {
		nchannels = mh->nchan;
	} else { /* from end of file if UW-2 */
		fseek(readfs, (int)-4, SEEK_END);
		fread((void *)&no_expan_structs, 4, 1, readfs);
		if (swab_on_read)
			lswab((unsigned char*)&no_expan_structs,1,sizeof(no_expan_structs),dataByteOrder,machineByteOrder);
		if (no_expan_structs <= 0) {
			fprintf(stderr,"Structure index count not >= 0\n");
			exit(0);
		}
		fseek(readfs, (int)(-12*no_expan_structs - 4), SEEK_END);
		for (i = 0; i < no_expan_structs; ++i) {
			fread((void *)&exps1, sizeof(exps1), 1, readfs);
			if (swab_on_read)
				lswab((unsigned char*)&exps1.numstructs,2,sizeof(exps1.numstructs),dataByteOrder,machineByteOrder);
			if (strncmp(exps1.structag,"CH2",3) == 0) {
				nchannels = exps1.numstructs;
				break;
			}
		}
	}

	/* Time to read channel headers */
	if (uw_fmt_flg == 1) { /* UW-1 format; read channel headers following
		master header; stuff UW-2 channel headers */
		for (i = 0; i < nchannels; ++i) {
			fread((void *)(ch1+i), sizeof(struct chhead1), 1, readfs);
			strib((ch1+i)->name);
			if (swab_on_read)
				lswab((unsigned char *)&((ch1+i)->lta),3,sizeof(short),dataByteOrder,machineByteOrder);
			/* Stuff ch2 style channel headers for compatibility */
			if ( sta_mapping ) {
				if ((j = channel_map((ch1+i)->name)) >= 0) {
					strcpy((ch2+i)->name, sta_map[j].new_name);
					strcpy((ch2+i)->compflg, sta_map[j].comp_flg);
					strcpy((ch2+i)->chid, sta_map[j].id);
					strcpy((ch2+i)->src, sta_map[j].src);
				} else {
					strcpy((ch2+i)->name, (ch1+i)->name);
					strcpy((ch2+i)->compflg, sta_map[def_map].comp_flg);
					strcpy((ch2+i)->chid, sta_map[def_map].id);
					strcpy((ch2+i)->src, sta_map[def_map].src);
				}
			} else {
				strcpy((ch2+i)->name, (ch1+i)->name);
				strcpy((ch2+i)->compflg,"");
				strcpy((ch2+i)->chid,"");
				strcpy((ch2+i)->src,"");
			}
			strcpy((ch2+i)->fmt,"S");
			(ch2+i)->chlen = mh->length;
			(ch2+i)->lrate = mh->lrate;
			(ch2+i)->start_lmin = mh->lmin;
			(ch2+i)->start_lsec = mh->lsec;
			(ch2+i)->offset = i*2*mh->length;
			(ch2+i)->lta = (ch1+i)->lta;
			(ch2+i)->trig = (ch1+i)->trig;
			(ch2+i)->bias = (ch1+i)->bias;
			strcpy((ch2+i)->src,"");
			(ch2+i)->expan1 = 0;
			(ch2+i)->fill = 0;
		}
	} else { /* Assume UW-2 format; read chan headers rel to end of file */
		fseek(readfs,(int)exps1.offset, SEEK_SET);
		fread((void *)ch2, sizeof(struct chhead2), (size_t)nchannels, readfs);
		for (i = 0; i < nchannels; ++i) {
			(ch2+i)->name[7] = '\0'; /* Make sure name is terminated */
			strib((ch2+i)->name);
			if (swab_on_read) {
				lswab((unsigned char *)&((ch2+i)->chlen),6,sizeof(int),dataByteOrder,machineByteOrder);
				lswab((unsigned char *)&((ch2+i)->lta),4,sizeof(short),dataByteOrder,machineByteOrder);
			}
			(ch2+i)->fmt[1] = '\0';
			(ch2+i)->compflg[3] = '\0';
			(ch2+i)->chid[3] = '\0';
			(ch2+i)->src[3] = '\0';
			strib((ch2+i)->compflg); /* Strip trailing blanks for strings */
			strib((ch2+i)->chid);
			strib((ch2+i)->src);
		}
	}
	/* Read station table and stuff values for channels */
	get_station_locs();
	get_revs();
	/* If UW-1 format, close header file and open trace file */
	if (uw_fmt_flg == 1) {
		fclose(readfs);
		LAST_CHAR(name_for_open) = 'd';
		if ((readfs = fopen(name_for_open,"rb")) == NULL) {
			sprintf(str,"UWDFopen_file: can't open new file: %s",
			name_for_open);
			perror(str);
			return FALSE;
		}
		/* If UW-2 format, set master header time to earliest start */
	} else if (uw_fmt_flg == 2) {
		set_earliest_starttime();
	}
	return TRUE;
}

int UWDFinit_sta_table(char *name) /* init and turn on sta location file */
{
	int ii;
	char line[LNSIZ], stname[8], ss[8], plflg, stcom[80], ns, ew;
	int latd, latm, lond, lonm;
	float lat, lon, elev, lats, lons;
	static FILE *stain = (FILE *)NULL;

	if ( *name == '\0' ) {
		/* Null station name */
		if ( stain != (FILE *)NULL ) {
		/* close currently open station file and return */
		fclose(stain);
		stain = (FILE *)NULL;
		n_sta_table = 0;
		return TRUE;
	} else
		return FALSE;
	}

	/* Read fresh station table */
	if ( stain != (FILE *)NULL ) {
		/* close currently open station file */
		fclose(stain);
		stain = (FILE *)NULL;
	}
	n_sta_table = 0;
	if ( (stain = fopen(name, "r")) == (FILE *)NULL ) {
		fprintf(stderr,"Can't open station location file %s: %s\n",
					name, strerror(errno));
		return FALSE;
	}

	/* read station name and location */
	while ( fgets(line,sizeof(line),stain) != (char *)NULL ) {
		if ( *line == '#' || strlen(line) < 31u )
			continue;		/* skip comments and bad lines */

		/* minimum info is station name, lat and lon */
		if ( sscanf(line,"%4s",stname) != 1 )
			continue;
		if ( sscanf(line+6, "%d%c%d%f %d%c%d%f",
					&latd, &ns, &latm, &lats,
					&lond, &ew, &lonm, &lons) != 8 )
			continue;

		lat = ABS(latd) + latm / 60. + lats / 3600.;
		if ( ns == 'S' || ns == 's' || latd < 0 )
			lat = -lat;
		lon = ABS(lond) + lonm / 60. + lons / 3600.;
		if ( ew == 'W' || ew == 'w' || ew == ' ' || lond < 0 )
			lon = -lon;

		/* get the elevation if present */
		if ( strlen(line) < 39u )
			elev = 0.;
		else {
			strncpy(ss,line+33,6);	/* depth in km */
			ss[6] = '\0';
			elev = atof(ss);
		}
	
		/* get the plot position flag if present */
		if ( strlen(line) < 41u )
			plflg = ' ';
		else
			plflg = line[40];

		/* get the station comment if present */
		if ( strlen(line) > 64u )
			strcpy(stcom,line+64);
		else
			*stcom = '\0';
    	
		ii = n_sta_table++;
		strcpy(complete_sta_table[ii].name, stname);
		complete_sta_table[ii].lat = lat;
		complete_sta_table[ii].lon = lon;
		complete_sta_table[ii].elev = elev;
		complete_sta_table[ii].plflg = plflg;
		strcpy(complete_sta_table[ii].stcom,stcom);
    }
    fclose(stain);
    
    return TRUE;
}

struct sta_loc *UWDFget_sta_loc(char *name)
{
	int ii;
	static struct sta_loc loc;

	for ( ii = 0 ; ii < n_sta_table ; ii++ )
		if ( strcmp(name,complete_sta_table[ii].name) == 0 )
			break;
	if ( ii == n_sta_table )
		return(NULL);

	loc.lat = complete_sta_table[ii].lat;
	loc.lon = complete_sta_table[ii].lon;
	loc.elev = complete_sta_table[ii].elev;
	loc.plflg = complete_sta_table[ii].plflg;
	strcpy(loc.stcom,complete_sta_table[ii].stcom);

	return ( &loc );
}

int UWDFinit_rev_table(char *name, int revOn) /* init and turn on sta reversal file */
{
	int i;
	char line[LNSIZ];
	FILE *inp;

	if ( (inp = fopen(name,"r")) == (FILE *)NULL ) {
		fprintf(stderr,"ERROR - can't open reversal file: %s\n", name);
		return FALSE;
	}
	i = 0;
	while( fgets(line, sizeof(line), inp) != (char *)NULL ) {
		if ( *line == '#' || strlen(line) < 5 )
			continue;
		strcpy(complete_rev_table[i].line, line);
		++i;
		if ( i >= MAXREVS ) {
			fprintf(stderr,"Reversal file %s has more than %d entries - skipping rest\n",
						name,MAXREVS);
			break;
		}
	}
	n_rev_table = i;
	rev_on = revOn;
	fclose(inp);

	return TRUE;
}

int UWDFinit_for_new_write(char *newfilename) /* Initialize for writing new file(s)*/
/* The newfilename is squirreled away until file is closed */
{
	/* Save new file name and make temporary file name for writing */
	strcpy(save_new_filename, newfilename);
	strcpy(temp_new_filename,"tempWXXXXXX");
#ifndef _WIN32
	if ( mktemp(temp_new_filename) == NULL || *temp_new_filename == '\0' ) {
		fprintf(stderr,"can't make temp file name for: %s\n",newfilename);
		return FALSE;
	}
#else
	GetTempFileName("\\TEMP","tempW",0,temp_new_filename);
#endif
	if ((newfile = fopen(temp_new_filename,"wb")) == NULL) {
		fprintf(stderr,"can't open new file: %s\n",temp_new_filename);
		return FALSE;
	}
	/* Make space for new master header - free any old header */
	if ( new_mhead != NULL )
		free((void *)new_mhead);
	new_mhead = (struct masthead *)calloc(1,sizeof(struct masthead));
	if ( new_mhead == NULL ) {
		fprintf(stderr,"can't get space for new master header\n");
		return FALSE;
	}

	/* Make space to save new channel headers - free any old headers */
	if ( new_chead != NULL )
		free((void *)new_chead);
	new_chead = (struct chhead2 *)calloc(MAXSTAS,sizeof(struct chhead2));
	if ( new_chead == NULL ) {
		fprintf(stderr,"can't get space for new channel headers\n");
		return FALSE;
	}

	/* Initialize anything in new masthead here */

	/* Initialize the channel head proto here */
	strcpy(new_chead_proto.name,"");
	new_chead_proto.lta = 0;
	new_chead_proto.trig = 0;
	new_chead_proto.bias = 0;
	new_chead_proto.chlen = 0;
	new_chead_proto.offset = 0;
	new_chead_proto.start_lmin = 0;
	new_chead_proto.start_lsec = 0;
	new_chead_proto.lrate = 0;
	strcpy(new_chead_proto.fmt,"");
	strcpy(new_chead_proto.compflg,"");
	strcpy(new_chead_proto.chid,"");
	strcpy(new_chead_proto.src,"");
	new_chead_proto.expan1 = 0;
	new_chead_proto.fill = 0;
	return TRUE;
}

int UWDFclose_file(void) /* Close current file open for reading */
{
	if ( readfs == NULL )
		return FALSE; /* OK to call with no file open */

	fclose(readfs);
	readfs = NULL;

	free((void *)mh);
	free((void *)ch1);
	free((void *)ch2);
	free((void *)sta_loc_table);
	mh = NULL; ch1 = NULL; ch2 = NULL; sta_loc_table = NULL;

	return TRUE;
}

int UWDFclose_new_file(void) /* Complete writing and close new file */
{
	struct chhead2 *out_ch2;
	struct expanindex out_exps1;
	int out_no_expan_structs;

	if ( new_chead == NULL ) {
		perror("UWDFclose_new_file: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	/* copy new channel headers, and swap bytes if necessary */
	if ( (out_ch2 = calloc(no_chans_written,sizeof(struct chhead2))) == NULL ) {
		fprintf(stderr,"UWDFclose_new_file: can't get space for copy of %d channel headers:\n\t%s\n",
						no_chans_written,strerror(errno));
		return FALSE;
	}
	memcpy(out_ch2,new_chead,no_chans_written*sizeof(struct chhead2));
	if (swab_on_write) {
		int i;
		for ( i = 0 ; i < no_chans_written ; i++ ) {
			lswab((unsigned char *)&((out_ch2+i)->chlen),6,sizeof(int),machineByteOrder,outDataByteOrder);
			lswab((unsigned char *)&((out_ch2+i)->lta),4,sizeof(short),machineByteOrder,outDataByteOrder);
		}
	}

	/* Write channel headers to new file */
	exps1.offset = ftell(newfile); /* grab current offset of channel headers */
	if ( fwrite((void *)out_ch2,sizeof(struct chhead2),
				(size_t)no_chans_written,newfile) != no_chans_written) {
		perror("UWDFclose_new_file: ch_head write failed");
		return FALSE;
	}
	free(out_ch2);

	/* Update expansion structure */
	strcpy(exps1.structag,"CH2");
	exps1.numstructs = no_chans_written;
	/* copy expansion structure, and swap bytes if necessary */
	out_exps1 = exps1;
	if (swab_on_write)
			lswab((unsigned char*)&out_exps1.numstructs,2,sizeof(int),machineByteOrder,outDataByteOrder);
	/* write expansion structure following the channel headers */
	if ( fwrite((void *)&out_exps1, sizeof(struct expanindex), 1, newfile) != 1 ) {
		perror("UWDFclose_new_file: ch_head_exp write failed");
		return FALSE;
	}

	/* Write number of expan structs as last integer in file */
	no_expan_structs = 1;
	out_no_expan_structs = no_expan_structs;
	if (swab_on_write)
			lswab((unsigned char*)&out_no_expan_structs,1,sizeof(int),machineByteOrder,outDataByteOrder);
	if ( fwrite((void *)&out_no_expan_structs, sizeof(int), 1, newfile) != 1 ) {
		perror("UWDFclose_new_file: ch_head_no_exp write failed");
		return FALSE;
	}

	/* Close file, and move to correct file name */
	fclose(newfile);
	newfile = NULL;
	if(rename(temp_new_filename, save_new_filename)!= 0) {
		perror("UWDFclose_new_file: Error renaming new file");
		return FALSE;
	}

	no_chans_written = 0;
	return TRUE;
}

int UWDFdelete_new_file(void) /* Clean up and remove new file */
{
	/* Close and remove file */
	if ( newfile != (FILE *)NULL ) {
		fclose(newfile);
		newfile = NULL;
		if ( remove(temp_new_filename) !=  0) {
			fprintf(stderr,"UWDFdelete_new_file: Error removing temp file %s: %s\n",
								temp_new_filename,strerror(errno));
			return FALSE;
		}
	}
	no_chans_written = 0;
	return TRUE;
}

/* UWDF HEADER FUNCTIONS (RETURN) */

char *UWDFdhcomment(void) /* Return pointer to header comment string */
{
	if ( mh == NULL ) {
		perror("UWDFdhcomment: must first call UWDFopen_file");
		return NULL;
	}
	return (mh->comment);
}

int UWDFdhevno(void) /* Return event number in integer */
{
	if ( mh == NULL ) {
		perror("UWDFdhevno: must first call UWDFopen_file");
		return -1;
	}
	return (mh->eventnum);
}

int UWDFdhtapeno(void) /* Return tape number in integer */
{
	if ( mh == NULL ) {
		perror("UWDFdhtapeno: must first call UWDFopen_file");
		return -1;
	}
	return (mh->tapenum);
}

char *UWDFdhext(void) /* Return character pointer to extra character string */
{
	if ( mh == NULL ) {
		perror("UWDFdhext: must first call UWDFopen_file");
		return (char *)NULL;
	}
	return (mh->extra);
}

short *UWDFdhflgs(void) /* Return ptr to array of short int flags */
{
	if ( mh == NULL ) {
		perror("UWDFdhflgs: must first call UWDFopen_file");
		return (short *)NULL;
	}
	return (mh->flg);
}

int UWDFdhnchan(void) /* Return number of channels */
{
	return (nchannels);
}

int UWDFdhref_stime(struct stime *time) /* Return reference time in header */
{
	int *tim;

	if ( mh == NULL ) {
		perror("UWDFdhref_stime: must first call UWDFopen_file");
		return FALSE;
	}

	tim = &time->yr;
	mingrg(tim, &(mh->lmin));
	time->sec = mh->lsec * 1.0e-06;
	return TRUE;
}

int UWDFdhfmt_type_for_read(void) /* Return format type for read; 1 or 2 */
{
	return (uw_fmt_flg);
}

/* UWDF CHANNEL FUNCTIONS (RETURN) */

int UWDFchret_trace(int chno, char fmt, void *seis) /* Return trace in buffer */
/*	chno	- channel number
 *	fmt	- requested data format (may differ from data file).  Must
 *		  be one of 'S' (short), 'L' (int), or 'F' (float).
 *	seis	- pointer to space to hold requested data.  This space
 *		  must be of size UWDFchlen(chan) * sizeof(short|int|float)
 *		  or more, and must be allocated before call.
 *
 *	Modified  12/30/93 CKW
 *		Uses pointers to traverse buffers, and allocates read buffer so
 *		calling routines need only to provide space for returned data.
 */
{
	int position, len;
	char data_fmt;
	short *sp, *sp1;
	int *lp, *lp1, data_max, data_bias, max_amp, clip_margin = 10;
	float *fp, *fp1;
	void *vp;

	/* Do some integrity checking */
	if ( mh == NULL || ch1 == NULL || ch2 == NULL ) {
		perror("UWDFchret_trace: must first call UWDFopen_file");
		return FALSE;
	}
	if ( (chno < 0 || chno > nchannels - 1) ||
					(fmt != 'S' && fmt != 'L' && fmt != 'F') ) {
		perror("UWDFchret_trace: bad trace");
		return FALSE;
	}

	/* set length and data type based on format */
	if ( uw_fmt_flg == 1 ) {	/* UW-1 format */
		len = mh->length;
		data_fmt = 'S';		/* UW-1 data always short */
	} else {			/* UW-2 format */
		len = (ch2+chno)->chlen;
		data_fmt = *((ch2+chno)->fmt);
	}
	if ( data_fmt != 'S' && data_fmt != 'L' && data_fmt != 'F' )
		return FALSE;

	/* seek to start of data */
	position = (ch2+chno)->offset;
	fseek(readfs,(int)position,0);

	/* get DC shift that has been subtracted from the trace data */
	data_bias = UWDFchbias(chno);

	/* read data and load into seis */
	switch ( data_fmt ) {
		case 'S':	/* input data short */
			/* get space. read data. swap bytes if necessary. */
			if ( (vp = calloc((size_t)len,sizeof(short))) == NULL ) {
				fprintf(stderr,"can't get space to read trace\n");
				return FALSE;
			}
			fread(vp,sizeof(short),(size_t)len,readfs);
			if (swab_on_read)
				lswab((unsigned char *)vp,len,sizeof(short),dataByteOrder,machineByteOrder);
			/* stuff into seis */
			switch ( fmt ) {
				case 'S':	/* S -> S */
					memcpy(seis,vp,(size_t)(sizeof(short) * len));
					/* get maximum */
					for ( sp = (short *)vp, data_max = 0, sp1 = sp ; sp1 < sp + len ; sp1++ )
						data_max = MAX(ABS(*sp1+data_bias),data_max);
					break;
				case 'L':	/* S -> L */
					sp = (short *)vp;
					lp = (int *)seis;
					for ( sp1 = sp, data_max = 0 ; sp1 < sp + len ; sp1++ ) {
						*lp++ = *sp1;
						data_max = MAX(ABS(*sp1+data_bias),data_max);
					}
					break;
				case 'F':	/* S -> F */
					sp = (short *)vp;
					fp = (float *)seis;
					for ( sp1 = sp, data_max = 0 ; sp1 < sp + len ; sp1++ ) {
						*fp++ = *sp1;
						data_max = MAX(ABS(*sp1+data_bias),data_max);
					}
					break;
			}
			/* determine if data is clipped */
			max_amp = 2048; /* 12-bit A/D */
			if ( data_max > max_amp )
				max_amp = 32768; /* 16-bit A/D */
			/* set static global with current channel clip status - use UWDFclipped() to read */
			ch_data_clipped = data_max > max_amp - clip_margin ? TRUE : FALSE;
			break;
		case 'L':	/* input data int */
			if ( (vp = calloc((size_t)len,sizeof(int))) == NULL ) {
				fprintf(stderr,"can't get space to read trace\n");
				return FALSE;
			}
			fread(vp,sizeof(int),(size_t)len,readfs);
			if (swab_on_read)
				lswab((unsigned char *)vp,len,sizeof(int),dataByteOrder,machineByteOrder);
			switch ( fmt ) {
				case 'S':	/* L -> S */
					lp = (int *)vp;
					sp = (short *)seis;
					for ( lp1 = lp, data_max = 0 ; lp1 < lp + len ; lp1++ ) {
						*sp++ = *lp1;
						data_max = MAX(ABS(*lp1+data_bias),data_max);
					}
					break;
				case 'L':	/* L -> L */
					memcpy(seis,vp,(size_t)(sizeof(int) * len));
					/* get maximum */
					for ( lp = (int *)vp, data_max = 0, lp1 = lp ; lp1 < lp + len ; lp1++ )
						data_max = MAX(ABS(*lp1+data_bias),data_max);
					break;
				case 'F':	/* L -> F */
					lp = (int *)vp;
					fp = (float *)seis;
					for ( lp1 = lp, data_max = 0 ; lp1 < lp + len ; lp1++ ) {
						*fp++ = *lp1;
						data_max = MAX(ABS(*lp1+data_bias),data_max);
					}
					break;
			}
			max_amp = 2048;	/* 12-bit A/D */
			if ( data_max > max_amp )
				max_amp = 32768;	/* 16-bit A/D */
			if ( data_max > max_amp )
				max_amp = 8388608;	/* 24-bit A/D */
			/* set static global with current channel clip status - use UWDFclipped() to read */
			ch_data_clipped = data_max > max_amp - clip_margin ? TRUE : FALSE;
			break;
		case 'F':	/* input data float */
			if ( (vp = calloc((size_t)len,sizeof(float))) == NULL ) {
				fprintf(stderr,"can't get space to read trace\n");
				return FALSE;
			}
			fread(vp,sizeof(float),(size_t)len,readfs);
			if (swab_on_read)
				lswab((unsigned char *)vp,len,sizeof(float),dataByteOrder,machineByteOrder);
			switch ( fmt ) {
				case 'S':	/* F -> S */
					fp = (float *)vp;
					sp = (short *)seis;
					for ( fp1 = fp, data_max = 0. ; fp1 < fp + len ; fp1++ )
						*sp++ = ROUND(*fp1);
					break;
				case 'L':	/* F -> L */
					fp = (float *)vp;
					lp = (int *)seis;
					for ( fp1 = fp, data_max = 0. ; fp1 < fp + len ; fp1++ )
						*lp++ = ROUND(*fp1);
					break;
				case 'F':	/* F -> F */
					memcpy(seis,vp,(size_t)(sizeof(float) * len));
					break;
			}
			/* set static global with current channel clip status - use UWDFclipped() to read */
			ch_data_clipped = FALSE; /* can't tell with float data, so assume not */
			break;
		}
		free(vp);

		/* Do final sign reversal if needed */
		if ( rev_on && is_rev[chno] ) {
			switch ( fmt ) {
				case 'S':
					sp = (short *)seis;
					for ( sp1 = sp ; sp1 < sp + len ; sp1++ )
						*sp1 = -*sp1;
					break;
				case 'L':
					lp = (int *)seis;
					for ( lp1 = lp ; lp1 < lp + len ; lp1++ )
						*lp1 = -*lp1;
					break;
				case 'F':
					fp = (float *)seis;
					for ( fp1 = fp ; fp1 < fp + len ; fp1++ )
						*fp1 = -*fp1;
					break;
			}
		}
		return TRUE;
}

int UWDFchclipped(void) { /* return whether last channel read was clipped */
	return ch_data_clipped;
}

/*
 * Interface to the llnl filtering functions:
 *	Private structure to hold current filtering parameters
 * CKW 2/16/08
 */
typedef struct {
	int zero_phase;
	int iord; /* filter order: 1-10 */
	char *type;	/* LP, HP, BP, or BR */
	char *aproto; /* BU, BE, C1, or C2 */
	float a; /* Chebyshev stopband attenuation factor */
	float trbndw; /* Chebyshev transition bandwidth (fraction of lowpass prototype passband width) */
	float fl; /* Low-frequency cutoff */
	float fh; /* High-frequency cutoff */
	float sr; /* Sampling rate (in Hz) */
} FilterInfo;
static FilterInfo *filter_info = NULL;

/*
 * Interface to the llnl filtering functions:
 *	Clear any existing filter design parameters
 * CKW 2/16/08
 */
int LlnlFilterClear( void ) {
	if ( filter_info != NULL ) {
		free(filter_info);
		filter_info = NULL;
	}
	return TRUE;
}

/*
 * Interface to the llnl filtering functions:
 *	Set filter design parameters
 * CKW 2/16/08
 */
int LlnlFilterDesign(int iord, char *type, char *aproto, float a, float trbndw, float fl, float fh, int zp) {
	const char *my_name = "LlnlFilterDesign";

	/* create filter info first time through */
	if ( filter_info == NULL ) {
		if ( (filter_info = malloc(sizeof(*filter_info))) == NULL ) {
			fprintf(stderr,"%s: error: can't get space for filter paramaters\n",my_name);
			return FALSE;
		}
	}

	filter_info->zero_phase = zp > 0 ? 1 : 0;
	if ( (filter_info->iord = iord) < 1 || iord > 10 ) {	
			fprintf(stderr,"%s: error: filter order must be between 1 and 10\n",my_name);
			LlnlFilterClear();
			return FALSE;
	}
	filter_info->type =
		(strncasecmp(type,"lp",2) == 0 || strncasecmp(type,"lowp",4) == 0 ||
			strncasecmp(type,"lo-p",4) == 0 || strncasecmp(type,"low-p",5) == 0 ||
			strncasecmp(type,"lo p",4) == 0 || strncasecmp(type,"low p",5) == 0 ||
			strncasecmp(type,"lop",3) == 0 || strncasecmp(type,"hic",3) == 0 ||
			strncasecmp(type,"hi-c",4) == 0 || strncasecmp(type,"high-c",5) == 0 ||
			strncasecmp(type,"hi c",4) == 0 || strncasecmp(type,"high c",6) == 0 ||
			strncasecmp(type,"hc",2) == 0 || strncasecmp(type,"highc",5) == 0) ? "LP" :
		(strncasecmp(type,"hp",2) == 0 || strncasecmp(type,"hip",3) == 0 ||
			strncasecmp(type,"hi-p",4) == 0 || strncasecmp(type,"high-p",5) == 0 ||
			strncasecmp(type,"hi p",4) == 0 || strncasecmp(type,"high p",6) == 0 ||
			strncasecmp(type,"highp",5) == 0 || strncasecmp(type,"loc",3) == 0 ||
			strncasecmp(type,"lo-c",4) == 0 || strncasecmp(type,"low-c",5) == 0 ||
			strncasecmp(type,"lo c",4) == 0 || strncasecmp(type,"low c",5) == 0 ||
			strncasecmp(type,"lc",2) == 0 || strncasecmp(type,"lowc",4) == 0) ? "HP" :
		(strncasecmp(type,"bp",2) == 0 || strncasecmp(type,"bandp",5) == 0 ||
			strncasecmp(type,"band-p",6) == 0 || strncasecmp(type,"band p",6) == 0) ? "BP" :
		(strncasecmp(type,"br",2) == 0 || strncasecmp(type,"bandr",5) == 0 ||
			strncasecmp(type,"band-r",6) == 0 || strncasecmp(type,"band r",6) == 0) ? "BR" :
		/* error */ NULL;		
	if ( filter_info->type == NULL ) {
			fprintf(stderr,"%s: error: unknown filter type: %s\n",my_name,type);
			LlnlFilterClear();
			return FALSE;
	}
	filter_info->aproto =
		strncasecmp(aproto,"bu",2) == 0 ? "BU" :
		strncasecmp(aproto,"be",2) == 0 ? "BE" :
		(strncasecmp(aproto,"c1",2) == 0 || strncasecmp(aproto,"chebyshev type 1",16) == 0 ||
				strncasecmp(aproto,"chebyshev-type-1",16) == 0 || strncasecmp(aproto,"cheby1",6) == 0 ||
				strncasecmp(aproto,"cheby 1",7) == 0 || strncasecmp(aproto,"cheby-1",7) == 0 ||
				strncasecmp(aproto,"chebyshev 1",11) == 0 || strncasecmp(aproto,"chebyshev-1",11) == 0) ? "C1" :
		(strncasecmp(aproto,"c2",2) == 0 || strncasecmp(aproto,"chebyshev type 2",16) == 0 ||
				strncasecmp(aproto,"chebyshev-type-2",16) == 0 || strncasecmp(aproto,"cheby2",6) == 0 ||
				strncasecmp(aproto,"cheby 2",7) == 0 || strncasecmp(aproto,"cheby-2",7) == 0 ||
				strncasecmp(aproto,"chebyshev 2",11) == 0 || strncasecmp(aproto,"chebyshev-2",11) == 0) ? "C2" :
		/* error */ NULL;
	if ( filter_info->aproto == NULL ) {
		fprintf(stderr,"%s: error: unrecognized filter analog prototype: %s\n",my_name,aproto);
		LlnlFilterClear();
		return FALSE;
	}
	if ( (filter_info->a = a) <= 0. && strncasecmp(filter_info->aproto,"C",1) == 0 ) {
		fprintf(stderr,"%s: error: Chenyshev attenuation factor <= 0\n",my_name);
		LlnlFilterClear();
		return FALSE;
	}
	if ( ((filter_info->trbndw = trbndw) <= 0. || trbndw >= 1.) && strncasecmp(filter_info->aproto,"C",1) == 0 ) {	
		fprintf(stderr,"%s: error: bad Chenyshev attenuation factor\n",my_name);
		LlnlFilterClear();
		return FALSE;
	}
	if ( (filter_info->fl = fl) < 0. && strncasecmp(filter_info->type,"LP",2) != 0 ) {	
		fprintf(stderr,"%s: error: low frequency cutoff < 0\n",my_name);
		LlnlFilterClear();
		return FALSE;
	}
	if ( (filter_info->fh = fh) < 0. && strncasecmp(filter_info->type,"HP",2) != 0 ) {
		fprintf(stderr,"%s: error: bad high frequency cutoff\n",my_name);
		LlnlFilterClear();
		return FALSE;		
	}
	if ( fl >= fh && (strncasecmp(filter_info->type,"BP",2) == 0 || strncasecmp(filter_info->type,"BR",2) == 0) ) {
		fprintf(stderr,"%s: error: high frequency cutoff (%.3f) must be greater than low frequency cutoff (%.3f)\n",my_name,fh,fl);
		LlnlFilterClear();
		return FALSE;		
	}
	filter_info->sr = -1.;

	return TRUE;
}

/*
 * Interface to the llnl filtering functions:
 *	returns a string with the filter info
 * CKW 2/16/08
 */
char *LlnlFilterPrint(void) {
	static char msg[256];
	if ( filter_info == NULL ) {
		sprintf(msg,"No filter is configured.");
	} else {
		sprintf(msg,"%s %d%s-order %s %s",
			filter_info->zero_phase ? "zero-phase" : "fwd-only",
			filter_info->iord,
			(filter_info->iord == 1 ? "st" :
				filter_info->iord == 2 ? "nd" :
				filter_info->iord == 3 ? "rd" : "th"),
			(strncasecmp(filter_info->aproto,"bu",2) == 0 ? "Butterworth" :
				strncasecmp(filter_info->aproto,"be",2) == 0 ? "Bessel" :
				strncasecmp(filter_info->aproto,"c1",2) == 0 ? "Chebyshev Type 1" :
				strncasecmp(filter_info->aproto,"c2",2) == 0 ? "Chebyshev Type 2" : "unknown"),
			(strncasecmp(filter_info->type,"lp",2) == 0 ? "low-pass" :
				strncasecmp(filter_info->type,"hp",2) == 0 ? "high-pass" :
				strncasecmp(filter_info->type,"bp",2) == 0 ? "band-pass" :
				strncasecmp(filter_info->type,"br",2) == 0 ? "band-reject" : "unknown"));
		if ( strncasecmp(filter_info->type,"hp",2) == 0 ||
					strncasecmp(filter_info->type,"bp",2) == 0 ||
					strncasecmp(filter_info->type,"br",2) == 0 )
			sprintf(msg+strlen(msg),", low-cut freq = %.3f Hz",filter_info->fl);
		if ( strncasecmp(filter_info->type,"lp",2) == 0 ||
					strncasecmp(filter_info->type,"bp",2) == 0 ||
					strncasecmp(filter_info->type,"br",2) == 0 )
			sprintf(msg+strlen(msg),", high-cut freq = %.3f Hz",filter_info->fh);
		if ( strncasecmp(filter_info->aproto,"c1",2) == 0 ||
					strncasecmp(filter_info->aproto,"c2",2) == 0 ) {
			sprintf(msg+strlen(msg),", stopband attn = %.1f",filter_info->a);
			sprintf(msg+strlen(msg),", trans bandwidth = %.4f (frac of low-pass bw)",filter_info->trbndw);
		}
	}
	return msg;
}

/*
 * Interface to the llnl filtering functions:
 *	filter the data
 * CKW 2/16/08
 */
int LlnlFilterApply(float *input_data, int nsamp, float srate, void *filtered_data, char out_fmt)
/*
 *	input_data	- pointer to input data.  This space must be of size
 *		nsamp * sizeof(float)
 *	nsamp - number of samples in input data
 *	srate - sample rate of input data, in units of samples per second
 *	out_fmt	- requested output data format. Must be one of 
 *		'S' (short), 'L' (int), or 'F' (float).
 *	filtered_data	- pointer to space to hold filtered data.  This space
 *		must be of size nsamp * sizeof(short|int|float)
 *		or more, and must be allocated before call.
 *
 *	Added  2/16/08 CKW
 */
{
	const char *my_name = "LlnlFilterApply";
	float *work;
	short *sp;
	int *lp;
	float *fp;

	/* sanity checks */
	if ( filter_info == NULL ) {
		fprintf(stderr,"%s: error: must first design a filter\n",my_name);
		return FALSE;
	} else if ( nsamp < 1 ) {
		fprintf(stderr,"%s: error: input data length (%d) <= 0\n",my_name,nsamp);
		return FALSE;
	} else if ( srate <= 0. ) {
		fprintf(stderr,"%s: error: sample rate (%.3f) <= 0\n",my_name,srate);
		return FALSE;
	} else if ( input_data == NULL ) {
		fprintf(stderr,"%s: error: NULL pointer to input data\n",my_name);
		return FALSE;
	} else if ( filtered_data == NULL ) {
		fprintf(stderr,"%s: error: caller must provide space for filtered data\n",my_name);
		return FALSE;
	}

	/* design filter if parameters or sample rate have changed. Note: sr member is
	   set to the magic value of -1 whenever LlnlFilterDesign is called */
	if ( filter_info->sr != srate ) {
		float ts = 1. / srate;
		design(filter_info->iord,filter_info->type,filter_info->aproto,
						filter_info->a,filter_info->trbndw,filter_info->fl,
						filter_info->fh,ts);
		filter_info->sr = srate;
	}

	/* get working space to filter data so that input is not modified by filtering function */
	if ( (work = calloc(nsamp,sizeof(*work))) == NULL ) {
		fprintf(stderr,"%s: error: can't get work space to filter data\n",my_name);
		return FALSE;
	}
	memcpy(work,input_data,nsamp * sizeof(*input_data));

	/* filter the data */
	apply(work, nsamp, filter_info->zero_phase);

	/* convert the data to the requested format */
	switch ( out_fmt ) {
		case 'S':	/* return short data */
			sp = (short *)filtered_data;
			for ( fp = work ; fp < work + nsamp ; fp++ )
				*sp++ = ROUND(*fp);
			break;
		case 'L':	/* return int data */
			lp = (int *)filtered_data;
			for ( fp = work ; fp < work + nsamp ; fp++ )
				*lp++ = ROUND(*fp);
			break;
		case 'F':	/* return float data */
			/* no conversion needed - already float */
			memcpy(filtered_data,work,nsamp*sizeof(float));
			break;
		}

		free(work);

		return TRUE;
}

int UWDFchret_filtered_trace(int chno, char fmt, void *seis) /* Return trace in buffer */
/*	chno	- channel number
 *	fmt	- requested data format (may differ from data file).  Must
 *		  be one of 'S' (short), 'L' (int), or 'F' (float).
 *	seis	- pointer to space to hold requested data.  This space
 *		  must be of size UWDFchlen(chan) * sizeof(short|int|float)
 *		  or more, and must be allocated before call.
 *
 *	Added  1/31/08 CKW
 */
{
	const char *my_name = "UWDFchret_filtered_trace";
	int len;
	float *trace_data = NULL;

	/* sanity checks */
	if ( filter_info == NULL ) {
		/* if no filter is set, silently return unfiltered data */
		return UWDFchret_trace(chno, fmt, seis);
	} else if ( (len = UWDFchlen(chno)) < 1 ) {
		fprintf(stderr,"%s: error: trace length <= 0\n",my_name);
		return FALSE;
	}

	/* get space for trace data, as float */
	if ( (trace_data = calloc(len,sizeof(*trace_data))) == NULL ) {
		fprintf(stderr,"%s: can't get space for trace\n",my_name);
		return FALSE;
	}

	/* get the trace data, as float */
	if ( UWDFchret_trace(chno, 'F', trace_data) == FALSE ) {
		free(trace_data);
		return FALSE;
	}

	/* filter the data and put into the requested output format */
	if ( LlnlFilterApply(trace_data, len, UWDFchsrate(chno), seis, fmt) == FALSE ) {
		free(trace_data);
		return FALSE;
	}

	free(trace_data);
	return TRUE;
}

int UWDFchlen(int chno) /* Return length of channel "chno" */
{
	if ( ch2 == NULL ) {
		perror("UWDFchlen: must first call UWDFopen_file");
		return -1;
	}
	return ((ch2+chno)->chlen);
}

char UWDFchfmt(int chno) /* Return character specifying data type {S,L, or F} */
{
	if ( ch2 == NULL ) {
		perror("UWDFchfmt: must first call UWDFopen_file");
		return('\0');
	}
	return (((ch2+chno)->fmt)[0]);
}

char *UWDFchname(int chno) /* Return string pointer to channel name */
{
	if ( ch2 == NULL ) {
		perror("UWDFchname: must first call UWDFopen_file");
		return NULL;
	}
	return ((ch2+chno)->name);
}

int UWDFchlta(int chno) /* Return value of lta for chan */
{
	if ( ch2 == NULL ) {
		perror("UWDFchlta: must first call UWDFopen_file");
		return -9999999;
	}
	return ((int)((ch2+chno)->lta));
}

int UWDFchtrig(int chno) /* Return value of trig for chan */
{
	if ( ch2 == NULL ) {
		perror("UWDFchtrig: must first call UWDFopen_file");
		return -9999999;
	}
	return ((int)((ch2+chno)->trig));
}

int UWDFchbias(int chno) /* Return value of bias for chan */
{
	if ( ch2 == NULL ) {
		perror("UWDFchbias: must first call UWDFopen_file");
		return -9999999;
	}
	return ((int)((ch2+chno)->bias));
}

int UWDFchref_stime(int chno, struct stime *time) /* Return channel reference time */
{
	int *t;

	if ( ch2 == NULL ) {
		perror("UWDFchref_stime: must first call UWDFopen_file");
		return FALSE;
	}

	t = &time->yr;
	mingrg(t, &((ch2+chno)->start_lmin));
	time->sec = (ch2+chno)->start_lsec / 1000000.;
	return TRUE;
}

float UWDFchsrate(int chno) /* Return channel sample rate in samples/sec */
{
	if ( ch2 == NULL ) {
		perror("UWDFchsrate: must first call UWDFopen_file");
		return -1;
	}
	return( (float)((ch2+chno)->lrate / 1000.) );
}

char *UWDFchcompflg(int chno) /* Return pointer to component flag string */
{
	if ( ch2 == NULL ) {
		perror("UWDFchcompflg: must first call UWDFopen_file");
		return NULL;
	}
	return ((ch2+chno)->compflg);
}

char *UWDFchid(int chno) /* Return pointer to channel id string */
{
	if ( ch2 == NULL ) {
		perror("UWDFchid: must first call UWDFopen_file");
		return NULL;
	}
	return ((ch2+chno)->chid);
}

char *UWDFchsrc(int chno) /* Return a character pointer to the data source ID */
{
	return ((ch2+chno)->src);
}

int UWDFchno(char *chname, char *compflg, char *chid) /* Return channel index from name, ... */
/* These are critical id strings */
/* Note: chname must be a non-null station table string
         compflg, if a null string, is ignored in search
	 chid, if a null string, is also ignored in search
*/
{
	int i;

	if ( ch2 == NULL ) {
		perror("UWDFchno: must first call UWDFopen_file");
		return -1;
	}

	for (i = 0; i < nchannels; ++i) {
		if (strcmp(chname,(ch2+i)->name) == 0){
			if ((strcmp(compflg, "") != 0) &&
						(strcmp(compflg, (ch2+i)->compflg) != 0))
				continue;
			if ((strcmp(chid, "") != 0) &&
						(strcmp(chid, (ch2+i)->chid) != 0))
				continue;
			return (i);
		}
	}
	return (-1); /* return -1 (illegal value) if channel not found */
}

struct sta_loc UWDFchloc(int chno) /* Return channel location structure */
{
	if ( sta_loc_table == NULL )
		perror("UWDFchloc: must first call UWDFopen_file");

	return (sta_loc_table[chno]);
}

/* return TRUE if trace returned by UWDFchret_trace has reversed polarity,
   i.e., channel is known to be reversed, but UWDFchret_trace did not
   correct it because rev_on was false */
int UWDFch_is_rev(int chno)
{
	return( !rev_on && is_rev[chno] );
}

/* UWDF NEW FILE WRITING FUNCTIONS - HEADERS */

void UWDFwrite_ieee_byte_order(void) {
	outDataByteOrder = 'I';
}
void UWDFwrite_intel_byte_order(void) {
	outDataByteOrder = 'L';
}
void UWDFwrite_dec_byte_order(void) {
	outDataByteOrder = 'D';
}
void UWDFwrite_host_byte_order(void) {
	outDataByteOrder = '\0';
}

int UWDFwrite_new_head(void) /* Write new header file after opening */
{
	struct masthead mh_out;

	if ( new_mhead == NULL ) {
		perror("UWDFwrite_new_head: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	/* Determine byte orer to write */
	set_swab_on_write();

	no_chans_written = 0;
	/* new_mhead->nchan = 65535; */
	/* new_mhead->lrate = 0; */
	/* new_mhead->lmin = 0; */
	/* new_mhead->lsec = 0; */
	new_mhead->extra[1] = outDataByteOrder; /* Ensure that machine is correct */
	new_mhead->extra[2] = '2'; /* Set to UW-2; only style allowed for write */

	/* make a copy of the new header (in case we need to swap bytes) */
	mh_out = *new_mhead;
	
	/* Swabbing is done in blocks within the copy of the new masthead structure */
	if ( swab_on_write ) {
		lswab((unsigned char*)&mh_out.nchan,1,sizeof(mh_out.nchan),machineByteOrder,outDataByteOrder);
		lswab((unsigned char*)&mh_out.lrate,4,sizeof(mh_out.lrate),machineByteOrder,outDataByteOrder);
		lswab((unsigned char*)&mh_out.tapenum,12,sizeof(mh_out.tapenum),machineByteOrder,outDataByteOrder);
	}

	/* write out muxhead structure */
	if ( fwrite((void *)&mh_out.nchan, 2, 1, newfile) != 1 ) {
		perror("UWDFwrite_new_head: nchan write failed");
		return FALSE;
	}
	if ( fwrite((void *)&mh_out.lrate, 1, 50, newfile) != 50 ) {
		perror("UWDFwrite_new_head: lrate write failed");
		return FALSE;
	}
	if ( fwrite((void *)mh_out.comment, 1, 80, newfile) != 80 ) {
		perror("UWDFwrite_new_head: comment write failed");
		return FALSE;
	}

	return TRUE;
}

int UWDFdup_masthead(void) /* Duplicate master header for writing from current */
{
	if ( new_mhead == NULL ) {
		perror("UWDFdup_masthead: must first call UWDFinit_for_new_write");
		return FALSE;
	}
	if ( mh == NULL ) {
		perror("UWDFdup_masthead: must first call UWDFopen_file");
		return FALSE;
	}

	*new_mhead = *mh;
	return TRUE;
}

int UWDFset_dhtapeno(short int new_tapenum) /* Set master header tape number */
{
	if ( new_mhead == NULL ) {
		perror("UWDFset_dhtapeno: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	new_mhead->tapenum = new_tapenum;
	return TRUE;
}

int UWDFset_dhevno(short int new_evno) /* Set master header event number */
{
	if ( new_mhead == NULL ) {
		perror("UWDFset_dhevno: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	new_mhead->eventnum = new_evno;
	return TRUE;
}

int UWDFset_dhflgs(short int new_flags[]) /* Set master header flag array */
/* Remember there are 10 of these flags */
{
	int i;

	if ( new_mhead == NULL ) {
		perror("UWDFset_dhflgs: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	for (i = 0; i < 10; ++i)
		new_mhead->flg[i] = new_flags[i];
	return TRUE;
}

int UWDFset_dhextra(char extra[]) /* Set master header "extra" array */
/* extra must be a 10 length char array */
{
	int i;

	if ( new_mhead == NULL ) {
		perror("UWDFset_dhextra: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	for (i = 0; i < 10; ++i)
		new_mhead->extra[i] = extra[i];
	return TRUE;
}

int UWDFset_dhcomment(char *comment) /* Set master header comment string */
/* comment must be correctly terminated string */
{
	char tmpstr[80];

	if ( new_mhead == NULL ) {
		perror("UWDFset_dhcomment: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	strncpy(tmpstr,comment,79);
	tmpstr[79] = '\0';
	strib(tmpstr);
	strcpy(new_mhead->comment,tmpstr);
	return TRUE;
}

int UWDFset_dhref_stime(struct stime time) /* Set master header reference time from structure type time */
{
	int tmpmin;
	float tmpsec;

	if ( new_mhead == NULL ) {
		perror("UWDFset_dhref_stime: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	grgmin(&time.yr, &tmpmin);

	/* CKW - correct for sec < 0. or sec >= 60. */
	tmpsec = time.sec;
	while ( tmpsec < 0. ) {
		tmpmin -= 1;
		tmpsec += 60.;
	}
	while ( tmpsec >= 60. ) {
		tmpmin += 1;
		tmpsec -= 60.;
	}

	new_mhead->lmin = tmpmin;
	new_mhead->lsec = ROUND(tmpsec * 1.0e+06);

	return TRUE;    
}

/* UWDF NEW FILE WRITING FUNCTIONS - CHANNELS */

int UWDFwrite_new_chan(char wr_fmt, int len, void *seis, char seis_fmt) /* Write next seismogram */
/*	wr_fmt	- Char to indicate format of data to be written
 *	len	- Number of data samples to write
 *	seis	- Input seismogram pointer.  Space pointed to must be of
 *		  length len * sizeof(short|int|float) or more.
 *	seis_fmt	- Char to indicate format of input seis data
 *
 *	Modified to use pointers. Chris Wood 12/30/93
 */
{
	short *sp, *sp1;
	int *lp, *lp1;
	float *fp, *fp1;

	if ( new_chead == NULL ) {
		perror("UWDFwrite_new_chan: must first call UWDFinit_for_new_write");
		return FALSE;
	}

	if ( wr_fmt != 'S' && wr_fmt != 'L' && wr_fmt != 'F' ) {
		perror("UWDFwrite_new_chan: unknown write format");
		return FALSE;
	}
	if ( seis_fmt != 'S' && seis_fmt != 'L' && seis_fmt != 'F' ) {
		perror("UWDFwrite_new_chan: unknown input format");
		return FALSE;
	}

	new_chead_proto.fmt[0] = wr_fmt;		/* Set format character */
	new_chead_proto.chlen = len;		/* Set length of channel */
	new_chead_proto.offset = ftell(newfile);	/* Set pointer to channel */

	/* Copy chan header proto to structure array */
	*(new_chead + no_chans_written) = new_chead_proto;

	/* Write data to new file */
	switch ( seis_fmt ) {
		case 'S':	/* input data are short */
			sp = (short *) seis;
			switch ( wr_fmt ) {
				case 'S':	/* S -> S */
					if (swab_on_write)
						lswab((unsigned char *)sp,len,sizeof(short),machineByteOrder,outDataByteOrder);
					if ( fwrite((void *)sp,sizeof(short),(size_t)len,newfile)
								!= len) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					break;
				case 'L':	/* S -> L */
					if ( (lp = (int *)calloc((size_t)len,sizeof(int)))
								== NULL ) {
						fprintf(stderr,"can't get space to convert trace\n");
						return FALSE;
					}
					for ( sp1 = sp, lp1 = lp ; sp1 < sp + len ; sp1++ )
						*lp1++ = *sp1;
					if (swab_on_write)
						lswab((unsigned char *)lp,len,sizeof(int),machineByteOrder,outDataByteOrder);
					if ( fwrite((void *)lp,sizeof(int),(size_t)len,newfile)
						!= len) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					free((void *)lp);
					break;
				case 'F':	/* S -> F */
					if ( (fp = (float *)calloc((size_t)len,sizeof(float)))
								== NULL ) {
						fprintf(stderr,"can't get space to convert trace\n");
						return FALSE;
					}
					for ( sp1 = sp, fp1 = fp ; sp1 < sp + len ; sp1++ )
						*fp1++ = *sp1;
					if ( fwrite((void *)fp,sizeof(float),(size_t)len,newfile)
								!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					free((void *)fp);
					break;
			}
			break;
		case 'L':	/* input data are int */
			lp = (int *) seis;
			switch ( wr_fmt ) {
				case 'S':	/* L -> S */
					if ( (sp = (short *)calloc((size_t)len,sizeof(short)))
								== NULL ) {
						fprintf(stderr,"can't get space to convert trace\n");
						return FALSE;
					}
					for ( lp1 = lp, sp1 = sp ; lp1 < lp + len ; lp1++ )
						*sp1++ = *lp1;
					if (swab_on_write)
						lswab((unsigned char *)sp,len,sizeof(short),machineByteOrder,outDataByteOrder);
					if ( fwrite((void *)sp,sizeof(short),(size_t)len,newfile)
								!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					free((void *)sp);
					break;
				case 'L':	/* L -> L */
					if (swab_on_write)
						lswab((unsigned char *)lp,len,sizeof(int),machineByteOrder,outDataByteOrder);
					if ( fwrite((void *)lp,sizeof(int),(size_t)len,newfile)
								!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					break;
				case 'F':	/* L -> F */
					if ( (fp = (float *)calloc((size_t)len,sizeof(float)))
							== NULL ) {
						fprintf(stderr,"can't get space to convert trace\n");
						return FALSE;
					}
					for ( lp1 = lp, fp1 = fp ; lp1 < lp + len ; lp1++ )
						*fp1++ = *lp1;
					if ( fwrite((void *)fp,sizeof(float),(size_t)len,newfile)
							!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					free((void *)fp);
					break;
			}
			break;
		case 'F':	/* input data are float */
			fp = (float *) seis;
			switch ( wr_fmt ) {
				case 'S':	/* F -> S */
					if ( (sp = (short *)calloc((size_t)len,sizeof(short)))
								== NULL ) {
						fprintf(stderr,"can't get space to convert trace\n");
						return FALSE;
					}
					for ( fp1 = fp, sp1 = sp ; fp1 < fp + len ; fp1++ )
						*sp1++ = ROUND(*fp1);
					if (swab_on_write)
						lswab((unsigned char *)sp,len,sizeof(short),machineByteOrder,outDataByteOrder);
					if ( fwrite((void *)sp,sizeof(short),(size_t)len,newfile)
							!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					free((void *)sp);
					break;
				case 'L':	/* F -> L */
					if ( (lp = (int *)calloc((size_t)len,sizeof(int)))
							== NULL ) {
						fprintf(stderr,"can't get space to convert trace\n");
						return FALSE;
					}
					for ( fp1 = fp, lp1 = lp ; fp1 < fp + len ; fp1++ )
						*lp1++ = ROUND(*fp1);
					if (swab_on_write)
						lswab((unsigned char *)lp,len,sizeof(int),machineByteOrder,outDataByteOrder);
					if ( fwrite((void *)lp,sizeof(int),(size_t)len,newfile)
							!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					free((void *)lp);
					break;
				case 'F':	/* F -> F */
					if ( fwrite((void *)fp,sizeof(float),(size_t)len,newfile)
							!= len ) {
						perror("UWDFwrite_new_chan: write failed");
						return FALSE;
					}
					break;
			}
			break;
	}

	++no_chans_written;

	return TRUE;
}

int UWDFdup_chead_into_proto(int chno) /* Duplicate next channel header from current "chno" of file being read */
{
	if ( ch2 == NULL ) {
		perror("UWDFdup_chead_into_proto: must first call UWDFopen_file");
		return FALSE;
	}

	new_chead_proto = *(ch2+chno);
	return TRUE;
}

int UWDFset_chname(char *chaname) /* Set name string for channel */
/* chaname must be proper string */
{
	char tmpname[8];
	/* Ensure proper termination of name */
	strncpy(tmpname,chaname,7);
	tmpname[7] = '\0';
	strib (tmpname);
	strcpy(new_chead_proto.name,tmpname);
	return TRUE;
}

int UWDFset_chcompflg(char *compflg) /* Set channel component flag */
{
	char tmpstr[4];
	strncpy(tmpstr,compflg,3);
	tmpstr[3] = '\0';
	strib(tmpstr);
	strcpy(new_chead_proto.compflg,tmpstr);
	return TRUE;
}

int UWDFset_chsrate(float new_samprate) /* Set channel sample rate in s/sec */
{
	new_chead_proto.lrate = ROUND(new_samprate * 1000.);
	return TRUE;
}

int UWDFset_chref_stime(struct stime time) /* Set channel reference time from structure type time */
{
	int tmpmin;
	float tmpsec;

	grgmin(&time.yr, &tmpmin);

	/* CKW - correct for sec < 0. or sec >= 60. */
	tmpsec = time.sec;
	while ( tmpsec < 0. ) {
		tmpmin -= 1;
		tmpsec += 60.;
	}
	while ( tmpsec >= 60. ) {
		tmpmin += 1;
		tmpsec -= 60.;
	}

	new_chead_proto.start_lmin = tmpmin;
	new_chead_proto.start_lsec = ROUND(tmpsec * 1.0e+06);

	return TRUE;    
}

int UWDFset_chid(char chanid[]) /* Set channel id string */
{
	char tmpstr[4];
	strncpy(tmpstr,chanid,3);
	tmpstr[3] = '\0';
	strib(tmpstr);
	strcpy(new_chead_proto.chid,tmpstr);
	return TRUE;
}

BOOL UWDFset_chsrc(char *chansrc) /* Set the channel source string*/
{
	char tmpstr[4];
	strncpy(tmpstr,chansrc,3);
	tmpstr[3] = '\0';
	strib(tmpstr);
	strcpy(new_chead_proto.src,tmpstr);
	return TRUE;
}

int UWDFset_chlta(int lta) /* Set channel lta value */
{
	new_chead_proto.lta = lta;
	return TRUE;
}

int UWDFset_chtrig(int trig) /* Set channel trig value */
{
	new_chead_proto.trig = trig;
	return TRUE;
}

int UWDFset_chbias(int bias) /* Set channel bias value */
{
	new_chead_proto.bias = bias;
	return TRUE;
}

/* UWDF MISCELLANEOUS GENERAL UTILITIES */

/* return distance in km, and azimuths between two coordinate locations,
	coord1 & coord2 */
float UWDFdist_az(struct sta_loc coord1, struct sta_loc coord2, float *az,
			float *baz)
{
	float delta;
	double earth_rad = 6370.8;

	if ( coord1.lat == coord2.lat && coord1.lon == coord2.lon )
		return( 0. );
	else {
		azdist(coord1.lat, coord1.lon, coord2.lat, coord2.lon,
					&delta, az, baz);
		return ( (M_PI/180.) * delta * earth_rad );
	}
}

/* return distance in km between two coordinate locations, coord1 & coord2 */
float UWDFcoord_dist(struct sta_loc coord1, struct sta_loc coord2)
{
	float az, baz;
	return ( UWDFdist_az(coord1,coord2,&az,&baz) );
}

int UWDFsta_mapping_on(char *map_filename) /* turn on station name mapping */
{
	sta_mapping = init_channel_map(map_filename);
	return sta_mapping;
}

char* UWDFmapped_sta(char *old_name) {
	int jj = channel_map(old_name);
	return jj >= 0 ? sta_map[jj].new_name : old_name;
}

char* UWDFmapped_chan(char *old_name) {
	int jj = channel_map(old_name);
	return jj >= 0 ? sta_map[jj].comp_flg : sta_map[def_map].comp_flg;
}

char* UWDFmapped_id(char *old_name) {
	int jj = channel_map(old_name);
	return jj >= 0 ? sta_map[jj].id : sta_map[def_map].id;
}

char* UWDFmapped_src(char *old_name) {
	int jj = channel_map(old_name);
	return jj >= 0 ? sta_map[jj].src : sta_map[def_map].src;
}

/* return time difference in float secs between time structures (time1 - time2) */
float UWDFtime_diff(struct stime time1, struct stime time2)
{
	int *tim1, *tim2;
	int lmin1, lmin2;

	tim1 = &time1.yr;
	grgmin(tim1, &lmin1);
	tim2 = &time2.yr;
	grgmin(tim2, &lmin2);
	return( (float)((lmin1-lmin2) * 60. + (time1.sec-time2.sec)));
}

int UWDFis_uw2(void) /* Return TRUE if read format is UW-2 */
{
	return (uw_fmt_flg == 2 ? TRUE : FALSE);
}

int UWDFmax_trace_len(void) /* Return the maximum trace lenth in current file */
{
	int i, maxlen;

	if ( ch2 == NULL ) {
		perror("UWDFmax_trace_len: must first call UWDFopen_file");
		return -1;
	}

	maxlen = 0;
	for (i = 0; i < nchannels; ++i) {
		maxlen = MAX(maxlen, (ch2+i)->chlen);
	}
	return (maxlen);
}

struct masthead *UWDFret_mast_struct(void) /* Struct -> to read masthead */
{
	if ( mh == NULL )
	perror("UWDFret_mast_struct: warning: mh not allocated");

	return (mh);
}

struct chhead2 *UWDFret_chead_struct(int i) /* Struct -> to i'th chan header */
{
	if ( ch2 == NULL )
	perror("UWDFret_chead_struct: warning: ch2 not allocated");

	return ((ch2+i));
}

struct chhead2 *UWDFret_chead_proto_struct(void) /* Struct -> to proto chhead */
{
	return (&new_chead_proto);
}

struct masthead *UWDFret_new_mhead_struct(void) /* Struct -> to new mhead */
{
	if ( new_mhead == NULL )
		perror("UWDFret_new_mhead_struct: warning: new_mhead not allocated");

	return (new_mhead);
}

int UWDFconv_to_doy(int yr, int mon, int dom, int *doy) /* Convert to day-of-year from month & day */
{
	int day_of_year, firstday;
	int igreg[5];

	/* convert from month-day to day-of-year */
	igreg[0] = yr;
	igreg[1] = 1;
	igreg[2] = 1;
	julian(igreg, &firstday);
	igreg[1] = mon;
	igreg[2] = dom;
	julian(igreg, &day_of_year);
	*doy = day_of_year - firstday + 1;
	return TRUE;
}


int UWDFconv_to_mon_day(int yr, int doy, int *mon, int *dom) /* Convert to month & day from day-of-year */
{
	int day_of_year, firstday;
	int igreg[5];

	igreg[0] = yr;
	igreg[1] = 1;
	igreg[2] = 1;
	julian(igreg, &firstday);
	day_of_year = doy + firstday -1;
	gregor(igreg, &day_of_year);
	*mon = igreg[1];
	*dom = igreg[2];
	return TRUE;
}

#define TOL 0.0001	/* tolerance of 100 micro-second */ 
/* carry seconds if necessary */
void UWDFcarry_sec(struct stime *tim)
{
	int tmpmin;
	/* get minutes since midnight, 12/31/1559 */
	grgmin((int *)tim,&tmpmin);
	/* carry seconds if outside of [0.-TOL,60.-TOL) */
	while ( tim->sec + TOL >= 60. ) {
		tim->sec -= 60.;
		tmpmin++;
	}
	while ( tim->sec + TOL < 0. ) {
		tim->sec += 60.;
		tmpmin--;
	}
	/* get new time structure from minutes */	
	mingrg((int *)tim, &tmpmin);
}

/* CALCULATE NUMBER OF MINUTES FROM MIDNITE, DEC 31, 1599 */
int grgmin(int *igreg, int *lmin)
{
	int ljul;

	julian(igreg, &ljul);
	ljul += -2305448;
	*lmin = ljul * 1440 + igreg[3] * 60 + igreg[4];
    return TRUE;
}

/* CALCULATE GREGORIAN DATE FROM MINUTES PAST MIDNITE, 12,31,1599 */
int mingrg(int *igreg, int *lmin)
{
	int imin, ljul;

	ljul = *lmin / 1440;
	imin = *lmin - ljul * 1440;
	ljul += 2305448;
	gregor(igreg, &ljul);
	igreg[3] = imin / 60;
	igreg[4] = imin - igreg[3] * 60;
	return TRUE;
}

/* LOCAL UTILITITES BELOW HERE */
/* swab bytes in place for short, int, and long long */
static void lswab(unsigned char *buf, int items, int size, char fromByteOrder, char toByteOrder)    
{
	register int i;

	/* handle input errors by returning silently */
	if ( buf == NULL || items <= 0 )
		return;
	if ( size != 2 && size != 4 && size != 8 )
		return;
	if ( fromByteOrder != 'I' && fromByteOrder != 'L' && fromByteOrder != 'D' )
		return;
	if ( toByteOrder != 'I' && toByteOrder != 'L' && toByteOrder != 'D' )
		return;

	/* handle do-nothing cases by returning */
	if ( fromByteOrder == toByteOrder )
		return;	/* input and output are the same byte order */
	if ( (fromByteOrder != 'I' && toByteOrder != 'I') && size == 2 )
		return;	/* 2-byte object with D <-> L */

	if ( size > 0 && (fromByteOrder == 'I' || toByteOrder == 'I') ) {
		/* first, swap adjacent bytes if going to/from IEEE */
		for ( i = 0 ; i < items*size ; i += 2 ) {
			register unsigned char tmp = buf[i];
			buf[i] = buf[i+1];
			buf[i+1] = tmp;
		}
	}
	if ( size > 2 && (fromByteOrder == 'L' || toByteOrder == 'L') ) {
		/* next, swap adjacent pairs of bytes */
		for ( i = 0 ; i < items*size ; i += 4 ) {
			unsigned short tmpS;
			memcpy(&tmpS,buf+i,2);
			memcpy(buf+i,buf+i+2,2);
			memcpy(buf+i+2,&tmpS,2);
		}
	}
	if ( size > 4 && (fromByteOrder == 'L' || toByteOrder == 'L') ) {
		/* next, swap adjacent quads of bytes */
		for ( i = 0 ; i < items*size ; i += 8 ) {
			unsigned int tmpL;
			memcpy(&tmpL,buf,4);
			memcpy(buf+i,buf+i+4,4);
			memcpy(buf+i+4,&tmpL,4);
		}
	}
	return;
}

static void strib(char *string) /* strip trailing blanks or newline */
{
	int i,length;
	length = strlen(string);
	if (length == 0) {
		return;
	} else {
		for(i = length-1; i >= 0; i--) {
			if (string[i] == ' ' || string[i] == '\n') {
				string[i] = '\0';
			} else {
				return;
			}
		}
		return;
	}
}

static int mo[24] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273,
    304, 334, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };
static void julian(int *igreg, int *ljul)
{
	/* Initialized data */
	static int d100 = 36524;
	static int d400 = 146097;
	static int d4 = 1461;
	int leap, n4, n100, n400, ida, imo, iyr, jyr;
	/* FUNCTION: CONVERT GREGORIAN DATE TO JULIAN DATE */
	/* 4 YEAR, 100 YEAR AND 400 YEAR RULES OBSERVED */
	/* BLAME: CARL JOHNSON */
	iyr = igreg[0];
	imo = igreg[1];
	ida = igreg[2];
	*ljul = 0;
	jyr = iyr;
	if (jyr >= 1) {
		--jyr;
		*ljul = 365;
		/* FOUR HUNDRED YEAR RULE */
		n400 = jyr / 400;
		*ljul += n400 * d400;
		jyr -= n400 * 400;
		/* 100 YEAR RULE */
		n100 = jyr / 100;
		*ljul += n100 * d100;
		jyr -= n100 * 100;
		/* 4 YEAR RULE */
		n4 = jyr / 4;
		*ljul += n4 * d4;
		jyr -= n4 << 2;
		/* ONE YEAR RULE */
		*ljul += jyr * 365;
	}
	leap = 0;
	if (iyr % 4 == 0)
		leap = 12;
	if (iyr % 100 == 0)
		leap = 0;
	if (iyr % 400 == 0)
		leap = 12;
	*ljul = *ljul + mo[imo + leap - 1] + ida + 1721060;
	return;
}

static void gregor(int *igreg, int *ljul)
{
	/* System generated locals */
	int i_1;
	/* Local variables */
	int leap, left, i, ltest, ig[5];
	/* FUNCTION: CALCULATE GREGORIAN DATE FROM JULIAN DATE */
	/* BLAME: CARL JOHNSON (11 JULY 1979) */
	ig[0] = (*ljul - 1721061) / 365;
	ig[1] = 1;
	ig[2] = 1;
	julian(ig, &ltest);
	if (ltest <= *ljul)
		goto L110;
L20:
	--ig[0];
	julian(ig, &ltest);
	if (ltest > *ljul)
		goto L20;
	goto L210;
L105:
	++ig[0];
	julian(ig, &ltest);
	L110:
	if ((i_1 = ltest - *ljul - 366) < 0) {
		goto L210;
	} else if (i_1 == 0) {
		goto L120;
	} else {
		goto L105;
	}
L120:
	if (ig[0] % 400 == 0)
		goto L210;
	if (ig[0] % 100 == 0)
		goto L105;
	if (ig[0] % 4 == 0)
		goto L210;
	goto L105;
L210:
	left = *ljul - ltest;
	igreg[0] = ig[0];
	leap = 0;
	if (ig[0] % 4 == 0)
		leap = 12;
	if (ig[0] % 100 == 0)
		leap = 0;
	if (ig[0] % 400 == 0) leap = 12;
	for (i = 2; i <= 12; ++i) {
		if (mo[i + leap - 1] <= left)
			goto L250;
		igreg[1] = i - 1;
		igreg[2] = left - mo[i + leap - 2] + 1;
		return;
L250: ;
	}
	igreg[1] = 12;
	igreg[2] = left - mo[leap + 11] + 1;
	return;
}

static void set_swab_on_read(void)
{
	/* Determine if we are big (I for IEEE), little (L for little), or DEC (D for DEC) endian */
	/* CKW: Big-endian byte order = 4321; Little-endian = 1234; DEC-endian = 3412
		For compatibility, data should be stored as DEC-endian or big-endian.
		*/
	short i2 = 1;
	int i4 = 1;
	char *c2 = (char *) &i2, *c4 = (char *) &i4;
	
	/* determine machine byte order */
	machineByteOrder = (*c4 == 1) ? 'L' : (*c2 == 1) ? 'D' /* whoa, time warp */ : 'I';

	/* determine data byte order */
	if (uw_fmt_flg == 1) {
		/* If UW-1 format, use first two bytes to determine byte order.
			This is a kludge that will fail when nchan > 255 */
		char *ctest = (char *)&mh->nchan;
		dataByteOrder = ! *ctest ? 'I' : 'D';	/* assume non-IEEE UW-1 data is always DEC-endian */
	} else { /* UW-2 format; interpret mh->extra[1] */
		dataByteOrder = (mh->extra[1] == ' ' || mh->extra[1] == '\0') ? 'I' : mh->extra[1];
	}

	swab_on_read = (dataByteOrder != machineByteOrder) ? TRUE : FALSE;
	return;
}

static void set_swab_on_write() {
	short i2 = 1;
	int i4 = 1;
	char *c2 = (char *) &i2, *c4 = (char *) &i4;
	
	/* determine machine byte order */
	machineByteOrder = (*c4 == 1) ? 'L' : (*c2 == 1) ? 'D' /* whoa, time warp */ : 'I';

	/* determine output data byte order */
#if defined(WRITE_DEC_IF_LITTLE_ENDIAN)
	/* for compatibility with ancient codes, force little-endian machines to write DEC-endian */
	outDataByteOrder = machineByteOrder == 'L' ? 'D' : machineByteOrder;
#elif defined(WRITE_IEEE)
	/* for compability with external codes, always write big-endian byte order */
	outDataByteOrder = 'I';
#elif defined(WRITE_LITTLE_ENDIAN)
	/* for some reason, always write little-endian byte order */
	outDataByteOrder = 'L';
#else
	/* if not set, just write data using machine's byte order */
	if ( outDataByteOrder == '\0' )
		outDataByteOrder = machineByteOrder;
#endif

	swab_on_write = machineByteOrder != outDataByteOrder ? TRUE : FALSE;
}

static int init_channel_map(char *map_filename) /* Initialize channel map from external file */
{
	/* Read channel mapping info from default file */
	int i;
	size_t siz;
	char line[LNSIZ], s1[LNSIZ], s2[LNSIZ], s3[LNSIZ], s4[LNSIZ], s5[LNSIZ];
	FILE *ftmp;

	if ( map_filename == NULL ) {
		return FALSE;
	}

	/* Read the mapping file; if file not available, return FALSE */
	i = 0;
	if ((ftmp = fopen(map_filename, "r")) != NULL) { /* found mapping file */
		while (fgets(line, sizeof(line), ftmp) != NULL) {
			if (line[0] == '#')
				continue;
			siz = (i + 1) * sizeof(struct sm);
#if defined(sun) || defined(__sun)
			if ( sta_map == (struct sm *)NULL ) {
				if ( (sta_map = (struct sm *) malloc(siz))
							== (struct sm *)NULL ) {
					fprintf(stderr,"Can't get station map space\n");
					exit(1);
				}
			} else {
				if ( (sta_map = (struct sm *) realloc((void *)sta_map,siz))
							== (struct sm *)NULL ) {
					fprintf(stderr,"Can't get station map space\n");
					exit(1);
				}
			}
#else
			if ( (sta_map = (struct sm *) realloc((void *)sta_map,siz))
					== (struct sm *)NULL ) {
				fprintf(stderr,"Can't get station map space\n");
				exit(1);
			}
#endif
			/* read line, avoid buffer overruns */
			sscanf(line,"%s %s %s %s %s", s1, s2, s3, s4, s5);
			strncpy(sta_map[i].old_name,s1,7); sta_map[i].old_name[7] = '\0';
			strncpy(sta_map[i].new_name,s2,7); sta_map[i].new_name[7] = '\0';
			strncpy(sta_map[i].comp_flg,s3,3); sta_map[i].comp_flg[3] = '\0';
			strncpy(sta_map[i].id,s4,3); sta_map[i].id[3] = '\0';
			strncpy(sta_map[i].src,s5,3); sta_map[i].src[3] = '\0';

			if (strncmp(sta_map[i].comp_flg, "\"\"",3) == 0)
				sta_map[i].comp_flg[0] = '\0';
			if (strncmp(sta_map[i].comp_flg, "none",3) == 0)
				sta_map[i].comp_flg[0] = '\0';
			if (strncmp(sta_map[i].id, "\"\"",3) == 0)
				sta_map[i].id[0] = '\0';
			if (strncmp(sta_map[i].id, "none",3) == 0)
				sta_map[i].id[0] = '\0';
			if (strncmp(sta_map[i].src, "\"\"",3) == 0)
				sta_map[i].src[0] = '\0';
			if (strncmp(sta_map[i].src, "none",3) == 0)
				sta_map[i].src[0] = '\0';
			if (strncmp(sta_map[i].old_name, "any", 7) == 0)
				def_map = i;
			if (strncmp(sta_map[i].old_name, "default", 7) == 0)
				def_map = i;
			++i;
		}
		fclose(ftmp);
	} else { /* no station mapping file */
		fprintf(stderr,"UWdfif: Station mapping off: Can't open %s:\n\t%s\n",
				map_filename, strerror(errno));
		return FALSE;
	}
	nmap = i;
	/* Already read mapping file; go straight to mapping function */
	return TRUE;
}

static int channel_map(char *old_name) /* return index to channel mapping for UW-1 to UW-2 */
{
	int i;
	for (i = 0; i < nmap; ++i) {
		if (strcmp(sta_map[i].old_name, old_name) == 0) {
			return i;
		}
	}
	return -1;
}

/* get_station_locs loads the station
location structures for each station from the complete station table;
this would be best done with a station table server, rather than
a static internal table */
static void get_station_locs(void) /* Load station coordinates table */
{
	int i,j;
	char s[8];

	if (n_sta_table == 0)
	return;
	for (i = 0; i < nchannels; ++i) { /* Fill loc table from complete table */
		strcpy(s, (ch2+i)->name);
		for (j = 0; j < n_sta_table; ++j) {
			if (strcmp(s, complete_sta_table[j].name) == 0) {
				sta_loc_table[i].lat = complete_sta_table[j].lat;
				sta_loc_table[i].lon = complete_sta_table[j].lon;
				sta_loc_table[i].elev = complete_sta_table[j].elev;
				sta_loc_table[i].plflg = complete_sta_table[j].plflg;
				strcpy(sta_loc_table[i].stcom,complete_sta_table[j].stcom);
				break;
			}
		}
		if (j == n_sta_table) { /* went through complete station table;
			station not found; issue warning and set invalid values */
			sta_loc_table[i].lat = sta_loc_table[i].lon = 1000.;
			sta_loc_table[i].elev = 0.0;
			sta_loc_table[i].plflg = ' ';
			sta_loc_table[i].stcom[0] = '\0';
			/* CKW - add more general timecode support */
			if ( ISTIMECODE(s) )
				continue;
			if (strcmp(s, "ZZZ") == 0)
				continue;
			fprintf(stderr,"WARNING: uwdfif - station %s not in station table\n", s);
		}
	}
	return;
}

/* get_revs assigns polarity reversals to each channel from the complete
   station reversal table */
/* determine what stas have reversed first motion at this time, and fill
   table is_rev[] */
static void get_revs(void)
{
	int now, i;
	struct stime time;
	int *t;

	/* set reversal flags all false */
	for ( i = 0 ; i < nchannels ; ++i )
		is_rev[i] = FALSE;
	if ( n_rev_table == 0 )
		return;
	t = &time.yr;
	mingrg(t, &((ch2)->start_lmin));
	/* get 8-digit number yyyymmdd representing current date */
	now = 10000 * time.yr + 100 * time.mon + time.day;
	for ( i = 0 ; i < n_rev_table ; ++i ) {
		int is_reversed = FALSE;
		char *tp, reversed_sta_name[10];
		char tmpline[LNSIZ];

		/* copy line for strtok to modify, and grab station name  */
		strcpy(tmpline,complete_rev_table[i].line);
		if ( (tp = strtok(tmpline," \t")) == (char *)NULL )
			continue;
		if ( ! strlen(tp) )
			continue;
		strcpy(reversed_sta_name, tp);
		while ( (tp = strtok((char *)NULL," \t")) != (char *)NULL ) {
			int early, late, tmpnow = now;
			sscanf(tp,"%d-%d",&early,&late);
			/* handle old-style, 6-digit dates */
			if ( late < 1000000 )
				tmpnow %= 1000000;
			if ( tmpnow >= early && now <= late ) {
				is_reversed = TRUE;
				break;
			}
		}
		if ( is_reversed ) {
			int j;
			for ( j = 0 ; j < nchannels ; ++j ) {
				if ( strcmp(reversed_sta_name, (ch2+j)->name) == 0 ) {
					is_rev[j] = TRUE;
					continue; /* handle multiple traces with same name */
				}
			}
		}
	}
	return;
}

static void azdist(float stalat, float stalon, float evtlat, float evtlon,
    float *delta, float *az, float *baz)
{
	/* Local variables */
	double dbaz, elon, slon;
	double rad, piby2;
	double a, b, c, d, e, g, h, k, aa, bb, cc, dd, ee, gg, hh, kk, 
			ecolat, scolat, del, daz, sph, rhs1, rhs2;

/* Subroutine to calculate the Great Circle Arc distance */
/*    between two sets of geographic coordinates */

/* Given:  stalat => Latitude of first point (+N, -S) in degrees */
/* 	  stalon => Longitude of first point (+E, -W) in degrees */
/* 	  evtlat => Latitude of second point */
/* 	  evtlon => Longitude of second point */

/* Returns:  delta => Great Circle Arc distance in degrees */
/* 	    az    => Azimuth from pt. 1 to pt. 2 in degrees */
/* 	    baz   => Back Azimuth from pt. 2 to pt. 1 in degrees */

/* If you are calculating station-epicenter pairs, pt. 1 is the station */

/* Equations taken from Bullen, pages 154, 155 */

/* T. Owens, September 19, 1991 */
/* Sept. 25 -- fixed az and baz calculations */

	piby2 = M_PI / 2.;
	rad = M_PI / 180.;

/* scolat and ecolat are the geocentric colatitudes */
/* as defined by Richter (pg. 318) */

/* Earth Flattening of 1/298.257 take from Bott (pg. 3) */

	sph = .00335281317789691;
	scolat = piby2 - atan((1. - sph) * (1. - sph) * tan((stalat) * rad));
	ecolat = piby2 - atan((1. - sph) * (1. - sph) * tan((evtlat) * rad));
	slon = (stalon) * rad;
	elon = (evtlon) * rad;

/*  a - e are as defined by Bullen (pg. 154, Sec 10.2) */
/*     These are defined for the pt. 1 */

	a = sin(scolat) * cos(slon);
	b = sin(scolat) * sin(slon);
	c = cos(scolat);
	d = sin(slon);
	e = -cos(slon);
	g = -c * e;
	h = c * d;
	k = -sin(scolat);

/*  aa - ee are the same as a - e, except for pt. 2 */

	aa = sin(ecolat) * cos(elon);
	bb = sin(ecolat) * sin(elon);
	cc = cos(ecolat);
	dd = sin(elon);
	ee = -cos(elon);
	gg = -cc * ee;
	hh = cc * dd;
	kk = -sin(ecolat);

/*  Bullen, Sec 10.2, eqn. 4 */

	del = acos(a * aa + b * bb + c * cc);
	*delta = del / rad;

/*  Bullen, Sec 10.2, eqn 7 / eqn 8 */

/*    pt. 1 is unprimed, so this is technically the baz */

/*  Calculate baz this way to avoid quadrant problems */

	rhs1 = (aa - d)*(aa - d) + (bb - e)*(bb - e) + cc * cc - 2.;
	rhs2 = (aa - g)*(aa - g) + (bb - h)*(bb - h) + (cc - k)*(cc - k) - 2.;
	dbaz = atan2(rhs1, rhs2);
	if (dbaz < 0.)
		dbaz += M_PI * 2;
	*baz = dbaz / rad;

/*  Bullen, Sec 10.2, eqn 7 / eqn 8 */

/*    pt. 2 is unprimed, so this is technically the az */

	rhs1 = (a - dd)*(a - dd) + (b - ee)*(b - ee) + c*c - 2.;
	rhs2 = (a - gg)*(a - gg) + (b - hh)*(b - hh) + (c - kk)*(c - kk) - 2.;
	daz = atan2(rhs1, rhs2);
	if (daz < 0.)
		daz += M_PI * 2;
	*az = daz / rad;

/*   Make sure 0.0 is always 0.0, not 360. */

	if ( ABS(*baz - 360.) < 1e-5 ) {
		*baz = 0.;
	}
	if ( ABS(*az - 360.) < 1e-5 ) {
		*az = 0.;
	}
	return;
} /* azdist */

/* Find earliest start time among all channels, and set this into the
master header time field;  This makes it possible to always use the
function UWDFdhref_stime() to return the minimum channel start time */
static void set_earliest_starttime(void)
{
	int i, nchan, temp;
	int minimum_lmin, minimum_lsec, tmp_lmin, tmp_lsec;
	nchan = UWDFdhnchan();
	temp = (ch2+0)->start_lsec;
	minimum_lmin = temp/60000000 + (ch2+0)->start_lmin;
	minimum_lsec = temp%60000000;
	for (i = 1; i < nchan; ++i) {
		temp = (ch2+i)->start_lsec;
		tmp_lmin = temp/60000000 + (ch2+i)->start_lmin;
		tmp_lsec = temp%60000000;
		if (tmp_lmin < minimum_lmin) { /* clearly earlier time */
			minimum_lmin = tmp_lmin;
			minimum_lsec = tmp_lsec;
		} else if (tmp_lmin == minimum_lmin) { /* may be earlier */
			if (tmp_lsec < minimum_lsec) { /* Yes, earlier */
				minimum_lsec = tmp_lsec;
			}
		}
	}
	mh->lmin = minimum_lmin;
	mh->lsec = minimum_lsec;
	return;
}
