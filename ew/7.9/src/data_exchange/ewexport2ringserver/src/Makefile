
LINUX_FLAGS=-lpthread -lm
SOLARIS_FLAGS=-lnsl -lsocket -lrt -lpthread -lc
SPECIFIC_FLAGS=$($(PLATFORM)_FLAGS)

# Build environment can be configured the following
# environment variables:
#   CC : Specify the C compiler to use
#   GLOBALFLAGS : Specify compiler options to use

# Required compiler parameters
CFLAGS = $(GLOBALFLAGS) -I../../../libsrc/libdali -I$(EW_HOME)/$(EW_VERSION)/include

LDFLAGS = -L$(EW_HOME)/$(EW_VERSION)/lib 
LDLIBS  = $(SPECIFIC_FLAGS) -ldali -lmseed

# For SunOS/Solaris uncomment the following line
#LDLIBS = -lpthread -ldali -lmseed -lsocket -lnsl -lrt

BIN = $(EW_HOME)/$(EW_VERSION)/bin/ewexport2ringserver

OBJS = ewexport2ringserver.o network.o util.o

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

static: $(OBJS)
	$(CC) $(CFLAGS) -static -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

debug:
	$(MAKE) "CC=$(CC)" "CFLAGS=-g $(CFLAGS)"

clean:
	rm -f $(OBJS) $(BIN)
