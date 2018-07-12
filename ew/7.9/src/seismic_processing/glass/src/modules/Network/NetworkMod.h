#include "module.h"

class CStation;
class CMod : public CModule {
// attributes
public:
	CStation *pStaList;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};