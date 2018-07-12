// opcalc_const.h

// This file contains the constant assignments for the opcalc library
// routines.  This file should ONLY BE INCLUDED BY opcalc.c
// All other routines making use of these constants should
// include opcalc.h

//USED
extern  int OPCALC_fAffinityStatistics = AFFINITY_USE_NEQ | AFFINITY_USE_RESIDUAL |
                                         AFFINITY_USE_DISTANCE | AFFINITY_USE_GAP;
//USED
extern  double OPCALC_dCutoffMultiple = 4.0;   // modal distance multiplier for dist cutoff.

//USED
extern  double OPCALC_dResidualCutOff = 10.0;  // residual cutoff in seconds
//USED
extern  int    OPCALC_nMinStatPickCount = 10;  // Minimum number of picks required to do
                                             // statistical computations on picks for an
                                             // origin
//USED
extern  double OPCALC_dAffMinAssocValue = 0.9; // minimum affinity value required to associate 
                                             // a pick with an origin.
//USED
extern  double OPCALC_dAffinitySlop   =  0.5;  // residual slop (gray area between 
                                             // inclusion and exclusion)
                                             // included to avoid oscollations between
                                             // including and excluding picks that are
                                             // close to the cutoff.
                                             // Set this high to avoid flip flopping picks in and
                                             // out of an origin around the MinStatPickCount number
                                             // of phases, when the Origin affinity statistics kick in.
// USED
extern  double OPCALC_dSecondaryAssociationPrePickTime  = 2400.0;
                                             // Number of seconds before a pick time to search for
                                             // origins to which it might be associated.  
                                             // 2400 seconds = 40 minutes, enough to pull in P'P'

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994
#define OPCALC_MAX_BELIEVABLE_DISTANCE_FOR_P_PHASE 110.0


/* THIS VALUE IS NOT USED */
extern  double OPCALC_dPrimaryAssociationPrePickOriginTime  = 820.0;
                                             // Origin time as number of seconds before a pick time to attempt
                                             // associate a new origin.  
                                             // 820 ~ 14 minutes   
                                             // (DK approx travetime for 100 deg P wave from surface source)
                                             //   Setting this number to a high value causes a large time window
                                             //   of origins to be tried, and thus involves more computations.
                                             //   Setting the value high, will help to detect moderate sized
                                             //   earthquakes in parts of the world that have limited network
                                             //   coverage (such as Southeast of the Loyalty Islands)
/* THIS VALUE IS NOT USED */

extern  double OPCALC_dPrimaryAssociationPrePickPickTime  = 600.0;
                                             // Number of seconds before a pick time to search for
                                             // picks with which the new pick might associate into an Origin.
                                             // 820 secs (P - 100 degees) - 220 secs (P - 15 degrees) = 600.
                                             //   Setting this number to a high value causes a large time window
                                             //   of picks to be searched, and thus involves more computations.
                                             //   Setting the value high, will help to detect moderate sized
                                             //   earthquakes in parts of the world that have limited network
                                             //   coverage (such as Southeast of the Loyalty Islands)

/* THIS VALUE IS NOT USED */

extern  double OPCALC_dPrimaryAssociationPostPickPickTime  = 600.0;
                                             // Number of seconds after a pick time to search for
                                             // picks with which the new pick might associate into an Origin.
                                             // Set equal to the PrePickPickTime, so that the order
                                             // of pick arrival to the associator is not an issue.
