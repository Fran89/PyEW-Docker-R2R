
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: gdfontg.h 10 2000-02-14 18:56:41Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 18:47:58  lucky
 *     Initial revision
 *
 *
 */

#ifndef GDFONTG_H
#define GDFONTG_H 1

/* gdfontg.h: brings in the largest of the provided fonts.
	Also link with gdfontg.c. */

#include "gd.h"

/* 9x15B font derived from a public domain font in the X
        distribution. Contains the 127 standard ascii characters. */

extern gdFontPtr gdFontGiant;

#endif
