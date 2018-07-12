#include <stdio.h>
#include <unistd.h>
#include "earthworm.h"
#include "transport.h"
#include "externs.h"
#include "gcf_input_types.h"
#include "gcf_udp.h"


extern unsigned long   total_dropped_udp_packet_counter;
extern unsigned long   total_udp_packet_counter;
extern unsigned long   blocks_read;
extern unsigned long   blocks_missed; 
extern char input_type;

/* errmap = an optional integer value that maps to a statmgr error 
	see the die.h and gcf2ew.desc
*/

void gcf2ew_die( int errmap, char * str ) {

/* prototype for message_send() from heart.c */
void message_send( unsigned char, short, char *);

	if (errmap != -1) {
		/* use the statmgr reporting to notify of this death */
#ifdef DEBUG
		fprintf(stderr, "SENDING MESSAGE to statmgr: %d %s\n",
			errmap, str);
#endif /* DEBUG */
		message_send(TypeErr, errmap, str);
	}

	if (input_type== GCF_INPUT_GCFSERV) {
		blocks_read = total_udp_packet_counter;
		blocks_missed = total_dropped_udp_packet_counter;
		/* tell the server we are done */
		gcf_udp_client_sendcmd(gcf_socket_fd, GCF_STOP_DATA);
	}
	logit("et", "%s: Total Blocks Read %ld, Total blocks missed %ld\n", 
		Progname, blocks_read, blocks_missed);
	
	/* this next bit must come after the possible tport_putmsg()
		above!! */
	logit("et", "%s: Exiting because %s\n", Progname, str);
	if (Region.mid != -1) {
		/* we attached to an EW ring buffer */
		tport_detach( &Region );
	} 

	if (gcf_socket_fd  != -1) {
		close(gcf_socket_fd);
	}
	exit(0);
}
