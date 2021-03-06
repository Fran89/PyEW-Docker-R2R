C--ARCPRINT REFORMATS A HYPOINVERSE ARCHIVE FILE INTO A FORMAT SUITABLE
C  FOR PRINTING. HANDLES BOTH OLD & Y2000 FORMAT FILES. 6/04 VERSION.
C--ARCPRINTS IS WRITTEN AS A FILTER & HAS FEWER OPTIONS.
C--BEHAVIOR IS CONTROLLED BY THE FLAG LIA, CHANGE LINES WITH CX:
C  LIA=T  INTERACTIVE, SUMLIST2000, ARCPRINT
C  LIA=F  NOT INTERACTIVE, SUMLIST2000S, ARCPRINTS

C--FORMAT IS SENSED BY THE FIRST SUMMARY LINE WITH THESE CHARACTERISTICS:
C INPUT FORMAT IFOR:
C 2 HYPOINV:	COL 9 NON-BLANK AND COL 17 NOT A DIGIT (NORMALLY BLANK OR 'S')
C 1 HYPOINV-2000:	COL 12 NON-BLANK AND COL 17 A DIGIT

	DIMENSION E(3,3),IAZ(3),IDIP(3),SERR(3)
	CHARACTER STA*4, STA5*1, STAU*5, LSTAU*5, SLOC*2, CDUR4*4
	CHARACTER REMK3*3,REMK1*1,REMK2*1, CXMAG*4,CFMAG*4,CPMAG*4, C4*4
	CHARACTER ARCFIL*80, PRTFIL*80, CARD*132, SCARD*164
	CHARACTER PRMK*3, CPW*1, SRMK*2, CSW*1, CAMP*19
	CHARACTER NET*2, COMP1*1, COMP3*3, MONTH(12)*3
	CHARACTER CXWT*1, CFWT*1, STARMK*1, CDUR*13, CX*14, FMTYP*1
	CHARACTER SOU*1, DSOU(3)*1, CRUS*3, STAR*1
	CHARACTER BMTYP*1, PMTYP*1, FMTYP2*1, FMNOT*1,XMNOT*1
	CHARACTER XMTYP*1, XMTYP2*1, CF*14, VERS*2,AUTH*1
	CHARACTER EVTRMK(10)*14, EVTRM(10)*1, MAGLIN*53, WTCODE*1
	LOGICAL LPT, LASK
	DATA ARCFIL,PRTFIL,RDEG,LPT,ISEQ /2*' ',57.2958,.FALSE.,0/

	PARAMETER (IIN=2,IOUT=3)	!CX
CX	PARAMETER (IIN=5,IOUT=6)	!CX
	LOGICAL LDIG17, LIA
	DATA LIA /.TRUE./		!CX ARCPRINT
CX	DATA LIA /.FALSE./		!CX ARCPRINTS

C--REGION CODES & FULL NAMES
	CHARACTER CD(131)*3, FN(131)*25
	DATA CD /
C--NO. CALIF.
	2 'SFP','BLM','LOM','SAR','BUS','SJB','STN','PIN','HOL','PAI',
	3 'BVL','BIT','SLA','MID','GOL','HAY','MIS','CON','DAN','SUN',
	4 'ALU','SFL','CYN','CYS','QUI','GRN','HAM','ORT','PAN','CRV',
	5 'COA','SFB','SCV','ANN','MON','SAL','SUR','ROB','SSM','SIM',
	6 'MAR','ROG','GEY','TOL','GVL','PAR','MAA','BAR','MEN','EUR',
	7 'DEL','SAC','JQN','KLA','SHA','LAS','ALM','ORO','AUB','YOS',
	8 'WAK','MOB','BRC','KAI','LVC','VOT','RSM','SCA','BAK','ORE',
	9 'MOD','NEV','MCA','PON','POS','GLA','MOL','CHV','WHI','COS',
	9 'IWV','WWF','OWV','DEV','INC','WMO','NMO','EMO','SMO','DOM',
	1 'MAM','HCF','GAR','CAS','RVL','CHA','MOR','SHE','SIL','WCN',
	2 'WCS','SBA',
C-hawaii
	3 'SNC','SSC','SEC','SER','SME','KOA','SSF','SLE','SF1','SF2',
	4 'SF3','SF4','SF5','LER','MLO','LSW','GLN','SWR','INT','KAO',
	5 'DEP','DLS','DML','LOI','HUA','KOH','KEA','HIL','DIS'/

C--NO. CALIF.
	DATA (FN(i),i=1,45) /
	2 'S.F. Peninsula', 'Black Mountain', 'Loma Prieta',
	2 'Sargent Fault', 'Busch Fault', 'San Juan Bautista',
	2 'Stone Canyon', 'Pinnacles', 'Hollister', 'Paicines',
	3 'Bear Valley', 'Bitterwater Valley', 'Slack Canyon',
	3 'Middle Mountain', 'Gold Hill', 'Hayward Fault',
	3 'Mission Fault', 'Concord Fault', 'Danville', 'Sunol',
	4 'Alum Rock', 'San Felipe', 'Coyote North', 'Coyote South',
	4 'Quiensabe', 'Greenville Fault', 'Mt. Hamilton',
	4 'Ortigaleta Fault', 'Panoche Pass', 'Ciervo Hills',
	5 'Coalinga', 'South S.F. Bay', 'Santa Clara Valley', 'Anno Nuevo',
	5 'Monterey Bay', 'Salinas Valley', 'Big Sur (Hosgri Fault)',
	5 'Paso Robles', 'San Simeon', 'Simmler',
	6 'Marin', 'Rogers Creek Fault', 'Geysers', 'Tolay Fault',
	6 'Green Valley Fault'/

	DATA (FN(i),i=46,90) /
     *  'Point Arena', 'Maacama Fault',
     * 'Bartlett Springs Fault', 'Mendocino Escarpment', 'Eureka',
     * 'Del Norte', 'Sacramento Valley', 'San Joaquin Valley',
     * 'Klamath Mountains', 'Shasta', 'Lassen', 'Lake Almanor',
     * 'Oroville', 'Auburn', 'Yosemite',
     * 'Walker Lane', 'Mono Basin %', 'Basin & Range Calif %',
     * 'Kaiser Peak', 'Long Valley Caldera %', 'Volcanic Tableland %',
     * 'Red Slate Mountain %', 'Southern Calif.', 
     * 'Bakersfield', 'Oregon',
     * 'Modoc Plateau', 'Nevada', 
     * 'Mono Caldera (MOB)','Pacific Ocean N',
     * 'Pacific Ocean S', 'Glass Mtn. (MOB)', 'Mono Lake (MOB)',
     * 'Chalfant Valley (BRC)', 'White Mountains (BRC)', 
     * 'Coso Range (BRC)',
     * 'Indian Wells Val. (BRC)', 'White Wolf Fault',
     * 'Owens Valley (BRC)', 'Death Valley (BRC)', 'Inyo Craters (LVC)',
     * 'West Moat (LVC)', 'North Moat (LVC)', 'East Moat (LVC)',
     * 'South Moat (LVC)', 'Resurgent Dome (LVC)'/

	DATA (FN(i),i=91,131) /
	1 'Mammoth Mtn. (LVC)', 'Hilton Crk. Flt. (LVC)', 
	1 'June Lake (LVC)',
	1 'Casa Diablo Mtn. (VOT)', 'Round Valley (VOT)',
	1 'Chalk Bluffs (VOT)',
	1 'Mt. Morrison (RSM)', 'Sherwin Lakes (RSM)', 
	1 'Silver Peak (RSM)',
	1 'Wheeler Crest No. (RSM)', 'Wheeler Crest So. (RSM)',
	1 'Santa Barbara',
C--HAWAII
	3 'Shallow N. Caldera', 'Shallow S. Caldera', 'Shallow E. Caldera',
	3 'Shallow E. Rift', 'Shallow Middle E.R.', 'Koae fault zone',
	3 'Shallow So. flank', 'Shallow Lower E.R.', 'South flank 1', 
	3 'South flank 2',
	4 'South flank 3', 'South flank 4', 'South flank 5',
	4 'Lower East Rift', 'Mauna Loa', 'Lower SW Rifts', 'Glenwood',
	4 'SW Rift', 'Intermed. depth caldera', 'Kaoiki',
	5 'Deep Kilauea', 'Deep lower SW Rift', 'Deep Mauna Loa',
	5 'Loihi', 'Hualalai', 'Kohala', 'Mauna Kea', 'Hilo', 'Distant'/

C--EVENT REMARKS
	DATA EVTRM /'Q','R','E','N','F','M','B','T','L',' '/
	DATA EVTRMK /
	2 'Quarry blast  ','Explosion     ','Explosion     ',
	3 'Nuclear Test  ','Felt          ','Multiple event',
	4 'Blast         ','Tremor assoc. ','Long period   ',' '/

C--MONTHS
	DATA MONTH /'JAN','FEB','MAR','APR','MAY','JUN',
	2 'JUL','AUG','SEP','OCT','NOV','DEC'/
	WTCODE=' '

C--GET & OPEN FILES
2	IF (LIA) THEN
	  CALL ASKC('INPUT HYPOINVERSE ARCHIVE FILE',ARCFIL)
	  CALL OPENR(IIN,ARCFIL,'F',IOS)
	  IF (IOS.GT.0) THEN
	    WRITE (6,*) 'CANT FIND INPUT ARC FILE'
	    STOP
	  END IF

	  CALL ASKC('OUTPUT PRINT FILE',PRTFIL)
	  CALL OPENW (IOUT,PRTFIL,'F',IOS,'S')
	  IF (IOS.GT.0) THEN
	    WRITE (6,*) 'CANT OPEN OUTPUT PRINT FILE'
	    STOP
	  END IF

	  LPT = LASK('PRINT UNWEIGHTED STATIONS',LPT)
	END IF

C--ATTEMPT TO DETERMINE FILE FORMAT & INFORM USER
	READ (IIN,'(A)',END=91,ERR=90) SCARD
	IDIG17=ICHAR(SCARD(17:17))
	LDIG17=IDIG17.GT.47 .AND. IDIG17.LT.58   !T IF A NUMERICAL DIGIT
C	REWIND (IIN)

C--THESE TESTS COULD BE PERFORMED IN ANY ORDER, BUT DO MOST COMMON FIRST:
	IF (LDIG17 .AND. SCARD(12:12).NE.' ') THEN   !HYPOINV-2000
	  IFOR=1
	  IF (LIA) WRITE (6,*) 'READING HYPOINVERSE-2000 FORMAT'
	ELSE IF (.NOT.LDIG17 .AND. SCARD(9:9).NE.' ') THEN   !HYPOINVERSE
	  IFOR=2
	  IF (LIA) WRITE (6,*) 'READING OLD HYPOINVERSE FORMAT'
	ELSE
	  WRITE (6,*) 'UNRECOGNIZED ARCHIVE FORMAT, ARCPRINT QUITS'
	  STOP
	END IF
	GOTO 6

C--READ SUMMARY LINE
5	READ (IIN,1006,END=99) SCARD
C--SKIP IF A SHADOW RECORD
6	IF (SCARD(1:1).EQ.'$') GOTO 5

C--HYPOINVERSE 2000
	IF (IFOR.EQ.1) THEN
	  READ (SCARD,2000,ERR=90) IY,IM,ID,IH,IN,SEC, LAT,XLAT,LON,XLON,
	2 Z,XMAG,NR,IGAP,MIND,RMS, (IAZ(I),IDIP(I),SERR(I),I=1,2),
	3 FMAG,REMK3,SERR(3),REMK1,REMK2, NS,ERH,ERZ,NFM,
	4 XMN,FMN,XMRMS,FMRMS, CRUS, AUTH,DSOU,FMTYP, NVR,XMTYP,
	5 BMTYP,BMAG,BMAGN, XMTYP2,XMAG2,XMAGN2, IDNO,
	6 PMTYP,PMAG,PMAGN, FMTYP2,FMAG2,FMAGN2, VERS

2000	  FORMAT (I4,4I2,F4.2, I2,1X,F4.2,I3,1X,F4.2,
	2 F5.2,F3.2,3I3,F4.2, 2(I3,I2,F4.2),
	3 F3.2,A3,F4.2,2A1, I3,2F4.2,I3,
	4 2F4.1,2F3.2, A3, 5A1, I3,A1,
	5 2(A1,F3.2,F3.1), I10,
	6 2(A1,F3.2,F4.1), A2)

C--OLD HYPOINVERSE
	ELSE IF (IFOR.EQ.2) THEN
	  READ (SCARD,1000,ERR=90) IY,IM,ID,IH,IN,SEC, LAT,XLAT,LON,XLON,Z,
	2 XMAG,NR,IGAP,MIND,RMS, (IAZ(I),IDIP(I),SERR(I),I=1,2),
	3 FMAG,REMK3,SERR(3),REMK1,REMK2, NS,ERH,ERZ,NFM,
	4 XMN,FMN,XMRMS,FMRMS, CRUS, AUTH,DSOU,FMTYP, NVR,XMTYP,
	5 BMTYP,BMAG,BMAGN, XMTYP2,XMAG2,XMAGN2, IDNO,
	6 PMTYP,PMAG,PMAGN, FMTYP2,FMAG2,FMAGN2, VERS

1000	  FORMAT (5I2,F4.2, I2,1X,F4.2,I3,1X,F4.2,F5.2,
	2 F2.1,3I3,F4.2, 2(I3,I2,F4.2),
	3 F2.1,A3,F4.2,2A1, I2,2F4.2,I2,
	4 2F3.1,2F3.2, A3, 5A1, I3,A1,
	5 2(A1,F3.2,F3.1), I10,
	6 2(A1,F3.2,F3.1), A2)
	END IF

C--INITIALIZE FOR EACH EVENT
	LSTAU=' '
	NLPT=0
	IF (IY.LT.1900) IY=IY+1900
	ISEQ=ISEQ+1

C--COMPLETE ERROR ELLIPSE. ORIENTATION OF 3RD STD ERROR IS CROSS PRODUCT
C  OF FIRST TWO
	DO I=1,2
	  E(I,3)=SIN(IDIP(I)/RDEG)
	  T1=SQRT(1.-E(I,3)**2)
	  E(I,1)=COS(IAZ(I)/RDEG)*T1
	  E(I,2)=SIN(IAZ(I)/RDEG)*T1
	END DO

	DO I=1,3
	  J=I+1
	  K=I+2
	  IF (J.GT.3) J=J-3
	  IF (K.GT.3) K=K-3
	  E(3,I)=E(1,J)*E(2,K)-E(1,K)*E(2,J)
	END DO

C--GET THE ORIENTATION OF SERR(3) FROM THE UNIT VECTORS
	IF (E(3,2).EQ.0. .AND. E(3,1).EQ.0.) THEN
	  IAZ(3)=0.
	ELSE
	  IAZ(3)=RDEG* ATAN2 (E(3,2), E(3,1))
	END IF
	IF (IAZ(3).LT.0) IAZ(3)=IAZ(3)+360
	IDIP(3)=RDEG* ABS( ATAN2( E(3,3), SQRT( E(3,1)**2 +E(3,2)**2)))

1004	FORMAT (I1)
1006	FORMAT (A)
1010	FORMAT (I4)

C--CHANGE MAGNITUDES TO ALPHA
	CXMAG='   '
	CFMAG='   '
	CPMAG='   '
	IF (XMAG.GT.0.) WRITE (CXMAG,1001) XMAG
	IF (FMAG.GT.0.) WRITE (CFMAG,1001) FMAG
	IF (PMAG.GT.0.) WRITE (CPMAG,1001) PMAG
1001	FORMAT (F4.2)

	CX=' '
	CF=' '
	IF (XMN.GT.0.) WRITE (CX,1002) XMN,XMRMS,XMTYP
	IF (FMN.GT.0.) WRITE (CF,1002) FMN,FMRMS,FMTYP
1002	FORMAT (F6.1,F6.2,1X,A1)

C--WRITE SUMMARY LOCATION DATA ---------------------------------
C--WRITE IDNO
	WRITE (IOUT,1011) ID,MONTH(IM),IY, IH,IN,
	2 ISEQ,IDNO
1011	FORMAT (/22('####'),/I3,1X,A3,I5,',', I3,':',I2.2,
	2 '  SEQUENCE NO.',I5, ', ID NO. ',I10)

C--WRITE ERROR ELLIPSE
	WRITE (IOUT,1022) (SERR(I),IAZ(I),IDIP(I),I=1,3)
1022	FORMAT ('ERROR ELLIPSE: <SERR AZ DIP>', 3('-<',F6.2,I4,I3,'>'))

C--WRITE LOCATION AND MAGNITUDES
	WRITE (IOUT,1005) IY,IM,ID,IH,IN,SEC, LAT,XLAT,LON,XLON,Z,
	4 RMS,ERH,ERZ, CXMAG,CFMAG,CPMAG, PMTYP,
	7 MIND,CRUS, IGAP,NFM,NR,NS,NVR, REMK3,REMK1,REMK2,VERS,
	8 CX,CF, DSOU 

1005	FORMAT (22('----')/'YEAR MO DA  --ORIGIN--  --LAT N',
	2 '-  --LON W--  DEPTH   RMS   ERH   ERZ  XMAG  FMAG   PMAG'/

	1 I4,'-',I2.2,'-',I2.2,2X,2I2.2,F6.2, I4,F6.2,I5,F6.2,F7.2,
	4 3F6.2, 3(2X,A4), A1,/ 75X,'SOURCE'/

	5 'DMIN MODEL GAP NFM NWR NWS NVR  REMARKS',
	6 '-VE N.XMAG-XMMAD-T  N.FMAG-FMMAD-T  L F X'/

	7 I4,2X,A3, I5,4I4,2X, A3,1X,2A1,2X,A2,1X,
	8 A14,2X,A14, 1X,3(1X,A1)/)

C--WRITE ANY ADDITIONAL MAGNITUDES
	IF (XMAG2.GT.0. .OR. FMAG2.GT.0. .OR. PMAG.GT.0.) THEN
	  MAGLIN=' '

	  IF (XMAG2.GT.0.) WRITE (MAGLIN(1:16),1014) XMAG2,XMAGN2,
	2 XMTYP2,DSOU(3)
	  IF (FMAG2.GT.0.) WRITE (MAGLIN(19:34),1014) FMAG2,FMAGN2,
	2 FMTYP2,DSOU(2)
	  IF (PMAG.GT.0.) WRITE (MAGLIN(40:53),1014) PMAG,PMAGN,PMTYP
1014	  FORMAT (F5.2,F7.2,1X,A1,1X,A1)

	  WRITE (IOUT,1026) MAGLIN
1026	  FORMAT ('XMAG2-N.XMG2-T-S  FMAG2-N.FMG2-T-S',
	2 '  PREF.MAG-N.PMAG-T'/A/)
	END IF

	IF (BMAG.GT.0.) WRITE (IOUT,1025) BMAG,BMAGN,BMTYP
1025	FORMAT ('EXTERNAL (BERKELEY) MAG=',F5.2,
	2'  NUMBER READINGS=',F5.1,'  TYPE=',A1)

C--DETERMINE FULL TRANSLATION OF EVENT REMARK
	DO I=1,10
	  IF (REMK1.EQ.EVTRM(I)) IT=I
	END DO

C--TRNASLATE & WRITE REGION NAME
C  HANDLE KON AS A SPECIAL CASE BECAUSE IT IS USED IN BOTH NETS
	IF (REMK3 .EQ. 'KON') THEN
	  IF (LON.LT.130) THEN
	    WRITE (IOUT,1050) 'Konocti Bay',EVTRMK(IT)
	  ELSE
	    WRITE (IOUT,1050) 'South Kona ',EVTRMK(IT)
	  END IF
	  GOTO 41
	END IF
1050	FORMAT ('REGION= ',A25,'   EVENT REMARK=',A14)
1051	FORMAT ('REGION= ',A25)

	DO I=1,130
	  IF (REMK3 .EQ. CD(I)) THEN
	    IF (REMK1.EQ.' ') THEN
	      WRITE (IOUT,1051) FN(I)
	    ELSE
	      WRITE (IOUT,1050) FN(I),EVTRMK(IT)
	    END IF
	    GOTO 41
	  END IF
	END DO

C--WRITE OTHER REMARKS
	IF (REMK2.EQ.'-') WRITE (IOUT,*) 'DEPTH CONTROL IS POOR, ',
	2 'DEPTH HELD FIXED'
	IF (REMK2.EQ.'#') WRITE (IOUT,*) 'SOLUTION HAD CONVERGENCE ',
	2 'PROBLEMS AND MAY BE POOR'

C--STATION LIST HEADER
41	WRITE (IOUT,1041)
1041	FORMAT(/'STA NET COM L C DIST AZM  AN P/S WT  SEC  (TOBS -TCAL ',
	2 '-DLY  =RES)  WT   SR  INFO DUR-W-FMAG-T   AMP-PER-W-XMAG-T')
	GOTO 10

C--ERROR MESSAGE
93	WRITE (6,1093) CARD
	WRITE (IOUT,1093) CARD
1093	FORMAT (' *** BAD STATION ARCHIVE RECORD:'/1X,A)

C------------------ STATION LOOP ------------------------------
C--READ DATA FOR ONE STATION
10	READ (IIN,1006,END=91) CARD
	C4=CARD(1:4)

C--SKIP IF A SHADOW RECORD
	IF (CARD(1:1).EQ.'$') GOTO 10

C--SKIP CUSP TIME TRACE "STATIONS"
	IF (C4.EQ.'WWVB' .OR. C4.EQ.'IRG1' .OR. C4.EQ.'IRG2' .OR.
	2 C4.EQ.'IRG ') GOTO 10

C--IF THIS IS A TERMINATOR, LIST UNWEIGHTED STAS & LOOK FOR OPTIONAL ID NUMBER
	IF (C4 .EQ. '    ') THEN
	  IF (NLPT.GT.0)
	2 WRITE (IOUT,'(I5,'' UNWEIGHTED STATIONS NOT PRINTED'')') NLPT
	  READ (CARD,1007,ERR=92) IDNO2
1007	  FORMAT (62X,I10)
C	  IF (IDNO2.NE.0) WRITE (IOUT,1008) IDNO2
1008	  FORMAT ('ID NUMBER OF ABOVE EVENT: ',I10)
	  GOTO 5
	END IF

C--DECODE ALL DATA FOR THIS STATION
C--HYPOINVERSE 2000
	IF (IFOR.EQ.1) THEN
	  READ (CARD,2009,ERR=93) STAU,NET,COMP1,COMP3,
	2 PRMK,IPW, JH,JN,P,PRES,PWT, 
	3 S,SRMK,ISW, SRES,RAMP,SWT, PDLY,SDLY,DIST,IAN, CXWT,CFWT,PER,
	4 STARMK,CDUR4,JAZ, FMAG,XMAG,PINFO,SINFO, SOU,
	5 FMAGTYP,XMAGTYP, SLOC,XMNOT,FMNOT

2009	  FORMAT (A5,A2,1X,A1,A3,
	2 1X,A3,I1, 8X,2I2,F5.2,F4.2,F3.2,
	3 F5.2,A2,1X,I1, F4.2,F7.2,2X,F3.2, 2F4.2,F4.1,I3, 2A1,F3.2,
	4 A1,A4,I3, 2F3.2,2F4.3, A1,
	5 2A1, A2,5X,2A1)

C--OLD HYPOINVERSE
	ELSE IF (IFOR.EQ.2) THEN
	  READ (CARD,1009,ERR=93) STA,PRMK,IPW,COMP1, JH,JN,P,PRES,PWT, 
	2 S,SRMK,ISW, SRES,RAMP,SWT, PDLY,SDLY,DIST,IAN, CXWT,CFWT,PER,
	3 STARMK,CDUR4,JAZ, FMAG,XMAG,PINFO,SINFO, WTCODE,SOU,
	4 FMAGTYP,XMAGTYP, STA5,COMP3,NET,SLOC

1009	  FORMAT (A4,A3,I1,A1, 6X,2I2,F5.2,F4.2,F3.2,
	2 F5.2,A2,1X,I1, F4.2,F3.0,F3.2, 2F4.2,F4.1,I3, 2A1,F3.2,
	3 A1,A4,I3, 2F2.1,2F4.3, 2A1,
	4 3A1, A3,2A2)
	  STAU=(STA//STA5)
	  XMNOT=' '
	  FMNOT=' '
	END IF
	
C--TURN SOME OF THE NUMERIC DATA TO ALPHA
	CPW=' '
	IF (IPW.GT.0) WRITE (CPW,1004) IPW

C--SET XMAG INFO
	IF (CXWT.EQ.'0') CXWT=' '
	CAMP=' '

C--MAKE SURE AMPS AND PERIODS ARE WITHIN RANGE
	IF (RAMP.GT.999.) THEN
	  IAMP=RAMP
	  WRITE (CAMP,1016) IAMP,PER,CXWT,XMAG,XMNOT, XMAGTYP
1016	  FORMAT (I6,F4.1,1X,A1,F5.2,2A1)
	ELSE IF (RAMP.GT.0.) THEN
	  WRITE (CAMP,1013) RAMP,PER,CXWT,XMAG,XMNOT, XMAGTYP
1013	  FORMAT (F6.1,F4.1,1X,A1,F5.2,2A1)
	END IF

	IF (PER.LT.1. .AND. RAMP.GT.0.) THEN
	  WRITE (CAMP(7:10),'(F4.2)') PER
	  CAMP(7:7)=' '
	END IF

C--GET NUMERIC DURATION, CONVERT TO INTEGER FOR OUTPUT IF NO FRACTIONAL PART
	READ (CDUR4,'(F4.0)',ERR=93) XDIR
	IDUR=XDIR
	IF ((XDIR-IDUR).EQ.0.) WRITE (CDUR4,'(I4)') IDUR

C--SKIP LISTING UNWEIGHTED STATIONS
	IF (LPT) GOTO 20
	READ (CFWT,'(I1)') IFWT
	READ (CXWT,'(I1)') IXWT

	IF ((IPW.GT.3 .OR. PRMK.EQ.'   ') .AND.
	2   (ISW.GT.3 .OR. SRMK.EQ.'  ') .AND.
	3   (IDUR.LE.0 .OR. IFWT.GT.3) .AND.
	4   (RAMP.EQ.0. .OR. IXWT.GT.3)) THEN
	  NLPT=NLPT+1
	  GOTO 10
	END IF

C--BLANK OUT STATION NAME IF ITS THE SAME AS THE LAST ONE
20	IF (STAU.EQ.LSTAU) THEN
	  STAU=' '
	ELSE
	  LSTAU=STAU
	END IF

C--SET FMAG INFO
	IF (CFWT.EQ.'0') CFWT=' '
	CDUR=' '
	IF (IDUR.GT.0) WRITE (CDUR,1023) CDUR4,CFWT,FMAG,FMNOT,FMAGTYP
1023	FORMAT (A4,1X,A1,F5.2,2A1)

C--FILL IN P TRAVEL TIMES
	TOBS= ((JH-IH)*60 +(JN-IN))*60 +(P-SEC)
	TCAL=TOBS-PDLY-PRES

C--CANCEL 3 TIMES IF THEY ARE BOGUS
	IF (PRES.EQ.-9.99 .OR. PRES.EQ.99.99) THEN
	  PRES=0.
	  TCAL=0.
	  TOBS=0.
	END IF

C--SET THE RESIDUAL FLAG
	STAR=' '
	IF (ABS(PRES) .GT. .5) STAR='*'
	IF (IPW.GE.4) STAR='X'

C--WRITE ONE LINE FOR S DATA AND NO P DATA-------------------
	IF ((IPW.GT.3 .OR. PRMK.EQ.'   ') .AND.
	2   (ISW.LE.3 .AND. SRMK.NE.'  ')) THEN

	  CSW=' '
	  IF (ISW.GT.0) WRITE (CSW,1004) ISW

C--FILL IN S TRAVEL TIMES
	  TOBS=S-SEC
	  TCAL=TOBS-SDLY-SRES

C--CANCEL 3 TIMES IF THEY ARE BOGUS
	  IF (SRES.EQ.-9.99 .OR. SRES.EQ.99.99) THEN
	    SRES=0.
	    TCAL=0.
	    TOBS=0.
	  END IF

C--SET THE RESIDUAL FLAG
	  STAR=' '
	  IF (ABS(SRES) .GT. .5) STAR='*'
	  IF (ISW.GE.4) STAR='X'

C--WRITE THE S DATA WITH THE BASIC STATION DATA
	  WRITE (IOUT,1019) STAU,NET,COMP3,SLOC,COMP1, DIST,JAZ,IAN,
	1 SRMK,CSW,WTCODE,
	2 S,TOBS,TCAL,SDLY,SRES, STAR,SWT,SOU,STARMK, SINFO

1019	  FORMAT (A5,A2,1X,A3,1X,A2,A1, F5.1,2I4, 1X,
	1 A2,2X,2A1,
	2 3F6.2,F5.2,F6.2, A1,F5.2,'S ',2A1, F6.3)
	  GOTO 10
	END IF

C--WRITE THE P DATA------------------------
	WRITE (IOUT,1012) STAU,NET,COMP3,SLOC,COMP1, DIST,JAZ,IAN, PRMK,
	2 CPW,WTCODE, P,TOBS,TCAL,PDLY,PRES, STAR,PWT,SOU,STARMK,
	3 PINFO,CDUR,CAMP

1012	FORMAT (A5,A2,1X,A3,1X,A2,A1, F5.1,2I4, 1X,A3,
	2 1X,2A1, 3F6.2,F5.2,F6.2, A1,F5.2,2X,2A1,
	3 F6.3,A13,A19)

C--ALSO WRITE S DATA ON ANOTHER LINE IF THE S REMARK IS NON BLANK
	IF (SRMK.NE.'  ') THEN
	  CSW=' '
	  IF (ISW.GT.0) WRITE (CSW,1004) ISW

C--FILL IN S TRAVEL TIMES
	  TOBS=S-SEC
	  TCAL=TOBS-SDLY-SRES

C--CANCEL 3 TIMES IF THEY ARE BOGUS
	  IF (SRES.EQ.-9.99 .OR. SRES.EQ.99.99) THEN
	    SRES=0.
	    TCAL=0.
	    TOBS=0.
	  END IF

C--SET THE RESIDUAL FLAG
	  STAR=' '
	  IF (ABS(SRES) .GT. .5) STAR='*'
	  IF (ISW.GE.4) STAR='X'

C--WRITE THE S DATA
	  WRITE (IOUT,1015) SRMK,CSW,WTCODE,
	2 S,TOBS,TCAL,SDLY,SRES, STAR,SWT,SOU,STARMK, SINFO

1015	  FORMAT (29X, A2,2X,2A1,
	2 3F6.2,F5.2,F6.2, A1,F5.2,'S ',2A1, F6.3)
	END IF
	GOTO 10

C--ERROR MESSAGES
90	WRITE (6,1090) SCARD
	WRITE (IOUT,1090) SCARD
1090	FORMAT (' *** BAD SUMMARY (HEADER) RECORD:'/1X,A)
	GOTO 10

91	WRITE (6,1091) IY,IM,ID,IH,IN
1091	FORMAT (' *** PREMATURE END-OF-FILE IN MIDDLE OF EVENT ',5I2)
	STOP

92	WRITE (6,1092) CARD
1092	FORMAT (' *** BAD TERMINATOR RECORD:'/1X,A)
99	STOP
	END
