#ifndef _ORIGIN_H_
#define _ORIGIN_H_
#include "geo.h"

#define NDST	18
#define	DDST	5.0
#define NAZM	18
#define DAZM	20.0

class COrigin : public CGeo {
public:

// Attributes
	int nState;		// Current processing state
	int iOrigin;	// Origin index (external use)
	double dTouch;	// Time of last state transition
	int	nChange;	// Change count (e.g. phases added or delete)
	int nPicks;		// Number of picks used in location
	int nPrimary;	// Number of primary phases (P)
	double dRms;	// RMS of solution
	double dMin;	// Minimum distance to first station (radians)
	double dAvg;	// Average distance
	double dMed;	// Median distance
	double dGap;	// Gap (radians)

// Methods
	COrigin();
	virtual ~COrigin();
	virtual void Clone(COrigin *org);
};

#endif
