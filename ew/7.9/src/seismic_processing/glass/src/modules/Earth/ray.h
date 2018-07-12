#ifndef _RAY_H_
#define _RAY_H_
#include "geo.h"

enum {
	RAY_Pup,
	RAY_P,
	RAY_Pdiff,
	RAY_PP,
	RAY_PPP,
	RAY_PKP,
	RAY_PKIKP,
	RAY_PKPab,
	RAY_PKPbc,
	RAY_PKPdf,
	RAY_PcP,
	RAY_Sup,
	RAY_S,
	RAY_Sdiff,
	RAY_SS,
	RAY_SSS
};

class CTerra;
class CSite;
class CGeo;
class CRay : public CGeo{
public:
	CTerra		*pTerra;
	CSite		*pSite;
	int			iPhase;	// Index of phase in Phase[] list.
	double		dP1;	// Minimum ray parameter for phase
	double		dP2;	// Maximum ray parameter for phase
	int			nTau;	// Number of points in tau function
	double		dPinc;	// Step size for Tau function
	double		*dP;	// Array of p values, for now equally spaced
	double		*dTau;	// Tau function (descrete), dTau[nTau]
	double		*dTau2;	// Second tau derivative (for spline interpolation)

// Methods
	CRay();
	virtual ~CRay();
	void Attach(CTerra *pterra);
	void Attach(CSite *psite);
	int setPhase(const char *phs);
	void Init();
//	void Generate(double rcvr);
//	double Refine(double tol);
	double Travex(double delta);
	double Travel(double delta);
	double Travel(double delta, double rcvr);
	double Travel(double delta, double rcvr, double *p);
	double Travel(CGeo *geo);
	double Travel(CGeo *geo, double *p);
	double Delta(double t, double *p);
//	void Curve(double p, double *t, double *d);
	double T(double p, double rcvr);
	double D(double p, double rcvr);
	double Tau(double p, double rcvr);
	double Param(int par, double p, double rcvr);
	double Fun(double x);
	void MnBrak(double *x);
	double Brent(double *xx, double tol, double *xmin);
};

#endif