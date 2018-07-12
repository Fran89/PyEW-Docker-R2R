#ifndef _TERRA_H_
#define _TERRA_H_

#define MLAYER 200

// It is significant that the S phase is 1 greater
// than the corresponding P phase.
#define FUN_TEST 1000
#define FUN_P_TIME	0
#define FUN_P_DELTA	2
#define FUN_P_TAU   4
#define FUN_S_TIME	1
#define FUN_S_DELTA 3
#define FUN_S_TAU   5

class CTerra {
public:
// Attributes
	char sPath[64];		// Model file path
	int nBreak;			// Number of seismic discontinuities
	int iBreak[20];		// Index, inside of discontinuity
	int iInner;			// Index, top of inner core
	int	iOuter;			// Index, top of outer core
	long nLayer;		// Layers in velocity model
	double dRadius;		// Radius of earth (layer with 0 depth)
	double dR[MLAYER];	// Radius (km), from center out
	double dP[MLAYER];	// P velocity (km/s)
	double dS[MLAYER];	// S velocity (km.s)

// Methods
	CTerra();
	virtual ~CTerra();
	bool Init(char *file);
	double P(double r);
	double S(double r);
	double Vel(double r, double *vel);
	double Turn(int i1, int i2, double *vel, double pray);
	double Fun(int mode, double r);
	double MidPnt(int mode, double r1, double r2, int n);
	double MidSqu(int mode, double r1, double r2, int n);
	double MidSql(int mode, double r1, double r2, int n);
	double RaySeg(int mode, double r1, double r2, double p);
	double Romberg(int mode, double r1, double r2, double p);

};
#endif