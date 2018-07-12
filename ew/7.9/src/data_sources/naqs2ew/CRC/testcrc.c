/* testcrc.cpp */

#include "crcmodel.h"
#include <stdio.h>

/**
 *  Program to test the crcmodel.c code.
 */
void main(int argc, char* argv[])
{
  /* test string */
  char* test = "123456789";
  /* published CRC for test string */
  unsigned int check  = 0xCBF43926;
  int crc    = 0;
  int ix;

  /* step 1: declare a struct holding the CRC params */
  cm_t cm_params;
  p_cm_t p_cm = &cm_params;

  /* step 2: assign appropriate values for 32-bit CRC */
  p_cm->cm_width = 32;
  p_cm->cm_poly  = 0x04C11DB7L;
  p_cm->cm_init  = 0xFFFFFFFFL;
  p_cm->cm_refin = TRUE;
  p_cm->cm_refot = TRUE;
  p_cm->cm_xorot = 0xFFFFFFFFL;

  /* step 3: initialize the CRC */
  cm_ini(p_cm);

  /* step 4: process bytes */
  for (ix = 0; ix < 9; ix++)
    cm_nxt(p_cm, test[ix]);

  /* step 5: extract and print the CRC value */
  crc = cm_crc(p_cm);
  printf("CRC result is 0x%x\n", crc);
  printf("should be     0x%x\n", check);
  if (crc == check)
    printf("result checks OK\n");
  else
    printf("ERROR:  result does not equal published test value\n");
}
