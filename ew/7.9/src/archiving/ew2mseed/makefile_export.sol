
#
#   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
#   CHECKED IT OUT USING THE COMMAND CHECKOUT.
#
#    $Id: makefile_export.sol 5281 2013-01-07 21:16:28Z tim $
#
#	Revision history:
#	$Log$
#	Revision 1.1  2010/03/10 17:14:58  paulf
#	first check in of ew2mseed to EW proper
#
#	Revision 1.5  2008/12/30 02:25:27  ilya
#	Modified build process
#	
#	Revision 1.4  2007/09/17 16:29:48  ilya
#	Fixing problems with 78.1 EW integration
#	
#	Revision 1.3  2006/09/20 21:12:13  ilya
#	Apperantly fixed the lepseconds bug
#	
#	Revision 1.2  2003/07/24 17:04:19  ilya
#	Integerated new program wsSniffer
#	
#	Revision 1.1.1.1  2002/01/24 18:32:05  ilya
#	Exporting only ew2mseed!
#	
#	Revision 1.1.1.1  2001/11/20 21:47:00  ilya
#	First CVS commit
#	
# Revision 1.2  2001/02/23  22:02:55  comserv
# a file isti_ws_client.o  is added to a list of makefile tagets
#
# Revision 1.1  2001/02/23  21:35:51  comserv
# Initial revision
#
# Revision 1.1  2000/11/17  06:50:14  comserv
# Initial revision
#
CC = $(CC) $(QINCLUDE)
EW_HOME=..
EW_VERSION = earthworm_lib
CFLAGS = -O2 -D_SPARC -D_SOLARIS -DSUPPORT_SCNL -I $(EW_HOME)/$(EW_VERSION)/include

B = .
L = $(EW_HOME)/$(EW_VERSION)/lib
QLIB = ./qlib2/libqlib2nl.a
QINCLUDE = -I ./qlib2

ALL = earthworm_lib qlib ew2mseed wsSniffer

EW_LIBS = \
        $L/logit.o \
        $L/sema_ew.o \
        $L/threads_ew.o \
        $L/time_ew.o \
        $L/sleep_ew.o \
        $L/ws_clientII.o \
        $L/socket_ew.o \
        $L/socket_ew_common.o

SRCS = wsSniffer.c
OBJS = $(SRCS:%.c=%.o)

OBJ = ew2mseed.o \
      ew2mseed_config.o \
      ew2mseed_log.o \
      ew2mseed_files.o \
      ew2mseed_swap.o \
      ew2mseed_utils.o \
      ws_requests.o \
      isti_ws_client.o \
      $(L)/kom.o \
      $(L)/logit.o \
      $(L)/time_ew.o \
      $(L)/ws_clientII.o \
      $(L)/socket_ew.o \
      $(L)/socket_ew_common.o \
      $(L)/sleep_ew.o \
      $(L)/chron3.o \
      $(L)/swap.o	

all:	$(ALL)

ew2mseed: $(OBJ); \
           $(CC) -o $(B)/ew2mseed $(OBJ) $(I) $(QLIB) -lsocket -lnsl -lposix4  -lm
 
wsSniffer:  $(OBJS); \
           $(CC) -o $(B)/wsSniffer  $(OBJS) $(EW_LIBS) -lc -lsocket -lnsl -lposix4  -lm

qlib: 
	(echo "making qlib2: -NO_LEAPSECONDS!"; \
	cd ./qlib2; \
	make; pwd)

earthworm_lib:
	(echo 'makeing Earthworm lib'; \
	cd ../earthworm_lib; \
	make -f makefile.sol; pwd;) 


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/ew2mseed $B/wsSniffer

clean_qlib2:
	(cd qlib2; make clean)
