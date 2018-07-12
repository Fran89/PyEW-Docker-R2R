#ifndef EW2MOLEDB_QUAKELINK_H
#define EW2MOLEDB_QUAKELINK_H 1

typedef struct {
    long   qkseq;
    char   timestr[20];
    double lat;
    double lon;
    double z;
    float rms;
    float dmin;
    float ravg;
    float gap;
    int nph;
} QUAKE2K;

typedef struct {
    long qkseq;
    int iinstid;
    int isrc;
    int pkseq;
    int iphs;
} LINK;

int read_quake2k(char *msg, int msg_size, QUAKE2K *quake);
int read_link(char *msg, int msg_size, LINK *link);

char *get_sqlstr_from_quake2k_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname);
char *get_sqlstr_from_link_ew_msg(char *msg, int msg_size, char *ewinstancename, char *modname);

#endif

