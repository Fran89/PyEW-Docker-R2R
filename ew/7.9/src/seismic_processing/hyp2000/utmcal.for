      SUBROUTINE UTMCAL(ORLAT,ORLON,DLAT,DLON,XS,YS,DIST)
C--CALCULATE THE POSITION OF ONE POINT RELATIVE TO ANOTHER ON A UNIVERSAL
C  TRANSVERSE MERCATOR GRID

C      ORLAT AND ORLON ARE THE LATITUDE AND LONGITUDE OF THE REFERENCE POINT
C         IN DECIMAL DEGREES
C      DLAT AND DLON ARE THE LATITUDE AND LONGITUDE OF THE POINT
C         TO BE PROJECTED IN DECIMAL DEGREES
C      XS IS THE X COORDINATE OF THE PROJECTION
C      YS IS THE Y COORDINATE OF THE PROJECTION
C      DIST IS THE DISTANCE BETWEEN THE TWO POINTS

      IMPLICIT REAL*8 (A-H,O-Z)
      REAL*4 ORLAT,ORLON,DLAT,DLON,XS,YS,DIST

      DATA A /6378206.4/
      DATA ESQ /0.00676866/
C      DATA AKO /0.9996/
      DATA AKO /1.0/

C      DEGRAD = 3.14159/180.
      DATA DEGRAD /.0174532/

      OLATD = ORLAT
      OLAT = OLATD*DEGRAD
      OLON = ORLON*DEGRAD
      ALATD = DLAT
      ALAT = ALATD*DEGRAD
      ALON = DLON*DEGRAD
      SLAT = DSIN(ALAT)
      CLAT = DCOS(ALAT)
      TLAT = DTAN(ALAT)

      EPSQ = ESQ/(1.- ESQ)
      BIGN = A/DSQRT(1. - ESQ*SLAT*SLAT)
      BIGT = TLAT*TLAT
      BIGC = EPSQ*CLAT*CLAT
      BIGA = (ALON-OLON)*CLAT

      BIGM  = 111132.0894*ALATD - 16216.94*DSIN(2.*ALAT) + 
     +          17.21*DSIN(4.*ALAT) - 0.02*DSIN(6.*ALAT)
      BIGMO = 111132.0894*OLATD - 16216.94*DSIN(2.*OLAT) + 
     +          17.21*DSIN(4.*OLAT) - 0.02*DSIN(6.*OLAT)
      
      BIGA2 = BIGA*BIGA
      BIGA3 = BIGA2*BIGA
      BIGA4 = BIGA3*BIGA
      BIGA5 = BIGA4*BIGA
      BIGA6 = BIGA5*BIGA

      BIGT2 = BIGT*BIGT
      BIGC2 = BIGC*BIGC

      X = AKO*BIGN*(BIGA + (1. - BIGT + BIGC)*BIGA3/6. + 
     +      (5. - 18.*BIGT + BIGT2 + 72*BIGC - 58.*EPSQ)*BIGA5/120.)

      Y = AKO*(BIGM - BIGMO + BIGN*TLAT*(BIGA2/2. + (5. - 
     +       BIGT + 9.*BIGC + 4.*BIGC2)*BIGA4/24. + (61. - 58.*BIGT + 
     +       BIGT2 + 600.*BIGC - 330.*EPSQ)*BIGA6/720.))

      XS = X/1000.
      YS = Y/1000.
      DIST = SQRT(X*X + Y*Y)/1000.

      RETURN
      END
