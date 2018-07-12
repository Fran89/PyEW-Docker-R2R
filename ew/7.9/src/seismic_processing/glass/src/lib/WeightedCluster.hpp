// rank.h
#ifndef WEIGHTED_CLUSTER_H
#define WEIGHTED_CLUSTER_H

#define STA_WEIGHT_MAX  5.0
#define STA_MIN_WEIGHT  (1.0/STA_WEIGHT_MAX)

typedef struct _PointStruct
{
  double x,y,z;
} PointStruct;
  

typedef struct _IntersectStruct
{
  PointStruct pt[2];
  int         iNumPts;
  double      dWeight;
} IntersectStruct;

class CWeightedCluster {
public:
// Attributes

// Methods
	CWeightedCluster();
	virtual ~CWeightedCluster();
	static double Cluster(double dReqWeight, int iNumIntersections, 
                        IntersectStruct *IntersectArray, 
                        PointStruct **ppBestPoint);


};

#endif
