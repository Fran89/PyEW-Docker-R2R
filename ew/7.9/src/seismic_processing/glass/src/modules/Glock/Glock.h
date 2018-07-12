/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: Glock.h 3212 2007-12-27 18:05:23Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.3  2007/12/27 18:05:23  paulf
 *     sync from hydra_proj circa Oct 2007
 *
 *     Revision 1.2  2006/05/04 23:06:33  davidk
 *     Added constructor that takes a Module pointer, instead of just the default
 *     constructor, so the class can have something other than a hokey
 *     global extern link back to its module.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/11/01 06:33:21  davidk
 *     Modified to work with new hydra traveltime library.
 *     Cleaned up struct ITravelTime references.
 *
 *     Revision 1.2  2004/04/01 22:09:18  davidk
 *     v1.11 update
 *     Now utilizes OPCalc routines for calculating Origin/Pick association params.
 *
 *     Revision 1.4  2003/11/07 22:41:05  davidk
 *     Added functions described in previous description.
 *
 *     Revision 1.3  2003/11/07 22:38:59  davidk
 *     Added RCS Header.
 *     Added two member functions: Locate_InitOrigin() and CompareOrigins().
 *
 *
 **********************************************************/

 #ifndef GLOCK_H
#define GLOCK_H

#include <IGlint.h>

#define MAX_PCK 2000

struct ISolve;
class  CMod;
class CGlock {
public:
// Attributes
	int			nIter;
//	int			nEq;
//	double		dRms;
	bool		   bFree[4];	// True if parameter free
	ITravelTime	*pTT;
	IGlint		*pGlnt;
	ISolve		*pSlv;
	int			   nPck;
	ORIGIN		*pOrg;
	PICK		  *pPick[MAX_PCK];
  CMod      *pMod;

// Methods
	CGlock(CMod * pMod);
	virtual ~CGlock();
	int Locate(char *ent, char *mask);
	int Locate(char *ent);
//	int Iterate(bool solve);
  int Iterate(bool solve, ORIGIN * poCurrent, ORIGIN * poNext);
  void Affinity();
protected:
  int  ProcessLocation(bool solve, ORIGIN * pOrg, PICK ** pPck, int nPck);
  void Locate_InitOrigin(ORIGIN * pOrigin, const char * idOrigin);
  int  CompareOrigins(ORIGIN * po1, ORIGIN * po2);
  int  AddPickCoefficientsForSolve(PICK * pPick, const ORIGIN * pOrg);
};

#endif
