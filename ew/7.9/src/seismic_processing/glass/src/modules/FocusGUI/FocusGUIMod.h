#include <module.h>
#include <ITravelTime.h>

class CCanvas;
class CMod : public CModule {
// attributes
public:
	CCanvas *pCan;
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