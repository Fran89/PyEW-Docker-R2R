#ifndef CHAN_NUM_MAP_BY_NET_H
#define CHAN_NUM_MAP_BY_NET_H  1

#define MAX_CHAN_NUMBER 4

typedef struct {
	char net[3];
	char ChannelNumberMap[MAX_CHAN_NUMBER];
} CHAN_NUM_MAP_BY_NET;

#endif
