/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: phase.c 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:24  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.4  2004/11/18 23:00:43  davidk
 *     Modified GetPhaseType() declaration to remove a compiler warning.
 *
 *     Revision 1.3  2004/11/04 00:58:42  michelle
 *     added GetPhaseColorByType and getPhaseColorByName
 *
 *     Revision 1.2  2004/11/01 17:46:28  davidk
 *     Added GetNumberOfPhases() function to return the number of Phase types, based
 *     on the size of the Phases[] array.
 *
 *     Revision 1.1  2004/10/22 19:51:51  davidk
 *     update do include working version of new traveltime library code.
 *
 *     Revision 1.1  2004/10/20 18:25:36  davidk
 *     no message
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <phase.h>
#include <phase_list.h>
#include <string.h>

PhaseType GetPhaseType(const char * szPhase)
{
  PhaseType iPhase;
  int       iSize;

  iSize = sizeof(Phases) / sizeof(Phase);
  
  for(iPhase = (PhaseType)0; iPhase < iSize; iPhase++)
  {
    if(strcmp(szPhase, Phases[iPhase].szName) == 0)
      return(iPhase);
  }
  return(PHASE_Unknown);
}

int GetPhaseColorByName(const char *szPhase)
{
  PhaseType iPhase;
  iPhase = GetPhaseType(szPhase);
  return GetPhaseColorByType(iPhase);
}

char * GetPhaseName(PhaseType iPhaseNum)
{
  int       iSize;

  iSize = sizeof(Phases) / sizeof(Phase);
  if(iPhaseNum >= iSize)
    return(Phases[PHASE_Unknown].szName);
  else
    return(Phases[iPhaseNum].szName);
}


int GetPhaseColorByType(PhaseType iPhaseNum)
{
  int       iSize;

  iSize = sizeof(Phases) / sizeof(Phase);
  if(iPhaseNum >= iSize)
    return(Phases[PHASE_Unknown].iColor);
  else
    return(Phases[iPhaseNum].iColor);
}

PhaseClass GetPhaseClass(PhaseType iPhaseNum)
{
  int iSize;

  iSize = sizeof(Phases) / sizeof(Phase);
  if(iPhaseNum >= iSize)
    return(Phases[PHASE_Unknown].iClass);
  else
    return(Phases[iPhaseNum].iClass);
}

int GetNumberOfPhases()
{
  return(sizeof(Phases) / sizeof(Phase));
}


