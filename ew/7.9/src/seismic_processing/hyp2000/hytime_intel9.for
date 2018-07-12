      SUBROUTINE HYTIME (CURTIM)
C
C--RETURNS THE CURRENT TIME AND DATE AS A 24 CHARACTER STRING
C
C   File: hytime.f77
C   Version for Intel Visual Fortran running on Windows XP
C
      USE IFPORT
C
      CHARACTER*24 CURTIM
C
      CALL FDATE(CURTIM)
C
      RETURN
      END
