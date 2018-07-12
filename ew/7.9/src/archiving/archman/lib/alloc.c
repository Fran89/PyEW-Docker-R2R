
#include <stdlib.h>
#ifdef _WINNT
#include <malloc.h>
#endif
#include <time.h>

#include "geolib.h"

/**************************************************************************
 * alloc_more_mem
 *
 * Description: reallocate a block of memory if the new size is larger
 *              than that currently allocted
 *
 * Input paramters: block - the clock to allocate
 *                  new_size - the requested size
 *                  current_size - the current size
 * Output parameters: block and size may be modified
 * Returns: TRUE if the allocation was successfull, FALSE otherwise
 *
 * Comments:
 *
 **************************************************************************/
int alloc_more_mem (void **block, int new_size, int *current_size)

{
  void *ptr;


  /* check if more memory is needed */
  if (new_size <= *current_size) return GEO_TRUE;

  /* attempt the allocation */
  ptr = realloc (*block, new_size);
  if (! ptr) return GEO_FALSE;

  /* sort out callers variables and return */
  *block = ptr;
  *current_size = new_size;
  return GEO_TRUE;

}

/**************************************************************************
 * heap_dump
 *
 * Description: debug memory allocation
 *
 * Input parameters: fp - the stream to send information to
 *                   short_form - TRUE for summary data, FALSE for detail data
 * Output parameters: none
 * Returns: none
 *
 * Comments:
 *
 **************************************************************************/
#ifdef WIN32
void heap_dump (FILE *fp, int short_form)

{

  struct _heapinfo h_info;
  int heap_status, entry_count, total_size;
  time_t bintime;
  struct tm *unix_time;


  time (&bintime);
  unix_time = gmtime (&bintime);
  if (! short_form)
  {
    fprintf (fp, "Heap dump at %02d-%02d-%04d %02d:%02d:%02d\n\n",
             unix_time->tm_mday, unix_time->tm_mon +1,
	         unix_time->tm_year + 1900, unix_time->tm_hour,
	  	     unix_time->tm_min, unix_time->tm_sec);
    fprintf (fp, "Address      Size        Status\n");
  }

  h_info._pentry = NULL;
  total_size = 0;
  entry_count = 0;
  for (;;)
  {
    heap_status = _heapwalk( &h_info );
    if (heap_status != _HEAPOK) break;
    if (! short_form)
      fprintf (fp, "%12Fp %12d %s\n", h_info._pentry, h_info._size,
               (h_info._useflag == _USEDENTRY ? "used" : "free"));
    entry_count ++;
    total_size += h_info._size;
  }

  switch (heap_status)
  {
  case _HEAPEND:
  case _HEAPEMPTY:
    if (short_form)
      fprintf (fp, "Heap dump %02d-%02d-%04d %02d:%02d:%02d, Entries %d, Size %d\n",
               unix_time->tm_mday, unix_time->tm_mon +1,
	           unix_time->tm_year + 1900, unix_time->tm_hour,
	    	   unix_time->tm_min, unix_time->tm_sec,
               entry_count, total_size);
    else
      fprintf (fp, "\nTotal of %d entries, %d bytes memory used\n", entry_count, total_size);
    break;
  case _HEAPBADBEGIN:
    fprintf (fp, "ERROR - heap is damaged\n");
    break;
  case _HEAPBADPTR:
    fprintf (fp, "ERROR - bad pointer to heap\n");
    break;
  case _HEAPBADNODE:
    fprintf (fp, "ERROR - bad node in heap\n");
    break;
  }

  fflush (fp);

}
#endif 

