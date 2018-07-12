/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: arc2trig.h 2 2000-02-14 16:16:56Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/02/14 16:04:49  lucky
 *     Initial revision
 *
 *
 */

/*
 * arc2trig.h : Include file for arc2trig.c;
 */

#define MAX_STR		     255
#define SHORT_STR             64
#define MEDIUM_STR  (SHORT_STR*2)

/* Things read from config file
 ******************************/
char    OutputDir[SHORT_STR]; /* directory to write "triggers" to  */ 
char    TrigFileBase[SHORT_STR/2]; /* prefix of trigger file name  */

/* Function prototypes
 *********************/
int writetrig_init( void );   /*writetrig.c*/
int writetrig( char * );      /*writetrig.c*/ 
void writetrig_close( );      /*writetrig.c*/
