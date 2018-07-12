#
# q2ew - quanterra to earthworm interface
#

CFLAGS = -D_REENTRANT ${GLOBALFLAGS} -D_SPARC -D_SOLARIS

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

CFLAGS += -xCC

SRCS = convert.c getconfig.c main.c scn_map.c cs_status.c \
	heart.c misc_seed_utils.c die.c logo.c options.c

OBJS = convert.o getconfig.o main.o scn_map.o cs_status.o \
	heart.o misc_seed_utils.o die.o logo.o options.o
 
EW_LIBS = $L/logit_mt.o 
	$L/chron3.o \
	$L/getutil.o \
	$L/kom.o \
	$L/sleep_ew.o \
	$L/threads_ew.o \
	$L/time_ew.o \
	$L/transport.o \
	$L/socket_ew.o \
	$L/socket_ew_common.o \
	$L/mem_circ_queue.o \
	$L/sema_ew.o \
	$L/parse_trig.o \
	$L/ws_clientII.o \
	$L/swap.o 

q2ew: $(OBJS); \
        cc -o $(B)/q2ew $(OBJS) $(EW_LIBS) -lsocket -lnsl -lposix4 -lthread -lm


# Clean-up rules
clean:
	rm -f a.out core *.o *.obj *% *~

clean_bin:
	rm -f $B/q2ew*
