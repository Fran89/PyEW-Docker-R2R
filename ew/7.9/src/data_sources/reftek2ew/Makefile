# @(#)Makefile	1.4 07/01/98
# ESSW style makefile for import_rtp

CC      = cc
VPATH   = 
BINDIR  = $(VPATH)../../../bin/$(PLATFORM)
LIBDIR  = $(VPATH)../../../lib/$(PLATFORM)
INCDIR  = $(VPATH)../../include
EWBIN   = $(EW_HOME)/$(EW_VERSION)/bin
EWLIB   = $(EW_HOME)/$(EW_VERSION)/lib
EWINC   = $(EW_HOME)/$(EW_VERSION)/include
INCS    = -I$(INCDIR) -I$(EWINC)
OPTMIZ  = -g
DEFS    = -D$(OSNAME) -D$(OSTYPE) -D_REENTRANT
CFLAGS += $(INCS) $(OPTMIZ) $(DEFS)
LIBS    = -L$(LIBDIR) -lrtp -lreftek -lutil -lthread -lposix4 -lm $(SOCKLIBS)
EWOBJS  = $(EWLIB)/transport.o
EWOBJS += $(EWLIB)/getutil.o
EWOBJS += $(EWLIB)/kom.o
EWOBJS += $(EWLIB)/logit.o
EWOBJS += $(EWLIB)/sema_ew.o
EWOBJS += $(EWLIB)/sleep_ew.o
EWOBJS += $(EWLIB)/time_ew.o
EWOBJS += $(EWLIB)/threads_ew.o
OBJS    = main.o
OBJS   += hbeat.o
OBJS   += init.o
OBJS   += notify.o
OBJS   += params.o
OBJS   += scn.o
OBJS   += send.o
OBJS   += samprate.o
OBJS   += terminate.o
OUTPUT = import_rtp

all: FORCE
	cd OBJS/$(PLATFORM); \
	$(MAKE) -f ../../Makefile VPATH=../../ $(OUTPUT)

install:
	cd OBJS/$(PLATFORM); \
	$(MAKE) -f ../../Makefile VPATH=../../ doinstall

clean: FORCE
	rm -f OBJS/$(PLATFORM)/*

remove: FORCE
	cd $(BINDIR); rm -f $(OUTPUT)

doinstall: $(OUTPUT)
	$(INSTALL) -c -m 755 $(OUTPUT) $(BINDIR)

FORCE:

$(OUTPUT): $(OBJS) $(LIBDIR)/librtp.a $(LIBDIR)/libreftek.a \
           $(LIBDIR)/libutil.a
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(EWOBJS) $(LIBS)

$(OBJS): import_rtp.h

watch: watch.c
	$(CC) $(CFLAGS) -DDEBUG_TEST -o OBJS/$(PLATFORM)/$@ watch.c $(EWOBJS) $(LIBS)

partest: params.c scn.c
	$(CC) $(CFLAGS) -DDEBUG_TEST -o OBJS/$(PLATFORM)/$@ params.c scn.c $(LIBS)
