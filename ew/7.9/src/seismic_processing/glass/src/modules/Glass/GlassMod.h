#include "module.h"


#define GLASS_VERSION_STRING "v1.61 - build 1 20050815"
class CGlass;
class CMonitor;
class CGlass;
struct IReport;
class CMod : public CModule {
// attributes
public:
	HINSTANCE hInst;
	CMonitor *pMon;
	CGlass *pGlass;
	IReport *pReport;
	bool bSpockReport;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
	bool Poll();
};

