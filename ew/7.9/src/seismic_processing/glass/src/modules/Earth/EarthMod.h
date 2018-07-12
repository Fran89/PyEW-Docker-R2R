#include "module.h"
#include <tttlist.hpp>

class CMod : public CModule {
// attributes
public:
  CTTTList List;

// Methods
public:
	CMod();
	~CMod();
	virtual bool Action(IMessage *msg);
	int Init(char *file);
};
