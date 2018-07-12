/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2ew_buffer.c 3540 2009-01-21 16:17:28Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.10  2009/01/21 16:17:28  tim
 *     cleaned up and adjusted data collection for minimal latency
 *
 *     Revision 1.9  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

#include "samtac2ew_buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include "samtac_comif.h"
#include "glbvars.h"
#include "samtac2ew_errs.h"
#include "terminat.h"



queue* QueueCreate(int max_size) { //Returns the queue object
	queueNodeT *current;
	int i = 0;
	queue *my_list = (queue *)malloc(sizeof (queue));
	my_list->start = (queueNodeT *)malloc(sizeof (queueNodeT));
	my_list->end = my_list->start;
	current = my_list->start;
	current->next = my_list->start;
	
	for (i=1;i<max_size;i++){
		current->next = (queueNodeT *)malloc(sizeof (queueNodeT));
		current->next->next = my_list->start;
		current = current->next;
	}
	return my_list;
}
void QueueInsert(queue *my_list, char * buffer, int buffer_length) {
	//start at my_list->end
	int i;
	//printf("DEBUG: Entering QueueInsert\n");

	for (i=0; i< buffer_length; i++) {
		my_list->end->value = buffer[i];
		my_list->end = my_list->end->next;
	}
}
int Queue_packetStart(queue *my_list, char * device_id) {
	static char msg_txt[180];
	//Find the location of STX
	queueNodeT * current = my_list->start;
	last_found_packetstart = 0;
	//printf("DEBUG: Entering packetStart\n");

	while (last_found_packetstart < (PACKET_MAX_SIZE * 3)){
		if (current == my_list->end || current->next->next->next->next->next->next->next == my_list->end){
			//We got to the end of the data without finding a proper STX.  
			//Get more data
			//printf("PacketStart: entering queuefill\n");
			QueueFill(my_list, PACKET_HEADER_SIZE);
			//printf("PacketStart: done with queueFill\n");
			continue;
			//return 0;
		}
		if (current->value == 0x02) {
			//printf("PACKETSTART: value was 0x02\n");
			if (current->next->next->next->next->next->next->value == device_id[0] && current->next->next->next->next->next->next->next->value == device_id[1]) {
				//printf("PACKETSTART: other values were device_id\n");
				my_list->start = current;
				if (gcfg_debug > 3) {
					printf( "size = %x, %x, %x\n", current->next->next->value, current->next->next->next->value, current->next->next->next->next->value);
				}
				return (0xff00 & ((current->next->next->value) << 8)) + (0xff & (current->next->next->next->value) << 0);
			}
		}
		current = current->next;
		last_found_packetstart++;
	}
	//If it gets here, data is corrupted or deviceID is incorrect, exit
	sprintf (msg_txt, "Data is corrupted or DeviceID is incorrect");
	samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_COMMERR, msg_txt);
	samtac2ew_exit(0);

	
}

void QueueExportBuffer(queue *my_list, char * buffer, int buffer_length) {
	//buffer must be pre allocated
	int i = 0;
	for (i=0; i< buffer_length; i++) {
		if (my_list->start == my_list->end) {
			//Get some more data
			QueueFill(my_list, buffer_length - i);
		}
		buffer[i] = my_list->start->value;
		my_list->start = my_list->start->next;
	}
}

void QueueFill(queue *my_list, int size){
	static char msg_txt[180];

	int retval;
	//printf("DEBUG: Entering QueueFill\n");

	if ((retval = samtac_recv_buff(samtac_initial_buffer, size,
                                 gcfg_commtimeout_itvl, 0)) <= 0){ //gcfg_commtimeout_itvl is the timeout in ms
		//Error!
		sprintf (msg_txt, "Received:  %d bytes  after timeout of %d ms'\n", retval, gcfg_commtimeout_itvl);
		//logit("et", "samtac2ew: %s\n", msg_txt);
		samtac2ew_enter_exitmsg(SAMTACTERM_SAMTAC_COMMERR, msg_txt);
		samtac2ew_exit(0);                  /* exit program with code value */

		//exit(dcnt);
	}
	if (gcfg_debug>2) { logit("et", "buffer QueueFill: received:  %d bytes\n", retval); }
	QueueInsert(my_list, samtac_initial_buffer, retval);
}
	