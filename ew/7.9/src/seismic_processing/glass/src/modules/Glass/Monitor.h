/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: Monitor.h 2176 2006-05-22 16:04:15Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:22  michelle
 *     New Hydra Import
 *
 *     Revision 1.2  2003/11/07 22:34:55  davidk
 *     Added RCS Header.
 *     Added new member function UpdateStatus().
 *
 *
 **********************************************************/

#ifndef _MONITOR_H_
#define _MONITOR_H_
#include "win.h"

#define MAXTXT 1000

class CGlass;
class CMonitor : public CWin {
public:
	
// Attributes
	HFONT	hFont;
	CGlass	*pGlass;
	int		hText;
	int		hLine;

// Methods
	CMonitor();
	virtual ~CMonitor();
	virtual void Init(HINSTANCE hinst, char *title);
	virtual void Refresh();
	virtual void Size(int w, int h);
	virtual void Draw(HDC hdc);
	virtual void Line(HDC hdc, char *txt);
	virtual void CatOut();
	        void UpdateStatus();
};

#endif
