/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tracebuf2double.c 6803 2016-09-09 06:06:39Z et $
 *
 *    Revision history:
 *     $Log: tracebuf2double.c,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2005/06/20 20:57:03  cjbryan
 *     removed unused include
 *
 *     Revision 1.3  2005/02/03 21:05:55  davidk
 *     Updated to SCNL.
 *
 *     Revision 1.2  2004/11/01 02:03:27  cjbryan
 *     removed all amplitude determination code from the picker and changed all error
 *     codes to positive numbers
 *     CVS ----------------------------------------------------------------------
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:27  michelle
 *     New Hydra Import
 *
 *
 *
 */
/*
 * Implementations of functions used to convert from tracebuf
 * datatypes into the common datatype used within the raypicker
 * (SERIES_DATA).
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

/* system includes */
#include <string.h>

/* earthworm includes */
#include <earthworm.h>

/* raypicker includes */ 
#include "tracebuf2double.h"
#include "returncodes.h"

/*
 * The next couple of functions are used to resolve the
 * different datatypes in a otherwise transparent way.
 */


static short g_datatype;

static int32_t * g_lData;
static short  * g_sData;
static double * g_dData;
static float  * g_fData;

/*********************************************************
 *          DecodeTraceBuffDataType                      *
 *                                                       *
 * Converts a tracebuf data type code into a datatype    *
 * byte width description (DATA_TYPE_xxx).               *
 *                                                       *
 *********************************************************/
int DecodeTraceBuffDataType(const char *typecode)
{
    if (typecode == NULL )
      return DATA_TYPE_BAD;
   
    if (strcmp(typecode, SERIES_DATA_CODE) == 0)
      return DATA_TYPE_SERIES;
   
    if (strcmp(typecode, "i4") == 0 
		|| strcmp(typecode, "s4") == 0)
      return DATA_TYPE_LONG;
   
    if (strcmp(typecode, "i2") == 0
		|| strcmp(typecode, "s2") == 0)
      return DATA_TYPE_SHORT;
   
    if (strcmp(typecode, "f8") == 0
		|| strcmp(typecode, "t8") == 0)
      return DATA_TYPE_DOUBLE;
   
    if (strcmp(typecode, "f4") == 0
		|| strcmp(typecode, "t4") == 0)
      return DATA_TYPE_FLOAT;

    return DATA_TYPE_BAD;
}

/*********************************************************
 *              NewBuffer()                              *
 * Prepares for the conversion of a tracebuf data buffer *
 * into the common series datatype.                      *
 *                                                       *
 * @param datatypecode is TRACE2_HEADER.datatype          *
 * @param bufferStart is the pointer to the first byte   *
 *   in the tracebuf data section                        *
 *********************************************************/
int NewBuffer(const char *datatypecode, const char *bufferStart)
{
    if (datatypecode == NULL || bufferStart == NULL)
      return -1;
   
    if ((g_datatype = DecodeTraceBuffDataType(datatypecode)) == DATA_TYPE_BAD)
      return -2;
   
    g_lData = (int32_t *)bufferStart;
    g_sData = (short *)bufferStart;
    g_dData = (double *)bufferStart;
    g_fData = (float *)bufferStart;
   
    return EW_SUCCESS;
}

/*********************************************************
 *              NextBufferValue()                        *
 * Returns the next tracebuf data value in the form of   *
 * the common series datatype.                           *
 *                                                       *
 * NOTE: NewBuffer() must be called before this.         *
 *                                                       *
 * WARNING: There are no safeguards against running past *
 * the end of the tracebuf message. Therefore, usage is  *
 * commonly to call this inside of a loop that is        *
 * constrained to the length of the tracebuf data len    *
 *********************************************************/
SERIES_DATA NextBufferValue()
{
    SERIES_DATA r_val;
   
    switch(g_datatype)
    {
      case DATA_TYPE_LONG:
          r_val = (SERIES_DATA)(*g_lData);
          g_lData++;
          break;
          
      case DATA_TYPE_SHORT:
          r_val = (SERIES_DATA)(*g_sData);
          g_sData++;
          break;
          
      case DATA_TYPE_DOUBLE:
      case DATA_TYPE_SERIES:
          r_val = (SERIES_DATA)(*g_dData);
          g_dData++;
          break;
          
      case DATA_TYPE_FLOAT:
          r_val = (SERIES_DATA)(*g_fData);
          g_fData++;
          break;
          
      default:
          r_val = (SERIES_DATA)0.0;
          break;       
    }
    return r_val;
}


