#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "earthworm.h"	/* need this for the logit() call */
#include "scn_map.h"	/* contains only public methods */
#include "externs.h"
#include "misc.h"

/* 
	
	USES A LINKED LIST!!!!

*/

static SCN *SCN_head = NULL;

int validateSCN(char *S,char *C,char *N, char *L);
int equivSysStrm(char *Sys, char *Strm, SCN *scn);

/*****************************************************************/
/* validate the SCN lengths and non-null states 
	SUCCESS returns 0
	FAILURE returns -1 if any of S,C,N, L are invalid lengths
*/
int validateSCN(char *S,char *C,char *N, char *L) {
	if (S == NULL || strlen(S) > SEED_SITE_MAX_CHAR) { return (-1);}
	if (C == NULL || strlen(C) > SEED_CHAN_MAX_CHAR) { return (-1);}
	if (N == NULL || strlen(N) > SEED_NET_MAX_CHAR) { return (-1);}
	if (L != NULL) { if (strlen(L) > SEED_LOC_MAX_CHAR) { return (-1);} }
	return(0);
}
/*****************************************************************/
/* compares the System_id and Stream_id to an SCN struct combo
	SUCCESS returns 0 (match!)
	FAILURE returns -1 (no match)
*/
int equivSysStrm(char *Sys,char *Strm, SCN *scn) {
	char tmp[SYS_STRM_COMBO_SIZE]; 
	sprintf(tmp, "%s%s", Sys, Strm);
	return (strcmp(tmp, scn->comb));
}
/*****************************************************************/
/* mallocs and inserts an SCN into the linked list
	SUCCESS returns 0 (inserted!)
	FAILURE returns -1 (bad SCN or memory alloc() problem)
*/
int insertSCN( char *Sys, char *Strm, char * S, char *C, char *N) {
	SCN *scn;
	if (validateSCN(S,C,N,NULL) == -1) {return (-1);}
	if ((scn = (SCN *) calloc(sizeof(SCN),1)) == NULL) {return (-1);}
	strcpy(scn->site, S);
	strcpy(scn->chan, C);
	strcpy(scn->net, N);
	scn->System_id = strdup(Sys);
	scn->Stream_id = strdup(Strm);

	sprintf(scn->comb, "%s%s", Sys, Strm);

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
int insertSCNL( char *Sys, char *Strm, char * S, char *C, char *N, char *L) {
	SCN *scn;
	if (validateSCN(S,C,N,L) == -1) {return (-1);}
	if ((scn = (SCN *) calloc(sizeof(SCN),1)) == NULL) {return (-1);}
	strcpy(scn->site, S);
	strcpy(scn->chan, C);
	strcpy(scn->net, N);
	if (L==NULL) {
		strcpy(scn->loc,"--" );
   	} else {
		strcpy(scn->loc,L);
	}
	scn->System_id = strdup(Sys);
	scn->Stream_id = strdup(Strm);

	sprintf(scn->comb, "%s%s", Sys, Strm);

	if (SCN_head == NULL) {
		/* init the linked list */
		SCN_head = scn;
		scn->next = NULL;
	} else {
		/* push the scn onto the list */
		scn->next = SCN_head;
		SCN_head = scn;
	}
#ifdef DEBUG_SCNL
	fprintf(stderr, "DEBUG: insertSCNL() inserted %s.%s.%s.%s\n", scn->site, scn->chan, scn->net, scn->loc);
#endif
	return(0);
}
/*****************************************************************/
/* finds a SCN for a given stream and system id
	SUCCESS returns 1
	FAILURE returns 0  
*/
int getSCN( char *Sys, char *Strm, SCN **scn)  {
	SCN *ptr;
	ptr = SCN_head;
	while (ptr != NULL) {
#ifdef DEBUG_SCNL
		fprintf(stderr, "DEBUG: getSCN() searching %s.%s.%s.%s for %s\n", ptr->site, ptr->chan, ptr->net, ptr->loc, ptr->comb);
#endif
		if (equivSysStrm(Sys, Strm, ptr) == 0) {
			*scn = ptr;
#ifdef DEBUG_SCNL
			fprintf(stderr, "DEBUG: getSCN() - match found\n");
#endif
			return(1);
		} else {
			ptr=ptr->next;
		}
	}
#ifdef DEBUG_SCNL
	if (Verbose)
		fprintf(stderr, "DEBUG: getSCN() returns no match found for %s %s\n", Sys, Strm);
#endif
	return (0);
}
/* get the first SCN that matches this system id, since all we want is Station and Network code */
int getFirstSCN(char *Sys, SCN **scn) {
	SCN *ptr;
	ptr = SCN_head;
	while (ptr != NULL) {
		if ( strcmp(ptr->System_id, Sys) == 0 ) {
			*scn = ptr;
			return(1);
		} else {
			ptr=ptr->next;
		}
	}
	return(0);
}
