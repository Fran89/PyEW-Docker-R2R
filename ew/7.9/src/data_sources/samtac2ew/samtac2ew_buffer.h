/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: samtac2ew_buffer.h 3540 2009-01-21 16:17:28Z tim $
 * 
 *    Revision history:
 *     $Log$
 *     Revision 1.5  2009/01/21 16:17:28  tim
 *     cleaned up and adjusted data collection for minimal latency
 *
 *     Revision 1.4  2009/01/15 22:09:51  tim
 *     Clean up
 *
 *
 */

typedef struct queueNodeTag {
	char value;
	struct queueNodeTag *next;
	char used;
} queueNodeT;

typedef struct SamtacQueue {
	queueNodeT *start;
	queueNodeT *end;
} queue;


queue* QueueCreate(int max_size); //Returns the queue object
void QueueInsert(queue *my_list, char * buffer, int buffer_length);
int Queue_packetStart(queue *my_list, char * device_id);
void QueueExportBuffer(queue *my_list, char * buffer, int buffer_length);
void QueueFill(queue *my_list, int size);
