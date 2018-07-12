int     read_statrig_line( char *, STATRIG*);   /* reads a station line from triglist message and fills STATRIG structure */
void    write_hyp_staline( STATRIG *, char **); /* writes a hyp2000 phase line and its shadow with infos from STATRIG structure */
void    write_hyp_header( STATRIG *, char **);  /* writes a hyp2000 header line and its shadow with infos from STATRIG structure */
void    write_hyp_term( STATRIG *, char **);            /* writes a hyp2000 terminator line and its shadow */
