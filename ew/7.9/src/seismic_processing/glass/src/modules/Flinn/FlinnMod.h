#include "module.h"

class CFlinn;
class CMod : public CModule {
// attributes
public:
	CFlinn	*pFlinn;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};