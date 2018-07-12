#include "module.h"

class CGlint;
class CMod : public CModule {
// attributes
public:
	CGlint	*pGlint;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};