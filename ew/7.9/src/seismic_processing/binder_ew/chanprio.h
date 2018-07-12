#ifndef CHAN_PRIO_H
#define CHAN_PRIO_H  1

#define MAX_ENTRIES 400

typedef struct {
	char Channel12[3]; /* first 2 channel identifiers (band and sensor type), orientation is unimportant for this test */
	int prio;	/* priority level, higher value, higher priority for use  if SNL equivalent */
} CHAN_PRIO_MAP;

#endif
