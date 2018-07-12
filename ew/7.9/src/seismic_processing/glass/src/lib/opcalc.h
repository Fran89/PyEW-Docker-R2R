#ifndef OPCALC_H
#define OPCALC_H


#ifndef _OPCALC_SO_EXPORT 
# define _OPCALC_SO_EXPORT  __declspec( dllimport)
#endif // _OPCALC_SO_EXPORT

#include <IGlint.h>

#define      OPCALC_MAX_PICKS 2000  
// Maximum number of picks that will ever
// be simultaneously evaluated for an origin

/************************************************
 *  Origin-Pick Affinity components
 ************************************************/



// Use historical origin locations - short term & long term  (higher = better)
#define AFFINITY_USE_ORG_HIST 0x40

// Use pick station quality   (higher=better)  ~ associable picks / total picks  (per channel)
#define AFFINITY_USE_STA_QUAL 0x20

// Use phase probability density based on Dist, Depth, and PhaseType(higher probability = better)
#define AFFINITY_USE_PPD      0x10

// Use the maximum azimuthal gap of the origin (smaller = better)
#define AFFINITY_USE_GAP      0x08

// Use the pick distance from the origin (closer = better) (relative to modal distance)
#define AFFINITY_USE_DISTANCE 0x04

// Use the pick travel-time residual (smaller abs(value) = better)
#define AFFINITY_USE_RESIDUAL 0x02

// Use the number of picks contributing to an origin (more = better)
#define AFFINITY_USE_NEQ      0x01

// Flag indicating which affinity components to use in the overall statistic
_OPCALC_SO_EXPORT  int OPCALC_fAffinityStatistics;

// modal distance multiplier for dist cutoff.
_OPCALC_SO_EXPORT  double OPCALC_dCutoffMultiple;   

  // residual cutoff in seconds
_OPCALC_SO_EXPORT  double OPCALC_dResidualCutOff;

// Minimum number of picks required to do
// statistical computations on picks for an origin
_OPCALC_SO_EXPORT  int    OPCALC_nMinStatPickCount;
 
// minimum affinity value required to associate a pick with an origin.
_OPCALC_SO_EXPORT  double OPCALC_dAffMinAssocValue; 

// residual slop (gray area between 
// inclusion and exclusion)
// included to avoid oscollations between
// including and excluding picks that are
// close to the cutoff.
// Set this high to avoid flip flopping picks in and
// out of an origin around the MinStatPickCount number
// of phases, when the Origin affinity statistics kick in.
_OPCALC_SO_EXPORT  double OPCALC_dAffinitySlop;

// Number of seconds before a pick time to search for
// origins to which it might be associated.  
// 1680 seconds = 28 minutes
_OPCALC_SO_EXPORT  double OPCALC_dSecondaryAssociationPrePickTime;


// Origin time as number of seconds before a pick time to attempt
// associate a new origin.  
// 820 ~ 14 minutes   
// (DK approx travetime for 100 deg P wave from surface source)
//   Setting this number to a high value causes a large time window
//   of origins to be tried, and thus involves more computations.
//   Setting the value high, will help to detect moderate sized
//   earthquakes in parts of the world that have limited network
//   coverage (such as Southeast of the Loyalty Islands)
_OPCALC_SO_EXPORT  double OPCALC_dPrimaryAssociationPrePickOriginTime;

// Number of seconds before a pick time to search for
// picks with which the new pick might associate into an Origin.
// 820 secs (P - 100 degees) - 220 secs (P - 15 degrees) = 600.
//   Setting this number to a high value causes a large time window
//   of picks to be searched, and thus involves more computations.
//   Setting the value high, will help to detect moderate sized
//   earthquakes in parts of the world that have limited network
//   coverage (such as Southeast of the Loyalty Islands)
_OPCALC_SO_EXPORT  double OPCALC_dPrimaryAssociationPrePickPickTime;


// Number of seconds after a pick time to search for
// picks with which the new pick might associate into an Origin.
// Set equal to the PrePickPickTime, so that the order
// of pick arrival to the associator is not an issue.
_OPCALC_SO_EXPORT  double OPCALC_dPrimaryAssociationPostPickPickTime;



_OPCALC_SO_EXPORT  double OPCalc_BellCurve(double xx);
_OPCALC_SO_EXPORT  int    OPCalc_CalcOriginAffinity(ORIGIN * pOrg, PICK ** pPick, int nPck);
_OPCALC_SO_EXPORT  int    OPCalc_CalcPickAffinity(ORIGIN * pOrg, PICK * pPick);
_OPCALC_SO_EXPORT  void   OPCalc_SetTravelTimePointer(ITravelTime * pTravTime);
_OPCALC_SO_EXPORT  int    OPCalc_CalculateOriginPickParams(PICK * pPick, ORIGIN * pOrg);
_OPCALC_SO_EXPORT  int    OPCalc_ProcessOrigin(ORIGIN * pOrg, PICK ** pPick, int nPck);




#endif //#ifndef OPCALC_H






