#ifndef _GERROR_H
#define _GERROR_H

/* COPYRIGHT 1997. Paul Friberg and Sid Hellman, RPSC */


#define ERR_SRATEBAD 	0
#define ERR_REVINTBAD 	1
#define ERR_TIMEBAD 	2
#define ERR_COMPCODEBAD 3
#define ERR_DURATNBAD 	4
#define ERR_SYSIDBAD 	5
#define ERR_FRSTDIFBAD 	6
#define ERR_TRANCSUMBAD	7


#ifdef USE_ERROR_STRUCT
static struct _GCF_ERROR {
	char * code;
	char * explain;
} gcf_err[] = {
	{ "ERR_SRATEBAD", "Sample rate is not an even number" } ,
	{ "ERR_REVINTBAD", "Reverse Integration constant does not match" } ,
	{ "ERR_TIMEBAD", "Invalid Time code, value is greater than 23:59:59" } ,
	{ "ERR_COMPCODEBAD", "Invalid compression code, not 1, 2, or 4" } ,
	{ "ERR_DURATNBAD", "Block duration is not an even number of seconds" } ,
	{ "ERR_SYSIDBAD", "System ID is suspicious" } ,
	{ "ERR_FRSTDIFBAD", "First Difference value is not zero" } ,
	{ "ERR_TRANCSUMBAD", "Transmission Checksum on GCF packet fails" } 
};
#endif

/* the printing function */

char * gerror_str();	/* get an error code and explanation */

#ifdef STDC
void gerror(char *str);
void fgerror(FILE *f, char *str);
#else
void gerror();
void fgerror();
#endif

#endif /* _GERROR_H */
