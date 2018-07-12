      subroutine hypo_ew( strn, iresr )
c
c  Subroutine hypo_ew calls Fred Klein's subroutine hypoinv.
c  hypoinv() is the function version of hypoinverse.
c  Subroutine hypo_ew is not a standard part of hypoinverse.
c  It is compiled using gfortran, and called by a C function
c  compiled with gcc.

      character*(*) strn
      integer*4 iresr

c  Command argument to HYPOINV must be an 80 character string
      character*80  forstr


c     write (*,*) '--->Call HYPOINV( "', strn, '"[', LEN( strn ),
c    1            '], iresr )'
      forstr = strn
      call hypoinv( forstr, iresr )
c     write (*,*) '--->On return, iresr = ', iresr

      return
      end
