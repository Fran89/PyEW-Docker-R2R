// tr2dump.c - dump tank files
//


#include <stdio.h>
#include <swap.h>
#include <stdlib.h>
#include <string.h>
#include <trace_buf.h>

void usage(void);
int dump_packets(char* sTank);
int overlap_packets( char* sTank, char* sOutTank, long n, double amt);
int rename_packets(char* sTank, char* sOutTank, char* sSta, char* sNet, char* sChan, char* sLoc);

int main(int argc, char** argv) {


	if(argc > 1) {
		if(strcmp(argv[1],"-d") == 0) {
			dump_packets(argv[2]);
		} else if(strcmp(argv[1],"-a") == 0) {
			overlap_packets(argv[2], argv[5], atol(argv[3]), atof(argv[4]));
		} else if(strcmp(argv[1], "-r") == 0) {
			rename_packets(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
		} else {
			usage();
		}
	} else {
		usage();
	}
	return 0;
}

/*******************************************************************
 usage - print help to stdout
********************************************************************/
void usage() {
		fprintf(stderr,"tr2dump.exe - dump tank data including TRACE2_HEADER data.\n\n");
		fprintf(stderr,"usage: tr2dump.exe -[d|a|r] tankfile [n adjust output_tankfile] \n\n");
		fprintf(stderr,"\t-d : dump formatted tank info to stdout.\n");
		fprintf(stderr,"\t-a : Add 'adjust' epoch seconds to every 'n'th trace_buf\n");
		fprintf(stderr,"\t-r : Rename SCNL components STA NET CHAN LOC.\n");
		fprintf(stderr,"\tand write to output_tankfile.\n");
}
/********************************************************************
 dump_packets - dumps packet data to stdout
*********************************************************************/
int dump_packets(char* sTank) {

	FILE *fp;
	TRACE2_HEADER trh;
	int32_t buf[MAX_TRACEBUF_SIZ];
	long i,j;
	long br;
	int rc = 0;
	int n;
	int bailOut;
        int32_t nsamp;
	double stime, etime;

	if((fp = fopen(sTank, "rb")) != NULL) {

		n= 0;
		bailOut = 0;
		printf("offset = %ld\n", ftell(fp));

		while((fread(&trh, sizeof(trh), 1, fp) == 1) && !bailOut) {
			nsamp = trh.nsamp;
			stime = trh.starttime;
			etime = trh.endtime;
#ifdef _INTEL			
			if (trh.datatype[0] == 's') {
				SwapInt32(&nsamp);
				SwapDouble(&stime);
				SwapDouble(&etime);
			}
#endif
#ifdef _SPARC			
			if (trh.datatype[0] == 'i') {
				SwapInt32(&nsamp);
				SwapDouble(&stime);
				SwapDouble(&etime);
			}
#endif
			printf("HEADER INDEX = %d\n", n++);
			printf("TRACE2_HEADER.pinno: %d\n", trh.pinno);
			printf("TRACE2_HEADER.nsamp: %d\n", (int)nsamp);
			printf("TRACE2_HEADER.starttime: %lf\n", stime);
			printf("TRACE2_HEADER.endtime: %lf\n", etime);
			printf("TRACE2_HEADER.sta: %.4s\n", trh.sta);
			printf("TRACE2_HEADER.net: %.2s\n", trh.net);
			printf("TRACE2_HEADER.chan: %.3s\n", trh.chan);
			printf("TRACE2_HEADER.loc: %.2s\n", trh.loc);
			printf("TRACE2_HEADER.version: %.2s\n", trh.version);
			printf("TRACE2_HEADER.datatype: %.3s\n", trh.datatype);
			printf("TRACE2_HEADER.quality: %.2s\n", trh.quality);
			printf("TRACE2_HEADER.pad: %.2s\n", trh.pad);

			/* read data*/
			if((strcmp(trh.datatype,"i2") == 0) ||
				(strcmp(trh.datatype,"s2") == 0)) 
			{

				printf("offset = %ld\n", ftell(fp));
				printf("Data[");
				for(i=0, br=0; i<nsamp; i+=br) {
					br = (long) fread(buf, 2, nsamp, fp);
					for(j=0; j<br; j++) {
						short sword;
						memcpy((void *) &sword, ((char *)buf + (j * 2)), 2);
#ifdef _INTEL			
						if (trh.datatype[0] == 's') {
							SwapShort(&sword);
						}
#endif
#ifdef _SPARC			
						if (trh.datatype[0] == 'i') {
							SwapShort(&sword);
						}
#endif
					
						printf("%s%d", ((j)?",":""), sword);
					}
				}
				printf("]\n");
				printf("offset = %ld\n", ftell(fp));

			} else if((strcmp(trh.datatype,"i4") == 0) ||
					(strcmp(trh.datatype,"s4") == 0))
			{
				printf("offset = %ld\n", ftell(fp));
				printf("Data[");
				for(i=0, br=0; i<nsamp; i+=br) {
					br = (long) fread(buf, sizeof(int32_t), nsamp, fp);
					for(j=0; j<br; j++) {
						int32_t lword;
						memcpy((void *)&lword, (void *)&buf[j], 4);
#ifdef _INTEL			
						if (trh.datatype[0] == 's') {
							SwapInt32(&lword);
						}
#endif
#ifdef _SPARC			
						if (trh.datatype[0] == 'i') {
							SwapInt32(&lword);
						}
#endif
						printf("%s%ld", ((j)?",":""), (long)lword);
					}
				}
				printf("]\n");
				printf("offset = %ld\n", ftell(fp));

			} else {
				printf("Data[ Unsupported datatype]\n");
				bailOut = 1;
			}
			printf("offset = %ld\n", ftell(fp));
		}

		/* check for errors */		if(ferror(fp) ) {
			printf("Error reading TRACE2_HEADER.\n");
		}


		fclose(fp);

	}
	return rc;
}
/****************************************************************
 overlap_packets - overlaps every nth packet by given amount (NOTE this was for making test data with overlaps on win only)

NO SWAPPING DONE HERE...

*****************************************************************/
int overlap_packets( char* sTank, char* sOutTank, long n, double amt) {

	FILE *fp;
	FILE* fpOut;
	TRACE2_HEADER trh;
	int32_t buf[MAX_TRACEBUF_SIZ];
	long i;
	int32_t br;
	int rc = 0;
	int bailOut;
	double runningAdjust = 0.0;

	if((fp = fopen(sTank, "rb")) == NULL) {
		return 0;
	}
	if((fpOut = fopen(sOutTank, "wb")) == NULL) {
		fclose(fp);
		return 0;
	}

	i = 0;
	bailOut = 0;
	while((fread(&trh, sizeof(trh), 1, fp) == 1) && !bailOut) {

		/*adjust header if we are on an nth header
		increment runningAdjust indexed from 1. */
		runningAdjust += ((i+1) % n) ? 0.0 : amt;

		/* always adjust by running adjustment*/
		trh.starttime += runningAdjust;
		trh.endtime += runningAdjust;

		/* output header*/
		fwrite(&trh, sizeof(trh), 1, fpOut);

		/* copy data*/
		if((strcmp(trh.datatype,"i4") == 0) ||
			(strcmp(trh.datatype,"s4") == 0))
		{
			br = (int32_t) fread(buf, sizeof(int32_t), trh.nsamp, fp);
			if(br == trh.nsamp) {
				fwrite(buf, sizeof(int32_t), br, fpOut);
			} else {
				printf("Error: reading trace data.");
				bailOut = 1;
			}

		} else {
			printf("Error: Unsupported datatype [%2.2s]\n", trh.datatype);
			bailOut = 1;
		}
		i++;
	}
	if(ferror(fp)) {
		printf("Error reading TRACE2_HEADER.\n");
	}

	return rc;

}

/****************************************************************
 rename_packets - overlaps every nth packet by given amount
*****************************************************************/
int rename_packets( char* sTank, char* sOutTank, char* sSta, char* sNet, char* sChan, char* sLoc) {

	FILE *fp;
	FILE* fpOut;
	TRACE2_HEADER trh;
	int32_t buf[MAX_TRACEBUF_SIZ];
	long i;
	int32_t br;
	int rc = 0;
	int bailOut;

	if((fp = fopen(sTank, "rb")) == NULL) {
		return 0;
	}
	if((fpOut = fopen(sOutTank, "wb")) == NULL) {
		fclose(fp);
		return 0;
	}

	i = 0;
	bailOut = 0;
	while((fread(&trh, sizeof(trh), 1, fp) == 1) && !bailOut) {

		if(sSta) {
			strncpy(trh.sta, sSta, sizeof(trh.sta)-1);
		}
		if(sNet) {
			strncpy(trh.net, sNet, sizeof(trh.net)-1);
		}
		if(sChan) {
			strncpy(trh.chan, sChan, sizeof(trh.chan)-1);
		}
		if(sLoc) {
			strncpy(trh.loc, sLoc, sizeof(trh.loc)-1);
		}

		/* output header*/
		fwrite(&trh, sizeof(trh), 1, fpOut);

		/* copy data*/
		br = (int32_t) fread(buf, sizeof(int32_t), trh.nsamp, fp);
		if(br == trh.nsamp) {
			fwrite(buf, sizeof(int32_t), br, fpOut);
		} else {
			printf("Error: reading trace data.");
			bailOut = 1;
		}

		i++;
	}
	if(ferror(fp)) {
		printf("Error reading TRACE2_HEADER.\n");
	}

	return rc;

}
