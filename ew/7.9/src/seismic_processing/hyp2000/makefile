#  HYP	    HYPOINVERSE EARTHQUAKE LOCATION PROGRAM.
hyp2000  :  hyp.o hybda.o hycmd.o hysta.o hydel.o hyate.o hyfmc.o hycal.o \
hyxmc.o hycrh.o hycrt.o hystl.o hyopen.o hyinit.o hyphs.o hysou.o \
hycin.o hycout.o hytrl.o hyloc.o hysol.o hysvd.o hytra.o hytrh.o hytrt.o \
hymag.o hyrep.o hylst.o hysum.o hyinp.o hypro.o medwt.o utmcal.o \
init_event.o hydelt.o hyedit.o hytime.o hybeg.o hypref.o hymagp.o \
hyfile.o
	f77 hyp.o hybda.o hycmd.o hysta.o hydel.o hyate.o hyfmc.o hypref.o \
	hyxmc.o hycrh.o hycrt.o hystl.o hyopen.o hyinit.o hyphs.o hycal.o \
	hycin.o hytrl.o hyloc.o hysol.o hysvd.o hytra.o hytrh.o hytrt.o \
	hymag.o hyrep.o hylst.o hysum.o hyinp.o hypro.o medwt.o utmcal.o \
	init_event.o hydelt.o hyedit.o hytime.o hybeg.o hycout.o \
	hymagp.o hysou.o hyfile.o \
	-Bstatic -L/home/calnet/klein/subs -lfs -o hyp2000
# alternate line for loading all "common" subroutines instead of library:
#	allsubs.o -o hyp2000
	cp hyp2000 /home/calnet/klein/bin
	tar cvf hyp`date '+%y%m%d'`.tar *.f* *.inc makefile README* subs/* 

#  HYPOINV  HYPOINVERSE EARTHQUAKE LOCATION SUBROUTINE TEST PROG.
hypoinv2-test  :  hypoinv-test.o hypoinv.o hybda.o hycmd.o hysta.o hydel.o \
hyate.o hyfmc.o hycal.o hysou.o \
hyxmc.o hycrh.o hycrt.o hystl.o hyopen.o hyinit.o hyphs.o \
hycin.o hycout.o hytrl.o hyloc.o hysol.o hysvd.o hytra.o hytrh.o hytrt.o \
hymag.o hyrep.o hylst.o hysum.o hyinp.o hypro.o medwt.o utmcal.o \
init_event.o hydelt.o hyedit.o hytime.o hybeg.o hypref.o
	f77 hypoinv-test.o hypoinv.o hybda.o hycmd.o hysta.o hydel.o \
	hyate.o hyfmc.o hypref.o \
	hyxmc.o hycrh.o hycrt.o hystl.o hyopen.o hyinit.o hyphs.o hycal.o \
	hycin.o hytrl.o hyloc.o hysol.o hysvd.o hytra.o hytrh.o hytrt.o \
	hymag.o hyrep.o hylst.o hysum.o hyinp.o hypro.o medwt.o utmcal.o \
	init_event.o hydelt.o hyedit.o hytime.o hybeg.o hycout.o \
	hymagp.o  hysou.o \
	-L/home/calnet/klein/subs -lfs -o hypoinv2-test

#  HYPOINV  HYPOINVERSE EARTHQUAKE LOCATION SUBROUTINE INTERACTIVE TEST PROG.
hypoinv-test2  :  hypoinv-test2.o hypoinv.o hybda.o hycmd.o hysta.o hydel.o \
hyate.o hyfmc.o hycal.o hysou.o \
hyxmc.o hycrh.o hycrt.o hystl.o hyopen.o hyinit.o hyphs.o \
hycin.o hycout.o hytrl.o hyloc.o hysol.o hysvd.o hytra.o hytrh.o hytrt.o \
hymag.o hyrep.o hylst.o hysum.o hyinp.o hypro.o medwt.o utmcal.o \
init_event.o hydelt.o hyedit.o hytime.o hybeg.o hypref.o
	f77 hypoinv-test2.o hypoinv.o hybda.o hycmd.o hysta.o hydel.o \
	hyate.o hyfmc.o hypref.o \
	hyxmc.o hycrh.o hycrt.o hystl.o hyopen.o hyinit.o hyphs.o hycal.o \
	hycin.o hytrl.o hyloc.o hysol.o hysvd.o hytra.o hytrh.o hytrt.o \
	hymag.o hyrep.o hylst.o hysum.o hyinp.o hypro.o medwt.o utmcal.o \
	init_event.o hydelt.o hyedit.o hytime.o hybeg.o hycout.o \
	hymagp.o  hysou.o \
	-L/home/calnet/klein/subs -lfs -o hypoinv-test2

#  HYP	   * MAIN HYPOINVERSE PROGRAM.
hyp.o  :  hyp.for common.inc
	f77 -c hyp.for

#  HYPOINV_TEST	   * MAIN HYPOINVERSE SUBROUTINE TEST PROG.
hypoinv-test.o  :  hypoinv-test.for
	f77 -c hypoinv-test.for

#  HYPOINV_TEST2	   * MAIN HYPOINVERSE SUBROUTINE TEST PROG.
hypoinv-test2.o  :  hypoinv-test2.for
	f77 -c hypoinv-test2.for

# READQ
readq.o : readq.for
	f77 -c readq.for

#  HYPOINV * MAIN HYPOINVERSE SUBROUTINE.
hypoinv.o  :  hypoinv.for common.inc
	f77 -c hypoinv.for

#  HYBDA   * BLOCK DATA INITIALIZATION OF COMMON.
hybda.o  :  hybda.for common.inc
	f77 -c hybda.for

#  HYBEG   * INITIALIZATION OF OTHER VARIABLES.
hybeg.o  :  hybeg.f common.inc
	f77 -c hybeg.f

#  HYCMD   * GETS AND PROCESSES COMMANDS.
hycmd.o  :  hycmd.for common.inc
	f77 -c hycmd.for

#  HYSTA   * READS IN STATIONS.
hysta.o  :  hysta.for common.inc
	f77 -c hysta.for

#  HYDEL   * READS IN STATION DELAYS (FOR MULTIPLE MODELS).
hydel.o  :  hydel.for common.inc
	f77 -c hydel.for

#  HYATE   * READS IN STATION ATTENUATION HISTORY.
hyate.o  :  hyate.for common.inc
	f77 -c hyate.for

#  HYCAL   * READS IN STATION CAL FACTOR HISTORY.
hycal.o  :  hycal.for common.inc
	f77 -c hycal.for

#  HYFMC   * READS IN STATION FMAG CORRECTIONS.
hyfmc.o  :  hyfmc.for common.inc
	f77 -c hyfmc.for

#  HYFILE   * DETERMINES INPUT FILE TYPE.
hyfile.o  :  hyfile.for
	f77 -c hyfile.for

#  HYXMC   * READS IN STATION XMAG CORRECTIONS.
hyxmc.o  :  hyxmc.for common.inc
	f77 -c hyxmc.for

#  HYCRH   * READS IN HOMOGENOUS LAYER CRUSTAL MODELS.
hycrh.o  :  hycrh.for common.inc
	f77 -c hycrh.for

#  HYCRT   * READS IN TRAVEL-TIME-TABLE CRUSTAL MODELS.
hycrt.o  :  hycrt.for common.inc
	f77 -c hycrt.for

#  HYSTL   * OUTPUTS STATIONS, CRUST & PARAMETERS TO PRINT FILE.
hystl.o  :  hystl.for common.inc
	f77 -c hystl.for

#  HYOPEN  * OPENS FILES FOR LOCATION RUN.
hyopen.o  :  hyopen.for common.inc
	f77 -c hyopen.for

#  HYINIT  * INITIALIZES SOME VARIABLES FOR LOCATION RUN.
hyinit.o  :  hyinit.for common.inc
	f77 -c hyinit.for

#  HYPHS   * READS IN PHASE DATA FOR ONE EVENT.
hyphs.o  :  hyphs.for common.inc
	f77 -c hyphs.for

#  HYCIN   * INPUTS PHASE DATA FROM CUSP MEM FILES (ALTERNATE TO HYPHS)
hycin.o  :  hycin.f
	f77 -c hycin.f

#  HYCOUT   * OUTPUTS PHASE DATA TO CUSP MEM FILES (ALTERNATE TO HYARC)
hycout.o  :  hycout.f
	f77 -c hycout.f

#  HYTRL   * SETS TRIAL HYPOCENTER.
hytrl.o  :  hytrl.for common.inc
	f77 -c hytrl.for

#  HYLOC   * LOCATES ONE EVENT.
hyloc.o  :  hyloc.for common.inc
	f77 -c hyloc.for

#  HYSOL   * PERFORMS INVERSION FOR ONE ITERATION.
hysol.o  :  hysol.for common.inc
	f77 -c hysol.for

#  HYSVD     CANNED SINGLE-VALUE-DECOMPOSITION ROUTINE.
hysvd.o  :  hysvd.f
	f77 -c hysvd.f

#  HYTRA   * MANAGE CRUST MODEL CHOICE & AVERAGING.
hytra.o  :  hytra.for common.inc
	f77 -c hytra.for

#  HYTRH   * CALC TRAVEL TIMES AND DERIVS FOR HOMO LAYER MODEL.
hytrh.o  :  hytrh.for common.inc
	f77 -c hytrh.for

#  HYTRT   * CALC TRAVEL TIMES AND DERIVS FROM TRAV-TIME TABLE.
hytrt.o  :  hytrt.for common.inc
	f77 -c hytrt.for

#  HYMAG   * COMPUTES DUR & MAX AMP MAGNITUDE.
hymag.o  :  hymag.for common.inc
	f77 -c hymag.for

#  HYMAGP   * COMPUTES P AMP MAGNITUDE.
hymagp.o  :  hymagp.for common.inc
	f77 -c hymagp.for

#  HYPREF  * SELECTS PREFERRED MAGNITUDE.
hypref.o  :  hypref.for common.inc
	f77 -c hypref.for

#  HYREP   * REPORTS A LOCATION ON THE TERMINAL.
hyrep.o  :  hyrep.for common.inc
	f77 -c hyrep.for

#  HYSOU   * TABULATES MOST COMMON DATA SOURCES.
hysou.o  :  hysou.for common.inc
	f77 -c hysou.for

#  HYLST   * OUTPUTS DATA BY STATION TO PRINT & ARCHIVE FILES.
hylst.o  :  hylst.for common.inc
	f77 -c hylst.for

#  HYSUM   * OUTPUTS SUMMARY RECORD (FINAL LOCATION).
hysum.o  :  hysum.for common.inc
	f77 -c hysum.for

#  HYINP     FOR INTERACTIVE ENTRY OF PHASE DATA.
hyinp.o  :  hyinp.for
	f77 -c hyinp.for

#  HYPRO   * INTERACTIVELY PROCESSES A SERIES OF EVENTS.
hypro.o  :  hypro.for common.inc
	f77 -c hypro.for

#  MEDWT     COMPUTES THE WEIGHTED MEDIAN OF A SERIES OF MAGNITUDES.
medwt.o  :  medwt.for
	f77 -c medwt.for

#  UTMCAL    CALCULATE STATION DISTANCES ON A UTM GRID
utmcal.o :  utmcal.for
	f77 -c utmcal.for

#  INIT_EVENT  INITIALIZES CUSP FOR READING OF MEM DATA FILES (VAX ONLY)
init_event.o  :  init_event.f
	f77 -c init_event.f

#  HYDELETE  DELETES FILES IN INTERACTIVE PROCESSING
hydelt.o  :  hydelt.f
	f77 -c hydelt.f

#  HYEDTIOR  RUNS AN EDTIOR WITHIN A PROCESS
hyedit.o  :  hyedit.f
	f77 -c hyedit.f

#  HYTIME    GETS CURRENT SYSTEM TIME FOR LABELING PRINT FILE
hytime.o  :  hytime.f
	f77 -c hytime.f

#  ALSO USES THESE GENERAL PURPOSE SUBROUTINES FROM THE SUBROUTINE LIBRARY
#  KLAS      ASSIGNS A NAME AND NUMBER TO AN EVENT BASED ON LOCATION.
#  KLASS (NET 1), BOX2 (NET2) - USED BY KLAS.
#  ISITIN    DETERMINES IF EVENT IS IN A PARTICULAR REGION.
#  UPSTR     CONVERTS A STRING TO UPPER CASE.
#  JASK	     INTERACTIVE PROMPT & ENTRY OF AN INTEGER.
#  ASKC      INTERACTIVE PROMPT AND ENTRY OF A STRING.
#  ASKR      INTERACTIVE PROMPT AND ENTRY OF A REAL VALUE.
#  LASK      INTERACTIVE PROMPT AND ENTRY OF A LOGICAL VALUE.
#  LENG      DETERMINES THE NON-BLANK LENGTH OF A STRING.

