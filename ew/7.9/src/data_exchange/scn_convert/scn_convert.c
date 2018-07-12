/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scn_convert.c 3363 2008-09-26 22:25:00Z kress $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <trace_buf.h>
#include "scn_convert.h"

static S2S *exp_scn = NULL;
static int max_exp = 0;
static int n_exp = 0;
static int incr_exp = INCR_SCN;
static S2S *wild_scn = NULL;
static int max_wild = 0;
static int n_wild = 0;
static int incr_wild = INCR_SCN;


static int compare_scn( const void *a, const void *b) 
{
    S2S *as = (S2S*)a;
    S2S *bs = (S2S*)b;
    int ns, nc;
    
    if ((ns = strcmp(as->scn.s, bs->scn.s)) != 0) return( ns );
    if ((nc = strcmp(as->scn.c, bs->scn.c)) != 0) return( nc );
    return(   strcmp(as->scn.n, bs->scn.n));
}


int s2s_com( void )
{
    char *str, *s, *c, *n;
    S2S *ss;
    int ws, wc, wc3, wn;
    
    if (k_its("SCN")) {
	s = k_str();
	c = k_str();
	n = k_str();
	if (s && c && n) {
	    ws = (strcmp(s, "*") == 0);
	    wc = (strcmp(c, "*") == 0);
	    wn = (strcmp(n, "*") == 0);
	    wc3 = (c[2] == '?');
	    if (ws || wc || wc3 || wn) {
		/* we have a wildcard */
		if (n_wild == max_wild) {
		    max_wild += incr_wild;
		    wild_scn = (S2S*) realloc(wild_scn, max_wild * 
					      sizeof(S2S));
		    if (wild_scn == NULL) {
			fprintf(stderr, "s2s_com: out of memory for wild_scn\n");
			exit(1);
		    }
		}
		ss = &wild_scn[n_wild];
		memset(ss, 0, sizeof(S2S));
		ss->scn.s = strdup(s);
		ss->scn.c = strdup(c);
		ss->scn.n = strdup(n);
		if ( ! (ss->scn.s && ss->scn.c && ss->scn.n)) {
		    fprintf(stderr, "s2s_com: out of memory for wild_scn\n");
		    exit(1);
		}
		if (ws) 
		    ss->ns = 0;
		else
		    ss->ns = strlen(s)+1;
		if (wc)
		    ss->nc = 0;
		else if (wc3)
		    ss->nc = 2;
		else
		    ss->nc = 4;
		if (wn)
		    ss->nn = 0;
		else
		    ss->nn = strlen(n)+1;
		n_wild++;
	    }
	    else {
		/* no wildcard */
		if (n_exp == max_exp) {
		    max_exp += incr_exp;
		    exp_scn = (S2S*) realloc(exp_scn, max_exp * 
					      sizeof(S2S));
		    if (exp_scn == NULL) {
			fprintf(stderr, "s2s_com: out of memory for exp_scn\n");
			exit(1);
		    }
		}
		ss = &exp_scn[n_exp];
		memset(ss, 0, sizeof(S2S));
		ss->scn.s = strdup(s);
		ss->scn.c = strdup(c);
		ss->scn.n = strdup(n);
		if ( ! (ss->scn.s && ss->scn.c && ss->scn.n)) {
		    fprintf(stderr, "s2s_com: out of memory for exp_scn\n");
		    exit(1);
		}
		n_exp++;
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scnl.s = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SNCL\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output station missing from <%s>\n",
			k_com());
		exit(1);
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scnl.c = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SNCL\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output channel missing from <%s>\n",
			k_com());
		exit(1);
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scnl.n = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SNCL\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output network missing from <%s>\n",
			k_com());
		exit(1);
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scnl.l = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SNCL\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output location missing from <%s>\n",
			k_com());
		exit(1);
	    }
	}
	else {
	    fprintf(stderr, "s2s_com: input SCN missing from <%s>\n", k_com());
	    exit(1);
	}
	return 1;
    }
    return(0);
}

void sort_scn(void)
{
    if (n_exp > 0)
	qsort(exp_scn, n_exp, sizeof(S2S), compare_scn);
    
    return;
}

int scn2scnl( S2S* s ) 
{
    S2S *found_scn;
    int i, found = 0;
    static char chan[TRACE2_CHAN_LEN];
    
    /* First look in the explicit list, if there are any scns in it */
    if (n_exp > 0) {
        found_scn = (S2S*) bsearch(s, exp_scn, n_exp, sizeof(S2S), compare_scn); /*LDD*/
	
	if (found_scn != NULL) {
	    s->scnl = found_scn->scnl;   /* structure copy */
	    return( 1 );
	}
    }
    
    /* not in explicit list; try wild-card list */
    for (i = 0; i < n_wild; i++) {
	if (wild_scn[i].ns > 0) {
	    /* station is not a wildcard */
	    if (strncmp(s->scn.s, wild_scn[i].scn.s, wild_scn[i].ns) != 0)
		continue;
	}
	if (wild_scn[i].nn > 0) {
	    /* network is not a wildcard */
	    if (strncmp(s->scn.n, wild_scn[i].scn.n, wild_scn[i].nn) != 0)
		continue;
	}
	if (wild_scn[i].nc > 0) {
	    /* channel is not "*"; could be 'XX?' */
	    if (strncmp(s->scn.c, wild_scn[i].scn.c, wild_scn[i].nc) != 0)
		continue;
	}
	found = 1;
	break;
    }
    
    if (! found) return( 0 );
    
    /* We found a match; fill in the to-be-returned scnl */
    if (wild_scn[i].scnl.s[0] == '*') 
	s->scnl.s = s->scn.s;
    else
	s->scnl.s = wild_scn[i].scnl.s;

    if (wild_scn[i].scnl.n[0] == '*')
	s->scnl.n = s->scn.n;
    else
	s->scnl.n = wild_scn[i].scnl.n;
    
    if (wild_scn[i].scnl.c[0] == '*')
	s->scnl.c = s->scn.c;
    else if (wild_scn[i].scnl.c[2] == '?') {
	strncpy(chan, wild_scn[i].scnl.c, TRACE2_CHAN_LEN);
	chan[2] = s->scn.c[2];
	chan[3] = '\0';
	s->scnl.c = chan;
    }
    else
	s->scnl.c = wild_scn[i].scnl.c;
    
    s->scnl.l = wild_scn[i].scnl.l;
    
    return( 1 );
}
