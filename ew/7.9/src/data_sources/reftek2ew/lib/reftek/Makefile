# @(#)Makefile	1.1 10/28/98
# Makefile for reftek packet library
 
CC      = cc
VPATH   = 
RANLIB  = ranlib
LIBDIR  = $(VPATH)../../../lib/$(PLATFORM)
INCDIR  = $(VPATH)../../include
OPTMIZ  = -g
INCS    = -I$(INCDIR) -I$(VPATH).
DEFS    = -D$(OSNAME) -D$(OSTYPE) -D_REENTRANT
CFLAGS  = $(OPTMIZ) $(INCS) $(DEFS)
ARCHIVE = libreftek.a
OUTPUT  = $(LIBDIR)/$(ARCHIVE)
OBJS   += $(OUTPUT)(ad.o)
OBJS   += $(OUTPUT)(cd.o)
OBJS   += $(OUTPUT)(ds.o)
OBJS   += $(OUTPUT)(dt.o)
OBJS   += $(OUTPUT)(eh.o)
OBJS   += $(OUTPUT)(et.o)
OBJS   += $(OUTPUT)(om.o)
OBJS   += $(OUTPUT)(sh.o)
OBJS   += $(OUTPUT)(sc.o)
OBJS   += $(OUTPUT)(com.o)
OBJS   += $(OUTPUT)(type.o)
OBJS   += $(OUTPUT)(steim.o)
OBJS   += $(OUTPUT)(string.o)
 
.PRECIOUS : $(OUTPUT)
 
all: FORCE
	cd OBJS/$(PLATFORM); \
	make -f ../../Makefile VPATH=../../ makestamp
 
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

testlib: $(OUTPUT) testlib.c
	$(CC) $(CFLAGS) -L$(LIBDIR) -o OBJS/$(PLATFORM)/$@ testlib.c -lreftek -lrtp -lutil
