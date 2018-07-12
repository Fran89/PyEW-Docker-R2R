/* $Id: mem.c 5714 2013-08-05 19:18:08Z paulf $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	Generic dynamic memory handling module with thread-safe logging.

	This module is dependent upon the generic logging functions in log.c

-----------------------------------------------------------------------*/

#include "mem.h"

/* Uncomment and build to perform unit testing */
//#define MEM_UNIT_TEST

#if defined MEM_LOGGING

/* Module constants ---------------------------------------------------*/

#define GARBAGE_BYTE			0xCC		/* Appropriate for x86 hardware */

/* Module types -------------------------------------------------------*/
typedef struct TAG_MEMORY_INFO {
	struct TAG_MEMORY_INFO *next;
	UINT8 *ptr;								/* Pointer to start of block */
	size_t size;							/* Size of block */
	UINT32 count;							/* Sequence number */
	BOOL referenced;						/* Referenced flag */
} MEMORY_INFO;

/* Module globals -----------------------------------------------------*/
static MUTEX mutex;							/* Mutual exclusion */
static MEMORY_INFO *head = NULL;			/* Memory log */
static UINT32 count = 0;					/* Allocation sequence number */
static UINT8 garbage_byte = (UINT8)GARBAGE_BYTE;

/* Module prototypes --------------------------------------------------*/
static BOOL CreateMemoryInfo(UINT8 *ptr, size_t size);
static MEMORY_INFO *LookupMemoryInfo(UINT8 *ptr);
static VOID UpdateMemoryInfo(UINT8 *old_ptr, UINT8 *new_ptr, size_t size);
static VOID ReleaseMemoryInfo(UINT8 *ptr);
static size_t SizeOfMemory(VOID *ptr);

/* Macros -------------------------------------------------------------*/

/* These macros compare arbitrary pointers.  The ANSI standard doesn't
   guarantee that this is a portable operation.  The following macros
   should work on any platform with "flat" pointers.
   They will *NOT* work for some real mode x86 memory models! */
#define PtrLess(left, right)			((left) < (right))
#define PtrGreater(left, right)			((left) > (right))
#define PtrEqual(left, right)			((left) == (right))
#define PtrLessOrEqual(left, right)		((left) <= (right))
#define PtrGreaterOrEqual(left, right)	((left) >= (right))

#endif										/* defined MEM_LOGGING */

/*---------------------------------------------------------------------*/
BOOL AllocateMemory(void **pptr, size_t length)
{

	ASSERT(pptr != NULL);
	ASSERT(length > 0);

	*pptr = malloc(length);

#if defined MEM_LOGGING
	if (*pptr != NULL) {
		/* Fill will garbage */
		memset(*pptr, garbage_byte, length);
		if (!CreateMemoryInfo(*pptr, length)) {
			/* Fail the whole affair... */
			free(*pptr);
			*pptr = NULL;
		}
	}
#endif

	return *pptr != NULL;
}

/*---------------------------------------------------------------------*/
BOOL ResizeMemory(void **old_pptr, size_t new_length)
{
	VOID *new_ptr;

#if defined MEM_LOGGING
	VOID *moved_ptr;
	size_t old_length;
#endif

	ASSERT(IsValidBasePointer(*old_pptr));
	ASSERT(new_length > 0);

#if defined MEM_LOGGING
	old_length = SizeOfMemory(*old_pptr);
	/* If we're getting smaller, fill the soon-to-be released memory at end of
	   block */
	if (new_length < old_length)
		memset(((UINT8 *)*old_pptr) + new_length, garbage_byte, old_length - new_length);
	/* If we're growing, force memory to move */
	else if (new_length > old_length) {
		if (AllocateMemory(&moved_ptr, old_length)) {
			memcpy(moved_ptr, *old_pptr, old_length);
			ReleaseMemory(*old_pptr);
			*old_pptr = moved_ptr;
		}
	}
#endif

	new_ptr = realloc(*old_pptr, new_length);

#if defined MEM_LOGGING
	if (new_ptr != NULL) {
		UpdateMemoryInfo(*old_pptr, new_ptr, new_length);
		/* If growing, fill new tail... */
		if (new_length > old_length)
			memset(((UINT8 *)new_ptr) + old_length, garbage_byte, new_length - old_length);
	}
#endif

	*old_pptr = new_ptr;

	return new_ptr != NULL;
}

/*---------------------------------------------------------------------*/
VOID ReleaseMemory(void *ptr)
{

	ASSERT(IsValidBasePointer(ptr));

#if defined MEM_LOGGING
	memset(ptr, garbage_byte, SizeOfMemory(ptr));
	ReleaseMemoryInfo(ptr);
#endif

	free(ptr);

	return;
}

/*---------------------------------------------------------------------*/
/* This function checks that ptr points anywhere within a valid block of memory */
BOOL IsValidPointer(void *ptr)
{
#if defined MEM_LOGGING
	BOOL result;

	ASSERT(ptr != NULL);

	result = FALSE;

	MUTEX_LOCK(&mutex);
	/* Returns NULL if ptr is invalid */
	if (LookupMemoryInfo(ptr) != NULL)
		result = TRUE;
	MUTEX_UNLOCK(&mutex);

	return result;
#else
	return ptr != NULL;
#endif
}

/*---------------------------------------------------------------------*/
/* This function checks that ptr points to the base of a valid block of memory */
BOOL IsValidBasePointer(void *ptr)
{
#if defined MEM_LOGGING
	BOOL result;
	MEMORY_INFO *info;

	ASSERT(ptr != NULL);

	result = TRUE;

	MUTEX_LOCK(&mutex);
	if ((info = LookupMemoryInfo(ptr)) == NULL)
		result = FALSE;
	if (result && !PtrEqual(ptr, info->ptr))
		result = FALSE;
	MUTEX_UNLOCK(&mutex);

	return result;
#else
	return ptr != NULL;
#endif
}

/*---------------------------------------------------------------------*/
/* This function checks that ptr and length point to a valid range
   within a valid block of memory */
BOOL IsValidMemoryRange(void *ptr, size_t length)
{
#if defined MEM_LOGGING
	BOOL result;
	MEMORY_INFO *start, *end;

	ASSERT(ptr != NULL);

	result = FALSE;

	MUTEX_LOCK(&mutex);
	/* Either will be NULL if invalid */
	start = LookupMemoryInfo(ptr);
	end = LookupMemoryInfo(((UINT8 *)ptr) + length - 1);

	/* Check that start and end are not NULL and are within the same block... */
	if (start != NULL && end != NULL && PtrEqual(start->ptr, end->ptr))
		result = TRUE;
	MUTEX_UNLOCK(&mutex);

	return result;
#else
	return ptr != NULL;
#endif
}

/* The following three functions are used to find dangling pointers and
   lost memory... */
/*---------------------------------------------------------------------*/
VOID ClearMemoryRefs(void)
{
#if defined MEM_LOGGING
	MEMORY_INFO *info;

	MUTEX_LOCK(&mutex);
	for (info = head; info != NULL; info = info->next)
		info->referenced = FALSE;
	MUTEX_UNLOCK(&mutex);
#endif
	return;
}

/*---------------------------------------------------------------------*/
VOID NoteMemoryRef(void *ptr)
{
#if defined MEM_LOGGING
	MEMORY_INFO *info;

	ASSERT(ptr != NULL);

	MUTEX_LOCK(&mutex);
	info = LookupMemoryInfo(ptr);
	ASSERT(info != NULL);

	info->referenced = TRUE;
	MUTEX_UNLOCK(&mutex);
#endif
	return;
}

/*---------------------------------------------------------------------*/
VOID CheckMemoryRefs(void)
{
#if defined MEM_LOGGING
	MEMORY_INFO *info;

	MUTEX_LOCK(&mutex);
	for (info = head; info != NULL; info = info->next) {
		/* If either of these fire, there's a bug in this module somewhere */
		ASSERT(info->ptr != NULL);
		ASSERT(info->size > 0);
		/* This is a check for lost or leaking memory.  If this fires, the app
		   has either lost track of this block or not all pointers have been
		   accounted for with NoteMemoryRef() */
		ASSERT(info->referenced);
	}
	MUTEX_UNLOCK(&mutex);
#endif
	return;
}

/*---------------------------------------------------------------------*/
VOID DumpMemoryInfo(UINT32 log_level)
{
#if defined MEM_LOGGING
	MEMORY_INFO *info;
	UINT32 bytes, items;

	printf("Dynamic memory information:\n");

	MUTEX_LOCK(&mutex);
	bytes = 0;
	items = 0;
	for (info = head; info != NULL; info = info->next) {
		printf(" Addr: %p, bytes: %u, seq: %u, ref: %s\n",
			info->ptr, info->size, info->count, (info->referenced ? "yes" : "no"));
		bytes += info->size;
		items++;
	}
	MUTEX_UNLOCK(&mutex);

	printf(" %u %s (%u bytes) in dynamic memory\n", items, (items == 1 ? "item" : "items"), bytes);
#endif
	return;
}

#if defined MEM_LOGGING
/* The rest of these functions are not externally visible... */
/*---------------------------------------------------------------------*/
static MEMORY_INFO *LookupMemoryInfo(UINT8 *ptr)
{
	MEMORY_INFO *info;

	for (info = head; info != NULL; info = info->next) {
		if (PtrGreaterOrEqual(ptr, info->ptr) && PtrLessOrEqual(ptr, ((UINT8 *)info->ptr) + info->size - 1))
			break;
	}

	/* info is NULL if not found */
	return info;
}

/*---------------------------------------------------------------------*/
static BOOL CreateMemoryInfo(UINT8 *ptr, size_t size)
{
	MEMORY_INFO *info;

	if ((info = (MEMORY_INFO *) malloc(sizeof(MEMORY_INFO))) == NULL)
		return FALSE;

	info->ptr = ptr;
	info->size = size;
	info->count = count++;

	MUTEX_LOCK(&mutex);
	info->next = head;
	head = info;
	MUTEX_UNLOCK(&mutex);

	return TRUE;
}

/*---------------------------------------------------------------------*/
static VOID ReleaseMemoryInfo(UINT8 *ptr)
{
	MEMORY_INFO *info, *prev;

	MUTEX_LOCK(&mutex);
	prev = NULL;
	for (info = head; info != NULL; info = info->next) {
		if (PtrEqual(ptr, info->ptr)) {
			if (prev == NULL)
				head = info->next;
			else
				prev->next = info->next;
			break;
		}
		prev = info;
	}
	MUTEX_UNLOCK(&mutex);

	ASSERT(info != NULL);

	memset(info, garbage_byte, sizeof(MEMORY_INFO));

	free(info);

	return;
}

/*---------------------------------------------------------------------*/
static VOID UpdateMemoryInfo(UINT8 *old_ptr, UINT8 *new_ptr, size_t size)
{
	MEMORY_INFO *info;

	MUTEX_LOCK(&mutex);
	info = LookupMemoryInfo(old_ptr);
	ASSERT(info != NULL);
	ASSERT(PtrEqual(info->ptr, old_ptr));

	info->ptr = new_ptr;
	info->size = size;
	MUTEX_UNLOCK(&mutex);

	return;
}

/*---------------------------------------------------------------------*/
static size_t SizeOfMemory(VOID *ptr)
{
	size_t size;
	MEMORY_INFO *info;

	MUTEX_LOCK(&mutex);
	info = LookupMemoryInfo(ptr);
	ASSERT(info != NULL);
	ASSERT(PtrEqual(info->ptr, ptr));

	size = info->size;
	MUTEX_UNLOCK(&mutex);

	return size;
}

#endif										/* defined MEM_LOGGING */

/*---------------------------------------------------------------------*/
#if defined MEM_UNIT_TEST

int main(int argc, char *argv[])
{
	UINT8 *base, *ptr;
	size_t i, n0, n1, n2;

	LogOpen(argv[0], "local3", "mem.log");

#if defined _DEBUG
	LogMessage(LOG_INFO, "Debug build\n");
#else
	LogMessage(LOG_INFO, "Release build\n");
#endif

#if defined MEM_LOGGING
	LogMessage(LOG_INFO, "Memory logging features are enabled\n");
#else
	LogMessage(LOG_INFO, "Memory logging features are disabled\n");
#endif

	/* Allocate some memory... */
	n0 = ((size_t)1 << 18);
	LogMessage(LOG_INFO, "Allocating %u bytes of memory\n", n0);
	if (!AllocateMemory((VOID **)&base, n0)) {
		LogMessage(LOG_INFO, "ERROR: AllocateMemory(%u) failed!\n", n0);
		exit(1);
	}
	//LogMessage(LOG_INFO, "Base address: %p\n", base);
	DumpMemoryInfo();

	LogMessage(LOG_INFO, "Validating pointers\n");
	/* All of these should return TRUE... */
	ASSERT(IsValidBasePointer(base));
	ASSERT(IsValidPointer(base));
	ASSERT(IsValidPointer(base + n0 - 1));
	ASSERT(IsValidMemoryRange(base, n0));

	/* All of these should return FALSE... */
#if defined MEM_LOGGING
	ASSERT(!IsValidBasePointer(base + 1));
	ASSERT(!IsValidPointer(base - 1));
	ASSERT(!IsValidPointer(base + n0));
	ASSERT(!IsValidMemoryRange(base, n0 + 1));
	ASSERT(!IsValidMemoryRange(base - 1, n0));
#endif

	/* Fill with acsending values... */
	LogMessage(LOG_INFO, "Filling memory with acsending byte values\n");
	ptr = base;
	NoteMemoryRef(base);
	CheckMemoryRefs();
	for (i = 0; i < n0; i++) {
		ASSERT(IsValidPointer(ptr));
		*ptr++ = (UINT8)(i % 255);
	}

	/* Make it bigger... */
	n1 = n0 + (n0 / 2);
	LogMessage(LOG_INFO, "Resizing memory to %u bytes\n", n1);
	if (!ResizeMemory((VOID **)&base, n1)) {
		LogMessage(LOG_INFO, "ERROR: ResizeMemory(%u) failed!\n", n1);
		exit(1);
	}
	//LogMessage(LOG_INFO, "Base address: %p\n", base);
	DumpMemoryInfo();

	LogMessage(LOG_INFO, "Validating pointers\n");
	ASSERT(IsValidBasePointer(base));
	ASSERT(IsValidPointer(base));
	ASSERT(IsValidPointer(base + n1 - 1));
	ASSERT(IsValidMemoryRange(base, n1));

#if defined MEM_LOGGING
	/* All of these should return FALSE... */
	ASSERT(!IsValidBasePointer(base + 1));
	ASSERT(!IsValidPointer(base - 1));
	ASSERT(!IsValidPointer(base + n1));
	ASSERT(!IsValidMemoryRange(base, n1 + 1));
	ASSERT(!IsValidMemoryRange(base - 1, n1));
#endif

	/* Check that everything is still there and add to the end... */
	ptr = base;
	NoteMemoryRef(base);
	CheckMemoryRefs();
	LogMessage(LOG_INFO, "Checking data integrity\n");
	for (i = 0; i < n0; i++) {
		ASSERT(IsValidPointer(ptr));
		ASSERT(*ptr++ == (UINT8)(i % 255));
	}
	LogMessage(LOG_INFO, "Extending fill into new memory\n");
	for (i = n0; i < n1; i++) {
		ASSERT(IsValidPointer(ptr));
		*ptr++ = (UINT8)(i % 255);
	}

	/* Make it smaller */
	n2 = n0 / 2;
	LogMessage(LOG_INFO, "Resizing memory to %u bytes\n", n2);
	if (!ResizeMemory((VOID **)&base, n2)) {
		LogMessage(LOG_INFO, "ERROR: ResizeMemory(%u) failed!\n", n2);
		exit(1);
	}
	//LogMessage(LOG_INFO, "Base address: %p\n", base);
	DumpMemoryInfo();

	LogMessage(LOG_INFO, "Validating pointers\n");
	ASSERT(IsValidBasePointer(base));
	ASSERT(IsValidPointer(base));
	ASSERT(IsValidPointer(base + n2 - 1));
	ASSERT(IsValidMemoryRange(base, n2));

#if defined MEM_LOGGING
	/* All of these should return FALSE... */
	ASSERT(!IsValidBasePointer(base + 1));
	ASSERT(!IsValidPointer(base - 1));
	ASSERT(!IsValidPointer(base + n2));
	ASSERT(!IsValidMemoryRange(base, n2 + 1));
	ASSERT(!IsValidMemoryRange(base - 1, n2));
#endif

	/* Check that the values are still there... */
	ptr = base;
	NoteMemoryRef(base);
	CheckMemoryRefs();
	LogMessage(LOG_INFO, "Checking data integrity\n");
	for (i = 0; i < n2; i++) {
		ASSERT(IsValidPointer(ptr));
		ASSERT(*ptr++ == (UINT8)(i % 255));
	}

	CheckMemoryRefs();
	ClearMemoryRefs();

	LogMessage(LOG_INFO, "Releasing memory\n");
	ReleaseMemory(base);
	DumpMemoryInfo();

#if defined MEM_LOGGING
	/* All of these should return FALSE... */
	ASSERT(!IsValidBasePointer(base));
	ASSERT(!IsValidPointer(base));
	ASSERT(!IsValidPointer(base + n2 - 1));
	ASSERT(!IsValidMemoryRange(base, n2));
#endif

	LogMessage(LOG_INFO, "No assertions fired, everything's working!\n");

	exit(0);
}

#endif										/* defined MEM_UNIT_TEST */
