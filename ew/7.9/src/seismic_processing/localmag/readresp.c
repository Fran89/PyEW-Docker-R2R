#include <stdio.h>
#include "transfer.h"

main (int argc, char ** argv)  {

	ResponseStruct rs;
	int ret;

	ret = readPZ(argv[1], &rs);

	fprintf(stdout, "readresp: read in file %s and return value from readPZ was %d\n", argv[1], ret);
	exit(0);
}
