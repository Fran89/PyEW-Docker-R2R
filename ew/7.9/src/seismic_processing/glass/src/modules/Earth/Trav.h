// trav.h : header file
//
#ifndef _TRAV_H_
#define _TRAV_H_
#include "str.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAXPHS 10

class CTerra;
class CTrav {

// Attributes
public:
	CTerra		*pTerra;
	bool	bCreate;
	int		iPhase;		// Phase index set by T(...) and D(...)
	double	dToa;		// Take off angle set by T(...) and D(...)
	char	cTrv[32];	// Table name set by Save() or Restore()
	int		nPhase;
	CStr	cPhase[MAXPHS];
	double	dDelT;
	double	dMinT;
	double	dMaxT;
	double	dDelD;
	double	dMinD;
	double	dMaxD;
	double	dDelZ;
	double	dMinZ;
	double	dMaxZ;
	int		nD;
	int		nT;
	int		nZ;
	double	*dT;	// Travel time table (function of D and Z)
	int		*iT;	// Phase identifier (-1 if null)
	double	*dD;	// Distance table (function of T and Z)
	int		*iD;	// Phase identifier (-1 if null)
	// Layout parameters used in testing and development
	int		ixT;
	int		iyT;
	int		ixTst;
	int		iyTst;
	int		ixRed;
	int		iyRed;
	int		hRed;
	int		wRed;
	int		hTst;
	int		wTst;

// Operations
public:
	CTrav();           // protected constructor used by dynamic creation
	virtual ~CTrav();
	void Init(CTerra *terra);
	void RangeD(double dmin, double dinc, double dmax);
	void RangeZ(double zmin, double zinc, double zmax);
	void Phase(char *phs);
	void Generate();
	void Save(char *file);
	void Load(char *file);
	double T(double d, double z);
	double D(double t, double z);
	void List(char *file);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrav)
	//}}AFX_VIRTUAL

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
