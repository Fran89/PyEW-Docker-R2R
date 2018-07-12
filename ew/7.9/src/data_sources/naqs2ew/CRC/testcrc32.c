/* testcrc32.c */

#include <stdio.h>
#include <string.h>

void crc32_init( void );
void crc32_update( unsigned char *blk_adr, unsigned long blk_len );
unsigned long crc32_value( void );

/*
 *  Program to test the crc32.c code.
 */
void main(int argc, char* argv[])
{
  /* test string */
  char *test = "123456789";
  /* published CRC for test string */
  unsigned int check  = 0xCBF43926;
  int crc    = 0;
  int ix;

  printf("\nCRC calculated from \"123456789\":\n");

  crc32_init( );
  crc32_update( (unsigned char *)test, strlen(test) );
  crc = crc32_value();

  printf("CRC result is 0x%x\n", crc);
  printf("should be     0x%x\n", check);
  if (crc == check)
    printf("result checks OK\n");
  else
    printf("ERROR:  result does not equal published test value\n");


  printf("\nsame CRC incremented by small blocks:\n");

  crc32_init( );
  crc32_update( (unsigned char *)"1234", strlen("1234") );
  crc32_update( (unsigned char *)"567",  strlen("567")  );
  crc32_update( (unsigned char *)"89",   strlen("89")   );
  crc = crc32_value();

  printf("CRC result is 0x%x\n", crc);
  printf("should be     0x%x\n", check);
  if (crc == check)
    printf("result checks OK\n");
  else
    printf("ERROR:  result does not equal published test value\n");
}
