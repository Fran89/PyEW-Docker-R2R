#ifndef IGLINT_H
#define IGLINT_H

#include <ITravelTime.h>

// IGlint.h : Glint interface
#define GLINT_VERSION	1

#define GLINT_STATE_WAIF		0
#define	GLINT_STATE_ASSOC		1

#define SCNL_UNKNOWN_CHAR '-'
#define SCNL_UNKNOWN_STR  "--"
#define PHASE_UNKNOWN_STR "?"

typedef struct _ORIGIN {
	short	iVersion;		// Version control interlock
	char	idOrigin[18];	// Origin id
	int		iOrigin;		// Origin index
	double	dT;				// Origin time (GMT)
	double	dLat;			// Latitude
	double	dLon;			// Longitude
	double	dZ;				// Depth (meters)
	int	 	  nEq;		  // Number of equations in last location
	int		  nPh;		  // Number of phases associated with location
	double	dRms;			// RMS of last location
 	double	dGap;			  // Gap in degrees
	char	  sTag[32];		// Earthworm designator (type.mod.inst.seq)
	double	dDelMin;		// Delta of closest station (degrees)
	double	dDelMod;		// Modal delta (degrees)
	double	dDelMax;		// Delta of furthest station (degrees)
  int     nNextExtProc;
                      // Number of phases when we will next do
                      // extended processing on this quake.
  // CARL120303
 	int		  nLocate;		// Number of times located
 	double	tLocate;		// Time last located

  // DK010704
  double  dAff;       // Summary Affinity Statistic for Origin
  double  dAffGap;    // Affinity Gap Statistic
  double  dAffNumArr; // Affinity Equations used in Origin Calculation
 	char	  idMinPick[18];		// Pick id
 	char	  idMaxPick[18];		// Pick id
  bool    bIsValid;   // Indicates whether or not the Origin is valid

  // DK111904
	double	dFocusedT;				// Origin time (GMT)
	double	dFocusedZ;				// Depth (meters)

} ORIGIN;

typedef struct _PICK {
	short	iVersion;		// Version control interlock
	char	idPick[18];		// Pick id
	int		iPick;			// Pick index
	int		iOrigin;		// Origin index (if associated)
	int		iState;			// State (0=waif, 1=associated)
	double	dT;				// Arrival time (GMT)
	double	dLat;			// Latitude (degress)
	double	dLon;			// Longitude (degrees)
	double	dElev;			// Station elevation (meters)
	char	sTag[32];		// Earthworm designator (type.mod.inst.seq)
	// Follow 3 members (SCN) should probably be replaced with idStation
	char	sSite[6];		// Station name
	char	sComp[5];		// Component
	char	sNet[5];		// Network designator
	char	sLoc[5];		// Location designator
	// Derived parameters (calculated by locater)
	char	sPhase[8];		// Phase description
	double	dTrav;			// Travel time (seconds)
	double	dDelta;			// Distance (degrees)
	double	dAzm;			// Azimuth of station from event
	double	dToa;			// Take off angle
	double	dAff;			// Affinity (associate if < 1.0)
  double  dPickQual; // Pick Quality
  double  dStaQual;  // Station Quality

  // DK010704
  double  tRes;     // Residual time (dT - O->dT - ttt->dT) - saved to limit recomputation
  TTEntry ttt;      // Travel Time Table entry for Origin/Pick
  bool    bTTTIsValid;  // Flag indicating whether ttt info is valid

  // DK011404   Calculated values written to displays
  double dAffRes;     // Affinity statistic - Pick Residual
  double dAffDis;     // Affinity statistic - Pick Distance
  double dAffPPD;    // Affinity statistic - Pick Probability


} PICK;

struct IGlint {
	virtual bool putPick(PICK *pck)						= 0;
	virtual bool putOrigin(ORIGIN *org)					= 0;
	virtual PICK *getPickFromidPick(char *ent)					= 0;
	virtual PICK *getPicksForTimeRange(double t1, double t2, size_t* hRef)			= 0;
	virtual PICK *getWaifsForTimeRange(double t1, double t2, size_t* hRef)			= 0;
	virtual PICK *getPicksFromOrigin(ORIGIN *org, size_t* hRef)					= 0;
	virtual ORIGIN *getOrigin(char *ent)				= 0;
	virtual ORIGIN *getOrigin(double t1, double t2)		= 0;
	//virtual ORIGIN *getOriginFromNum(int iOriginNum) =0;
  virtual ORIGIN *getOriginForPick(PICK * pPick) =0;
	virtual bool OriginPick(char *org, char *pck)		= 0;
	virtual void Pau()									= 0;
  virtual int ComparePickChannels(PICK * pck, PICK * pPick) = 0;
	virtual bool UnOriginPick(char *org, char *pck) = 0;
  virtual void endPickList(size_t * piPickRef) =0;
  virtual bool deleteOrigin(char *ent) =0;


};


#endif
