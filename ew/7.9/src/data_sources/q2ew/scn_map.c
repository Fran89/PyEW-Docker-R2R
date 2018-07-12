#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "earthworm.h"	/* need this for the logit() call */
#include "scn_map.h"	/* contains only public methods */
#include "externs.h"
#include "misc.h"

/* HOW TO GET AN SCN into a PINNO...for those EW installations using
	exclusively pinno's and not SCN's 
	
	USES A LINKED LIST!!!!

	note pinno exclusivity not checked!....last SCN pin map is
	the most valid one!

	THIS IS NOT THE FASTEST IMPLEMENTATION POSSIBLE since a linear
	search is performed for each packet. We are presuming that this
	code is being used in a small network ( < 100 stations/chan/nets)
	such that the linear search is rather fast...
	Improvement if this proves to be a bottleneck:
		create a hash table to map SCN as a key to pinno's
*/
/* public */

int insertSCN( char * S, char *C, char *N, int pin);
int insertSCNL( char * S, char *C, char *N, char *L, int pin);
int getPinFromSCN( char * S, char *C, char *N);
int getPinFromSCNL( char * S, char *C, char *N, char *L);

/* private procs /defines / variables
	PRIVATE in C == not used outside this file! */

#define SEED_SITE_MAX_CHAR 5
#define SEED_CHAN_MAX_CHAR 3
#define SEED_NET_MAX_CHAR 2
#define SEED_LOC_MAX_CHAR 2


typedef struct _SCN {
	char site[SEED_SITE_MAX_CHAR+1];	/* site name SEED */
	char chan[SEED_CHAN_MAX_CHAR+1];	/* channel name SEED */
	char net[SEED_NET_MAX_CHAR+1];		/* network code SEED */
	char loc[SEED_LOC_MAX_CHAR+1];	/* location code */
	char comb[SEED_SITE_MAX_CHAR+SEED_CHAN_MAX_CHAR+SEED_NET_MAX_CHAR+1];
	int pinno;	/* EW PinNumber */
	struct _SCN *next;	/* linked list to next mapping */
} SCN;

static SCN *SCN_head = NULL;

int validateSCN(char *S,char *C,char *N, int pinno);
int validateSCNL(char *S,char *C,char *N, char *L, int pinno);
int equivSCN(char *S,char *C,char *N, SCN *scn);

/*****************************************************************/
/* validate the SCN lengths and non-null states 
	SUCCESS returns 0
	FAILURE returns -1 if any of S,C,N or pinno are invalid 
*/
int validateSCN(char *S,char *C,char *N, int pinno) {
	if (S == NULL || strlen(S) > SEED_SITE_MAX_CHAR) { return (-1);}
	if (C == NULL || strlen(C) > SEED_CHAN_MAX_CHAR) { return (-1);}
	if (N == NULL || strlen(N) > SEED_NET_MAX_CHAR) { return (-1);}
	if (pinno < 0) {return (-1);}
	return(0);
}

/*****************************************************************/
/* validate the SCNL lengths and non-null states 
	SUCCESS returns 0
	FAILURE returns -1 if any of S,C,N or pinno are invalid 
*/
int validateSCNL(char *S,char *C, char *N, char *L, int pinno) {
	if (S == NULL || strlen(S) > SEED_SITE_MAX_CHAR) { return (-1);}
	if (C == NULL || strlen(C) > SEED_CHAN_MAX_CHAR) { return (-1);}
	if (N == NULL || strlen(N) > SEED_NET_MAX_CHAR) { return (-1);}
	if (L == NULL || strlen(L) > SEED_LOC_MAX_CHAR) { return (-1);}
	if (pinno < 0) {return (-1);}
	return(0);
}
/*****************************************************************/
/* compares the S,C,N to an SCN struct
	SUCCESS returns 0 (match!)
	FAILURE returns -1 (no match)
*/
int equivSCN(char *S,char *C,char *N, SCN *scn) {
	char tmp[SEED_SITE_MAX_CHAR+SEED_CHAN_MAX_CHAR+SEED_NET_MAX_CHAR+1]; 
	sprintf(tmp, "%s%s%s", S, C, N);
	return (strcmp(tmp, scn->comb));
}

/*****************************************************************/
/* compares the S,C,N to an SCNL struct
	SUCCESS returns 0 (match!)
	FAILURE returns -1 (no match)
*/
int equivSCNL(char *S,char *C,char *N, char *L, SCN *scn) {
	char tmp[SEED_SITE_MAX_CHAR+SEED_CHAN_MAX_CHAR+SEED_NET_MAX_CHAR+SEED_LOC_MAX_CHAR+1]; 
	sprintf(tmp, "%s%s%s%s", S, C, N, L);
	return (strcmp(tmp, scn->comb));
}
/*****************************************************************/
/* mallocs and inserts an SCN into the linked list
	SUCCESS returns 0 (inserted!)
	FAILURE returns -1 (bad SCN or memory alloc() problem)
*/
int insertSCN( char * S, char *C, char *N, int pin) {
	SCN *scn;
	if (validateSCN(S,C,N,pin) == -1) {return (-1);}
	if ((scn = (SCN *) calloc(sizeof(SCN),1)) == NULL) {return (-1);}
	strcpy(scn->site, S);
	strcpy(scn->chan, C);
	strcpy(scn->net, N);
	sprintf(scn->comb, "%s%s%s", S, C, N);

	if (Verbose == TRUE)
		fprintf(stderr, "%s: read in SCN combo = %s pin = %d\n", 
			Progname, scn->comb, pin);
	scn->pinno=pin;
	if (SCN_head == NULL) {
		/* init the linked list */
		SCN_head = scn;
		scn->next = NULL;
	} else {
		/* push the scn onto the list */
		scn->next = SCN_head;
		SCN_head = scn;
	}
	return(0);
}

/*****************************************************************/
/* mallocs and inserts an SCN into the linked list
	SUCCESS returns 0 (inserted!)
	FAILURE returns -1 (bad SCN or memory alloc() problem)
*/
int insertSCNL( char * S, char *C, char *N, char *L, int pin) {
	SCN *scn;
	if (validateSCNL(S,C,N, L, pin) == -1) {return (-1);}
	if ((scn = (SCN *) calloc(sizeof(SCN),1)) == NULL) {return (-1);}
	strcpy(scn->site, S);
	strcpy(scn->chan, C);
	strcpy(scn->net, N);
	strcpy(scn->loc, L);
	sprintf(scn->comb, "%s%s%s%s", S, C, N, L);

	if (Verbose == TRUE)
		fprintf(stderr, "%s: read in SCN combo = %s pin = %d\n", 
			Progname, scn->comb, pin);
	scn->pinno=pin;
	if (SCN_head == NULL) {
		/* init the linked list */
		SCN_head = scn;
		scn->next = NULL;
	} else {
		/* push the scn onto the list */
		scn->next = SCN_head;
		SCN_head = scn;
	}
	return(0);
}
/*****************************************************************/
/* finds a pinno for a given SCN
	SUCCESS returns pinno matching SCN
	FAILURE returns 0  (default pin)
*/
int getPinFromSCN( char * S, char *C, char *N)  {
	SCN *ptr;
	ptr = SCN_head;
	while (ptr != NULL) {
		if (equivSCN(S,C,N, ptr) == 0) {
			return(ptr->pinno);
		} else {
			ptr=ptr->next;
		}
	}
	return (-1);
}

/*****************************************************************/
/* finds a pinno for a given SCNL
	SUCCESS returns pinno matching SCNL
	FAILURE returns 0  (default pin)
*/
int getPinFromSCNL( char * S, char *C, char *N, char *L)  {
	SCN *ptr;
	ptr = SCN_head;
	while (ptr != NULL) {
		if (equivSCNL(S,C,N, L, ptr) == 0) {
			return(ptr->pinno);
		} else {
			ptr=ptr->next;
		}
	}
	return (-1);
}
