#include "module.h"
#include <ITravelTime.h>
#include "glock.h"

struct IGlint;
struct ISolve;
class CGlock;

class CMod : public CModule {
// attributes
public:
	IGlint		*pGlint;
	ITravelTime	*pTT;
	ISolve		*pSolve;
  int       iNumLocatorIterations;
  CGlock * pGlock;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
	int Locate(char *ent, char *mask);
};