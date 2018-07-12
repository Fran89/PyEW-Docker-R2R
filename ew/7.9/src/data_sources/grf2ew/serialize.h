/* $Id: serialize.h 3145 2007-11-06 17:04:22Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	Serialization routines.

-----------------------------------------------------------------------*/
#if !defined _SERIALIZE_H_INCLUDED_
#define _SERIALIZE_H_INCLUDED_

#include "platform.h"

/* Constants ----------------------------------------------------------*/

/* Types --------------------------------------------------------------*/

/* Serialization template */
typedef struct _TEMPLATE {
	size_t offset;
	size_t length;
	BOOL swap;
} TEMPLATE;

/* Macros -------------------------------------------------------------*/

/* Prototypes ---------------------------------------------------------*/
size_t Serialize(VOID *structure, TEMPLATE * stemplate, UINT8 *out_ptr);
size_t Deserialize(VOID *structure, TEMPLATE * stemplate, UINT8 *in_ptr);

#endif
