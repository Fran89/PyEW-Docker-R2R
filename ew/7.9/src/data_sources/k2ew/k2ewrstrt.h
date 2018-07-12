/*  k2ewrstrt.h:  Header file for 'k2ewrstrt.c'              */

#ifndef K2EWRTSTRT_H             /* process file only once */
#define K2EWRTSTRT_H 1

void k2ew_write_rsfile( uint32_t dataseq, unsigned char stmnum);
void k2ew_read_rsfile(int *valid_restart, uint32_t *last_dataseq,
                      unsigned char *last_stmnum);


#endif

