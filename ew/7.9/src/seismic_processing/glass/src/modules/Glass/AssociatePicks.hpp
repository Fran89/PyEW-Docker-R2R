#ifndef ASSOCIATEPICKS_HPP
# define ASSOCIATEPICKS_HPP

# include <IGlint.h>
# include <ITravelTime.h>

/* pArrivals : list of picks to nucleate (assoc into new origin)
   iNumArrivals:  number of arrivals in list
   pOUT_Origin:   origin information (lat/lon/depth/time, num phases, (maybe RMS)
   return codes:
     EW_SUCCESS - worked
     EW_FAILURE - no origin nucleated
 **********************************************************************************/
double AssociatePicks(double torg, double depth, double *lat, double *lon, 
                      PICK ** Pck, int nPck, ITravelTime	* pTT, double dNCutWeight,
                      bool bUseStaQual=false, bool bLogDetailedInfo=false);

void Debug( char *, char *, ... );          

#endif
