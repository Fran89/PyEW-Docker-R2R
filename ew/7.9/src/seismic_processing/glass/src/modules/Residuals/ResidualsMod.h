#include "module.h"

class CCanvas;
class CMod : public CModule {
// attributes
public:
	CCanvas	*pCan;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};