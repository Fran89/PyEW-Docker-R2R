/*
 *   This file is managed using Concurrent Versions System (CVS).
 *
 *    $Id: scnl_convert.c 2578 2007-01-17 19:25:25Z lombard $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <trace_buf.h>
#include "scnl_convert.h"

static S2S *exp_scnl = NULL;
static int max_exp = 0;
static int n_exp = 0;
static int incr_exp = INCR_SCNL;
static S2S *wild_scnl = NULL;
static int max_wild = 0;
static int n_wild = 0;
static int incr_wild = INCR_SCNL;

extern int Debug;

static int compare_scnl( const void *a, const void *b) 
{
    S2S *as = (S2S*)a;
    S2S *bs = (S2S*)b;
    int ns, nc, nl;
    
    if ((ns = strcmp(as->scnl.s, bs->scnl.s)) != 0) return( ns );
    if ((nc = strcmp(as->scnl.c, bs->scnl.c)) != 0) return( nc );
    if ((nl = strcmp(as->scnl.l, bs->scnl.l)) != 0) return( nl );
    return(   strcmp(as->scnl.n, bs->scnl.n));
}


int s2s_com( void )
{
    char *str, *s, *c, *n, *l;
    S2S *ss;
    
    if (k_its("SCNL")) {
	s = k_str();
	c = k_str();
	n = k_str();
	l = k_str();
	if (s && c && n && l) {
	    int ws, wc, wn, wc3, wl;
	    ws = (strcmp(s, "*") == 0);
	    wc = (strcmp(c, "*") == 0);
	    wn = (strcmp(n, "*") == 0);
	    wl = (strcmp(l, "*") == 0);
	    wc3 = (c[2] == '?');
	    if (ws || wc || wc3 || wn || wl) {
		/* we have a wildcard */
		if (n_wild == max_wild) {
		    max_wild += incr_wild;
		    wild_scnl = (S2S*) realloc(wild_scnl, max_wild * 
					      sizeof(S2S));
		    if (wild_scnl == NULL) {
			fprintf(stderr, "s2s_com: out of memory for wild_scnl\n");
			exit(1);
		    }
		}
		ss = &wild_scnl[n_wild];
		memset(ss, 0, sizeof(S2S));
		ss->scnl.s = strdup(s);
		ss->scnl.c = strdup(c);
		ss->scnl.n = strdup(n);
		ss->scnl.l = strdup(l);
		if ( ! (ss->scnl.s && ss->scnl.c && ss->scnl.n && ss->scnl.l)) {
		    fprintf(stderr, "s2s_com: out of memory for wild_scnl\n");
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
		if (wl)
		    ss->nl = 0;
		else
		    ss->nl = strlen(l)+1;
		n_wild++;
	    }
	    else {
		/* no wildcard */
		if (n_exp == max_exp) {
		    max_exp += incr_exp;
		    exp_scnl = (S2S*) realloc(exp_scnl, max_exp * 
					      sizeof(S2S));
		    if (exp_scnl == NULL) {
			fprintf(stderr, "s2s_com: out of memory for exp_scnl\n");
			exit(1);
		    }
		}
		ss = &exp_scnl[n_exp];
		memset(ss, 0, sizeof(S2S));
		ss->scnl.s = strdup(s);
		ss->scnl.c = strdup(c);
		ss->scnl.n = strdup(n);
		ss->scnl.l = strdup(l);
		if ( ! (ss->scnl.s && ss->scnl.c && ss->scnl.n && ss->scnl.l)) {
		    fprintf(stderr, "s2s_com: out of memory for exp_scnl\n");
		    exit(1);
		}
		n_exp++;
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scn.s = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SCN\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output station missing from <%s>\n",
			k_com());
		exit(1);
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scn.c = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SCN\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output channel missing from <%s>\n",
			k_com());
		exit(1);
	    }
	    if ((str = k_str()) != 0) {
		if ((ss->scn.n = strdup(str)) == NULL) {
		    fprintf(stderr, "s2s_com: out of memnory for SCN\n");
		    exit(1);
		}
	    }
	    else {
		fprintf(stderr, "s2s_com: output network missing from <%s>\n",
			k_com());
		exit(1);
	    }
	}
	else {
	    fprintf(stderr, "s2s_com: input SCNL missing from <%s>\n", k_com());
	    exit(1);
	}
	return 1;
    }
    return(0);
}

void sort_scnl(void)
{
    if (n_exp > 0)
	qsort(exp_scnl, n_exp, sizeof(S2S), compare_scnl);
    
    return;
}

int scnl2scn( S2S* s ) 
{
    S2S *found_scnl;
    int i, found = 0;
    static char chan[TRACE_CHAN_LEN];
    
    if (Debug) 
	logit("t", "%s.%s.%s.%s -> ",
	      s->scnl.s, s->scnl.c, s->scnl.n, s->scnl.l);

    /* First look in the explicit list, if there are any scnls in it */
    if (n_exp > 0) {
        found_scnl = (S2S*) bsearch(s, exp_scnl, n_exp, sizeof(S2S), 
				    compare_scnl); /*LDD*/
	
	if (found_scnl != NULL) {
	    s->scn = found_scnl->scn;   /* structure copy */
	    if (Debug) 
		logit("", "%s.%s.%s exp\n", s->scn.s, s->scn.c, s->scn.n);
	    
	    return( 1 );
	}
    }
    
    /* not in explicit list; try wild-card list */
    for (i = 0; i < n_wild; i++) {
	if (wild_scnl[i].ns > 0) {
	    /* station is not a wildcard */
	    if (strncmp(s->scnl.s, wild_scnl[i].scnl.s, wild_scnl[i].ns) != 0)
		continue;
	}
	if (wild_scnl[i].nn > 0) {
	    /* network is not a wildcard */
	    if (strncmp(s->scnl.n, wild_scnl[i].scnl.n, wild_scnl[i].nn) != 0)
		continue;
	}
	if (wild_scnl[i].nc > 0) {
	    /* channel is not "*"; could be 'XX?' */
	    if (strncmp(s->scnl.c, wild_scnl[i].scnl.c, wild_scnl[i].nc) != 0)
		continue;
	}
	if (wild_scnl[i].nl > 0) {
	    /* location is not a wildcard */
	    if (strncmp(s->scnl.l, wild_scnl[i].scnl.l, wild_scnl[i].nl) != 0)
		continue;
	}
	found = 1;
	break;
    }
    
    if (! found) {
	if (Debug) logit( "", "not found\n");
	return( 0 );
    }
    
    /* We found a match; fill in the to-be-returned scnl */
    if (wild_scnl[i].scn.s[0] == '*') 
	s->scn.s = s->scnl.s;
    else
	s->scn.s = wild_scnl[i].scn.s;

    if (wild_scnl[i].scn.n[0] == '*')
	s->scn.n = s->scnl.n;
    else
	s->scn.n = wild_scnl[i].scn.n;
    
    if (wild_scnl[i].scn.c[0] == '*')
	s->scn.c = s->scnl.c;
    else if (wild_scnl[i].scn.c[2] == '?') {
	strncpy(chan, wild_scnl[i].scn.c, TRACE_CHAN_LEN);
	chan[2] = s->scnl.c[2];
	chan[3] = '\0';
	s->scn.c = chan;
    }
    else
	s->scn.c = wild_scnl[i].scn.c;
    
    if (Debug)
	logit("", "%s.%s.%s wild\n", s->scn.s, s->scn.c, s->scn.n);
    
    return( 1 );
}

void log_s2s_config()
{
    int i;
    S2S *ss;
    
    if (n_wild > 0) {
	logit("", "wildcard map:\n");
	for (i = 0; i< n_wild; i++) {
	    ss = &wild_scnl[i];
	    logit("", "%s.%s.%s.%s (%d.%d.%d.%d) -> %s.%s.%s\n", 
		  ss->scnl.s, ss->scnl.c, ss->scnl.n, ss->scnl.l,
		  ss->ns, ss->nc, ss->nn, ss->nl,
		  ss->scn.s, ss->scn.c, ss->scn.n);
	}
	
    } else {
	logit("", "no wildcard map\n");
    }
    
    if (n_exp > 0) {
	logit("", "explicit map:\n");
	for (i = 0; i< n_exp; i++) {
	    ss = &exp_scnl[i];
	    logit("", "%s.%s.%s.%s -> %s.%s.%s\n", 
		  ss->scnl.s, ss->scnl.c, ss->scnl.n, ss->scnl.l,
		  ss->scn.s, ss->scn.c, ss->scn.n);
	}
	
    } else {
	logit("", "no explicit map\n");
    }
    
}
