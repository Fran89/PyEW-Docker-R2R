c
      function ntrbl(ia)
c
c $$$$$ calls no other routine $$$$$
c
c   Ntrbl returns the position of the last non-blank character stored
c   in character variable ia.  If ia is completely blank, then ntrbl
c   returns one so that ia(1:ntrbl(ia)) will always be a legal substring
c   with as many trailing blanks trimmed as possible.
c
      character*(*) ia
      ln=len(ia)
      do 1 i=ln,1,-1
      if(ichar(ia(i:i)).gt.32) go to 2
 1    continue
      i=1
 2    ntrbl=i
      return
      end
c ********************************************************
      subroutine strpph(inphs, phase)
c
c $$$$$ Calls nrtbl $$$$$
c      Subroutine to return the "pure" phase code (without first motion
c     and quality indicators.
c
      character*8 inphs          ! input full phase code
      character*8 phase          ! output "pure" phase code
      character*8 str
 
      integer iend
      integer nqual, nspdr, nlpdr
 
      phase = ' '
      str = inphs
c
c Strip off quality flag
c
      nqual = 0
      if (str(1:1) .eq. 'i') nqual = 1
      if (str(1:1) .eq. 'e') nqual = 2
      if (str(1:1) .eq. 'q') nqual = 3
      if (nqual .gt. 0) str = str(2:)
c
c Strip off long and short period first motion indicators, if any.
c Check for PKPbc, PKKPbc, pP'bc, SKSac etc. But note that PKPbcc or
c     PKPbcd is a possibility. Remember also that Pbc is a legitimate first
c     motion attached to the phase Pb. Note that the LR check is not needed
c     here because the first motion "r" is always lower case in the database.
c
      iend = ntrbl(str)
      nlpdr = 0
      if (str(iend:iend) .eq. 'u') nlpdr = 1
      if (str(iend:iend) .eq. 'r') nlpdr = 2
      if (nlpdr .gt. 0) str(iend:iend) = ' '
 
      iend = ntrbl(str)
      nspdr = 0
      if (str(iend:iend) .eq. 'd') nspdr = 2
      if (str(iend:iend) .eq. 'c' .and. str(max0(1,iend-1):iend) .ne.
     1     'ac' .and. str(max0(1,iend-3):iend) .ne. 'KPbc' .and.
     1     str(max0(1,iend-3):iend) .ne. 'P''bc') nspdr = 1
      if (nspdr .gt. 0) str(iend:iend) = ' '
c
c Strip the "?" if there is one.
c
      iend = ntrbl(str)
      if (str(iend:iend) .eq. '?') str(iend:iend) = ' '
 
      phase = str
      return
      end
c
c
      subroutine FREEUNIT(iunit)
c
c     get a free fortran file-unit number that is not yet in use
c
      implicit none
c
c     Purpose: Get a free FORTRAN-unit
c
c     Output: iunit  a free (file-)unit number
c
c     Urs Kradolfer, 16.7.90
c
      integer*4 min,mout,merr
      common/unit/min,mout,merr
      integer iunit
      logical lopen
c
      do iunit=10,999
         if(iunit.eq.999) then
	    write(merr,*)'FREEUNIT>>> no free unit found!'
	    call exit(102)
         endif
         inquire(unit=iunit,opened=lopen)
         if(.not.lopen) RETURN
      enddo
c
      RETURN
c
      end ! of subr. freeunit
