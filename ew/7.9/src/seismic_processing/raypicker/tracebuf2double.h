/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tracebuf2double.h 4475 2011-08-04 15:17:09Z kevin $
 *
 *    Revision history:
 *     $Log: tracebuf2double.h,v $
 *     Revision 1.1  2008/05/29 18:46:46  mark
 *     Initial checkin (move from \resp; also changes for station list auto-update)
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:50  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
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
 * Declarations of functions used to convert from tracebuf
 * datatypes into the common datatype used within the raypicker
 * (SERIES_DATA).
 * 
 * @author Dale Hanych, Genesis Business Group (dbh)
 * @version 1.0 August 2003, dbh
 */

#ifndef TRACEBUF2DOUBLE_H
#define TRACEBUF2DOUBLE_H


#include <memory.h>       /*  size_t  */
#include "ray_trigger.h"  /*  SERIES_DATA  */

#define SERIES_DATA_CODE "ser" /* code to distinguish from tracebuf datatypes (e.g. "i4") */


#define DATA_TYPE_BAD    0
#define DATA_TYPE_LONG   1
#define DATA_TYPE_SHORT  2
#define DATA_TYPE_DOUBLE 3
#define DATA_TYPE_FLOAT  4
#define DATA_TYPE_SERIES 9  /* data already converted to SERIES_DATA */


int DecodeTraceBuffDataType(const char *typecode);
int NewBuffer(const char *datatypecode, const char *bufferStart);
SERIES_DATA NextBufferValue();


#endif  /*  TRACEBUF2DOUBLE_H  */
