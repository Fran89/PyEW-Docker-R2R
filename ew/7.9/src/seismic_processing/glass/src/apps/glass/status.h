#ifndef _STATUS_H_
#define _STATUS_H_
#include "win.h"

#define MAXSTAT 40

class CStatus : public CWin {
public:
	
// Attributes
	char sAct[2048];
	int nStat;
	int iStat;
	char *sStat[MAXSTAT];
	char *sAction[MAXSTAT];

// Methods
	CStatus();
	virtual ~CStatus();
	virtual void Init(HINSTANCE hinst, char *title);
	virtual void Status(int istat, char *mod, char *msg);
	virtual void Set(char *msg);
	virtual void Draw(HDC hdc);
	virtual void Refresh();
};

#endif
