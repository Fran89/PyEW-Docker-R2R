#include <windows.h>
#include <math.h>
#include "comfile.h"
#include "ray.h"
#include "terra.h"
#include "origin.h"
#include "site.h"
//#include "spline.h"

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994

enum {
	PAR_TIME,
	PAR_RANGE,
	PAR_TAU
};

static int nPhase;
static struct {
	int		iph;
	char*	phs;
} Phase[] = {
	RAY_Pup,	"Pup",
	RAY_P,		"P",
	RAY_Pdiff,	"Pdiff",
	RAY_PP,		"PP",
	RAY_PPP,	"PPP",
	RAY_PKP,	"PKP",
	RAY_PKIKP,	"PKIKP",
	RAY_PKPab,	"PKPab",
	RAY_PKPbc,	"PKPbc",
	RAY_PKPdf,	"PKPdf",
	RAY_PcP,	"PcP",
	RAY_Sup,	"Sup",
	RAY_S,		"S",
	RAY_Sdiff,	"Sdiff",
	RAY_SS,		"SS",
	RAY_SSS,	"SSS"
};

//--------------------------------------------------------------------------CRay
CRay::CRay() {
	pTerra = NULL;
	pSite = NULL;
	int jj = sizeof(Phase[0]);
	nPhase = sizeof(Phase) / sizeof(Phase[0]);
	nTau = 0;
	dP = NULL;
	dTau = NULL;
	dTau2 = NULL;
}

//--------------------------------------------------------------------------~CRay
CRay::~CRay() {
	if(dP != NULL)
		delete [] dP;
	if(dTau != NULL)
		delete [] dTau;
	if(dTau2 != NULL)
		delete [] dTau2;
}

//--------------------------------------------------------------------------Attach(terra)
void CRay::Attach(CTerra *vel) {
	pTerra = vel;
}

//--------------------------------------------------------------------------Attach(site)
void CRay::Attach(CSite *site) {
	pSite = site;
}

//--------------------------------------------------------------------------setPhase
int CRay::setPhase(const char *phs) {
	int i;

	for(i=0; i<nPhase; i++) {
		if(strcmp(phs, Phase[i].phs) != 0)
			continue;
		iPhase = i;
		Init();
		return iPhase;
	}
	return -1;
}

//--------------------------------------------------------------------------Init
// Init(): Inialize ray whenever dependancies such as origin change.
void CRay::Init() {
	int ilay;

	dP1 = 0.0;
	dP2 = 1.0;
	int iph = Phase[iPhase].iph;
	switch(iph) {
	case RAY_Pup:
		dP1 = 0.0;
		dP2 = dRad / pTerra->P(dRad);
		break;
	case RAY_P:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		dP2 = dRad / pTerra->P(dRad);
		break;
	case RAY_Pdiff:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		dP2 = dP1;
		break;
	case RAY_PP:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		dP2 = dRad / pTerra->P(dRad);
		break;
	case RAY_PPP:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		dP2 = dRad / pTerra->P(dRad);
		break;
	case RAY_PKP:
	case RAY_PKPab:
	case RAY_PKPbc:
		ilay = pTerra->iInner+1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		ilay = pTerra->iOuter+1;
		dP2 = pTerra->dR[ilay] / pTerra->dP[ilay];
		break;
	case RAY_PKIKP:
	case RAY_PKPdf:
		ilay = 1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		ilay = pTerra->iInner;
		dP2 = pTerra->dR[ilay] / pTerra->dP[ilay];
		break;
	case RAY_PcP:
		ilay = 1;
		dP1 = pTerra->dR[ilay] / pTerra->dP[ilay];
		ilay = pTerra->iOuter+1;
		dP2 = pTerra->dR[ilay] / pTerra->dP[ilay];
		break;
	case RAY_Sup:
		dP1 = 0.0;
		dP2 = dRad / pTerra->S(dRad);
		break;
	case RAY_S:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dS[ilay];
		dP2 = dRad / pTerra->S(dRad);
		break;
	case RAY_Sdiff:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dS[ilay];
		dP2 = dP1;
		break;
	case RAY_SS:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dS[ilay];
		dP2 = dRad / pTerra->S(dRad);
		break;
	case RAY_SSS:
		ilay = pTerra->iOuter+1;
		dP1 = pTerra->dR[ilay] / pTerra->dS[ilay];
		dP2 = dRad / pTerra->S(dRad);
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------------------Travel(d, z)
// Travel: Calculate travel time (seconds) for given distance (radians)
double dFunFac = 1.0;	// Note: This is not thread safe, make attribute
double dDelta = 1.0;	//       Both used by Fun
double dRcvr = 0.0;
double CRay::Travel(double delta) {
	return Travel(delta, pTerra->dRadius);
}
double CRay::Travel(double delta, double rcvr) {
	double p;
	return Travel(delta, rcvr, &p);
}
// Calculate minimum travel time from Tau curve for current
// branch. Returns least travel time as well as ray parameter
// at minimum.
double CRay::Travel(double delta, double rcvr, double *pmin) {
	int ip;
	int iph;
	double p;
	double pinc;
	double x[3];
	double y0, y1, y2;
	double t;
	double d;
	double trav;
	double rturn;
	int nbranch;

	trav = -10.0;	// Indicates no arrivals detected
	dDelta = delta;
	dRcvr = rcvr;
	iph = Phase[iPhase].iph;
	switch(iph) {
	case RAY_Pdiff:
		rturn = pTerra->dR[pTerra->iOuter+1];
		d = 2.0*pTerra->RaySeg(FUN_P_DELTA, rturn, dRad, dP1)
		  +     pTerra->RaySeg(FUN_P_DELTA, dRad, rcvr, dP1);
		if(delta < d)
			return trav;
		t = 2.0*pTerra->RaySeg(FUN_P_TIME, rturn, dRad, dP1)
		  +     pTerra->RaySeg(FUN_P_TIME, dRad, rcvr, dP1);
		trav = t + RAD2DEG * (delta - d) * pTerra->dR[pTerra->iOuter+1] * 111.111
			     / rcvr / pTerra->dP[pTerra->iOuter+1];
		*pmin = dP1;
		return trav;
	case RAY_Sdiff:
		rturn = pTerra->dR[pTerra->iOuter+1];
		d = 2.0*pTerra->RaySeg(FUN_S_DELTA, rturn, dRad, dP1)
		  +     pTerra->RaySeg(FUN_S_DELTA, dRad, rcvr, dP1);
		if(delta < d)
			return trav;
		t = 2.0*pTerra->RaySeg(FUN_S_TIME, rturn, dRad, dP1)
		  +     pTerra->RaySeg(FUN_S_TIME, dRad, rcvr, dP1);
		trav = t + RAD2DEG * (delta - d) * pTerra->dR[pTerra->iOuter+1] * 111.111
			     / rcvr / pTerra->dS[pTerra->iOuter+1];
		*pmin = dP1;
		return trav;
	}
	double p0, p1, p2;
	double pincmax = 2.0;
	double pincmin = 0.005;
	pinc = pincmin;
	p2 = dP2 + pinc;
	ip = 0;
	nbranch = 0;	// Branch index to resolve multiple phases
	p1 = y1 = y2 = 0.0;
	while(p2 > dP1) {
		p0 = p1;
		p1 = p2;
		p2 -= pinc;
		p = p2;
		if(p < dP1)
			p = dP1;
		y0 = y1;
		y1 = y2;
		y2 = Fun(p);
		if(ip++ < 2)	// Skip first two calculations
			continue;
		if(y1 < y0 && y1 < y2) {
			dFunFac = 1.0;
			pinc = pincmin;
		} else if (y1 > y0 && y1 > y2) {
			dFunFac = -1.0;
			pinc = pincmin;
		} else {	// Accelerate search by expanding search mesh
			pinc *= 2.0;
			if(pinc > pincmax)
				pinc = pincmax;
			continue;
		}
		x[0] = p;
		x[1] = p1;
		x[2] = p0;
		t = dFunFac*Brent(x, 2.0e-3, &p);
		nbranch++;
		switch(iph) {
		case RAY_PKPab: // First extrema in p order
			if(nbranch == 1) {
				trav = t;
				*pmin = p;
			}
			break;
		case RAY_PKPbc:	// Second extrema in p order
			if(nbranch == 2) {
				trav = t;
				*pmin = p;
			}
			break;
		default: // For all other multiple branches accept the earliest arrival
			if(trav < 0.0 || t < trav) {
				trav = t;
				*pmin = p;
			}
			break;
		}
		dFunFac = 1.0;
	}
	switch(iph) {
	case RAY_PKPbc: // For PKPbc to exist, PKPab must also, otherwise phase is PKPab
		if(nbranch < 2)
			trav = -12.0;
		break;
	}
	return trav;
}

//--------------------------------------------------------------------------Travel(geo)
// Travel: Calculate travel time (seconds) to given geo point
// This is kludged for now since it ignores the station elevation - later! =)
double CRay::Travel(CGeo *geo) {
	double pmin;
	return Travel(geo, &pmin);
}
double CRay::Travel(CGeo *geo, double *pmin) {
	double delta = CGeo::Delta(geo);
	return Travel(delta, geo->dRad, pmin);
}

//--------------------------------------------------------------------------Travex
// Travel: Calculate travel time (seconds) for given distance (radians)
double CRay::Travex(double delta) {
	double x[6];
	double trav;
	double t;
	double xmin;
	int i;

	x[1] = dP1;
	x[2] = dP1 + 1.0;
	dDelta = delta;
	dFunFac = 1.0;
	if(Fun(x[2]) > Fun(x[1]))
		dFunFac = -1.0;
	for(i=0; i<2; i++) {
		x[0] = x[1];
		x[1] = x[2];
		MnBrak(x);
		t = dFunFac*Brent(x, 2.0e-3, &xmin);
		if(i == 0)
			trav = t;
		if(t < trav)
			trav = t;
		if(x[2] >= dP2)
			break;
		x[1] = xmin; 
		dFunFac = -dFunFac;
	}
	return trav;
}

//--------------------------------------------------------------------------Delta
// Find the minimum delta given a travel-time
// Returns -1.0 for no such arrival
double CRay::Delta(double t, double *pret) {
	double dmax = -1.0;
	double pmax;
	double p;
	double t1;
	double t2;
	double p1;
	double p2;
	double tcal;
	double dcal = -1.0;
	int n = 1000;
	int iref;
	double tol = 0.5;
	double t2save;
	double pdel = (dP2 - dP1)/(n-1);
	for(int i=0; i<n; i++) {
		p2 = dP1 + i*pdel;
		t2 = Param(FUN_P_TIME, p2, pTerra->dRadius);
		if(!i) {
			t1 = t2;
			continue;
		}
		if((t1 < t && t2 > t) || (t2 < t && t1 > t)) {
			p1 = p2 - pdel;
			t2save = t2;
			for(iref=0; iref<5; iref++) {
				p = p1 + (t - t1)*(p2 - p1)/(t2 - t1);
				tcal = Param(FUN_P_TIME, p, pTerra->dRadius);
				dcal = Param(FUN_P_DELTA, p, pTerra->dRadius);
				if(fabs(tcal-t) < tol)
					break;
				if(t2 > t1) {
					if(tcal > t) {
						p2 = p;
						t2 = tcal;
					} else {
						p1 = p;
						t1 = tcal;
					}
				} else {
					if(tcal > t) {
						p1 = p;
						t1 = tcal;
					} else {
						p2 = p;
						t2 = tcal;
					}
				}
			}
			t2 = t2save;
			if(dcal > dmax) {
				dmax = dcal;
				pmax = p;
			}
		}
		t1 = t2;
	}
	*pret = pmax;
	return dmax;
}


//--------------------------------------------------------------------------T
// Calculate travel time as a function of ray parameter.
double CRay::T(double p, double rcvr) {
	return Param(FUN_P_TIME, p, rcvr);
}

//--------------------------------------------------------------------------D
// Calculate distance (radians) as a function of ray parameter
double CRay::D(double p, double rcvr) {
	if(p < 0.0001)
		return 0.0;
	return Param(FUN_P_DELTA, p, rcvr);
}

//--------------------------------------------------------------------------Tau
// Tau(p): Calculate Tau as a function of ray parameter
double CRay::Tau(double p, double rcvr) {
	return Param(FUN_P_TAU, p, rcvr);
}

//--------------------------------------------------------------------------Param
// Param(par, p): Time, range, and tau integrals over depth.
//		This is an internal routine.
double CRay::Param(int par, double p, double rcvr) {
	double rturn;	// Turning radius (if applicable)
	int ilay1;		// Innermost model index
	int ilay2;		// Outermost model index
	double val;		// Integral accumulater

	if(p < dP1 || p > dP2)
		return -100.0;

	int iph = Phase[iPhase].iph;
	dRad;
	val = 0.0;
	switch(iph) {
	case RAY_Pup:
		val = pTerra->RaySeg(par, dRad, rcvr, p);
		break;
	case RAY_P:
		ilay1 = pTerra->iOuter+1;
		ilay2 = pTerra->nLayer-1;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dP, p);
		val = 2.0*pTerra->RaySeg(par, rturn, dRad, p)
			+     pTerra->RaySeg(par, dRad, rcvr, p);
		break;
	case RAY_Pdiff:
		val = 0.0;
		break;
	case RAY_PP:
		ilay1 = pTerra->iOuter+1;
		ilay2 = pTerra->nLayer-1;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dP, p);
		val = 4.0*pTerra->RaySeg(par, rturn, dRad, p)
			+ 3.0*pTerra->RaySeg(par, dRad, rcvr, p);
		break;
	case RAY_PPP:
		ilay1 = pTerra->iOuter+1;
		ilay2 = pTerra->nLayer-1;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dP, p);
		val = 6.0*pTerra->RaySeg(par, rturn, dRad, p)
			+ 5.0*pTerra->RaySeg(par, dRad, rcvr, p);
		break;
	case RAY_PKP:
	case RAY_PKPab:
	case RAY_PKPbc:
		ilay1 = pTerra->iInner+1;
		ilay2 = pTerra->iOuter;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dP, p);
		val = 2.0*pTerra->RaySeg(par, rturn, dRad, p)
			+     pTerra->RaySeg(par, dRad, rcvr, p);
		break;
	case RAY_PKIKP:
	case RAY_PKPdf:
		ilay1 = 1;
		ilay2 = pTerra->iInner;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dP, p);
		val = 2.0*pTerra->RaySeg(par, rturn, dRad, p)
			+     pTerra->RaySeg(par, dRad, rcvr, p);
		break;
	case RAY_PcP:
		rturn = pTerra->dR[pTerra->iOuter+1];
		val = 2.0 * pTerra->RaySeg(par, rturn, dRad, p)
			+       pTerra->RaySeg(par, dRad, rcvr, p);

		break;
	case RAY_Sup:
		val = pTerra->RaySeg(par+1, dRad, rcvr, p);
		break;
	case RAY_S:
		ilay1 = pTerra->iOuter+1;
		ilay2 = pTerra->nLayer-1;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dS, p);
		val = 2.0*pTerra->RaySeg(par+1, rturn, dRad, p)
			+     pTerra->RaySeg(par+1, dRad, rcvr, p);
		break;
	case RAY_Sdiff:
		return 0.0;
	case RAY_SS:
		ilay1 = pTerra->iOuter+1;
		ilay2 = pTerra->nLayer-1;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dS, p);
		val = 4.0*pTerra->RaySeg(par+1, rturn, dRad, p)
			+ 3.0*pTerra->RaySeg(par+1, dRad, rcvr, p);
		break;
	case RAY_SSS:
		ilay1 = pTerra->iOuter+1;
		ilay2 = pTerra->nLayer-1;
		rturn = pTerra->Turn(ilay1, ilay2, pTerra->dS, p);
		val = 6.0*pTerra->RaySeg(par+1, rturn, dRad, p)
			+ 5.0*pTerra->RaySeg(par+1, dRad, rcvr, p);
		break;
	default:
		break;
	}
	return val;
}

//--------------------------------------------------------------------------Fun
// Fun: Glue code to associate function with local code.
//      The routines Fun, MnBrak, and	Brent are meant to be pretty
//      much self contained so that they can be pasted in with the
//      only significant chanbes being in the Fun method.
//		Calculates theta function after Buland and Chapman(1983)
//		Assumes x in radians
double CRay::Fun(double x) {
	double p = x;
	if(p > dP2)
		p = 2.0*dP2 - p;	// Reflect at dP2
	double theta = Tau(p, dRcvr) + p*dDelta;
	double fun = dFunFac*theta;
	return fun;
}

//--------------------------------------------------------------------------MnBrak
// MnBrak: Bracket function minima (1-dimensional)
static double dArg1;
static double dArg2;
#define ITMAX 50
#define SHFT(a,b,c,d) (a)=(b);(b)=(c);(c)=(d);
#define GOLD 1.618034
#define GLIMIT 100.0
#define TINY 1.0e-20
#define SIGN(a,b) ((b) > 0.0 ? fabs(a) : -fabs(a))
#define FMAX(a,b) (dArg1=(a), dArg2=(b), (dArg1 > dArg2) ? dArg1 : dArg2)
void CRay::MnBrak(double *x) {
	double ulim;
	double u;
	double r;
	double q;
	double fu;
	double dum;
	double ax = x[0];
	double bx = x[1];
	double cx = x[2];
	double fa = x[3];
	double fb = x[4];
	double fc = x[5];

	fa = Fun(ax);
	fb = Fun(bx);
	if(fb > fa) {
		SHFT(dum, ax, bx, dum)
		SHFT(dum, fb, fa, dum)
	}
	cx = bx + GOLD*(bx - ax);
	fc = Fun(cx);
	while(fb > fc) {
		r = (bx - ax) * (fb - fc);
		q = (bx - cx) * (fb - fa);
		u = bx - ((bx - cx) * q - (bx - ax) * r) /
			(2.0 * SIGN(FMAX(fabs(q-r),TINY),q-r));
		ulim = bx + GLIMIT * (cx - bx);
		if((bx-u)*(u-cx) > 0.0) {
			fu = Fun(u);
			if(fu < fc) {
				ax = bx;
				bx = u;
				fa = fb;
				fb = fu;
				goto pau;
			} else if(fu > fb) {
				cx = u;
				fc = fu;
				goto pau;
			}
			u = cx + GOLD * (cx - bx);
			fu = Fun(u);
		} else if((cx - u)*(u - ulim) > 0.0) {
			fu = Fun(u);
			if(fu < fc) {
				SHFT(bx, cx, u, cx+GOLD*(cx - bx))
				SHFT(fb, fc, fu, Fun(u))
			}
		} else if ((u - ulim)*(ulim - cx) >= 0.0) {
			u = ulim;
			fu = Fun(u);
		} else {
			u = cx + GOLD*(cx - bx);
			fu = Fun(u);
		}
		SHFT(ax, bx, cx, u);
		SHFT(fa, fb, fc, fu);
	}

pau:
	x[0] = ax;	x[1] = bx;	x[2] = cx;
	x[3] = fa;	x[4] = fb;	x[5] = fc;
	return;
}

#define CGOLD 0.3819660
#define ZEPS 1.0e-10
//--------------------------------------------------------------------------Brent
double CRay::Brent(double *xx, double tol, double *xmin) {
	int iter;
	double ax = xx[0];
	double bx = xx[1];
	double cx = xx[2];
	double a, b, d;
	double etemp;
	double fu, fv, fw, fx;
	double p, q, r;
	double tol1, tol2;
	double u, v, w, x, xm;
	double e = 0.0;

	a = (ax < cx ? ax : cx);
	b = (ax > cx ? ax : cx);
	x = w = v = bx;
	fw = fv = fx = Fun(x);
	for(iter=1; iter<=ITMAX; iter++) {
		xm = 0.5*(a+b);
		tol2 = 2.0*(tol1=tol*fabs(x)+ZEPS);
		if(fabs(x-xm) <= (tol2 - 0.5*(b-a))) {
			*xmin = x;
			return fx;
		}
		if(fabs(e) > tol1) {
			r = (x - w) * (fx - fv);
			q = (x - v) * (fx - fw);
			p = (x - v) * q - (x - w) * r;
			q = 2.0*(q - r);
			if(q > 0.0)
				p = -p;
			q = fabs(q);
			etemp = e;
			e = d;
			if(fabs(p) >= fabs(0.5*q*etemp)
			|| p <= q*(a - x)
			|| p >= q*(b - x))
			d = CGOLD*(e = (x >= xm ? a - x : b - x));
			else {
				d = p / q;
				u = x + d;
				if(u - a < tol2
				|| b - u < tol2)
					d = SIGN(tol1, xm-x);
			}
		} else {
			d = CGOLD * (e = (x >= xm ? a - x : b - x));
		}
		u = (fabs(d) >= tol1 ? x + d : x + SIGN(tol1, d));
		fu = Fun(u);
		if(fu <= fx) {
			if(u >= x)
				a = x;
			else
				b = x;
			SHFT(v, w, x, u)
			SHFT(fv, fw, fx, fu)
		} else {
			if(u < x)
				a = u;
			else
				b = u;
			if(fu <= fw || w == x) {
				v = w;
				w = u;
				fv = fw;
				fw = fu;
			} else if (fu <= fv || v == x || v == w) {
				v = u;
				fv = fu;
			}
		}
	}
	*xmin = x;
	return fx;
}
