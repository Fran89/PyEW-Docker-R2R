/*! \file
 *
 * \brief Memory management for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id $
 *
 */

#ifndef NMXP_MEMORY_H
#define NMXP_MEMORY_H 1

#ifndef NMXP_MEM_DEBUG

#define NMXP_MEM_MALLOC(size) malloc(size)
#define NMXP_MEM_STRDUP(str) strdup(str)
 #define NMXP_MEM_FREE(ptr) free(ptr); ptr=NULL;
#define NMXP_MEM_PRINT_PTR(print_items, print_sfs) nmxp_mem_null_function()
#define NMXP_MEM_PRINT_SFS() nmxp_mem_null_function()


/*! \brief 
 *
 *
 */
int nmxp_mem_null_function();

#else

#define NMXP_MEM_MALLOC(size) nmxp_mem_malloc(size, __FILE__, __LINE__)
#define NMXP_MEM_STRDUP(str) nmxp_mem_strdup(str, __FILE__, __LINE__)
#define NMXP_MEM_FREE(ptr) nmxp_mem_free(ptr, __FILE__, __LINE__)
#define NMXP_MEM_PRINT_PTR(print_items, print_sfs) nmxp_mem_print_ptr(print_items, print_sfs, __FILE__, __LINE__)
#define NMXP_MEM_PRINT_SFS() nmxp_mem_print_sfs(__FILE__, __LINE__)


#include <stdlib.h>

/*! \brief 
 *
 * \param 
 * \param 
 *
 */
inline void *nmxp_mem_malloc(size_t size, char *source_file, int line);


/*! \brief 
 *
 * \param 
 * \param 
 *
 */
inline char *nmxp_mem_strdup(const char *str, char *source_file, int line);


/*! \brief 
 *
 * \param 
 * \param 
 *
 */
inline void nmxp_mem_free(void *ptr, char *source_file, int line);


/*! \brief 
 *
 * \param 
 * \param 
 *
 */
inline int nmxp_mem_print_ptr(int print_items, int print_sfs, char *source_file, int line);


#endif

#endif

