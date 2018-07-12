/*
** harley2hinv.c
**
**  This program converts an ascii file from Harley's format to HInv format
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_LEN 120

int main( int argc, char* argv[] )
{
  /* IN LINE:
  **
  **   AAE    9.0291N  38.7655E 2442 IU Addis Ababa, Ethiopia
  **   AHID  42.7653N 111.1003W 1960 US Auburn Hatchery, Idaho, USA
  */

  /* OUT LINE:
  **
  **   AAE   IU  BHZ   9  1.7460N 38 45.9300E24420.0     0.00  0.00  0.00  0.00 1  0.00
  */

  FILE * infile = NULL
     , * outfile = NULL
     ;

  char   line[LINE_LEN+1]
     ,   work[9]
     , * wrkptr
     ;

  int cnt;

  char station[6]
     , nshemi[2]
     , ewhemi[2]
     , network[3]
     , lath[3]
     , lonh[4]
     , elev[5]
     ;

  int iwrk;

  double latm
       , lonm
       ;

  if ( argc < 3 )
  {
     printf( "Usage:\n\nharley2hinv <in_file> <out_file> <param_file>\n\n\n" );
     printf( "    where: <in_file>  is the input file name\n" );
     printf( "           <out_file> is the output file name\n\n" );
     return 0;
  }

  nshemi[1] = '\0';
  ewhemi[1] = '\0';


  if ( (infile = fopen(argv[1], "r")) == NULL )
  {
     printf( "Unable to open input file \"%s\"\n", argv[1] );
     return 1;
  }

  if ( (outfile = fopen(argv[2], "w")) == NULL )
  {
     fclose( infile );
     printf( "Unable to open output file \"%s\"\n", argv[2] );
     return 1;
  }

  while( fgets( line, LINE_LEN, infile ) != NULL )
  {
     strncpy( work, line, 5 );
     work[5] = '\0';
     strcpy( station, work );

     /*
     ** latitude hour
     */
     wrkptr = lath;
     if ( line[6] != ' ' )
     {
        *wrkptr = line[6];
        wrkptr += 1;
     }
     if ( line[7] == ' ' )
     {
        *wrkptr = '0';
     }
     else
     {
        *wrkptr = line[7];
     }
        wrkptr += 1;
        *wrkptr = '\0';

     /*
     ** latitude minute
     */
     wrkptr = work;
     strncpy( wrkptr, line + 9 , 4 );
     work[4] = '\0';
     latm = atof( work ) * 0.006;

     nshemi[0] = line[13];

     /*
     ** longitude hour
     */
     wrkptr = lonh;
     if ( line[15] != ' ' )
     {
        *wrkptr = line[15];
        wrkptr += 1;
     }
     if ( line[16] != ' ' )
     {
        *wrkptr = line[16];
        wrkptr += 1;
     }
     if ( line[17] == ' ' )
     {
        *wrkptr = '0';
     }
     else
     {
        *wrkptr = line[17];
     }
        wrkptr += 1;
        *wrkptr = '\0';

     ewhemi[0] = line[23];

     /*
     ** longitude minute
     */
     wrkptr = work;
     strncpy( wrkptr, line + 19 , 4 );
     work[4] = '\0';
     lonm = atof( work ) * 0.006;

     /*
     ** elevation
     */
     strncpy( elev, line + 25 , 4 );
     elev[4] = '\0';

     /*
     ** network
     */
     strncpy( network, line + 30 , 2 );
     network[2] = '\0';

     fprintf( outfile
            , "%s %s  BHZ  %2s %7.4f%s%3s %7.4f%s%s0.0     0.00  0.00  0.00  0.00 1  0.00\n"
            , station
            , network
            , lath
            , latm
            , nshemi
            , lonh
            , lonm
            , ewhemi
            , elev
            );
  }

  fclose( outfile );
  fclose( infile );

  return 0;
}
 
