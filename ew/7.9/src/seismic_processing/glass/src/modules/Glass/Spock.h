// spock.h
#ifndef SPOCK_H
#define SPOCK_H

class CGlass;
struct IReport;
class CSpock {
public:
// Attributes
	CGlass *pGlass;
	IReport *pRpt;

// Methods
	CSpock();
	virtual ~CSpock();
	void Init(CGlass *glass, IReport *rpt);
	void Report(double t, double lat, double lon, double z, int nxyz, double *xyz);
};

#endif
