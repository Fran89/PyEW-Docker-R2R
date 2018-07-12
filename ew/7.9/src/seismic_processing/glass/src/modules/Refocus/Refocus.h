#ifndef _REFOCUS_H_
#define _REFOCUS_H_
//#include <win.h>
#include <IGlint.h>

#define MAXPCK 2000

struct IGlint;
class Refocus{
public:
	
// Attributes
	IGlint	*pGlint;
	int		   nPck;
	ORIGIN	 *pOrg;
	PICK	  *pPck[MAXPCK];

// Methods
	Refocus();
	virtual ~Refocus();
	virtual void Focus();
	virtual void Quake(char *ent);

  bool bDeepFocus;
};

#endif
