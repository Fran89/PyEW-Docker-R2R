      SUBROUTINE HYTIME (CURTIM)
C
C--RETURNS THE CURRENT TIME AND DATE AS A 24 CHARACTER STRING
C
C   File: hytime.f77
C   Version for Digital Visual Fortran running on Windows NT
C
      USE DFPORT
C
      CHARACTER*24 CURTIM
C
      CURTIM = FDATE()
C
      RETURN
      END
