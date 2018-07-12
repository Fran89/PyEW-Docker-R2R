/*
TAULIB.C

These functions have been converted to C at the WCATWC.  These are used 
throughout the iaspei91 travel time programs provided by NEIC.

These were updated in December, 2012 to expand the phases delivered and clean-up
some sloppiness - PW.
*/

#ifdef _WINNT
 #include <windows.h>
 #include <winuser.h>
 typedef BOOL bool;
#else
 #define TRUE 1
 #define FALSE 0
 typedef char bool;
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <earthworm.h>
#include "earlybirdlib.h"
#include "iasplib.h"

#define NCMD		4
#define LCMD		16
#define	DEBUGL		0

/* Variables used in phase getting */
bool	bPrnt[3];
/* Segmsk is a logical array that actually implements the branch editing in 
   depset and depcor. */
bool	bSegmsk[JSEG];
double	dCoef[5][JOUT];
double	dDBrn[JBRN][2];
double	dFcs[JSEG][3];
double	dDepLim;
double	dODep;
double	dPk[JSEG];
double	dPm[JSRC][2];
double	dPt[JOUT];
double	dPu[JTSM0][2];
double	dPux[JXSM][2];
double	dPx[JBRN][2];
double	dTau[JOUT][4];
double	dTaua[5][2];
double	dTauc[JREC];
double	dTaut[JOUT];
double	dTauu[JTSM][2];
double	dTCoef[2][5][JOUT];
double	dTp[JBRNU][2];
double	dUa[5][2];
double	dUs[2];
double	dXBrn[JBRN][3];
double	dXLim[2][JOUT];
double	dXc[JXSM];
double	dXn, dPn, dTn, dDn, dHn;
double	dXt[JBRN][2];
double	dXu[JXSM][2];
double	dZm[JSRC][2];
double	dZs;
int	iDel[JBRN][3];          /* Based on 1 */
int	iIdx[JSEG];
int	iIndx[JSEG][2];
int	iInt0[2];
int	iIsrc[2];               /* Based on 0 */
int	iJIDex[JBRN];
int	iJNDex[JBRN][2];
int	iKa, iKi;
int	ikk[JSEG];
int	iKm[2];
int	iKndx[JSEG][2];
int	iKu[2];
int	iMbr1, iMbr2;
int	iMsrc[2];
int	iMt[2];
int	iNafl[JSEG][3];         /* Based on 1 */
int	iNBrn;
int	iNDex[JSRC][2];
int	iNph0;
int	iNSeg;
int	iRecLen;
char	szPhcd[JBRN][PHASE_LENGTH];

/*
 $$$$$ calls no other routines $$$$$

   Bkin reads a block of len double precision words into array buf(len)
   from record nrec of the direct access unformatted file connected to
   logical unit lu.
*/
void bkin( FILE *hFileTbl, int nrec, int len, double buf[] )
{
   int	  i;

   rewind( hFileTbl );
   if ( nrec > 0 )
   {
      if ( DEBUGL ) logit( "", "bkin read; nrec=%d, reclen=%d, len=%d\n",
                           nrec, iRecLen, len );
      fseek( hFileTbl, (nrec-1)*iRecLen, SEEK_SET );
      fread( buf, sizeof (double), len, hFileTbl );
      return;
   }
/* If the record doesn't exist, zero fill the buffer. */
   for ( i=0; i<len; i++ ) buf[i] = 0.;
   return;
}

/*
   Brnset takes character array pcntl(nn) as a list of nn tokens to be
   used to select desired generic branches.  Prflg(3) is the old
   bPrnt[1] debug print flags in the first two elements plus a new print
   flag which controls a branch selected summary from brnset.  Note that
   the original two flags controlled a list of all tau interpolations
   and a branch range summary respectively.  The original summary output
   still goes to logical unit 10 (ttim1.lis) while the new output goes
   to the standard output (so the caller can see what happened).  Each
   token of pcntl may be either a generic branch name (e.g., P, PcP,
   PKP, etc.) or a keyword (defined in the data statement for cmdcd
   below) which translates to more than one generic branch names.  Note
   that generic branch names and keywords may be mixed.  The keywords
   'all' (for all branches) and 'query' (for an interactive token input
   query mode) are also available.
*/
int brnset( int nn, char pcntl[10][PHASE_LENGTH], int prflg[] )
{
   bool    fnd, all;
   static int     nsgpt[JBRN];            /* Based on 0 */
   static int     ncmpt[NCMD][2] = { {1, 2}, {1, 7}, {1, 13}, {13, 16} };
   int     j1, j2, no;             /* Based on 1 */
   int     i, j, kseg, k, l;       /* Based on 0 */
   char	   segcd[JBRN][PHASE_LENGTH], 
           cmdcd[NCMD][PHASE_LENGTH] = {"P", "P+", "basic", "S+"}, 
           cmdlst[LCMD][PHASE_LENGTH] = {"P", "PKiKP", "PcP", "pP", "pPKiKP",
            "sP", "sPKiKP", "ScP", "SKP", "PKKP", "SKKP", "PP", "S", "ScS", 
            "sS", "pS"}, 
           phtmp[PHASE_LENGTH], 
           phlst[JSEG][PHASE_LENGTH];

/*
   The keywords do the following:
      P      gives P-up, P, Pdiff, PKP, and PKiKP
      P+     gives P-up, P, Pdiff, PKP, PKiKP, PcP, pP, pPdiff, pPKP,
             pPKiKP, sP, sPdiff, sPKP, and sPKiKP
      S+     gives S-up, S, Sdiff, SKS, sS, sSdiff, sSKS, pS, pSdiff,
             and pSKS
      basic  gives P+ and S+ as well as ScP, SKP, PKKP, SKKP, PP, and
             P'P'
   Note that generic S gives S-up, Sdiff, and SKS already and so
   doesn't require a keyword.
*/

/* Take care of the print flags. */
   bPrnt[0] = prflg[0];
   bPrnt[1] = prflg[1];
   if ( bPrnt[0] )          bPrnt[1]   = TRUE;
   for ( i=0; i<JSEG; i++ ) bSegmsk[i] = TRUE;
/* Copy the token list into local storage. */
   no = min( nn, JSEG );
   for ( i=0; i<no; i++ ) strcpy( phlst[i], pcntl[i] );

/* See if we are in query mode. */
/*
if (no <= 1 && (!strcmp (phlst[0], "query") || !strcmp (phlst[0], "QUERY")))
    {
	// In query mode, get the tokens interactively into local storage.
    TopIf:
    printf ("Enter desired branch control list at the prompts:\n");
    no = 0;
	// Terminate the list of tokens with a blank entry.
    for (;;)
        {
        gets (phlst[no]);
	if (strlen (phlst[no]) > 0)
            {
            no++;
            if (no >= JSEG) goto OutOfIf;
	    }
	  else
	    break;
	}

    if (no == 0)
        {
	// If the first token is blank, help the user out.
        printf ("\n\nYou must enter some branch control information!\n");
        printf ("     possibilities are:\n");
        printf ("          all\n");
	for (i=0; i<NCMD; i++) printf ("%s\n", cmdcd[i]);
        printf ("          or any generic phase name\n");
        goto TopIf;
	}
    }
OutOfIf:
*/

/* An 'all' keyword is easy as this is already the default. */
   all = FALSE;
   if ( no == 1 && (!strcmp( phlst[0], "all" ) || !strcmp( phlst[0], "ALL" )) ) 
      all = TRUE;
   if ( all && !prflg[2] ) return( 1 );

/* Make one or two generic branch names for each segment.  For example,
   the P segment will have the names P and PKP, the PcP segment will
   have the name PcP, etc. */
   kseg = -1;
   j    = -1;

/* Loop over the segments. */
   for ( i=0; i<iNSeg; i++ )
   {
      if ( !all ) bSegmsk[i] = FALSE;
/* For each segment, loop over associated branches. */
      JInc:
      j++;
      strcpy( phtmp, szPhcd[j] );
/* Turn the specific branch name into a generic name by stripping out
   the crustal branch and core phase branch identifiers. */
      for ( l=1; l<8; l++ )
      {
         TopFor:
         if ( phtmp[l] == ' ' ) break;
         if ( phtmp[l] == 'g' || phtmp[l] == 'b' || phtmp[l] == 'n' )
         {
            if ( l <  7 ) strcpy( &phtmp[l], &phtmp[l+1] );
            if ( l >= 7 ) phtmp[l] = '\0';
            goto TopFor;
         }
         if ( l >= 7 ) continue;
         if ( strncmp( &phtmp[l], "ab", 2 ) && strncmp( &phtmp[l], "ac", 2 ) && 
              strncmp( &phtmp[l], "df", 2 ) ) continue;
         phtmp[l] = '\0';
         break;
      }
      if ( DEBUGL ) logit( "", "j phcd phtmp = %d %s %s\n", j+1, szPhcd[j],
                           phtmp );
/* Make sure generic names are unique within a segment. */
      if ( kseg >= 0 )
         if ( !strcmp( phtmp, segcd[kseg] ) ) goto NextCheck;
      kseg++;
      strcpy( segcd[kseg], phtmp );
      nsgpt[kseg] = i;
      if ( prflg[2] && DEBUGL ) 
         logit( "", "kseg nsgpt segcd = %d %d %s\n",
                kseg+1, nsgpt[kseg]+1, segcd[kseg] );
      NextCheck:
      if ( iJIDex[j] < iIndx[i][1] ) goto JInc;
   }
   if ( all ) goto Branch;

/* Interpret the tokens in terms of the generic branch names. */
   for ( i=0; i<no; i++ )
   {                                              /* Try for a keyword first. */
      for ( j=0; j<NCMD; j++ ) if ( !strcmp( phlst[i], cmdcd[j] ) ) goto Match;
/* If the token isn't a keyword, see if it is a generic branch name. */
      fnd = FALSE;
      for ( k=0; k<=kseg; k++ )
      {
         if ( strcmp( phlst[i], segcd[k] ) ) continue;
         fnd        = TRUE;
         l          = nsgpt[k];
         bSegmsk[l] = TRUE;
         if ( DEBUGL ) 
	    logit( "", "Brnset:  phase found - i k l segcd = %d %d %d %s\n",
                   i, k, l, segcd[k] );
      }
/* If no matching entry is found, warn the caller. */
      if ( !fnd & DEBUGL )
         logit( "", "Brnset:  phase %s not found\n", phlst[i] );
      continue;

/* If the token is a keyword, find the matching generic branch names. */
      Match:
      j1 = ncmpt[j][0];
      j2 = ncmpt[j][1];
      for ( j=j1-1; j<j2; j++ )
         for ( k=0; k<=kseg; k++ )
            if ( !strcmp( cmdlst[j], segcd[k] ) )
            {
               l = nsgpt[k];
               bSegmsk[l] = TRUE;
               if ( DEBUGL ) 
                  logit( "", "Brnset:  cmdlst found - j k l segcd = "
                         "%d %d %d %s\n", i, k, l, segcd[k] );
            }
   }

/* Make the caller a list of the generic branch names selected. */
   Branch:
   if ( !prflg[2] ) return( 1 );
   fnd = FALSE;
   j2  = 0;
/* Loop over segments. */
   for ( i=0; i<iNSeg; i++ )
   {
      if ( !bSegmsk[i] ) continue;
/* If selected, find the associated generic branch names. */
      j2++;
      for ( j1=j2; j1<=kseg+1; j1++ )
         if ( nsgpt[j1-1] == i ) goto SkipAhead;
      if ( DEBUGL ) logit( "", "Brnset:  Segment pointer (%d) missing?\n", i );
      continue;
      SkipAhead:
      for ( j2=j1; j2<=kseg+1; j2++ )
         if ( nsgpt[j2-1] != i ) goto SkipAhead2;
      j2 = kseg + 2;
/* Print the result. */
      SkipAhead2:
      j2--;
      if ( !fnd & DEBUGL ) 
         logit( "", "Brnset:  the following phases have been selected:\n" );
      fnd = TRUE;
      if ( DEBUGL )
      {
         logit( "", "     %d  ", i+1 );
         for ( j=j1-1; j<j2; j++ ) logit( "", "%10s", segcd[j] );
         logit( "", "\n" );
      }
   }
   return( 1 );
}

void depcor( int nph, FILE *hFileTbl )
{
   bool   noend, noext;
   double dtol = 1.e-6;
   int    i, iph, is, j, kph, ks, mu = 0, k, ll, m;	/* Based on 0 */
   int    i1, i2, l, ms, n1, k1, k2;		        /* Based on 1 */
   int    lpower = 7, lp;
   double tol = 0.01;
   double tauus1[2], tauus2[2], xus1[2], xus2[2], ttau, tx, sgn, 
          umin = 0.0, u0 = 0.0, u1, z0 = 0.0, z1, fac = 0.0, du, ztol;
   double *tup;

   tup     = &dTauc[0];
   dDepLim = 1.1;
   iKa     = 4;

   if ( nph == iNph0 ) goto ConstructTau;
   iNph0 = nph;
   dUs[nph] = umod( dZs, &iIsrc[nph], nph );
/* If we are in a high slowness zone, find the slowness of the lid. */
   umin = dUs[nph];
   ks   = iIsrc[nph];
   for ( i=0; i<ks+1; i++ )
      if ( dPm[i][nph] <= umin ) umin = dPm[i][nph];
/* Find where the source slowness falls in the ray parameter array. */
   n1 = iKu[nph] + 1;
   for ( i=1; i<n1; i++ )
      if ( dPu[i][nph] > umin ) goto SkipIt;
   k2 = n1;
   if ( dPu[n1-1][nph] == umin ) goto ReadDep;
   logit( "", "Source slowness too large.\n" );
   return;
   SkipIt:
   k2 = i + 1;

/* Read in the appropriate depth correction values. */
   ReadDep:
   noext = FALSE;
   sgn   = 1.;
   if ( iMsrc[nph] == 0 ) iMsrc[nph] = 1;
/* See if the source depth coincides with a model sample */
   ztol = dXn * tol / (1. - dXn*dODep);
   if ( fabs( dZs-dZm[ks+1][nph] ) <= ztol )
      ks = ks + 1;
   else
      if ( fabs( dZs-dZm[ks][nph] ) > ztol ) goto Interp;
/* If so flag the fact and make sure that the right integrals are available. */
   noext = TRUE;

   if ( iMsrc[nph] == ks+1 ) goto Fiddle;
   bkin( hFileTbl, iNDex[ks][nph], iKu[nph]+iKm[nph], tup );
   goto MoveDepth;
/* If it is necessary to interpolate, see if appropriate integrals
   have already been read in. */
   Interp:
   if (iMsrc[nph] == ks+2)
   {
      ks++;
      sgn = -1.;
      goto Fiddle;
   }
   if ( iMsrc[nph] == ks+1 ) goto Fiddle;
/* If not, read in integrals for the model depth nearest the source depth. */
   if ( fabs( dZm[ks][nph]-dZs ) > fabs( dZm[ks+1][nph]-dZs) )
   {
      ks++;
      sgn = -1.;
   }
   bkin( hFileTbl, iNDex[ks][nph], iKu[nph]+iKm[nph], tup );
/* Move the depth correction values to a less temporary area. */
   MoveDepth:
   for (i=0; i<iKu[nph]; i++) 
   {
      dTauu[i][nph] = *tup;
      tup++;
   }
   for (i=0; i<iKm[nph]; i++)
   {
      dXc[i]      = *tup;
      dXu[i][nph] = *tup;
      tup++;
   }

/* Fiddle pointers. */
   Fiddle:	
   iMsrc[nph] = ks + 1;
   noend      = FALSE;
   if ( fabs( umin-dPu[k2-2][nph]) <= dtol*umin ) k2--;
   if ( fabs( umin-dPu[k2-1][nph]) <= dtol*umin ) noend = TRUE;
   if ( iMsrc[nph] <= 1 && noext ) iMsrc[nph] = 0;
   k1 = k2 - 1;
   if ( noend ) k1 = k2;
   if ( noext )
   {
/* If there is no correction, copy the depth corrections to working storage. */
      mu = 0;
      for ( k=0; k<k1; k++ )
      {
         dTauc[k] = dTauu[k][nph];
         if ( fabs (dPu[k][nph]-dPux[mu][nph] ) <= dtol )
         {
            dXc[mu] = dXu[mu][nph];
            mu++;
         }
      }
      goto CalcInt;
   }
/* Correct the integrals for the depth interval [dZm[iMsrc],dZs]. */
   ms = iMsrc[nph];
   if ( sgn >= 0. )
   {
      u0 = dPm[ms-1][nph];
      z0 = dZm[ms-1][nph];
      u1 = dUs[nph];
      z1 = dZs;
   }
   else
   {
      u0 = dUs[nph];
      z0 = dZs;
      u1 = dPm[ms-1][nph];
      z1 = dZm[ms-1][nph];
   }
   mu = 0;
   for ( k=0; k<k1; k++ )
   {
      tauint( dPu[k][nph], u0, u1, z0, z1, &ttau, &tx );
      dTauc[k] = dTauu[k][nph] + sgn*ttau;
      if ( fabs( dPu[k][nph]-dPux[mu][nph] ) <= dtol )
      {
         dXc[mu] = dXu[mu][nph] + sgn*tx;
         mu++;
      }
   }

/* Calculate integrals for the ray bottoming at the source depth. */
   CalcInt:
   xus1[nph] = 0.;
   xus2[nph] = 0.;
   mu--;
   if ( fabs( umin-dUs[nph] ) > dtol && fabs( umin-dPux[mu][nph] ) <= dtol ) 
      mu--;
/* This loop may be skipped only for surface focus as range is not
   available for all ray parameters. */
   if ( iMsrc[nph] <= 0 ) goto ConstructTau;
   is          = iIsrc[nph];
   tauus2[nph] = 0.;
   if ( fabs( dPux[mu][nph]-umin ) <= dtol && fabs( dUs[nph]-umin ) <= dtol )
   {      /* If we happen to be right at a discontinuity, range is available. */
      tauus1[nph] = dTauc[k1-1];
      xus1[nph]   = dXc[mu];
      goto TurnInt;
   }
/* Integrate from the surface to the source. */
   tauus1[nph] = 0.;
   j           = 0;
   if ( is >= 1 )
   {
      for ( i=1; i<is+1; i++ )
      {
         tauint( umin, dPm[j][nph], dPm[i][nph], dZm[j][nph], dZm[i][nph],
                 &ttau, &tx );
         tauus1[nph] += ttau;
         xus1[nph]   += tx;
         j = i;
      }
   }
   if ( fabs( dZm[is][nph]-dZs ) > dtol )
   {
/* Unless the source is right on a sample slowness, one more partial
   integral is needed. */
      tauint( umin, dPm[is][nph], dUs[nph], dZm[is][nph], dZs, &ttau, &tx );
      tauus1[nph] += ttau;
      xus1[nph]   += tx;
   }
   TurnInt:
   if ( dPm[is+1][nph] < umin ) goto ConvPhase;
/* If we are in a high slowness zone, we will also need to integrate
   down to the turning point of the shallowest down-going ray. */
   u1 = dUs[nph];
   z1 = dZs;
   for ( i=is+1; i<iMt[nph]; i++ )
   {
      u0 = u1;
      z0 = z1;
      u1 = dPm[i][nph];
      z1 = dZm[i][nph];
      if ( u1 < umin ) break;
      tauint( umin, u0, u1, z0, z1, &ttau, &tx );
      tauus2[nph] += ttau;
      xus2[nph]   += tx;
   }
   z1 = zmod( umin, i-1, nph );
   if ( fabs( z0-z1 ) <= dtol ) goto ConvPhase;
/* Unless the turning point is right on a sample slowness, one more
   partial integral is needed. */
   tauint( umin, u0, umin, z0, z1, &ttau, &tx );
   tauus2[nph] += ttau;
   xus2[nph]   += tx;

/* Take care of converted phases. */
   ConvPhase:
   iph         = (nph+1) % 2;
   xus1[iph]   = 0.;
   xus2[iph]   = 0.;
   tauus1[iph] = 0.;
   tauus2[iph] = 0.;
   if ( nph == 1 )
   {
      if ( umin > dPu[iKu[0]][0] ) goto PastPart;

/* If we are doing an S-wave depth correction, we may need range and
   tau for the P-wave which turns at the S-wave source slowness.  This
   would be needed for sPg and SPg when the source is in the deep mantle. */
      for ( j=0; j<iNBrn; j++ )
      {
         if ( (!strncmp( szPhcd[j], "sP", 2 ) || 
               !strncmp( szPhcd[j], "SP", 2 )) && dPx[j][1] > 0. )
            if ( umin >= dPx[j][0] && umin < dPx[j][1] ) goto Integral;
      }
      goto PastPart;
   }
   else if ( nph == 0 )
   {
/* If we are doing an P-wave depth correction, we may need range and
   tau for the S-wave which turns at the P-wave source slowness.  This
   would be needed for pS and PS. */
      for ( j=0; j<iNBrn; j++ )
      {
         if ( (!strncmp( szPhcd[j], "pS", 2 )  || 
               !strncmp( szPhcd[j], "PS", 2 )) && dPx[j][1] > 0. )
            if ( umin >= dPx[j][0] && umin < dPx[j][1] ) goto Integral;
      }
      goto PastPart;
   }
/* Do the integral. */
   Integral:
   j = 0;
   for ( i=1; i<iMt[iph]; i++ )
   {
      if ( umin >= dPm[i][iph] ) break;
      tauint( umin, dPm[j][iph], dPm[i][iph], dZm[j][iph], dZm[i][iph], &ttau, 
              &tx );
      tauus1[iph] +=  ttau;
      xus1[iph] += tx;
      j = i;
   }
   z1 = zmod( umin, j, iph );
   if ( fabs( dZm[j][iph]-z1) <= dtol ) goto PastPart;
/* Unless the turning point is right on a sample slowness, one more
   partial integral is needed. */
   tauint( umin, dPm[j][iph], umin, dZm[j][iph], z1, &ttau, &tx );
   tauus1[iph] += ttau;
   xus1[iph]   += tx;
    
   PastPart:
   dUa[0][nph] = -1.;
   if ( dODep >= dDepLim ) goto Construct;
   for ( i=0; i<iNSeg; i++ )
      if ( bSegmsk[i] )
         if ( (iNafl[i][0]) == nph+1 && iNafl[i][1] == 0 && iIdx[i] <= 0 ) 
            goto JumpOne;
   goto Construct;

/* If the source is very shallow, we will need to insert some extra
   ray parameter samples into the up-going branches. */
   JumpOne:
   du = min( 1.e-5 + (dODep-.4)*2.e-5, 1.e-5 );
   lp = lpower;
   k  = -1;
   for ( l=iKa; l>=1; l-- ) 
   {
      k++;
      dUa[k][nph] = dUs[nph] - pow( (double) l, (double) lp )*du;
      lp--;
      dTaua[k][nph] = 0.;
      j = 0;
      if ( is >= 1 ) 
      {
         for ( i=1; i<is+1; i++ )
         {
            tauint( dUa[k][nph], dPm[j][nph], dPm[i][nph], dZm[j][nph], 
                    dZm[i][nph], &ttau, &tx );
            dTaua[k][nph] += ttau;
            j = i;
         }
      }
      if ( fabs( dZm[is][nph]-dZs ) > dtol ) 
      {
/* Unless the source is right on a sample slowness, one more partial
   integral is needed. */
         tauint( dUa[k][nph], dPm[is][nph], dUs[nph], dZm[is][nph], dZs,
                 &ttau, &tx );
         dTaua[k][nph] += ttau;
      }
   }
   goto Construct;

/* Construct tau for all branches. */
   ConstructTau:
   mu++;
   Construct:   
   j = 0;
   for ( i=0; i<iNSeg; i++ )
   {
      if ( !bSegmsk[i] ) continue;
      if ( iIdx[i] > 0 || (abs( iNafl[i][0] )-1) != nph || (iMsrc[nph] <= 0 && 
         iNafl[i][0] > 0) ) continue;
      iph = iNafl[i][1] - 1;
      kph = iNafl[i][2] - 1;
/* Handle up-going P and S. */
      if ( iph <= -1 ) iph = nph;
      if ( kph <= -1 ) kph = nph;
      if ( iNafl[i][0] < 0 ) sgn = -1.;
      else                   sgn = 1.;
      i1 = iIndx[i][0];
      i2 = iIndx[i][1];
      m = 0;
      for ( k=i1-1; k<i2; k++ )
      {
         if ( dPt[k] > umin ) goto NextLeg;
         while ( fabs( dPt[k]-dPu[m][nph] ) > dtol ) m++;
         dTau[k][0] = dTaut[k] + sgn*dTauc[m];
      }
      k = i2 - 1;
      goto SkipALeg;
      NextLeg:
      if ( fabs( dPt[k-1]-umin ) <= dtol ) k--;
      iKi++;
      ikk[iKi-1] = k;
      dPk[iKi-1] = dPt[k];
      dPt[k]     = umin;
      fac        = dFcs[i][0];
      dTau[k][0] = fac * (tauus1[iph]+tauus2[iph]+tauus1[kph]+tauus2[kph]) +
                   sgn * tauus1[nph];
      SkipALeg:
      m = 0;
      while ( iJNDex[j][0] < iIndx[i][0] ) j++;
      MidLoop:
      iJNDex[j][1] = min (iJIDex[j], k+1);
      if ( iJNDex[j][0] >= iJNDex[j][1] ) 
      {
         iJNDex[j][1] = -1;
         continue;
      }
      for ( ll=0; ll<2; ll++ )
      {
         while ( fabs( dPux[m][nph]-dPx[j][ll] ) > dtol )
         {
            if ( m >= mu )
            {
               dXBrn[j][ll] = fac * (xus1[iph]+xus2[iph]+xus1[kph]+xus2[kph]) +
                              sgn*xus1[nph];
               goto EndInnerLoop;
            }
            m++;
	 }
         dXBrn[j][ll] = dXt[j][ll] + sgn*dXc[m];
         EndInnerLoop:;
      }
      if ( j+1 < iNBrn ) 
      {
         j++;
         if ( iJNDex[j][0] <= k+1 ) goto MidLoop;
      }
   }
   return;
}

void depset( double dep, double usrc[], FILE *hFileTbl )
{
   bool	  dop, dos;
   int	  i, ind, j, k, nph;            /* Based on 0 */
   int	  in = 0;			/* Based on 1 */
   double rdep;

   if ( max( dep, .011 ) == dODep )
   {
      dop = FALSE;
      dos = FALSE;
      for ( i=0; i<iNSeg; i++ )
      {
         if ( !bSegmsk[i] || iIdx[i] > 0 ) continue;
         if ( abs( iNafl[i][0]) <= 1 ) dop = TRUE;
         if ( abs( iNafl[i][0]) >= 2 ) dos = TRUE;
      }
      if ( !dop && !dos ) return;
      goto CallDepcor;
   }
   iNph0    = -1;
   iInt0[0] = 0;
   iInt0[1] = 0;
   iMbr1    = iNBrn + 1;
   iMbr2    = 0;
   dop      = FALSE;
   dos      = FALSE;
   for ( i=0; i<iNSeg; i++ )
   {
      if ( !bSegmsk[i]) continue;
      if ( abs( iNafl[i][0] ) <= 1 ) dop = TRUE;
      if ( abs( iNafl[i][0] ) >= 2 ) dos = TRUE;
   }
   for ( i=0; i<iNSeg; i++ )
   {
      if ( iNafl[i][1] > 0 || dODep < 0. ) 
      {
         iIdx[i] = -1;
         continue;
      }
      ind = iNafl[i][0] - 1;
      k = -1;
      for ( j=iIndx[i][0]-1; j<iIndx[i][1]; j++ )
      {
         k++;
         dPt[j] = dTp[k][ind];
      }
      iIdx[i] = -1;
   }
   for ( i=0; i<iNBrn; i++ ) iJNDex[i][1] = -1;
   if ( iKi > 0 )
   {
      for ( i=0; i<iKi; i++ )
      {
         j      = ikk[i];
         dPt[j] = dPk[i];
      }
      iKi = 0;
   }
/* Sample the model at the source depth. */
   dODep = max( dep, .011 );
   rdep  = dep;
   if ( rdep < .011 ) rdep = 0.;
   dZs = min( log( max( 1.-rdep*dXn, 1.e-30 ) ), 0. );
   dHn = 1. / (dPn*(1.-rdep*dXn));

   CallDepcor:
   if ( iNph0 <= 0 )
   {
      if ( dop ) depcor( 0, hFileTbl );
      if ( dos ) depcor( 1, hFileTbl );
   }
   else
   {
      if ( dos ) depcor( 1, hFileTbl );
      if ( dop ) depcor( 0, hFileTbl );
   }

/* Interpolate all tau branches. */
   j = 0;
   for ( i=0; i<iNSeg; i++ )
   {
      if ( !bSegmsk[i] ) continue;
      nph = abs( iNafl[i][0] ) - 1;
      if ( iIdx[i] > 0 || (iMsrc[nph] <= 0 && iNafl[i][0] > 0) ) continue;
      iIdx[i] = 1;
      if ( iNafl[i][1] <= 0 ) in = iNafl[i][0];
      if ( iNafl[i][1] > 0 && iNafl[i][1] == abs (iNafl[i][0]) ) 
         in = iNafl[i][1] + 2;
      if ( iNafl[i][1] > 0 && iNafl[i][1] != abs (iNafl[i][0]) )
         in = abs (iNafl[i][0]) + 4;
      if ( iNafl[i][1] > 0 && iNafl[i][1] != iNafl[i][2] ) in = iNafl[i][1]+6;
      while ( iJNDex[j][0] < iIndx[i][0] ) j++;
      iDelSet:
      iDel[j][2] = iNafl[i][0];
      spfit( j, in );
      iMbr1 = min( iMbr1, j+1 );
      iMbr2 = max( iMbr2, j+1 );
      if ( j+1 >= iNBrn ) continue;
      j++;
      if ( iJIDex[j] <= iIndx[i][1] && iJNDex[j][1] > 0 ) goto iDelSet;
   }
   usrc[0] = dUs[0] / dPn;
   usrc[1] = dUs[1] / dPn;
   return;
}

void findtt( int jb, double x0[], int *n, double tt[], double dtdd[], 
             double dtdh[], double dddp[], char phnm[60][PHASE_LENGTH] )
{
   int     i, in, ij, is, ie, jj, j, ln, nph;	/* Based on 0 */
   int     le;		       		 	/* Based on 1 */
   double  x, p0, p1, arg, dp, dps = 0.0, delp, tol=3.e-6, ps, deps=1.e-10,
           hsgn, dp0, temp, dsgn, dpn;

   nph = abs (iDel[jb][2]) - 1;
   if ( iDel[jb][2] < 0 ) hsgn = -1 * dHn;
   else                   hsgn = dHn;
   dsgn = pow( (-1.), (double) iDel[jb][0] ) * dDn;
   dpn = -1. / dTn;
   for ( ij=iDel[jb][0]-1; ij<iDel[jb][1]; ij++ )
   {
      x = x0[ij];
      dsgn *= -1.;
      if ( x < dXBrn[jb][0] || x > dXBrn[jb][1] ) goto SkipABit;
      j = iJNDex[jb][0] - 1;
      is = j + 1;
      ie = iJNDex[jb][1] - 1;
      for ( i=is; i<ie+1; i++ )
      {
         if ( x <= dXLim[0][j] || x > dXLim[1][j] ) goto InnerLoopEnd;
         le   = *n;
         p0   = dPt[ie]-dPt[j];
         p1   = dPt[ie]-dPt[i];
         delp = max (tol * (dPt[i]-dPt[j]), 1.e-3);
         if ( fabs( dTau[j][2] ) <= 1.e-30 )
         {
            dps = (x-dTau[j][1]) / (1.5*dTau[j][3]);
	    if (dps < 0.) dp = dps*dps*(-1.);
            else        dp = dps*dps;
            dp0 = dp;
            if ( dp < p1-delp || dp > p0+delp ) goto CheckHere;
            if ( *n >= MAX_PHASES ) goto SayWarning;
            *n = *n + 1;
            ps = dPt[ie] - dp;
            tt[*n-1] = dTn * (dTau[j][0]+dp*(dTau[j][1]+dps*dTau[j][3])+ps*x);
            dtdd[*n-1] = dsgn * ps;
            dtdh[*n-1] = hsgn * sqrt( fabs( dUs[nph]*dUs[nph]-ps*ps ) );
            dddp[*n-1] = dpn  * .75 * dTau[j][3] / max( fabs( dps ), deps );
            strcpy( phnm[*n-1], szPhcd[jb] );
            in = (int)(strstr( phnm[*n-1], "ab" ) - phnm[*n-1]);
            if ( in <= 0 ) goto InnerLoopEnd;
            if ( ps <= dXBrn[jb][2] ) 
            {
               phnm[*n-1][in]   = 'b';
               phnm[*n-1][in+1] = 'c';
            }
            goto InnerLoopEnd;
         }
         for ( jj=0; jj<2; jj++ )
         {
            if ( jj == 0 )
            {
               arg = 9.*dTau[j][3]*dTau[j][3] + 32.*dTau[j][2]*(x-dTau[j][1]);
               if ( dTau[j][3] < 0. ) temp = sqrt( fabs( arg ) ) * (-1.);
               else                   temp = sqrt( fabs( arg ) ); 
               dps = -(3.*dTau[j][3]+temp) / (8.*dTau[j][2]);
               if ( dps < 0. ) dp = dps * dps * (-1.);
               else            dp = dps * dps;
               dp0 = dp;
            }
            else
            {
               dps = (dTau[j][1]-x) / (2.*dTau[j][2]*dps);
               if ( dps < 0. ) dp = dps * dps * (-1.);
               else            dp = dps * dps;
            }
            if ( dp < p1-delp || dp > p0+delp ) continue;
            if ( *n >= MAX_PHASES ) goto SayWarning;
            *n = *n + 1;
            ps = dPt[ie] - dp;
            tt[*n-1] = dTn * (dTau[j][0]+dp*(dTau[j][1]+dp*dTau[j][2]+
                       dps*dTau[j][3])+ps*x);
            dtdd[*n-1] = dsgn * ps;
            dtdh[*n-1] = hsgn * sqrt( fabs( dUs[nph]*dUs[nph]-ps*ps ) );
            dddp[*n-1] = dpn * (2.*dTau[j][2]+.75*dTau[j][3]/
                         max( fabs( dps ), deps ));
            strcpy( phnm[*n-1], szPhcd[jb] );
            in = (int)(strstr( phnm[*n-1], "ab" ) - phnm[*n-1]);
            if ( in <= 0 ) continue;
            if ( ps <= dXBrn[jb][2] ) 
            {
               phnm[*n-1][in] = 'b';
               phnm[*n-1][in+1] = 'c';
            }
         }
         CheckHere:
         if ( *n > le ) goto InnerLoopEnd;
         if ( DEBUGL ) logit( "", "Failed to find phase:  %s %lf %lf %lf %lf %lf\n",
                        szPhcd[jb], x, dp0, dp, p1, p0 );
         InnerLoopEnd:
         j = i;
      }
SkipABit:
      if ( x < dDBrn[jb][0] || x > dDBrn[jb][1] ) continue;
      if ( *n >= MAX_PHASES ) goto SayWarning;
      j   = iJNDex[jb][0] - 1;
      i   = iJNDex[jb][1] - 1;
      dp  = dPt[i] - dPt[j];
      dps = sqrt( fabs( dp ) );
      *n = *n + 1;
      tt[*n-1] = dTn * (dTau[j][0]+dp*(dTau[j][1]+dp*dTau[j][2]+dps*dTau[j][3])+
                 dPt[j]*x);
      dtdd[*n-1] = dsgn * dPt[j];
      dtdh[*n-1] = hsgn * sqrt( fabs( dUs[nph]*dUs[nph]-dPt[j]*dPt[j] ) );
      dddp[*n-1] = dpn * (2.*dTau[j][2]+.75*dTau[j][3]/max( dps,deps ));
      ln = (int)(strchr( szPhcd[jb], ' ' ) - szPhcd[jb] - 1);
      if ( ln <= -1 ) ln = (int)strlen( szPhcd[jb] );
      for ( i=0; i<=ln; i++ ) phnm[*n-1][i] = szPhcd[jb][i];
      phnm[*n-1][ln+1] = '\0';
      strcat( phnm[*n-1], "diff" );
   }
   return;
   SayWarning:
   if ( DEBUGL ) logit( "", "More than %d arrivals found.\n", MAX_PHASES );
   return;
}

/*
 $$$$$ calls only library routines $$$$$
   Given ray parameter grid p;i (p sub i), i=1,2,...,n, corresponding
   tau;i values, and x;1 and x;n (x;i = -dtau/dp|p;i); tauspl finds
   interpolation I such that:  tau(p) = a;1,i + Dp * a;2,i + Dp**2 *
   a;3,i + Dp**(3/2) * a;4,i where Dp = p;n - p and p;i <= p < p;i+1.
   Interpolation I has the following properties:  1) x;1, x;n, and
   tau;i, i=1,2,...,n are fit exactly, 2) the first and second
   derivitives with respect to p are continuous everywhere, and
   3) because of the paramaterization d**2 tau/dp**2|p;n is infinite.
   Thus, interpolation I models the asymptotic behavior of tau(p)
   when tau(p;n) is a branch end due to a discontinuity in the
   velocity model.  Note that array a must be dimensioned at least
   a(4,n) though the interpolation coefficients will be returned in
   the first n-1 columns.  The remaining column is used as scratch
   space and returned as all zeros.  Programmed on 16 August 1982 by
   R. Buland.
*/
void fitspl( int i1, int i2, double tau2[JOUT][4], double x1, double xn2, 
             double coef2[5][JOUT] )
{
   double  a[2][60], ap[3], b[60], alr, g1, gn;
   int     i, ie, is, j, n, n1;

   if      ( i2-i1 <  0 ) return;
   else if ( i2-i1 == 0 ) 
   {
      tau2[i1-1][1] = x1;
      return;
   }
   n = 0;    
   for ( i=i1-1; i<i2; i++ )
   {
      n++;
      b[n-1] = tau2[i][0];
      for ( j=0; j<2; j++ )
         a[j][n-1] = coef2[j][i];
   }
   for ( j=0; j<3; j++ ) ap[j] = coef2[j+2][i2-1];
   n1 = n - 1;

/*   Arrays ap(*,1), a, and ap(*,2) comprise n+2 x n+2 penta-diagonal
     matrix A.  Let x1, tau2, and xn2 comprise corresponding n+2 vector b.
     Then, A * g = b, may be solved for n+2 vector g such that
     interpolation I is given by I(p) = sum(i=0,n+1) g;i * G;i(p).

     Eliminate the lower triangular portion of A to form A'.  A
     corresponding transformation applied to vector b is stored in
     a(4,*).   */

   alr = a[0][0] / coef2[2][i1-1];
   a[0][0] = 1. - coef2[3][i1-1]*alr;
   a[1][0] = a[1][0] - coef2[4][i1-1]*alr;
   b[0] = b[0] - x1*alr;
   j = 0;
   for ( i=1; i<n; i++ )
   {
      alr = a[0][i] / a[0][j];
      a[0][i] = 1. - a[1][j]*alr;
      b[i] = b[i] - b[j]*alr;
      j = i;
   }
   alr = ap[0] / a[0][n1-1];
   ap[1] = ap[1] - a[1][n1-1]*alr;
   gn = xn2 - b[n1-1]*alr;
   alr = ap[1] / a[0][n-1];
/* Back solve the upper triangular portion of A' for coefficients g;i.
   When finished, storage g(2), a(4,*), g(5) will comprise vector g. */
   gn = (gn - b[n-1]*alr) / (ap[2] - a[1][n-1]*alr);
   b[n-1] = (b[n-1] - gn*a[1][n-1]) / a[0][n-1];
   j = n-1;
   for ( i=n1-1; i>=0; i-- )
   {
      b[i] = (b[i] - b[j]*a[1][i]) / a[0][i];
      j = i;
   }
   g1 = (x1 - coef2[3][i1-1]*b[0] - coef2[4][i1-1]*b[1]) / coef2[2][i1-1];

   tau2[i1-1][1] = x1;
   is = i1 + 1;
   ie = i2-1;
   j = 0;
   for ( i=is-1; i<ie; i++ )
   {
      j++;
      tau2[i][1] = coef2[2][i]*b[j-1] + coef2[3][i]*b[j] + coef2[4][i]*b[j+1];
   }
   tau2[i2-1][1] = xn2;
   return;
}

void pdecu( int i1, int *i2, double x0, double x1, double xmin, int in ) 
{
   double dx, dx2, sgn, rnd, xm, axm, x, h1, h2, hh, xs;
   int    i, j, k, m;		/* Based on 0 */
   int    is, n, ie;		/* Based on 1 */

   if ( DEBUGL ) logit( "", "Pdecu:  ua = %lf\n", dUa[0][in] );
   if ( dUa[0][in] > 0. )
   {
      if ( DEBUGL ) logit( "", "Pdecu:  fill in new grid\n" );
      k = i1;
      for ( i=0; i<iKa; i++ )
      {
         dPt[k]     = dUa[i][in];
         dTau[k][0] = dTaua[i][in];
         k++;
      }
      dPt[k] = dPt[*i2-1];
      dTau[k][0] = dTau[*i2-1][0];
      *i2 = k + 1;
      if ( DEBUGL )
      {
         logit( "", " %d %lf\n", i, dPt[i] );
         for ( i=i1-1; i<*i2; i++ ) logit( "", "%lf ", dTau[i][0] );
         logit( "", "\n" );
      }
      return;
   }
   is = i1 + 1;
   ie = *i2 - 1;
   xs = x1;
   for ( i=ie-1; i>=i1-1; i-- )
   {
      x = xs;
      if ( i+1 == i1 ) xs = x0;
      else
      {
         h1 = dPt[i-1] - dPt[i];
         h2 = dPt[i+1] - dPt[i];
         hh = h1 * h2 * (h1-h2);
         h1 = h1 * h1;
         h2 = -h2 * h2;
         xs = -(h2*dTau[i-1][0]-(h2+h1)*dTau[i][0]+h1*dTau[i+1][0]) / hh;
      }
      if ( fabs( x-xs ) <= xmin ) goto PastReturn;
   }
   return;
   PastReturn:
   ie = i + 1;
   if ( fabs( x-xs ) <= .75*xmin && ie != *i2 )
   {
      xs = x;
      ie++;
   }
   n   = max( (int) ( fabs( xs-x0 )/xmin + .8 ), 1 );
   dx  = (xs-x0) / (double) n;
   dx2 = fabs (.5*dx);
   if ( dx >= 0 ) sgn = 1.;
   else           sgn = -1;
   rnd = 0.;
   if ( sgn > 0. ) rnd = 1.;
   xm  = x0 + dx;
   k   = i1 - 1;
   m   = is - 1;
   axm = 1.e10;
   for ( i=is-1; i<ie; i++ )
   {
      if (i+1 >= ie) x = xs;
      else
      {
         h1 = dPt[i-1] - dPt[i];
         h2 = dPt[i+1] - dPt[i];
         hh = h1 * h2 * (h1-h2);
         h1 = h1 * h1;
         h2 = -h2 * h2;
         x  = -(h2*dTau[i-1][0]-(h2+h1)*dTau[i][0]+h1*dTau[i+1][0]) / hh;
      }
      if ( sgn*(x-xm) > dx2 )
      {
         if ( k >= m )
            for (j=m; j<k+1; j++) dPt[j] = -1.;
         m   = k + 2;
         k   = i - 1;
         axm = 1.e10;
         xm  = xm + dx*(double) ((int) ((x-xm-dx2)/dx+rnd));
      }
      if ( fabs( x-xm ) >= axm ) continue;
      axm = fabs( x-xm );
      k = i - 1;
   }
   if ( k >= m )
      for ( j=m; j<k+1; j++ ) dPt[j] = -1.;
   k = i1 - 1;
   for ( i=is-1; i<*i2; i++ )
   {
      if ( dPt[i] < 0. ) continue;
      k++;
      dPt[k]     = dPt[i];
      dTau[k][0] = dTau[i][0];
   }
   *i2 = k + 1;
   if ( DEBUGL )
   {
      logit( "", " %d %lf\n", i, dPt[i] );
      for ( i=i1-1; i<*i2; i++ ) logit( "", "%lf ", dTau[i][0] );
      logit( "", "\n" );
   }
   return;
}

/*
 $$$$$ calls no other routine $$$$$

   R4sort sorts the n elements of array rkey so that rkey[i],
   i = 1, 2, 3, ..., n are in asending order.  R4sort is a trivial
   modification of ACM algorithm 347:  "An efficient algorithm for
   sorting with minimal storage" by R. C. Singleton.  Array rkey is
   sorted in place in order n*alog2(n) operations.  Coded on
   8 March 1979 by R. Buland.  Modified to handle real*4 data on
   27 September 1983 by R. Buland.
*/
void r4sort( int n, double rkey[], int iptr[] )
{
   int     i, ij, it, ib, j, k, kkk, l, m;   /* Indices based on 0 */
   int     il[10], iu[10];                   /* "      "       " */
   double  tmpkey, r;

/* Note:  il and iu implement a stack containing the upper and
   lower limits of subsequences to be sorted independently.  A
   depth of k allows for n<=2**(k+1)-1. */
   if ( n <= 0 ) return;
   for ( i=0; i<n; i++ ) iptr[i] = i;
   if ( n <= 1 ) return;
   r = 0.375;
   m = 0;
   i = 0;
   j = n - 1;

/* The first section interchanges low element i, middle element ij,
   and high element j so they are in order. */
   Top:
   if ( i >= j ) goto Fourth;
   AlmostTop:
   k = i;

/* Use a floating point modification, r, of Singleton's bisection
   strategy (suggested by R. Peto in his verification of the
   algorithm for the ACM). */
   if ( r > 0.58984375 ) r -= 0.21875;
   else                  r += 0.0390625;
   ij = (int) ((double) i + (double) (j-i) * r);
   if ( rkey[iptr[i]] > rkey[iptr[ij]] ) 
   {
      it = iptr[ij];
      iptr[ij] = iptr[i];
      iptr[i] = it;
   }
   l = j;
   if ( rkey[iptr[j]] < rkey[iptr[ij]] )
   {
      it = iptr[ij];
      iptr[ij] = iptr[j];
      iptr[j] = it;
      if ( rkey[iptr[i]] > rkey[iptr[ij]] ) 
      {
         it = iptr[ij];
         iptr[ij] = iptr[i];
         iptr[i] = it;
      }
   }
   tmpkey = rkey[iptr[ij]];
   goto SkipALittle;

/* The second section continues this process.  K counts up from i and
   l down from j.  Each time the k element is bigger than the ij
   and the l element is less than the ij, then interchange the
   k and l elements.  This continues until k and l meet. */
   Second:
   it      = iptr[l];
   iptr[l] = iptr[k];
   iptr[k] = it;
   SkipALittle:
   l--;
   if ( rkey[iptr[l]] > tmpkey ) goto SkipALittle;
   BackUp:
   k++;
   if ( rkey[iptr[k]] < tmpkey ) goto BackUp;
   if ( k <= l ) goto Second;

/* The third section considers the intervals i to l and k to j.  The
   larger interval is saved on the stack (il and iu) and the smaller
   is remapped into i and j for another shot at section one.  */
   if ( l-i > j-k ) 
   {
      il[m] = i;
      iu[m] = l;
      i = k;
      m++;
      goto MidFourth;
   }
   il[m] = k;
   iu[m] = j;
   j     = l;
   m++;
   goto MidFourth;

/* The fourth section pops elements off the stack (into i and j).  If
   necessary control is transfered back to section one for more
   interchange sorting.  If not we fall through to section five.  Note
   that the algorighm exits when the stack is empty. */
   Fourth:
   m--;
   if ( m < 0 ) return;
   i = il[m];
   j = iu[m];
   MidFourth:
   if ( j-i >= 11 ) goto AlmostTop;
   if ( i == 0 ) goto Top;
   i--;

/* The fifth section is the end game.  Final sorting is accomplished
   (within each subsequence popped off the stack) by rippling out
   of order elements down to their proper positions. */
   Fifth:
   i++;
   if ( i == j ) goto Fourth;
   if ( rkey[iptr[i]] <= rkey[iptr[i+1]] ) goto Fifth;
   k   = i;
   kkk = k + 1;
   ib  = iptr[kkk];
   MidFifth:
   iptr[kkk] = iptr[k];
   kkk = k;
   k--;
   if ( rkey[ib] < rkey[iptr[k]] ) goto MidFifth;
   iptr[kkk] = ib;
   goto Fifth;
}

void spfit( int jb, int in )
{
   double  dmn, pmn, dmx, hm, shm, thm, p0, p1, tau0, tau1, x0, x1, pe;
   double  pe0, spe0, scpe0, pe1, spe1, scpe1, dpe, dtau;
   double  dbrnch = 2.5307274;
   double  xmin = 3.92403e-3;
   double  dtol = 1.e-6;
   double  ptol = 2.e-6;
   char    szDisc[4];
   bool    newgrd, makgrd;
   int     i, j, k;                         /* Based on 0 */
   int     i1, i2, nn, is, mxcnt, mncnt;    /* Based on 1 */

   i1 = iJNDex[jb][0];
   i2 = iJNDex[jb][1];
   if ( i2-i1 <= 1 && fabs( dPt[i2-1]-dPt[i1-1] ) <= ptol ) 
   {
      iJNDex[jb][1] = -1;
      return;
   }
   newgrd = FALSE;
   makgrd = FALSE;
   if ( fabs( dPx[jb][1]-dPt[i2-1] ) > dtol ) newgrd = TRUE;
   if ( !newgrd ) 
      fitspl( i1, i2, dTau, dXBrn[jb][0], dXBrn[jb][1], dCoef );
   else
   {
      k = (in-1) % 2;
      if ( in != iInt0[k] ) makgrd = TRUE;
      if ( in <= 2 )
      {
         xmin = dXn * min( max( 2.*dODep, 2. ), 25. );
         pdecu( i1, &i2, dXBrn[jb][0], dXBrn[jb][1], xmin, in-1 );
         iJNDex[jb][1] = i2;
      }
      nn = i2 - i1 + 1;
      if ( makgrd ) tauspl( 1, nn, &dPt[i1-1], &dTCoef[k][0] );
      fitspl( 1, nn, &dTau[i1-1], dXBrn[jb][0], dXBrn[jb][1], &dTCoef[k][0] );
      iInt0[k] = in;
   }
   pmn   = dPt[i1-1];
   dmn   = dXBrn[jb][0];
   dmx   = dmn;
   mxcnt = 0;
   mncnt = 0;
   pe    = dPt[i2-1];
   p1    = dPt[i1-1];
   tau1  = dTau[i1-1][0];
   x1    = dTau[i1-1][1];
   pe1   = pe - p1;
   spe1  = sqrt (fabs (pe1));
   scpe1 = pe1 * spe1;
   j     = i1 - 1;
   is    = i1 + 1;
   for ( i=is-1; i<i2; i++ )
   {
      p0    = p1;
      p1    = dPt[i];
      tau0  = tau1;
      tau1  = dTau[i][0];
      x0    = x1;
      x1    = dTau[i][1];
      dpe   = p0 - p1;
      dtau  = tau1 - tau0;
      pe0   = pe1;
      pe1   = pe - p1;
      spe0  = spe1;
      spe1  = sqrt (fabs (pe1));
      scpe0 = scpe1;
      scpe1 = pe1 * spe1;
      dTau[j][3] = (2.*dtau-dpe*(x1+x0)) / (.5*(scpe1-scpe0)-1.5*spe1*spe0*
                   (spe1-spe0));
      dTau[j][2] = (dtau-dpe*x0-(scpe1+.5*scpe0-1.5*pe1*spe0)*dTau[j][3]) /
                   (dpe*dpe);
      dTau[j][1] = (dtau-(pe1*pe1-pe0*pe0)*dTau[j][2]-(scpe1-scpe0)*dTau[j][3])/
                   dpe;
      dTau[j][0] = tau0 - scpe0*dTau[j][3] - pe0*(pe0*dTau[j][2]+dTau[j][1]);
      dXLim[0][j] = min( x0, x1 );
      dXLim[1][j] = max( x0, x1 );
      if ( dXLim[0][j] < dmn )
      {
         dmn = dXLim[0][j];
         pmn = dPt[j];
         if (x1 < x0) pmn = dPt[i];
      }
      strcpy( szDisc, "   " );
      if ( fabs( dTau[j][2] ) <= 1.e-30 ) goto DownThere;
      shm = -.375 * dTau[j][3] / dTau[j][2];
      hm = shm * shm;
      if ( shm <= 0. || (hm <= pe1 || hm >= pe0) ) goto DownThere;
      thm = dTau[j][1] + shm*(2.*shm*dTau[j][2]+1.5*dTau[j][3]);
      dXLim[0][j] = min( dXLim[0][j], thm );
      dXLim[1][j] = max( dXLim[1][j], thm );
      if ( thm < dmn ) 
      {
         dmn = thm;
         pmn = pe - hm;
      }
      strcpy( szDisc, "max" );
      if ( dTau[j][3] < 0. ) strcpy( szDisc, "min" );
      if ( !strcmp( szDisc, "max" ) ) mxcnt++;
      if ( !strcmp( szDisc, "min" ) ) mncnt++;
      DownThere:
      dmx = max (dmx, dXLim[1][j]);
      j = i;
   }
   dXBrn[jb][0] = dmn;
   dXBrn[jb][1] = dmx;
   dXBrn[jb][2] = pmn;
   iDel[jb][0]  = 1;
   iDel[jb][1]  = 1;
   if ( dXBrn[jb][0] > PI )    iDel[jb][0] = 2;
   if ( dXBrn[jb][1] > PI )    iDel[jb][1] = 2;
   if ( dXBrn[jb][0] > TWOPI ) iDel[jb][0] = 3;
   if ( dXBrn[jb][1] > TWOPI ) iDel[jb][1] = 3;
   if ( in <= 2 ) 
   {
      szPhcd[jb][1] = '\0';
      i = jb;
      for ( j=0; j<iNBrn; j++ )
      {
         i = ((i+1) % iNBrn);
         if ( szPhcd[i][0] == szPhcd[jb][0] && szPhcd[jb][1] == '\0' && 
              szPhcd[i][1] != 'P' && (pe >= dPx[i][0] && pe <= dPx[i][1]) ) 
         {
            strcpy( szPhcd[jb], szPhcd[i] );
            if ( fabs( dPt[i2-1]-dPt[iJNDex[i][0]-1] ) <= dtol ) 
               strcpy( szPhcd[jb], szPhcd[i-1] );
            break;
         }
      }
   }
   if ( dDBrn[jb][0] > 0. )
   {
      dDBrn[jb][0] = dmx;
      dDBrn[jb][1] = dbrnch;
   }
   if ( mxcnt > mncnt || mncnt > mxcnt+1 ) 
      logit( "t", "Bad interpolation on %s\n", szPhcd[jb] );
   return;
}

int tabin( FILE * *hFileTbl, char *modnam )
{
   FILE   *hFileHed;
   int	  i, j, k, nph, ind, l;       /* Based on 0 */
   int    nl, len2;
   static char   phdif[6][PHASE_LENGTH];
   char   szFile[MAX_FILE_SIZE];

   strcpy( phdif[0], "P"  );
   strcpy( phdif[1], "S"  );
   strcpy( phdif[2], "pP" );
   strcpy( phdif[3], "sP" );
   strcpy( phdif[4], "pS" );
   strcpy( phdif[5], "sS" );

/* Create model header file name */
   strcpy( szFile, modnam );
   strcat( szFile, ".hed" );
/* Read in model header file */
   if ( (hFileHed = fopen( szFile, "rb" )) == NULL )
   {
      logit( "", "Header file (%s) not found in tabin\n", szFile );
      return( 0 );
   }
   fread( &iRecLen, sizeof (int),    1, hFileHed );
   fread( &nl,      sizeof (int),    1, hFileHed );
   fread( &len2,    sizeof (int),    1, hFileHed );
   fread( &dXn,     sizeof (double), 1, hFileHed );
   fread( &dPn,     sizeof (double), 1, hFileHed );
   fread( &dTn,     sizeof (double), 1, hFileHed );
   fread( &iMt[0],  sizeof (int),    1, hFileHed );
   fread( &iMt[1],  sizeof (int),    1, hFileHed );
   fread( &iNSeg,   sizeof (int),    1, hFileHed );
   fread( &iNBrn,   sizeof (int),    1, hFileHed );
   fread( &iKu[0],  sizeof (int),    1, hFileHed );
   fread( &iKu[1],  sizeof (int),    1, hFileHed );
   fread( &iKm[0],  sizeof (int),    1, hFileHed );
   fread( &iKm[1],  sizeof (int),    1, hFileHed );
   for ( j=0; j<3; j++ )
      for ( i=0; i<JSEG; i++ )
         fread( &dFcs[i][j],   sizeof (double), 1, hFileHed );
   for ( j=0; j<3; j++ )
      for ( i=0; i<JSEG; i++ )
         fread( &iNafl[i][j],  sizeof (int),    1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JSEG; i++ )
         fread( &iIndx[i][j],  sizeof (int),    1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JSEG; i++ )
         fread( &iKndx[i][j],  sizeof (int),    1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JSRC; i++ )
         fread( &dPm[i][j],    sizeof (double), 1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JSRC; i++ )
         fread( &dZm[i][j],    sizeof (double), 1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JSRC; i++ )
         fread( &iNDex[i][j],  sizeof (int),    1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JTSM0; i++ )
         fread( &dPu[i][j],    sizeof (double), 1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JXSM; i++ )
         fread( &dPux[i][j],   sizeof (double), 1, hFileHed );
   for ( i=0; i<JBRN; i++ )
      fread( szPhcd[i], PHASE_LENGTH, 1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JBRN; i++ )
         fread( &dPx[i][j],    sizeof (double), 1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JBRN; i++ )
         fread( &dXt[i][j],    sizeof (double), 1, hFileHed );
   for ( j=0; j<2; j++ )
      for ( i=0; i<JBRN; i++ )
         fread( &iJNDex[i][j], sizeof (int),    1, hFileHed );
   for ( i=0; i<JOUT; i++ )
      fread( &dPt[i],   sizeof (double), 1, hFileHed );
   for ( i=0; i<JOUT; i++ )
      fread( &dTaut[i], sizeof (double), 1, hFileHed );
   for ( j=0; j<JOUT; j++ )
      for ( i=0; i<5; i++ )
         fread( &dCoef[i][j],  sizeof (double), 1, hFileHed );
   fclose( hFileHed );

/* Create model table file name */
   strcpy( szFile, modnam );
   strcat( szFile, ".tbl" );
/* Get ready to read in model table file */
   if ( (*hFileTbl = fopen( szFile, "rb" )) == NULL )
   {
      logit( "", "Table file (%s) not found in tabin\n", szFile );
      return( 0 );
   }
   for ( nph=0; nph<2; nph++ ) dPu[iKu[nph]][nph] = dPm[0][nph];

   if ( DEBUGL ) 
   {
      logit( "", "nasgr nl len2 %d %d %d\n", iRecLen, nl, len2 );
      logit( "", "iNSeg iNBrn iMt ku km %d %d %d %d %d %d %d %d\n",
               iNSeg, iNBrn, iMt[0], iMt[1], iKu[0], iKu[1], iKm[0], iKm[1] );
      logit( "", "%lf %lf %lf\n", dXn, dPn, dTn );
      for ( i=0; i<iMt[1]; i++ )
         logit( "", "%d %d %lf %lf %d %lf %lf\n", i+1, iNDex[i][0],
                dPm[i][0], dZm[i][0], iNDex[i][1], dPm[i][1], dZm[i][1] );
      for ( i=0; i<iKu[1]+1; i++ )
         logit( "", "%d %lf %lf\n", i+1, dPu[i][0], dPu[i][1] );
      for ( i=0; i<iKm[1]; i++ )
         logit( "", "%d %lf %lf\n", i+1, dPux[i][0], dPux[i][1] );
      for ( i=0; i<iNSeg; i++ )
         logit( "", "%d %5d %5d %5d %5d %5d %5d %5d %lf %lf %lf\n",
                i+1, iNafl[i][0], iNafl[i][1], iNafl[i][2], iIndx[i][0], 
                iIndx[i][1], iKndx[i][0], iKndx[i][1], dFcs[i][0], dFcs[i][1], 
                dFcs[i][2] );
      for ( i=0; i<iNBrn; i++ )
         logit( "", "%d %d %d %lf %lf %lf %lf %s\n", i+1, iJNDex[i][0],
                iJNDex[i][1], dPx[i][0], dPx[i][1], DEG*dXt[i][0], 
                DEG*dXt[i][1], szPhcd[i] );
      for ( i=0; i<JOUT; i++ )
         logit( "", "%d %lf %lf %lf %lf %lf %lf %lf\n", i+1, dPt[i],
                dTaut[i],dCoef[0][i],dCoef[1][i], dCoef[2][i], dCoef[3][i], 
                dCoef[4][i] );
   }
   dTn      = 1. / dTn;
   dDn      = PI / (180.*dPn*dXn);
   dODep    = -1.;
   iKi      = 0;
   iMsrc[0] = 0;
   iMsrc[1] = 0;
   k        = 0;

   for ( i=0; i<iNBrn; i++ )
   {
      iJIDex[i] = iJNDex[i][1];
      for ( j=0; j<2; j++ ) dDBrn[i][j] = -1.;
      while ( iJNDex[i][1] > iIndx[k][1] ) k++;
      if ( iNafl[k][1] <= 0 )
      {
         ind = iNafl[k][0];
         l   = -1;
         for ( j=iJNDex[i][0]-1; j<iJNDex[i][1]; j++ )
         {
            l++;
            dTp[l][ind-1] = dPt[j];
         }
      }
      if ( iNafl[k][0] > 0 && (szPhcd[i][0] == 'P' || szPhcd[i][0] == 'S') ) 
         continue;
      for ( j=0; j<6; j++ )
         if ( !strcmp( szPhcd[i], phdif[j] ) )
	 {
            dDBrn[i][0] = 1.;
	    strcpy( phdif[j], "     " );
	    break;
	 }
   }
   if ( DEBUGL ) 
   {
      for ( i=0; i<iNBrn; i++ )
         logit( "", "%d %s %lf %lf %d\n", i+1, szPhcd[i], dDBrn[i][0],
                  dDBrn[i][1], iJIDex[i] );
      for ( i=0; i<JBRNU; i++ )
         logit( "", "%d %lf %lf\n", i+1, dTp[i][0], dTp[i][1] );
   }
   return 1;
}

/*
 $$$$$ calls warn $$$$$

   Tauint evaluates the intercept (tau2) and distance (x) integrals  for
   the spherical earth assuming that slowness is linear between radii
   for which the model is known.  The partial integrals are performed
   for ray slowness ptk between model radii with slownesses ptj and pti
   with equivalent flat earth depths zj and zi respectively.  The partial
   integrals are returned in tau2 and x.  Note that ptk, ptj, pti, zj, zi,
   tau2, and x are all double precision.
*/
void tauint( double ptk, double ptj, double pti, double zj, double zi, 
             double *tau2, double *x )
{
   char	   msg[72];
   double  xx, b, sqk, sqi, sqj, sqb, tmp;

   if ( fabs( zj -zi  ) <= 1.e-9 ) goto FunctionEnd;
   if ( fabs( ptj-pti ) >  1.e-9 )
   {
      if ( ptk > 1.e-9 || pti > 1.e-9 ) goto Compute;
/* Handle the straight through ray. */
      *tau2 = ptj;
      *x = 1.5707963267948966e0;
      goto ErrorHnd;
   }
   if ( fabs( ptk-pti ) <= 1.e-9 ) goto FunctionEnd;
   b   = fabs( zj-zi );
   sqj = sqrt( fabs( ptj*ptj - ptk*ptk ) );
   *tau2 = b*sqj;
   *x    = b*ptk/sqj;
   goto ErrorHnd;
   Compute:
   b = ptj - (pti-ptj) / (exp( zi-zj )-1.e0);
   if ( ptk <= 1.e-9 )
   {
      *tau2 = -(pti - ptj + b * log( pti/ptj ) - b * log 
              ( max( (ptj-b)*pti/((pti-b)*ptj),1.e-30 ) ));
      *x = 0.e0;
      goto ErrorHnd;
   }
   if ( ptk == pti ) goto FirstCheck;
   if ( ptk == ptj ) goto SecondCheck;
   sqk = ptk*ptk;
   sqi = sqrt( fabs( pti*pti-sqk ) );
   sqj = sqrt( fabs( ptj*ptj-sqk ) );
   sqb = sqrt( fabs( b*b-sqk ) );
   if ( sqb <= 1.e-30 ) 
   {
      xx = 0.e0;
      *x = ptk * (sqrt (fabs ((pti+b) / (pti-b))) - sqrt (fabs ((ptj+b) /
           (ptj-b)))) / b;
   }
   else
   {
      if ( b*b < sqk )
      {
         xx = asin( max( min( (b*pti - sqk) / (ptk*fabs (pti-b)), 1.e0 ), -1.e0 ) )-
              asin( max( min( (b*ptj - sqk) / (ptk*fabs (ptj-b)), 1.e0 ), -1.e0 ) );
         *x = -ptk*xx/sqb;
      }
      else
      {
         xx = log( max( (ptj-b) * (sqb*sqi + b*pti - sqk) / ((pti-b) *
              (sqb*sqj + b*ptj - sqk)), 1.e-30 ) );
         *x = ptk*xx/sqb;
      }
   }
   *tau2 = -(sqi - sqj + b*log ((pti+sqi) / (ptj+sqj)) - sqb*xx);
   goto ErrorHnd;
   FirstCheck:
   sqk = pti*pti;
   sqj = sqrt( fabs( ptj*ptj - sqk ) );
   sqb = sqrt( fabs( b*b - sqk ) );
   if ( b*b < sqk )
   {
      if ( b-pti < 0. ) tmp = -1.5707963267948966e0;
      else              tmp = 1.5707963267948966e0;
      xx = tmp - asin( max( min( (b*ptj - sqk) / (pti*fabs (ptj-b)), 1.e0 ), 
           -1.e0 ) );
      *x = -pti*xx/sqb;
   }
   else
   {
      xx = log( max( (ptj-b) * (b*pti - sqk) / ((pti-b) * (sqb*sqj + b*ptj-sqk)),
           1.e-30 ) );
      *x = pti*xx/sqb;
   }
   *tau2 = -(b*log (pti / (ptj+sqj)) - sqj - sqb*xx);
   goto ErrorHnd;
   SecondCheck:
   sqk = ptj*ptj;
   sqi = sqrt( fabs( pti*pti - sqk ) );
   sqb = sqrt( fabs( b*b - sqk ) );
   if ( b*b < sqk ) 
   {
      if ( b-ptj < 0. ) tmp = -1.5707963267948966e0;
      else              tmp = 1.5707963267948966e0;
      xx = asin( max( min( (b*pti-sqk)/(ptj*fabs( pti-b )), 1.e0 ), -1.e0 ) ) - 
           tmp;
      *x = -ptj*xx/sqb;
   }
   else
   {
      xx = log( max( (ptj-b) * (sqb*sqi + b*pti - sqk) / ((pti-b) * (b*ptj-sqk)),
           1.e-30 ) );
      *x = ptj*xx/sqb;
   }
   *tau2 = -(b*log( (pti+sqi) / ptj ) + sqi - sqb*xx);

/* Handle various error conditions. */
   ErrorHnd:
   if ( *x < -1.e-10 ) 
   {
      sprintf (msg, "Bad range: %12.4lf%12.4lf%12.4lf%12.4lf%12.4lf\n",
       ptk, ptj, pti, *tau2, *x);
      if ( DEBUGL ) logit( "", "%s\n", msg );
   }
   else
   {
      if ( *tau2 >= -1.e-10 ) return;
      sprintf( msg, "Bad tau2: %12.4lf%12.4lf%12.4lf%12.4lf%12.4lf\n",
       ptk, ptj, pti, *tau2, *x );
      if (DEBUGL) logit( "", "%s\n", msg );
      return;
   }
/* Trap null integrals and handle them properly. */
   FunctionEnd:
   *tau2 = 0.e0;
   *x    = 0.e0;
   return;
}

/*
$$$$$ calls only library routines $$$$$

   Given ray parameter grid pt2;i (pt2 sub i), i=i1,i1+1,...,i2, tauspl
   determines the i2-i1+3 basis functions for interpolation I such
   that:

      tau(p) = a;1,i + Dp * a;2,i + Dp**2 * a;3,i + Dp**(3/2) * a;4,i

   where Dp = pt2;n - p, pt2;i <= p < pt2;i+1, and the a;j,i's are
   interpolation coefficients.  Rather than returning the coefficients,
   a;j,i, which necessarily depend on tau(pt2;i), i=i1,i1+1,...,i2 and
   x(pt2;i) (= -d tau(p)/d p | pt2;i), i=i1,i2, tauspl returns the
   contribution of each basis function and its derivitive at each
   sample.  Each basis function is non-zero at three grid points,
   therefore, each grid point will have contributions (function values
   and derivitives) from three basis functions.  Due to the basis
   function normalization, one of the function values will always be
   one and is not returned in array coef2 with the other values.
   Rewritten on 23 December 1983 by R. Buland.

   Called by tauspl (.., .., &pt2[xx], 
   i1, i2, based on 1
*/
void tauspl( int i1, int i2, double pt2[], double coef2[5][JOUT] )
{
   double  ali, alr, b3h, b1h, bih, th0p, th2p, th3p, th2m;
   double  del[5], sdel[5], deli[5], d3h[4], d1h[4], dih[4], d[4];
   int     i, j, k, m;                  /* Based on 0 */
   int     n2, is, l;			/* Based on 1 (i1, i2 also) */

   n2 = i2 - i1 - 1;
   if ( n2 <= -1 ) return;
   is = i1 + 1;
/*
   To achieve the requisite stability, proceed by constructing basis
   functions G;i, i=0,1,...,n+1.  G;i will be non-zero only on the
   interval [p;i-2,p;i+2] and will be continuous with continuous first
   and second derivitives.  G;i(p;i-2) and G;i(p;i+2) are constrained
   to be zero with zero first and second derivitives.  G;i(p;i) is
   normalized to unity.

   Set up temporary variables appropriate for G;-1.  Note that to get
   started, the ray parameter grid is extrapolated to yeild p;i, i=-2,
   -1,0,1,...,n.
*/
   del[1]  = pt2[i2-1] - pt2[i1-1] + 3.*(pt2[is-1]-pt2[i1-1]);
   sdel[1] = sqrt( fabs( del[1] ) );
   deli[1] = 1. / sdel[1];
   m = 1;
   for ( k=2; k<5; k++ )
   {
      del[k]  = pt2[i2-1] - pt2[i1-1] + (4-k) * (pt2[is-1]-pt2[i1-1]);
      sdel[k] = sqrt( fabs( del[k] ) );
      deli[k] = 1. / sdel[k];
      d3h[m]  = del[k]*sdel[k] - del[m]*sdel[m];
      d1h[m]  = sdel[k]-sdel[m];
      dih[m]  = deli[k]-deli[m];
      m       = k;
   }
   l = i1 - 1;
   if ( n2 <= 0 ) goto NextLoop;
/* Loop over G;i, i=0,1,...,n-3. */
   for ( i=0; i<n2; i++ )
   {
      m = 0;
/* Update temporary variables for G;i-1. */
      for ( k=1; k<5; k++ )
      {
         del[m]  = del[k];
         sdel[m] = sdel[k];
         deli[m] = deli[k];
         if ( k < 4 ) 
         {
            d3h[m] = d3h[k];
            d1h[m] = d1h[k];
            dih[m] = dih[k];
         }
         m = k;
      }
      l++;
      del[4]  = pt2[i2-1]-pt2[l];
      sdel[4] = sqrt( fabs( del[4] ) );
      deli[4] = 1. / sdel[4];
      d3h[3]  = del[4]*sdel[4] - del[3]*sdel[3];
      d1h[3]  = sdel[4]-sdel[3];
      dih[3]  = deli[4]-deli[3];
/* Construct G;i-1. */
      ali  = 1. / (0.125*d3h[0] - (.75*d1h[0] + .375*dih[0]*del[2])*del[2]);
      alr  = ali*(0.125*del[1]*sdel[1] - (0.75*sdel[1] + 0.375*del[2]*deli[1] - 
             sdel[2])*del[2]);
      b3h  = d3h[1] + alr*d3h[0];
      b1h  = d1h[1] + alr*d1h[0];
      bih  = dih[1] + alr*dih[0];
      th0p = d1h[0]*b3h - d3h[0]*b1h;
      th2p = d1h[2]*b3h - d3h[2]*b1h;
      th3p = d1h[3]*b3h - d3h[3]*b1h;
      th2m = dih[2]*b3h - d3h[2]*bih;
/* The d;i's completely define G;i-1. */
      d[3] = ali * ((dih[0]*b3h - d3h[0]*bih)*th2p - th2m*th0p) / ((dih[3]*b3h -
             d3h[3]*bih)*th2p - th2m*th3p);
      d[2] = (th0p*ali - th3p*d[3]) / th2p;
      d[1] = (d3h[0]*ali - d3h[2]*d[2] - d3h[3]*d[3]) / b3h;
      d[0] = alr*d[1] - ali;
/* Construct the contributions G;i-1(p;i-2) and G;i-1(p;i).
   G;i-1(p;i-1) need not be constructed as it is normalized to unity. */
      coef2[0][l-1] = (0.125*del[4]*sdel[4] - (0.75*sdel[4] + 
                       0.375*deli[4]*del[3] - sdel[3])*del[3])*d[3];
      if ( i >= 2 ) coef2[1][l-3] = (0.125*del[0]*sdel[0] - (0.75*sdel[0] +
                    0.375*deli[0]*del[1] - sdel[1])*del[1])*d[0];
/* Construct the contributions -dG;i-1(p)/dp | p;i-2, p;i-1, and p;i. */
      coef2[2][l-1] = -0.75*(sdel[4] + deli[4]*del[3] - 2.*sdel[3]) * d[3];
      if ( i >= 1 ) coef2[3][l-2] = -0.75 * ((sdel[1] + deli[1]*del[2] -
                    2.*sdel[2])*d[1] - (d1h[0] + dih[0]*del[2])*d[0]);
      if ( i >= 2 ) coef2[4][l-3] = -0.75 * (sdel[0] + deli[0]*del[1] -
                    2.*sdel[1]) * d[0];
   }
/* Loop over G;i, i=n-2,n-1,n,n+1.  These cases must be handled
   seperately because of the singularities in the second derivitive
   at p;n. */
   NextLoop:
   for ( j=0; j<4; j++ )
   {        
      m = 0;
/* Update temporary variables for G;i-1. */
      for ( k=1; k<5; k++ )
      {
         del[m]  = del[k];
         sdel[m] = sdel[k];
         deli[m] = deli[k];
         if ( k < 4 )
         {
            d3h[m] = d3h[k];
            d1h[m] = d1h[k];
            dih[m] = dih[k];
         }
         m = k;
      }
      l++;
      del[4]  = 0.;
      sdel[4] = 0.;
      deli[4] = 0.;
/* Construction of the d;i's is different for each case.  In cases
   G;i, i=n-1,n,n+1, G;i is truncated at p;n to avoid patching across
   the singularity in the second derivitive. */
      if ( j >= 3 )
      {
/* For G;n+1 constrain G;n+1(p;n) to be .25. */
         d[0] = 2. / (del[0]*sdel[0]);
         goto SkipSome;
      }
/* For G;i, i=n-2,n-1,n, the condition dG;i(p)/dp|p;i = 0 has been
   substituted for the second derivitive continuity condition that
   can no longer be satisfied. */
      alr = (sdel[1] + deli[1]*del[2] - 2.*sdel[2]) / (d1h[0] + dih[0]*del[2]);
      d[1] = 1. / (0.125*del[1]*sdel[1] - (0.75*sdel[1] + 0.375*deli[1]*
             del[2] - sdel[2]) * del[2] - (0.125*d3h[0] - (0.75*d1h[0]+0.375*
             dih[0]*del[2]) * del[2]) * alr);
      d[0] = alr*d[1];
      if ( (j-1) < 0 )
/* For G;n-1 constrain ÿ;n-1(p;n) to be .25. */
      {
/* No additional constraints are required for G;n-2. */
         d[2] = -((d3h[1] - d1h[1]*del[3]) * d[1] + (d3h[0] - d1h[0]*del[3]) *
                d[0]) / (d3h[2] - d1h[2]*del[3]);
         d[3] = (d3h[2]*d[2] + d3h[1]*d[1] + d3h[0]*d[0]) / (del[3]*sdel[3]);
      }
      else if ( (j-1) == 0 )
         d[2] = (2. + d3h[1]*d[1] + d3h[0]*d[0]) / (del[2]*sdel[2]);
/* No additional constraints are required for G;n-2.
   Construct the contributions G;i-1(p;i-2) and G;i-1(p;i). */
      SkipSome:
      if ( j <= 1 ) coef2[0][l-1] = (0.125*del[2]*sdel[2] - 
                     (0.75*sdel[2] + 0.375*deli[2]*del[3] - 
                      sdel[3]) * del[3]) * d[2] - (0.125*d3h[1] - 
                     (0.75*d1h[1] + 0.375*dih[1]*del[3]) * del[3]) * d[1] - 
                     (0.125*d3h[0] - (0.75*d1h[0] + 0.375*dih[0]*del[3]) * 
                      del[3]) * d[0];
      if ( l-i1 > 1 ) coef2[1][l-3] = (0.125*del[0]*sdel[0] - (0.75*sdel[0] +
                       0.375*deli[0]*del[1] - sdel[1]) * del[1]) * d[0];
/* Construct the contributions -dG;i-1(p)/dp | p;i-2, p;i-1, and p;i. */
      if ( j <= 1 ) coef2[2][l-1] = -0.75 * ((sdel[2] + deli[2]*del[3] -
                     2.*sdel[3]) * d[2] - (d1h[1] + dih[1]*del[3]) * d[1] - 
                     (d1h[0]+dih[0]*del[3]) * d[0]);
      if ( j <= 2 && l-i1 > 0 ) coef2[3][l-2] = 0.;
      if ( l-i1 > 1 ) coef2[4][l-3] = -0.75 * (sdel[0] + deli[0]*del[1] -
                       2.*sdel[1]) * d[0];
   }
   return;
}

void trtm( double delta, int *n, double tt[], double dtdd[], 
           double dtdh[], double dddp[], char phnm[MAX_PHASES][PHASE_LENGTH] )
{
   static char   ctmp[60][PHASE_LENGTH];
   double        dtol = 1.e-6;
   double        atol = 0.005;
   static int    iptr[MAX_PHASES];
   int           i, j, k;                 /* Based on 0 */
   static double x[3], tmp[4][60];

   *n = 0;
   if ( iMbr2 <= 0 ) return;
   x[0] = fmod( fabs( RAD*delta ), TWOPI );
   if ( x[0] > PI ) x[0] = TWOPI - x[0];
   x[1] = TWOPI - x[0];
   x[2] = x[0] + TWOPI;
   if ( fabs( x[0] ) <= dtol )
   {
      x[0] = dtol;
      x[2] = -10.;
   }
   if ( fabs( x[0]-PI ) <= dtol ) 
   {
      x[0] = PI - dtol;
      x[1] = -10.;
   }
   for ( j=iMbr1-1; j<iMbr2; j++ )
      if ( iJNDex[j][1] > 0 ) findtt( j, x, n, &tmp[0][0], &tmp[1][0], 
                                      &tmp[2][0], &tmp[3][0], ctmp );
   if ( *n-1 <  0 )  return;
   if ( *n-1 == 0 ) iptr[0] = 0;
   else             r4sort( *n, &tmp[0][0], iptr );
   k = -1;
   for ( i=0; i<*n; i++ )
   {
      j = iptr[i];
      if ( k > 0 )
         if ( !strcmp( phnm[k], ctmp[j] ) && fabs( tt[k]-tmp[0][j] ) <= atol ) 
            continue;
      k++;
      tt[k]   = tmp[0][j];
      dtdd[k] = tmp[1][j];
      dtdh[k] = tmp[2][j];
      dddp[k] = tmp[3][j];
      strcpy( phnm[k], ctmp[j] );
   }
   *n = k + 1;
   return;
}

double umod( double zs, int *isrc, int nph )
{
   double dep;
   int    i, j;                /* Index based on 0 */
   double dtol = 1.e-6;

   for ( i=1; i<iMt[nph]; i++ ) 
      if ( dZm[i][nph] <= zs ) goto ItsOK;
   dep = (1. - exp (zs)) / dXn;
   return 0.;

   ItsOK:
   if ( fabs( zs-dZm[i][nph] ) > dtol || fabs( dZm[i][nph]-dZm[i+1][nph] ) > 
        dtol )  
   {
      j = i - 1;
      *isrc = j;
      return( dPm[j][nph] + (dPm[i][nph]-dPm[j][nph]) * (exp( zs-dZm[j][nph] ) -
              1.) / (exp( dZm[i][nph]-dZm[j][nph] )-1.) );
   }
   else
   {
      *isrc = i;
      return( dPm[i+1][nph] );
   }
}

double zmod( double uend, int js, int nph )
{
   int   i;

   i = js + 1;
   return( dZm[js][nph] + log( max( (uend-dPm[js][nph])*(exp( dZm[i][nph]-
           dZm[js][nph] )-1.) / (dPm[i][nph]-dPm[js][nph]) + 1., 1.e-30 ) ) );
}
