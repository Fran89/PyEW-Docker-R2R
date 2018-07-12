/*! \file
 *
 * \brief Log for Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp_log.h 3898 2010-03-25 13:25:46Z quintiliani $
 *
 */

#ifndef NMXP_LOG_H
#define NMXP_LOG_H 1


/*! normal output with time and package name */
#define NMXP_LOG_SET     -1

/*! normal output with time and package name */
#define NMXP_LOG_NORM     0

/*! error output with time and package name */
#define NMXP_LOG_ERR      1

/*! warning output with time and package name */
#define NMXP_LOG_WARN     2

/*! normal output without time and package name */
#define NMXP_LOG_NORM_NO  3

/*! normal output with only package name */
#define NMXP_LOG_NORM_PKG 4

/*! If s is NULL return "<null>", otherwise value of s */
#define NMXP_LOG_STR(s) (s == NULL)? "<null>" : s

/*! kind of log message */
#define NMXP_LOG_D_NULL        0
#define NMXP_LOG_D_CHANSTATE   1
#define NMXP_LOG_D_CHANNEL     NMXP_LOG_D_CHANSTATE << 1
#define NMXP_LOG_D_RAWSTREAM   NMXP_LOG_D_CHANSTATE << 2
#define NMXP_LOG_D_CRC         NMXP_LOG_D_CHANSTATE << 3
#define NMXP_LOG_D_CONNFLOW    NMXP_LOG_D_CHANSTATE << 4
#define NMXP_LOG_D_PACKETMAN   NMXP_LOG_D_CHANSTATE << 5
#define NMXP_LOG_D_EXTRA       NMXP_LOG_D_CHANSTATE << 6
#define NMXP_LOG_D_DATE        NMXP_LOG_D_CHANSTATE << 7
#define NMXP_LOG_D_GAP         NMXP_LOG_D_CHANSTATE << 8
#define NMXP_LOG_D_DOD         NMXP_LOG_D_CHANSTATE << 9
#define NMXP_LOG_D_ANY  \
( NMXP_LOG_D_CHANSTATE | NMXP_LOG_D_CHANNEL | NMXP_LOG_D_RAWSTREAM | NMXP_LOG_D_CRC | NMXP_LOG_D_CONNFLOW | \
  NMXP_LOG_D_PACKETMAN | NMXP_LOG_D_EXTRA | NMXP_LOG_D_DATE | NMXP_LOG_D_GAP | NMXP_LOG_D_DOD )

/*! \brief  Add prefix string for logging
 *
 * \param prefix string message
 *
 */
void nmxp_log_set_prefix(char *prefix);

/*! \brief  Print value of PACKAGE_NAME and PACKAGE_VERSION
 */
const char *nmxp_log_version();


/*! \brief Set function pointers for "normal logging" and "error logging"
 *
 * \param func_log Function pointer to the the function for "normal logging"
 * \param func_log_err Function pointer to the the function for "error logging"
 *
 */
void nmxp_log_init(int (*func_log)(char *), int (*func_log_err)(char *));


/*! \brief Add function pointers to "normal logging" and "error_logging"
 *
 * \param func_log Function pointer to the the function for "normal logging"
 * \param func_log_err Function pointer to the the function for "error logging"
 *
 *
 */
void nmxp_log_add(int (*func_log)(char *), int (*func_log_err)(char *));


/*! \brief Remove function pointers from "normal logging" and "error_logging"
 *
 * \param func_log Function pointer to the the function for "normal logging"
 * \param func_log_err Function pointer to the the function for "error logging"
 *
 */
void nmxp_log_rem(int (*func_log)(char *), int (*func_log_err)(char *));


/*! \brief Wrapper for fprintf to stdout and flushing
 *
 * \param msg String message
 *
 */
int nmxp_log_stdout(char *msg);


/*! \brief Wrapper for fprintf to stderror and flushing
 *
 * \param msg String message
 */
int nmxp_log_stderr(char *msg);


/*! \brief A generic logging/printing routine
 * 
 *   This function works in two modes:
 *
 *   -# Initialization, expecting 2 arguments with the first (level)
 *       being NMXP_LOG_SET and the second being verbosity bitmap. This will
 *       set the verbosity for all future calls, the default is NMXP_LOG_D_NULL.
 *       Can be used to change the verbosity at any time.
 *       I.e. 'nmxp_log(NMXP_LOG_SET, NMXP_LOG_D_PACKET | NMXP_LOG_D_CONNFLOW);'
 *   -# Expecting 3+ arguments, log level, verbosity, printf
 *       format, and printf arguments.  If the verbosity is included
 *       into the set verbosity bitmap (see mode 1), the printf
 *       format and arguments will be printed at the appropriate log
 *       level, where level represents:
 *       -# 0, normal output with time and package name
 *       -# 1, error output with time and package name
 *       -# 2, warning output with time and package name
 *       -# 3, normal output without time and package name
 *       -# 4, normal output with only package name
 *   N.B. Error messages will always be printed!
 *        TODO Optional for all warning messages
 *
 *
 *   \retval new_verbosity if using mode 1.
 *   \retval n the number of characters formatted on success, and a
 *     a negative value on error if using mode 2.
 *
 *   \param level
 *   \param verb
 *   \param ...
 */
int nmxp_log(int level, int verb, ... );

#endif

