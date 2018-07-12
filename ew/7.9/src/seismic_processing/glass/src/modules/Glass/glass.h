/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: glass.h 2176 2006-05-22 16:04:15Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.6  2005/02/28 17:49:28  davidk
 *     Added code to filter out spurious associated phases at time of publication.
 *     Added function PrunePick() so the publisher can communicate to the associator
 *     that a particular pick should be pruned from an origin.
 *     Added a MarkOriginAsScavenged flag to UnassociatePickWOrigin()
 *     in order to control the amount of origin rehashing that gets done.  Now
 *     the origin no longer gets scheduled for rehash when pruned by the publisher.
 *
 *     Revision 1.5  2004/12/02 20:54:46  davidk
 *     Added prototype for AlgValidateOrigin(), a network-specific function for event filtering.
 *
 *     Revision 1.4  2004/11/01 06:31:31  davidk
 *     Modified to work with new hydra traveltime library.
 *
 *     Revision 1.3  2004/08/06 00:25:05  davidk
 *     Added UnPutOrigin() function prototype.
 *     UnPutOrigin() sends out an intra-glass origin-deleted message.
 *
 *     Revision 1.2  2004/04/01 22:05:43  davidk
 *     v1.11 update
 *     Added internal state-based processing.
 *
 *     Revision 1.3  2003/11/07 22:30:39  davidk
 *     Added RCS Header.
 *     Pulled iReportInterval attribute, because it was no longer
 *     neccessary (reverted to v0.0 algorithms).
 *
 *
 **********************************************************/

#ifndef GLASS_H
#define GLASS_H

//#define MAX_ORIGIN  100
//#define MAX_PICK    5000
#include <IGlint.h>
#include <ITravelTime.h>
#include "comfile.h"

#define MAXASS 1000

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994
#define GLASS_RESIDUAL_THRESHOLD 10.0
#define MAXPCK 1000

#define GLASS_LOCATE_NORMAL 1
#define GLASS_LOCATE_FIXED  0
#define GLASS_NO_LOCATE    -1
#define ORIGIN_EXTPROC_INTERVAL 50

typedef struct {
	double	tIdle;		// Time since last pick finished
	double	tAsn;		// Time spent assigning (exclude loc)
	double	tAss;		// Time spent associating (exclude loc)
	double	tLoc;		// Time spent locating
	double	tOther;		// Remaining time used in pick processing
} MON;

// network specific event validation function
extern bool AlgValidateOrigin(ORIGIN * pOrigin, IGlint * pGlint, int nCut);

struct IGlint;
class CMod;
class CMonitor;
class CSummary;  // CARL 120303
class CGlass {
public:
// Attributes
	CMod		*pMod;
	ITravelTime	*pTT;
	IGlint		*pGlint;
  CSummary *pSummary;  // CARL 120303
	bool		bAssociate;		// Associate if true
	bool		bLocate;		// Adjust location if true
  int     nCycle;  // CARL 120303
	// Associator tuning parameters
	int			nCut;			// Number of partial locations in starting cluster
	double		dCut;			// Cluster distance threshold (km)
	double		dTimeBefore;	// Time previous to pick for nucleation consideration
	double		dTimeAfter;		// Time after pick for nucleaation consideration
	double		dTimeOrigin;	// Time before pick containing trail origins
	double		dTimeStep;		// Nucleation time mesh (seconds)
	int			nShell;			// Number of nucleation shells
	double		dShell[25];		// Nucleation shell depths (km)
	// Nucleation statistics
	int			nAss;
	double		dAss[MAXASS];
	// Monitor display parameters
	CMonitor	*pMon;
	char		sPick[128];
	char		sDisp[32];
	int			iLapse;		// Time since last pick (seconds)
	int			nPick;		// Number of picks processed
	int			nAssoc;		// Number of picks associated
	int			nQuake;		// Quakes associated
	double		tDone;		// Time last completed
	MON			Mon[2];
// DK 1.0 PULL    int     iReportInterval;

// Methods
	CGlass();
	virtual ~CGlass();
	bool Params(char *file);
	void Poll();
	void PutOrigin(char *idorg, bool bFull);
  void UnPutOrigin(char *idorg);
	void PutLink(char *idorg, char *idpck);
	void PutUnLink(char *idorg, char *idpck);
	void Origin(double t, double lat, double lon, double depth);
	void Pick(char *sta, char *comp, char *net, char * loc, char *phase,
		double t, double lat, double lon, double elev,
		char *logo, int iseq);
    void Pick(char *site, char *comp, char *cnet, char *phase,
		      double t, double lat, double lon, double elev,
			  char *logo, int iseq);
	void CatOut();
	void Grok(char *idorg);
	int  Locate(char *idorg, char *mask);
	int  Assign(char *idpick);
	int  Waif(char *idorg);
	int Associate(char *idpck);
	double Associate(double t, double z, double *lat, double *lon);
  bool AssociatePickWOrigin(char *idorg, PICK * pck);
  bool UnAssociatePickWOrigin(char *idorg, PICK * pck, bool bMarkOriginAsScavenged);
  bool HandleMatchingPicks(ORIGIN * pOrg, PICK * pPick);
  int  PerformExtendedProcessing(ORIGIN * pOrg);
  bool TrimOutliers(ORIGIN * pOrg);
  int  Scavenge(ORIGIN * pOrg);

  // CARL 120303
	int  Decode(char *act);
	void Task(char *task, int iact, int iActType);
	void Action();
	void Action(int iact, char *ent);
	int Prune(char *idorg);
	int Scavenge(char *idorg);
  int Focus(char *idorg);
  int PrunePick(char *idOrigin, char * idPick);

  bool SetTravelTimePointer(ITravelTime * ptt);
  int  DeleteOrigin(char * idOrigin);
  bool ValidateOrigin(char * idOrigin);
  void InitGlassStateData();
  int  RegisterUnassociatedPick(char * idPick);
  bool ConfigureTransitionRule(CComFile *pcf);
  int  MarkOriginAsScavenged(char * idOrigin);
  int UpdateGUI(char * idOrigin);

};
#endif
