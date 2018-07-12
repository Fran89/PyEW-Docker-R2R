#ifndef _SITE_H_
#define _SITE_H_
#include "geo.h"

class CSite : public CGeo {
public:
// Attributes
	char	sName[6];


// Methods
	CSite();
	virtual ~CSite();
};

#endif
