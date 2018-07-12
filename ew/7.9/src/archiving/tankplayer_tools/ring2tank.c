/* ring2tank - a quick and dirty module to write tracebuf2's to a tankplayer format,
   for testing and playback of raw waveforms from a ring
	
	Paul Friberg, ISTI, May 28, 2007 
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <earthworm.h>
#include <transport.h>
#include <trace_buf.h>

#define VER_NO "v0.0.5 2013-08-09"

int main (int argc, char ** argv) {
FILE *fp;
int RingKey, ret; 
int verbose=0;
int tbuf1=0;
long MsgLen;
SHM_INFO  Region;
MSG_LOGO  GetLogo, ReturnedLogo;
TracePacket tpkt;
TRACE2_HEADER *thdr;
unsigned long pkt_counter=0;
double num_bytes =0.0;

	if (argc < 2) {
		fprintf(stderr, "ring2tank RINGNAME tankfilename [1|v]\n");
		fprintf(stderr, "ring2tank version %s\n", VER_NO);
		fprintf(stderr, " v - option to echo each packet header\n");
		fprintf(stderr, " 1 - option to record Tracebuf1s, default is Tracebuf2s\n");
    		exit( 1 );
	}

	if ( argc==4 && (argv[3][0]=='v' || argv[3][1] == 'V') ) {
		verbose=1;
	}
	if ( argc==4 && argv[3][0]=='1' ) {
		tbuf1=1;
	}

	if (  (RingKey = GetKey(argv[1])) == -1) {
    		fprintf(stderr, "ring2tank: error finding ring %s\n", argv[1]);
    		exit( 1 );
  	}

	if ( (fp = fopen(argv[2], "wb")) == (FILE *)NULL) {
    		fprintf(stderr, "ring2tank: error opening %s\n", argv[2]);
    		exit( 1 );
  	}
	if ( GetType( "TYPE_TRACEBUF2", &GetLogo.type ) != 0 ) {
      		fprintf( stderr, "ring2tank: Error getting TYPE_TRACEBUF2.\n" );
    		exit( 1 );
  	}
	if ( tbuf1 ) {
                if ( GetType( "TYPE_TRACEBUF", &GetLogo.type ) != 0 ) {
      			fprintf( stderr, "ring2tank: Error getting TYPE_TRACEBUF.\n" );
    			exit( 1 );
		}
  	}
	if ( GetInst( "INST_WILDCARD", &GetLogo.instid ) != 0 ) {
      		fprintf( stderr, "ring2tank: Error getting INST_WILDCARD.\n" );
    		exit( 1 );
  	}
	if ( GetModId( "MOD_WILDCARD", &GetLogo.mod ) != 0 ) {
      		fprintf( stderr, "ring2tank: Error getting MOD_WILDCARD.\n" );
    		exit( 1 );
  	}
	tport_attach( &Region, RingKey );

	/* flush the contents first, only get traces from start of program */
	if (verbose)
		fprintf(stderr, "ring2tank: flushing old msgs in %s\n", argv[1]);
	while( tport_getmsg(&Region, &GetLogo, (short) 1, 
		&ReturnedLogo, &MsgLen, 
		(char*) &tpkt, MAX_TRACEBUF_SIZ) != GET_NONE );

	thdr = (TRACE2_HEADER *) &tpkt;


	while(tport_getflag( &Region ) != TERMINATE) {
		ret = tport_getmsg(&Region, &GetLogo, (short) 1, &ReturnedLogo, &MsgLen,  (char*) &tpkt, MAX_TRACEBUF_SIZ);
		if ( ret == GET_NONE ) {
         		sleep_ew( 100 );
         		continue;
      		}
		if (ret == GET_MISS || ret == GET_NOTRACK || 
		    ret == GET_TOOBIG || ret == GET_MISS_LAPPED || 
		    ret == GET_MISS_SEQGAP ) {
			fprintf(stderr, "ring2tank: problem reading from ring, miss, toobig, or lapped\n");
			continue;
		}
		/* we should have a tracepkt here */
		if (MsgLen > 0 && MsgLen <= MAX_TRACEBUF_SIZ) {
			/* write it out to the file */
			ret = fwrite( (void *) &tpkt, (size_t) 1, (size_t) MsgLen, fp ); 
			if (ret != MsgLen) {
				fprintf(stderr, "ring2tank: problem writing trace packet from ring of size: %ld bytes to file %s; fwrite() wrote %d bytes\n", MsgLen, argv[2], ret);
			} else {
				if (verbose) 
					fprintf(stderr, "wrote packet num %ld  size %ld bytes, %s.%s.%s.%s\n", pkt_counter, MsgLen,
					thdr->sta, thdr->net, thdr->chan, thdr->loc);
				pkt_counter++;
				num_bytes += MsgLen;
			}
		} else {
			fprintf(stderr, "ring2tank: trace packet from ring wrong size: %ld bytes\n", MsgLen);
		}
	}
	fclose(fp);
	fprintf(stderr, "Wrote %ld trace packets, %f bytes\n", pkt_counter, num_bytes);
        return(0);
}
