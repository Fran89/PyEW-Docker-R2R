#ifndef _MISC_H
#define _MISC_H

#define VER_NO "1.0.4 -  2016.139"
/* keep track of version notes here */
/*
Mario Aranha, Berkley Seismological Laboratory, University of California, Berkeley.
mario@seismo.berkeley.edu

2016/05/13   1.0.3 - 2016.133
Added support for ALL RabbitMQ exchange type connections: fanout, direct, topic, headers.
Upward compatible with previous version that supported named queues only.
Updated sample 'geojson2ew.d' file.  Existing *.d file(s) should work.
Changed File(s): misc.h json_conn.h json_conn.c getconfig.c geojson2ew.d


2016/05/24   1.0.4 - 2016.145
- Merged in 'geojson_socket'. Now also extensible to other message server types, e.g., ActiveMQ.
- Added optional SERVERTYPE keyword to config file; possible values are: socket, rabbitMQ.
- Code is now extensible to other message servers, e.g., activeMQ
- Existing *.d file(s) for 'geojson_socket' should work.
- Fixed bugs in message framing for socket server mode.
- Fixed bug in options parsing when -v or -h is used without a config file name.
- Added conditional compilation flag SOCKET_ONLY.
Changed File(s): README misc.h json_conn.h json_conn.c getconfig.c geojson2ew.d options.c makefile.unix
*/

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) x
#endif

#endif 
