/* $Id: mem.h 5714 2013-08-05 19:18:08Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	Generic memory handling module with logging.

-----------------------------------------------------------------------*/

#if !defined _MEM_H_INCLUDED_
#define _MEM_H_INCLUDED_

#include "platform.h"

/* Constants ----------------------------------------------------------*/

/* Define MEM_LOGGING to enable logging features */
//#define MEM_LOGGING

/* Types --------------------------------------------------------------*/

/* Prototypes ---------------------------------------------------------*/
BOOL AllocateMemory(void **ptr, size_t length);
BOOL ResizeMemory(void **ptr, size_t length);
VOID ReleaseMemory(void *ptr);

/* The following functions do nothing unless MEM_LOGGING is defined... */

/* The IsValid??? functions check if a pointer is within, the base of, or a
   range is within, a valid memory block */
BOOL IsValidPointer(void *ptr);
BOOL IsValidBasePointer(void *ptr);
BOOL IsValidMemoryRange(void *ptr, size_t length);

/* These functions are used to detect dangling pointers and lost memory. Each
   of these assert on errors. */
VOID NoteMemoryRef(void *ptr);
VOID CheckMemoryRefs(void);
VOID ClearMemoryRefs(void);

/* This function logs status of dynamic memory */
VOID DumpMemoryInfo(UINT32 log_level);

#endif
