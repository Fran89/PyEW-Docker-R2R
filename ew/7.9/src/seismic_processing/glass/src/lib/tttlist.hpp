/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: tttlist.hpp 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:37  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/10/31 19:09:06  davidk
 *     Added Load() function to load the traveltime table list from a file (which contains a
 *     list of traveltime table files).
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

#ifndef CTTTLIST_H
# define CTTTLIST_H

extern "C" 
{
#include <phase.h>
}

#include <vector>
#include <ttt.hpp>
 using namespace std;

class CTTTList
{

public:
            CTTTList();
  virtual  ~CTTTList();
  bool      Add(CTTT * pNewTable);
  int       Load(char * szFileName);
  TTEntry * TBest(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE);
  TTEntry * DBest(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE);
  TTEntry * TBestByClass(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE, PhaseClass iClass);
  TTEntry * DBestByClass(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE, PhaseClass iClass);
  TTEntry * TBestByPhase(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE, PhaseType iPhase);
  TTEntry * DBestByPhase(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE, PhaseType iPhase);
  TTEntry * TBestByAll(double dZkm, double dDdeg, double dTsec, TTEntry * pTTE, 
                         PhaseClass iClass, PhaseType iPhase);
  TTEntry * DBestByAll(double dZkm, double dTsec, double dDdeg, TTEntry * pTTE, 
                         PhaseClass iClass, PhaseType iPhase);

protected:
	std::vector<CTTT *> vTableList;

};

#endif /*CTTTLIST_H*/
