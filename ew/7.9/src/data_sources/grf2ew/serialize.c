/* $Id: serialize.c 3486 2008-12-04 17:21:08Z stefan $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

	Serialization routines.

-----------------------------------------------------------------------*/

#include "serialize.h"

/* Module constants ---------------------------------------------------*/

/* Module globals -----------------------------------------------------*/

/* Local prototypes ---------------------------------------------------*/

/*---------------------------------------------------------------------*/
size_t Serialize(VOID *structure, TEMPLATE * stemplate, UINT8 *out_ptr)
{
	UINT8 *ptr;
	size_t i, length, actual;

#ifdef LITTLE_ENDIAN_HOST
	UINT8 swap_buffer[8];
#endif

	ASSERT(structure != NULL);
	ASSERT(stemplate != NULL);
	ASSERT(out_ptr != NULL);

	actual = 0;

	i = 0;
	while (stemplate[i].offset != VOID_SIZE) {

		ptr = (UINT8 *)structure + stemplate[i].offset;

#ifdef LITTLE_ENDIAN_HOST
		if (stemplate[i].swap) {
			switch (stemplate[i].length) {
			case 2:
				swap_buffer[0] = ptr[1];
				swap_buffer[1] = ptr[0];
				ptr = swap_buffer;
				break;
			case 4:
				swap_buffer[0] = ptr[3];
				swap_buffer[1] = ptr[2];
				swap_buffer[2] = ptr[1];
				swap_buffer[3] = ptr[0];
				ptr = swap_buffer;
				break;
			case 8:
				swap_buffer[0] = ptr[7];
				swap_buffer[1] = ptr[6];
				swap_buffer[2] = ptr[5];
				swap_buffer[3] = ptr[4];
				swap_buffer[4] = ptr[3];
				swap_buffer[5] = ptr[2];
				swap_buffer[6] = ptr[1];
				swap_buffer[7] = ptr[0];
				ptr = swap_buffer;
				break;
			}
		}
#endif

		length = (UINT32)stemplate[i].length;
		memcpy(out_ptr, ptr, (size_t)length);
		out_ptr += length;
		actual += length;

		i++;
	}

	return actual;
}

/*---------------------------------------------------------------------*/
size_t Deserialize(VOID *structure, TEMPLATE * stemplate, UINT8 *in_ptr)
{
	UINT8 *ptr;
	size_t i, length, actual;

#ifdef LITTLE_ENDIAN_HOST
	UINT8 hold;
#endif

	ASSERT(structure != NULL);
	ASSERT(stemplate != NULL);
	ASSERT(in_ptr != NULL);

	actual = 0;

	i = 0;
	while (stemplate[i].offset != VOID_SIZE) {

		ptr = (UINT8 *)structure + stemplate[i].offset;

		length = (UINT32)stemplate[i].length;
		memcpy(ptr, in_ptr, (size_t)length);
		in_ptr += length;
		actual += length;

#ifdef LITTLE_ENDIAN_HOST
		if (stemplate[i].swap) {
			switch (stemplate[i].length) {
			case 2:
				hold = ptr[0];
				ptr[0] = ptr[1];
				ptr[1] = hold;
				break;
			case 4:
				hold = ptr[0];
				ptr[0] = ptr[3];
				ptr[3] = hold;
				hold = ptr[1];
				ptr[1] = ptr[2];
				ptr[2] = hold;
				break;
			case 8:
				hold = ptr[0];
				ptr[0] = ptr[7];
				ptr[7] = hold;
				hold = ptr[1];
				ptr[1] = ptr[6];
				ptr[6] = hold;
				hold = ptr[2];
				ptr[2] = ptr[5];
				ptr[5] = hold;
				hold = ptr[3];
				ptr[3] = ptr[4];
				ptr[4] = hold;
				break;
			}
		}
#endif

		i++;
	}

	return actual;
}
