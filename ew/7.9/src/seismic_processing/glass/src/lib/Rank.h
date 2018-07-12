// rank.h
#ifndef RANK_H
#define RANK_H

class CRank {
public:
// Attributes

// Methods
	CRank();
	virtual ~CRank();
	static int Bundle(int irank, int nxyz, double *xyz, double *best);
	static double Cluster(int irank, int nxyz, double *xyz);
	static double Cluster(int irank, int nxyz, double *xyz, int *ixyz);
	static double Cluster(int irank, int nxyz, double *xyz,
		double x, double y, double z);
};

#endif
