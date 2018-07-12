all: 
	make testcrc
	make crctable
	make testcrc32

testcrc: crcmodel.o testcrc.o
	cc -o testcrc testcrc.o crcmodel.o

crctable: crcmodel.o crctable.o
	cc -o crctable crctable.o crcmodel.o

testcrc32: testcrc32.o crc32.o
	cc -o testcrc32 testcrc32.o crc32.o
