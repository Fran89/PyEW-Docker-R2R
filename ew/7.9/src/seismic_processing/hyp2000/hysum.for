      SUBROUTINE HYSUM (IUNIT)
C--CALLED BY HYPOINVERSE TO OUTPUT SUMMARY DATA
C--IUNIT IS THE UNIT NUMBER FOR OUTPUT, 7 FOR ARCHIVE, 12 FOR SUMMARY FILE
      INCLUDE 'common.inc'
      CHARACTER LINE*188,CT1*1, CQ*1, Q*1
      CHARACTER*3 CFMAG2,CXMAG2,CPMAG,CF2MAG,CX2MAG,CBMAG
      CHARACTER CM5*5, CM2*2, CR5*5, CR2*2,CR2A*2, CMODT*1
      DIMENSION KSIG(3), CM5(5), CM2(5), CMODT(5)
      SAVE CMODT
      DATA CMODT /'T','H','V','L','E'/
      
C      CHARACTER*5 ,CF5,CX5,CE5
C      CHARACTER*2 ,CF2,CX2,CE2

C--CONVERT SOME DATA TO INTEGER FOR OUTPUT TO HYPOINVERSE FORMAT
      KLTM=NINT(XLTM*100.)
      KLNM=NINT(XLNM*100.)
      KQ=NINT(T1*100.)
      KZ=NINT(ZREP*100.)
      KZG=NINT(ZGEOID*100.)
      KDMIN=NINT(DMIN)
      IF (KDMIN.GT.999) KDMIN=999
      KRMS=NINT(RMS*100.)
      IF (KRMS.GT.9999) KRMS=9999
      KERH=NINT(ERH*100.)
      KERZ=NINT(ERZ*100.)

      KFMMAD=NINT(100.*FMMAD)
      IF (KFMMAD.GT.999) KFMMAD=999
      KXMMAD=NINT(100.*XMMAD)
      IF (KXMMAD.GT.999) KXMMAD=999
      DO 10 I=1,3
10    KSIG(I)=NINT(SERR(I)*100.)

      NFM2=NFRM			!NEW FORMAT
      IF (NFM2.GT.999) NFM2=999
      NWSR2=NWS
      IF (NWSR2.GT.999) NWSR2=999

C--CONVERT SOME DATA TO INTEGER FOR OUTPUT IN OLD FORMAT
      NFM=NFRM
      IF (NFM.GT.99) NFM=99
      NWSR=NWS
      IF (NWSR.GT.99) NWSR=99

C--CONVERT MAG DATA TO INTEGER FOR OUTPUT, CAN BE NEGATIVE
C--M*MAG* IS THE TOTAL OF STATION WEIGHTS *100 (NO LONGER USED)

C--DUR MAG
      CALL MAG2C3 (FMAG,CFMAG2)
      IF (NFMAG.GT.999) NFMAG=999	

      LFMAG=NINT(FMAG*10.)		!DUR MAG, OLD FORMAT
      IF (LFMAG.LT.-9) LFMAG=-9
      NFMA=NFMAG
      IF (NFMA.GT.99) NFMA=99

C--AMP MAG
      CALL MAG2C3 (XMAG,CXMAG2)
      IF (NXMAG.GT.999) NXMAG=999

      LXMAG=NINT(XMAG*10.)		!AMP MAG, OLD FORMAT
      IF (LXMAG.LT.-9) LXMAG=-9
      NXMA=NXMAG
      IF (NXMA.GT.99) NXMA=99

C--PREFERRED MAG
      CALL MAG2C3 (PMAG,CPMAG)
      IF (NPMAG.GT.999) NPMAG=999	!NEW FORMAT

      NPMAGSH=NPMAG			!PREFERRED MAG, OLD FORMAT
      IF (NPMAGSH.GT.99) NPMAGSH=99

C--ALTERNATE DUR MAG
      CALL MAG2C3 (FMAG2,CF2MAG)
      IF (NFMAG2.GT.999) NFMAG2=999	!NEW FORMAT

      NFMAG2SH=NFMAG2			!ALTERNATE DUR MAG, OLD FORMAT
      IF (NFMAG2SH.GT.99) NFMAG2SH=99

C--XMAG2 THE ALTERNATE AMP MAG
C--OVERWRITE XMAG2 WITH EXTERNAL XMAG (LABEL CODE A) IF IT WAS READ IN
      IF (BMAGX.GT.0. .OR. NBMAGX.GT.0) THEN
        XMAG2=BMAGX
        NXMAG2=NBMAGX
        CT1=BMTYPX
      ELSE
        CT1=LABX2
      END IF

C--ALTERNATE AMP MAG
      CALL MAG2C3 (XMAG2,CX2MAG)
      IF (NXMAG2.GT.99) NXMAG2=99
 
C--ADDITIONAL MAGNITUDES (BOTH FORMATS)
C--EXTERNAL (BERKELEY) MAG
      CALL MAG2C3 (BMAG,CBMAG)
      IF (NBMAG.GT.99) NBMAG=99

C--WRITE A SUMMARY RECORD
C--HYPO71 FORMAT--------------------------------------------------
      IF (IH71S.EQ.2 .AND. IUNIT.EQ.12) THEN
        CALL QUALITY (CQ,RMS,MAXGAP,ERH,ERZ,NWR,ZREP,DMIN)
        
C--YEAR 2000 HYPO71 FORMAT
        IF (L2000) THEN
          WRITE (12,1202) KYEAR2,KMONTH,KDAY,KHOUR,KMIN,T1,LAT,IS,
     2    XLTM,LON,IE,XLNM,ZREP, LABPR,PMAG,
     3    NWR,MAXGAP,DMIN,RMS,ERH,ERZ, RMK1,CQ,SOUCOD,RMK2,
     4    IDNO,CP2,REMK

1202      FORMAT (I4,2I2.2,1X,2I2.2,F6.2,I3,A1,
     2    F5.2,I4,A1,F5.2,F7.2,1X, A1,F5.2,
     3    I3,I4,F5.1,F5.2,2F5.1, 4A1,
     4    I10,1X,A1,A3)

C--OLD HYPO71 FORMAT ENHANCED
        ELSE
          WRITE (12,1002) KYEAR,KMONTH,KDAY,KHOUR,KMIN,T1,LAT,IS,
     2    XLTM,LON,IE,XLNM,ZREP, LABPR,PMAG,
     3    NWR,MAXGAP,DMIN,RMS,ERH,ERZ, RMK1,CQ,SOUCOD,RMK2,
     4    IDNO,CP2

1002      FORMAT (3I2.2,1X,2I2.2,F6.2,I3,A1,
     2    F5.2,I4,A1,F5.2,F7.2,1X, A1,F5.2,
     3    I3,I4,F5.1,F5.2,2F5.1, 4A1,
     4    I10,A1)
        END IF

C--READABLE AND COMMA-DELIM FORMATS----------------------------------
      ELSE IF ((IH71S.EQ.3 .OR. IH71S.EQ.4) .AND. IUNIT.EQ.12) THEN
        CALL QUALITY (CQ,RMS,MAXGAP,ERH,ERZ,NWR,ZREP,DMIN)
        Q=' '
        IF (IH71S.EQ.4) Q=','

C--PREPARE MAGNITUDE STRINGS
C  PREFERRED MAG
        CR5='     '
        CR2='  '
        CR2A='  '
        IF (PMAG.GT.VNOMAG) THEN
          WRITE (CR5,'(F5.2)') PMAG
          CR2=('M'//LABPR)
          CR2A='99'
          IF (NPMAG.LT.100) WRITE (CR2A,'(I2)') NPMAG
        ELSE IF (.NOT.LBLANKMAG) THEN
          WRITE (CR5,'(F5.2)') VNOMAG
          CR2=('M'//LABPR)
          CR2A=' 0'
        END IF

C--0-5 INDIVIDUAL MAGS AT END OF LINE (NOT RUN IF NRDMAG IS 0)
C--OPTIONALLY SWITCH FMAGS 1 & 2 IF EQ IN LAT/LON BOX
        DO I=1,NRDMAG
          IF (MRDMAG(I).EQ.1) THEN
            WMAG=FMAG
            CM2(I)=('M'//LABF1)
            IF (USEMAR .AND. LINBOX .AND. SWITCH12) THEN
              WMAG=FMAG2
              CM2(I)=('M'//LABF2)
            END IF
          ELSE IF (MRDMAG(I).EQ.2) THEN
            WMAG=FMAG2
            CM2(I)=('M'//LABF2)
            IF (USEMAR .AND. LINBOX .AND. SWITCH12) THEN
              WMAG=FMAG
              CM2(I)=('M'//LABF1)
            END IF
          ELSE IF (MRDMAG(I).EQ.3) THEN
            WMAG=XMAG
            CM2(I)=('M'//LABX1)
          ELSE IF (MRDMAG(I).EQ.4) THEN
            WMAG=XMAG2
            CM2(I)=('M'//LABX2)
          ELSE IF (MRDMAG(I).EQ.5) THEN
            WMAG=BMAG
            CM2(I)=('M'//BMTYP)
          END IF
          
          IF (WMAG.GT.VNOMAG) THEN
            WRITE (CM5(I),'(F5.2)') WMAG
          ELSE IF (.NOT.LBLANKMAG) THEN
            WRITE (CM5(I),'(F5.2)') VNOMAG
          ELSE IF (LBLANKMAG) THEN
            CM5(I)='     '
            CM2(I)='  '
          END IF
        END DO

C--WRITE READABLE SUMMARY LINE
        WRITE (12,1203) KYEAR2,KMONTH,KDAY,KHOUR,KMIN,Q, T1,Q,CLAT,Q,
     2  -CLON,Q,ZREP,Q, CR5,Q,CR2,Q,CR2A,Q, 
C     3  NWR,Q,MAXGAP,Q,DMIN,Q,RMS,Q, ERH,Q,ERZ,Q, RMK1,CQ,SOUCOD,RMK2,
     3  NWR,Q,MAXGAP,Q,DMIN,Q,RMS,Q, ERH,Q,ERZ,Q, CQ,RMK2,SOUCOD,RMK1,
     4  Q,IDNO,Q,REMK, (Q,CM5(I),Q,CM2(I),I=1,NRDMAG)

1203    FORMAT (I4,2('/',I2.2),1X,I2.2,':',I2.2,A1, F5.2,A1,F8.4,A1,
     2  F9.4,A1,F6.2,A1, A5,A1,A2,A1,A2,A1,
     3  I3,A1,I3,A1,F5.1,A1,F5.2,A1, 2(F5.1,A1), 4A1,
     4  A1,I10,A1,A3, 5(A1,A5,A1,A2))

      ELSE
C--YEAR 2000 HYPOINVERSE FORMAT-------------------------------------
        IF (L2000) THEN
          WRITE (LINE,1201) KYEAR2,KMONTH,KDAY,KHOUR,KMIN, KQ,LAT,IS,
     2    KLTM,LON,IE,KLNM,KZ, CXMAG2,NWR,MAXGAP, KDMIN,KRMS,
     3    (IAZ(I),IDIP(I),KSIG(I),I=1,2), CFMAG2,REMK,KSIG(3),
     4    RMK1,RMK2,NWSR2, KERH,KERZ,NFM2, NXMAG,NFMAG,KXMMAD,KFMMAD,
     5    CRODE(MOD),CP1, SOUCOD,FMSOU,XMSOU,LABF1, NVR,LABX1,
     6    BMTYP,CBMAG,NBMAG, CT1,CX2MAG,NXMAG2, IDNO,
     7    LABPR,CPMAG,NPMAG, LABF2,CF2MAG,NFMAG2, CP2,CP3,
     8    CDOMAN,CPVERS, CZFLAG,CMODT(MODTYP(MODS(1))+1),IDEPDAT,KZG
     
1201      FORMAT (I4,4I2.2, I4.4,I2,A1,
     2    I4,I3,A1,I4,I5, A3,3I3,I4,
     3    2(I3,I2,I4), 2A3,I4,
     4    2A1,I3, 2I4,I3, 2(I3,'0'),2I3,
     5    A3,A1, 4A1, I3,A1,
     6    2(A1,A3,I2,'0'), I10,
     7    2(A1,A3,I3,'0'), 2A1,
     8    2A2, 2A1,I4,I5)
     
          LENLIN=179

        ELSE
C--OLD HYPOINVERSE FORMAT
          WRITE (LINE,1001) KYEAR,KMONTH,KDAY,KHOUR,KMIN, KQ,LAT,IS,
     2    KLTM,LON,IE,KLNM,KZ, LXMAG,NWR,MAXGAP, KDMIN,KRMS,
     3    (IAZ(I),IDIP(I),KSIG(I),I=1,2), LFMAG,REMK,KSIG(3),
     4    RMK1,RMK2,NWSR, KERH,KERZ,NFM, NXMA,NFMA,KXMMAD,KFMMAD,
     5    CRODE(MOD),CP1, SOUCOD,FMSOU,XMSOU,LABF1, NVR,LABX1,
     6    BMTYP,CBMAG,NBMAG, CT1,CX2MAG,NXMAG2, IDNO,
     7    LABPR,CPMAG,NPMAGSH, LABF2,CF2MAG,NFMAG2SH, CP2,CP3

1001      FORMAT (5I2.2, I4.4,I2,A1,
     2    I4,I3,A1,I4,I5, I2,3I3,I4,
     3    2(I3,I2,I4), I2,A3,I4,
     4    2A1,I2, 2I4,I2, 2(I2,'0'),2I3,
     5    A3,A1, 4A1, I3,A1,
     6    2(A1,A3,I2,'0'), I10,
     7    2(A1,A3,I2,'0'), 2A1)

          LENLIN=154
        END IF

C--WRITE HYPOINVERSE FORMAT LINE
C--WE COULD TRIM LENGTH IF LATER FIELDS WERE UNUSED, BUT FOR NOW SPIT IT ALL
        WRITE (IUNIT,'(A)') LINE(1:LENLIN)
      END IF
      RETURN
      END

      SUBROUTINE QUALITY (CQ,RMS,MAXGAP,ERH,ERZ,NWR,Z1,DMIN)

C--COMPUTE QUALITY LABEL (HYPO71 ONLY) AS AVERAGE OF ERROR AND GEOMETRICAL
C  QUALITIES.
      CHARACTER CQ*1

      IF (RMS.LT.0.15 .AND. ERH.LE.1.0 .AND. ERZ.LE.2.0) THEN
        IQS = 1
      ELSE IF (RMS.LT.0.30 .AND. ERH.LE.2.5 .AND. ERZ.LE.5.0) THEN
        IQS = 2
      ELSE IF (RMS.LT.0.50 .AND. ERH.LE.5.0) THEN
        IQS = 3
      ELSE
        IQS = 4
      ENDIF

      IF (NWR.GE.6 .AND. MAXGAP.LE.90 .AND. 
     1 (DMIN.LE.Z1 .OR. DMIN.LE.5.0)) THEN
        IQD = 1
      ELSE IF (NWR.GE.6 .AND. MAXGAP.LE.135 .AND. 
     1 (DMIN.LE.2.*Z1 .OR. DMIN.LE.10.0)) THEN
	IQD = 2
      ELSE IF (NWR.GE.6 .AND. MAXGAP.LE.180 .AND. DMIN.LE.50.) THEN
        IQD = 3
      ELSE
        IQD = 4
      ENDIF

C--MAKE A LETTER QUALITY 1=A, 2=B, 3=C, 4=D
      IQ = NINT((IQS + IQD)/2.)
      CQ=CHAR(IQ+64)
      RETURN
      END

      SUBROUTINE MAG2C3 (AMAG,CM3)
C--CONVERT A REAL MAG TO 3-CHARACTER STRING
      CHARACTER CM3*3
      I=NINT(AMAG*100.)			!DUR MAG, NEW FORMAT
      IF (I.LT.-99) THEN
        WRITE (CM3,'(F3.0)') AMAG
      ELSE
        WRITE (CM3,'(I3)') I
      END IF
      RETURN
      END
