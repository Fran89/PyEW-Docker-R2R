#include <math.h>
#include "comfile.h"
#include "origin.h"
#include "ray.h"

#define DEGRAD  57.29577951308

COrigin::COrigin() {
	nState = 0;
	dTouch = 0.0;
	nChange = 0;
	nPicks = 0;
	nPrimary = 0;
	dRms = 0.0;
	dMin = 0.0;
	dAvg = 0.0;
	dMed = 0.0;
	dGap = 0.0;
	iOrigin = -1;
}

COrigin::~COrigin() {
}

void COrigin::Clone(COrigin *org) {
	CGeo::Clone(org);
	nState = org->nState;
	iOrigin = org->iOrigin;
	nChange = org->nChange;
	nPicks = org->nPicks;
	nPrimary = org->nPrimary;
	dRms = org->dRms;
	dMin = org->dMin;
	dAvg = org->dAvg;
	dMed = org->dMed;
	dGap = org->dGap;
}


