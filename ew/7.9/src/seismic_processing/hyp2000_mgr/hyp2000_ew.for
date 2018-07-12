
      subroutine hypo_ew( strn, iresr )
c
c  Subroutine hypo_ew calls Fred Klein's subroutine hypoinv.
c  hypoinv() is the function version of hypoinverse.
c  Subroutine hypo_ew is not a standard part of hypoinverse.
c  It is compiled using Digital Visual FORTRAN, and called
c  by a C function compiled with Microsoft Visual C++.
c
   !DEC$ ATTRIBUTES STDCALL :: hypo_ew
   !DEC$ ATTRIBUTES REFERENCE :: iresr
      integer   strlen
      parameter (strlen=80)
c
c   Arguments to this subroutine
c
      structure /string/
         character*1 c(strlen)
      end structure

      record /string/ strn
      integer*4 iresr
c
c   Local variables
c
      integer i
      character*80 forstr
c
c   Copy the incoming C string to a FORTRAN string
c
      forstr = ' '
c
c      write(6,1000)
c      write(6,1001) iresr
c      write(6,1002) strn
1000  FORMAT ('made it into hypo2000_ew fortran code')
1001  FORMAT ('iresr=', I)
1002  FORMAT ('str=', A)
      do i = 1, strlen
         if ( ichar(strn.c(i)) .eq. 0 ) goto 10
         forstr(i:i) = strn.c(i)
      end do
c
c   Call the subroutine version of hypoinverse.
c   iresr contains the result code.
c
10    call hypoinv( forstr, iresr )
c
      return
      end

