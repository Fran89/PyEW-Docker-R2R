
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: hypAssoc.h,v 1.1 2010/01/01 00:00:00 jmsaurel Exp $
 *
 *    Revision history:
 *     $Log: hypAssoc.h,v $
 *     Revision 1.1 2010/01/01 00:00:00 jmsaurel
 *     Initial revision
 *
 *
 */

#ifndef HYPASSOC_H
#define HYPASSOC_H

/*
 * hypAssoc.h : Include file for hypAssoc.c
 */

#include <chron3.h>
#include <earthworm.h>
#include <errno.h>
#include <kom.h>
#include <math.h>
#include <mem_circ_queue.h>
#include <read_arc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <trace_buf.h>
#include <transport.h>

#define	MAX_EVENT	10
#define	MAXTXT		150
#define	MAX_STR		255

#define ABS(a) ((a) > 0 ? (a) : -(a))           /* Absolute value */

typedef struct
{
    struct Hsum	Origin;
    struct Hpck	*Phases;
    time_t TimeCreated;
    int	   NSta;
} EVENT;

#endif
