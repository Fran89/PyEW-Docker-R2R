#include <module.h>
#include <ITravelTime.h>

class Refocus;
class CMod : public CModule {
// attributes
public:
	Refocus *pRefocus;
	ITravelTime *pTrv;
	bool	bFocus;		// Hypocentral adjustments if Focus succeeded
	double	dZ;
	double	dT;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
};