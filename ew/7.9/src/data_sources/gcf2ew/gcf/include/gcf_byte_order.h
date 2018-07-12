
/* 
   Motorola or Sun Byte order = MSB_FIRST = Big Endian = B
   Intel = LSB_FIRST	= Little Endian = L
*/


#define  MSB_FIRST 0
#define  LSB_FIRST 1

#ifdef  USE_MSB_STRS
static char byte_order_char[] = "BL";
#endif
