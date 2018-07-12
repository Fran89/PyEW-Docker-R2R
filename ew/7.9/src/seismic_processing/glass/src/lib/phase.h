/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: phase.h 2174 2006-05-22 15:15:05Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/22 15:15:05  paulf
 *     upgrade with hydra glass
 *
 *     Revision 1.5  2005/11/02 03:46:18  michelle
 *     added more phases to end of list (TT, sPKP, pwP, PKPpre)
 *
 *     Revision 1.4  2005/10/21 16:16:56  michelle
 *     added several phases per Ray. all the phases i added come after SFirst and have a color of 0x00900000
 *
 *     Revision 1.3  2005/10/13 21:52:33  davidk
 *     Added phase pPKPdf
 *
 *     Revision 1.2  2005/10/05 19:52:37  davidk
 *     Changed all PKS and SKP references to SKPdf
 *     Added additional SKPab, and SKPbc traveltime phases.
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:37  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.11  2005/05/31 18:22:22  davidk
 *     Added pPcP phase as witnessed in recent Guatemalan event.
 *
 *     Revision 1.10  2005/05/25 15:38:24  davidk
 *     Added PKPcd(PKiKP) phase type.
 *
 *     Revision 1.9  2004/12/02 17:00:20  davidk
 *     Added Pfirst and Sfirst phases for pick-editing display alignment.
 *
 *     Revision 1.8  2004/11/18 22:59:53  davidk
 *     Updated list of phases.  Changed colors for some phases.
 *     Modified GetPhaseType() declaration to remove a compiler warning.
 *
 *     Revision 1.7  2004/11/10 00:00:27  michelle
 *     added PHASE_sP "sP" to both phase and phase_list
 *
 *     Revision 1.6  2004/11/04 00:58:11  michelle
 *     added GetPhaseColorByType and GetPhaseColorByName
 *
 *     Revision 1.5  2004/10/31 19:07:30  davidk
 *     Added GetNumberOfPhases() function to return the number of phases based on the
 *     size of the Phases array.
 *
 *     Revision 1.4  2004/10/28 22:52:35  davidk
 *     Added Sb.
 *
 *     Revision 1.3  2004/10/27 23:08:14  davidk
 *     Added SKiKP, ScP, and Pb to accomodate locator.
 *
 *     Revision 1.2  2004/10/27 18:33:35  davidk
 *     Added PKPPKP phase.
 *
 *     Revision 1.1  2004/10/22 19:49:25  davidk
 *     no message
 *
 *
 *
 */

#ifndef PHASE_H
# define PHASE_H


typedef enum _PhaseType
{
 PHASE_Unknown,
 PHASE_Pg, PHASE_Pb, PHASE_Pn, PHASE_p, PHASE_P, PHASE_Pdiff,
 PHASE_Sg, PHASE_Sb, PHASE_Sn, PHASE_s, PHASE_S, PHASE_Sdiff,
 PHASE_PP, PHASE_PPP, PHASE_pP, PHASE_PcP, PHASE_pPcP, PHASE_PS, PHASE_ScP,
 PHASE_PKPab, PHASE_PKPbc, PHASE_PKPcd, PHASE_PKPdf, PHASE_PKKPbc, PHASE_PKKPab, PHASE_PKPPKP, 
 PHASE_SKPab, PHASE_SKPbc, PHASE_SKiKP, PHASE_SKPdf,
 PHASE_sP, PHASE_ScS, PHASE_pPKPdf, PHASE_firstP, PHASE_firstS,
 PHASE_SKS, PHASE_SKKS, PHASE_SKKP, PHASE_PKKS, PHASE_PKS, PHASE_PKKPdf, PHASE_PcS,
 PHASE_SP, PHASE_SS, PHASE_pSKS, PHASE_sSKS, PHASE_pS, PHASE_sS, PHASE_SKSSKS,
 PHASE_Lg, PHASE_Rg, PHASE_LR, PHASE_LQ, PHASE_TT, PHASE_sPKP, PHASE_pwP, PHASE_PKPpre
} PhaseType;
/* firstP and firstS are hacked up supersets of phases for use in the displays */



typedef enum _PhaseClass
{
 PHASECLASS_Unknown,
 PHASECLASS_P, PHASECLASS_S, PHASECLASS_Depth, PHASECLASS_Ancil
} PhaseClass;

typedef struct _Phase
{
  char        szName[10];
  PhaseType   iNum;
  PhaseClass  iClass;
  int         iColor;
} Phase;

extern Phase Phases[];   /* defined in phase_list.h */

PhaseType    GetPhaseType(const char * szPhase);
PhaseClass   GetPhaseClass(PhaseType iPhaseNum);
char *       GetPhaseName(PhaseType iPhaseNum);
int          GetNumberOfPhases();
int          GetPhaseColorByType(PhaseType iPhaseNum);
int          GetPhaseColorByName(const char * szPhase);

#endif /* PHASE_H */
/* end Phase */

