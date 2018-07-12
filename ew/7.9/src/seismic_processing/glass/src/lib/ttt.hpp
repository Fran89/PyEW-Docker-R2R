/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ttt.hpp 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.2  2005/10/13 21:55:47  davidk
 *     Added an accuracy parameter to T() and D(), so that accuracies of values
 *     from different tables can be compared.
 *     A value has accuracy of less than 1.0 when it is not a direct linear interpolation
 *     between 4 points in the table.  This normally occurs only at the fringe of the table.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:37  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.2  2004/10/23 07:18:52  davidk
 *     no message
 *
 *     Revision 1.1  2004/10/20 18:22:09  davidk
 *     no message
 *
 *
 *
 */

#ifndef CTTT_H
# define CTTT_H

extern "C" 
{
#include <ttt.h>
}

class CTTT 
{
  public:
// Methods
                CTTT();
                CTTT(char * szFileName);
    virtual    ~CTTT();
    bool        Load(char * szFileName);
    TTEntry *   T(double dZkm, double dDdeg, TTEntry * pTTE);
    TTEntry *   T(double dZkm, double dDdeg, TTEntry * pTTE, double * pdAccuracy);
    TTEntry *   D(double dZkm, double dTsec, TTEntry * pTTE);
    TTEntry *   D(double dZkm, double dTsec, TTEntry * pTTE, double * pdAccuracy);

    TTTableStruct * pTable;  /* make pTable available */

  protected:
    char            szTTTFileName[256];
    int             iNumRows;
    int             iNumDCols;
    int             iNumTCols;
};

#endif  /* ttt */

