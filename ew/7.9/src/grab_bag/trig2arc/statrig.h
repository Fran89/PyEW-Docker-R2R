typedef struct
{
    char    sta[TRACE2_STA_LEN];
    char    chan[TRACE2_CHAN_LEN];
    char    net[TRACE2_NET_LEN];
    char    loc[TRACE2_LOC_LEN];
    double  Sta_time;       /* Station trigger time as sec since 1600 */
    int     duration;
    int     Id;
} STATRIG;
