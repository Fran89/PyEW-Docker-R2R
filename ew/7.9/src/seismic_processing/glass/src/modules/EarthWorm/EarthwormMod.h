/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: EarthwormMod.h 2176 2006-05-22 16:04:15Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/08/06 00:16:47  davidk
 *     Added code to allow glass to produce global_loc and delete_global_loc
 *     EW messages.
 *
 *     Revision 1.1.1.1  2004/03/31 18:43:22  michelle
 *     New Hydra Import
 *
 *     Revision 1.3  2003/11/07 19:22:00  davidk
 *     Changed Poll() function def to match standard Nexus Poll() call,
 *     instead of being a separate special backdoor call.
 *
 *
 **********************************************************/

#include "module.h"
#include <IGlint.h>

class CRing;
class CMod : public CModule {
// attributes
public:
	unsigned char	cMod;
	unsigned char	cInst;
	unsigned char	cTypePick2K;
	unsigned char	cTypePickGlobal;
	unsigned char	cTypeQuake;
  unsigned char cTypeLocGlobal;
  unsigned char cTypeDelLocGlobal;
	unsigned char	cTypeLink;
	unsigned char	cTypeUnLink;
	unsigned char	cTypeHeartBeat;
	unsigned char	cTypeError;
	double	tPulse;
	int		iPid;
	int		iTimer;
	int		iPulse;
	bool	bFlush;	// Flush input while true
	bool	bLogFile;
	CRing	*pRingIn;
	CRing	*pRingOut;
  int   bLogPicks;  // DK 20030616  Log Picks to disk
	IGlint		*pGlint;
  GLOBAL_LOC_STRUCT  Loc;
  char * pLocBuffer;


// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
//	void Poll(long time);
	bool Poll();
	void Decode(int type, int mod, int src, char *msg);
	int GetKey(char *ring);
	int GetLocalInst();
	int GetType(char *msgtyp);
	int GetModId(char *modnam);
	int GetPar(char *group, char *parnam);
	void Logit(char *msg);
  int LogPick(char * txt);
  int SetPickLogging(int bLog);
};

