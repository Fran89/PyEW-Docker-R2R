# @(#)Makefile	1.1 10/28/98
# Makefile for util library
 
CC      = cc
VPATH   = 
RANLIB  = ranlib
LIBDIR  = $(VPATH)../../../lib/$(PLATFORM)
INCDIR  = $(VPATH)../../include
OPTMIZ  = -g
INCS    = -I$(INCDIR) -I$(VPATH).
DEFS    = -D$(OSNAME) -D$(OSTYPE) -D_REENTRANT
CFLAGS  = $(OPTMIZ) $(INCS) $(DEFS)
ARCHIVE = libutil.a
OUTPUT  = $(LIBDIR)/$(ARCHIVE)
OBJS    = $(OUTPUT)(bcd2long.o)
OBJS   += $(OUTPUT)(dump.o)
OBJS   += $(OUTPUT)(getline.o)
OBJS   += $(OUTPUT)(find.o)
OBJS   += $(OUTPUT)(mstime.o)
OBJS   += $(OUTPUT)(parse.o)
OBJS   += $(OUTPUT)(string.o)
OBJS   += $(OUTPUT)(swap.o)
OBJS   += $(OUTPUT)(timer.o)
OBJS   += $(OUTPUT)(timefunc.o)
 
.PRECIOUS : $(OUTPUT)
 
all: FORCE
	cd OBJS/$(PLATFORM); \
	gmake -f ../../Makefile VPATH=../../ makestamp
 
$(OUTPUT): makestamp
 
makestamp: $(OBJS)
	$(RANLIB) $(OUTPUT)
	touch makestamp
 
clean: FORCE
	rm -f OBJS/$(PLATFORM)/*.o
 
remove: FORCE
	rm -f $(OUTPUT) OBJS/$(PLATFORM)/makestamp
 
sccsinfo: FORCE
	sccs info

install: all
 
FORCE:

testfind: find.c
	$(CC) $(CFLAGS) -DDEBUG_TEST -o OBJS/$(PLATFORM)/$@ find.c -L$(LIBDIR) -lutil
