
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: giftogd.c 3363 2008-09-26 22:25:00Z kress $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2008/09/26 22:25:00  kress
 *     Fix numerous compile warnings and some tab-related fortran errors for linux compile
 *
 *     Revision 1.1  2000/02/14 18:47:58  lucky
 *     Initial revision
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "gd.h"

/* A short program which converts a .gif file into a .gd file, for
	your convenience in creating images on the fly from a
	basis image that must be loaded quickly. The .gd format
	is not intended to be a general-purpose format. */

int main(int argc, char **argv)
{
	gdImagePtr im;
	FILE *in, *out;
	if (argc != 3) {
		fprintf(stderr, "Usage: giftogd filename.gif filename.gd\n");
		exit(1);
	}
	in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "Input file does not exist!\n");
		exit(1);
	}
	im = gdImageCreateFromGif(in);
	fclose(in);
	if (!im) {
		fprintf(stderr, "Input is not in GIF format!\n");
		exit(1);
	}
	out = fopen(argv[2], "wb");
	if (!out) {
		fprintf(stderr, "Output file cannot be written to!\n");
		gdImageDestroy(im);
		exit(1);	
	}
	gdImageGd(im, out);
	fclose(out);
	gdImageDestroy(im);
}

