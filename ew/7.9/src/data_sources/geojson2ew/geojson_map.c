/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "earthworm.h"	/* need this for the logit() call */
#include "externs.h"
#include "geojson_map.h"

#define CLEAR_ENTRY(E)	memset(E, 0, sizeof(GEOJSON_MAP_ENTRY))
#define COPY_ENTRY(D,S)	memcpy(D, S, sizeof(GEOJSON_MAP_ENTRY))
#define CREATE_ENTRY()	malloc(sizeof(GEOJSON_MAP_ENTRY))

static GEOJSON_MAP_ENTRY *first_entry = NULL;
static GEOJSON_MAP_ENTRY *last_entry = NULL;

int add_channel(GEOJSON_MAP_ENTRY *eptr)
{
   // if invalid entry
   if (eptr == NULL || eptr->path == NULL || *eptr->path == 0)
   {
      logit("e", "invalid channel entry\n");
      return 1;
   }

   GEOJSON_MAP_ENTRY *new_eptr  = CREATE_ENTRY();
   if ( new_eptr == NULL )
   {
      logit("e", "could not allocate new channel entry\n");
      return -1;
   }
   // if no first entry
   if ( first_entry == NULL )
      first_entry = new_eptr; // this is first entry
   // if last entry
   if ( last_entry != NULL )
      last_entry->next = new_eptr; // this is next entry
   COPY_ENTRY(new_eptr, eptr);
   last_entry = new_eptr; // this is the last entry
   return 0;
}

GEOJSON_MAP_ENTRY *get_channel()
{
   return first_entry;
}
