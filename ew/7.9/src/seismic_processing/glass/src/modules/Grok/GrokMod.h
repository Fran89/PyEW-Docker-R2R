#include "module.h"

class CMapWin;
struct IGlint;
class CMod : public CModule {
// attributes
public:
	IGlint	*pGlint;
	CMapWin *pWin;

  int      bShowPicks;
// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};