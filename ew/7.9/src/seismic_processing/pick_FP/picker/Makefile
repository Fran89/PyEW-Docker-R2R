# Makefile for picker test
#
# Invocation:
#     Solaris: make -f Make_picker_func_test
#     Linux:   gmake -Rf Make_picker_func_test

BINDIR=./
INCLUDE_DIR=./


# Options specific for GCC
# this is now set in the env var
#ifndef CC
#CC = gcc
#endif
#
#ifndef CCFLAGS
#
CCFLAGS_BASIC =  -Wall -fPIC -I.. $(GLOBALFLAGS)
#
# optimized
#CCFLAGS = -O3 $(CCFLAGS_BASIC)
#
# profile
#CCFLAGS=-O3 -pg $(CCFLAGS_BASIC)
#
# debug - gdb, valgrind, ...
CCFLAGS = $(CCFLAGS_BASIC) -g
# valgrind --leak-check=yes  exe_name <args>
# valgrind --leak-check=full --show-reachable=yes exe_name exe_name <args>
#endif


all : ${BINDIR}/picker_func_test ${BINDIR}/picker_func_test_memory


OBJS_PICK=FilterPicker5.o FilterPicker5_Memory.o PickData.o

FilterPicker5.o : FilterPicker5.h FilterPicker5.c FilterPicker5_Memory.o PickData.o
FilterPicker5_Memory.o : FilterPicker5_Memory.h FilterPicker5_Memory.c
PickData.o : PickData.h PickData.c


%.o : %.c
	$(CC) -c $(CCFLAGS) $< -o $@


clean :
	rm -f *.o
	rm -f *.a
	rm -f ${BINDIR}/picker_func_test_memory ${BINDIR}/picker_func_test


# --------------------------------------------------------------------------
# picker_func_test_memory
#
OBJ_TEST1=picker_func_test_memory.o $(OBJS_PICK)
picker_func_test_memory : ${BINDIR}/picker_func_test_memory
picker_func_test_memory.o : picker_func_test_memory.c FilterPicker5.o
${BINDIR}/picker_func_test_memory : ${OBJ_TEST1}
	$(CC) ${OBJ_TEST1} ${CCFLAGS} -o ${BINDIR}/picker_func_test_memory -lm
# --------------------------------------------------------------------------
# --------------------------------------------------------------------------
# picker_func_test
#
OBJ_TEST2=picker_func_test.o $(OBJS_PICK)
picker_func_test : ${BINDIR}/picker_func_test
picker_func_test.o : picker_func_test.c FilterPicker5.o
${BINDIR}/picker_func_test : ${OBJ_TEST2}
	$(CC) ${OBJ_TEST2} ${CCFLAGS} -o ${BINDIR}/picker_func_test -lm
# --------------------------------------------------------------------------



