#include "module.h"

class CMod : public CModule {
// attributes
public:

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};