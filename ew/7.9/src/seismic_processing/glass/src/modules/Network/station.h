#ifndef _STATION_H_
#define _STATION_H_

#include "str.h"
#include "IGlint.h"

#define MAXSTA 10000
#define MIN_STATION_QUAL_THRESHOLD .001


class CStation  {
public:

// Attributes
	int		nSta;		// Number of stations loaded
	STATION	Sta[MAXSTA];

// Methods
	CStation();
	virtual ~CStation();
	bool Load();
	bool LoadHypoEllipse(const char *name);
	bool LoadHypoInverse(const char *name);
	const STATION * Get(const SCNL *name);
	bool GetList(STATION ** pStaList, int* pnLen);
	bool Put(const STATION * pStation);


protected:
  void SortStationList();
};


#endif
