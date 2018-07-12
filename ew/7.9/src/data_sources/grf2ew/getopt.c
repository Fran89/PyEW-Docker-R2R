/*
 * my_getopt - get option letter from argv
 *
 * This is a version of the public domain getopt() implementation by
 * Henry Spencer, changed for 4.3BSD compatibility (in addition to System V).
 * It allows rescanning of an option list by setting optind to 0 before
 * calling, which is why we use it even if the system has its own (in fact,
 * this one has a unique name so as not to conflict with the system's).
 * Thanks to Dennis Ferguson for the appropriate modifications.
 *
 * This file is in the Public Domain.
 */

 /*LINTLIBRARY*/
#include <stdio.h>
#include <stdlib.h>
#ifdef	lint
#undef	putc
#define	putc	fputc
#endif										/* lint */
char *my_optarg;							/* Global argument pointer. */
int my_optind = 0;							/* Global argv index. */
int my_opterr = 1;							/* for compatibility, should error be printed? */
int my_optopt;								/* for compatibility, option character checked */

static char *scan = NULL;					/* Private scan pointer. */
static const char *prog = "amnesia";

/*
 * Print message about a bad option.
 */
static int badopt(const char *mess, int ch)
{
	if (my_opterr) {
		fputs(prog, stderr);
		fputs(mess, stderr);
		(void)putc(ch, stderr);
		(void)putc('\n', stderr);
	}
	return ('?');
}

int my_getopt(int argc, char *argv[], const char *optstring)
{
	register char c;
	register const char *place;

	prog = argv[0];
	my_optarg = NULL;

	if (my_optind == 0) {
		scan = NULL;
		my_optind++;
	}

	if (scan == NULL || *scan == '\0') {
		if (my_optind >= argc || argv[my_optind][0] != '-' || argv[my_optind][1] == '\0') {
			return (EOF);
		}
		if (argv[my_optind][1] == '-' && argv[my_optind][2] == '\0') {
			my_optind++;
			return (EOF);
		}

		scan = argv[my_optind++] + 1;
	}

	c = *scan++;
	my_optopt = c & 0377;
	for (place = optstring; place != NULL && *place != '\0'; ++place)
		if (*place == c)
			break;

	if (place == NULL || *place == '\0' || c == ':' || c == '?') {
		return (badopt(": unknown option -", c));
	}

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			my_optarg = scan;
			scan = NULL;
		}
		else if (my_optind >= argc) {
			return (badopt(": option requires argument -", c));
		}
		else {
			my_optarg = argv[my_optind++];
		}
	}

	return (c & 0377);
}
