#include "module.h"

class CScroll;
class CMod : public CModule {
// attributes
public:
	CScroll	*pScroll;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};