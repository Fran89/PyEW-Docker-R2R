/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: exportfilter.h 107 2000-05-24 17:53:07Z lucky $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/05/24 17:53:07  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/03/29 16:16:00  whitmore
 *     Initial revision
 *
 *
 *
 */

/*   exportfilter.h   981112:ldd
 *   These are the prototypes for a generic filter for export.
 */

/*********************************************************
 * exportfilter_com() processes all config-file commands *
 *                    related to the filter code.        *
 * Returns  1 if the command was recognized an processed *
 *          0 if the command was not recognized          *
 * Note: this function may exit the process if it finds  *
 *       serious errors in any commands                  *
 *********************************************************/
int exportfilter_com( void );


/*********************************************************
 * exportfilter_init()   Make sure all the required      *
 *  commands were found in the config file, do any other *
 *  startup things necessary for filter to work properly *
 * Returns: 0 if all went well                           *
 *          non-zero if an error was detected            *
 *********************************************************/
int exportfilter_init( void );


/**********************************************************
 * exportfilter() looks at the candidate message.         *
 *                Analyzes it and possibly reformats it.  *
 * Returns: 1 if the resulting message is to be exported  *
 *          0 otherwise                                   *
 **********************************************************/
int exportfilter( char *, long,  unsigned char,     /* input message  */
                  char**, long*, unsigned char * ); /* output message */


/**********************************************************
 * exportfilter_shutdown()  frees allocated memory and    *
 *         does any other cleanup stuff                   *
 **********************************************************/
void exportfilter_shutdown( void );
