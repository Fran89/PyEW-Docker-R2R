
#ifndef NETWORK_H
#define NETWORK_H 1

extern int my_connect (char *host, char *port, int *socktimeout, int verbose);
extern int my_recv (int sock, char *buffer, int readlen, int socktimeout);
extern int my_send (int sock, char *buffer, int sendlen, int socktimeout);

#endif 
