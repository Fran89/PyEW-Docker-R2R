      subroutine output(nphass)
c
c $$$$$ calls drdwr $$$$$
c
c   Write the reloc output interface file.  Also writes a summary line
c   to the standard output device.  Nphass is the number of associated
c   phases.  Rewritten on 20 July 1983 by R. Buland from a routine by
c   E. R. Engdahl.
c
      implicit none
      include 'locprm.inc'
      character*1 qflg
      integer*4 nphass,nstass,nphuse,nstuse,ios,j
c	integer stdin, stdout
      real*4 gap,delmin,errh,errz,azmgap,azm(maxph)
c   Logical units numbers.
      integer*4 min,mout,merr
      common /unit/ min,mout,merr
c   Physical constants.
      real*4 pi,rad,deg,radius
      common /circle/pi,rad,deg,radius
c   Station sort scratch area.
      character*5 scode
	integer*4 ndx
	common /statmp/ndx(maxph),scode(maxph)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Event level analyst commands.
      include 'cmndcm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
c   Principal axies.
      include 'statcm.inc'
c
c	stdin = 5
c	stdout= 6
      if(nphass.gt.0) then
 
c
c   Update the hypocentral parameters from the computation.  Skip this
c   step if no stations were input as no computations were done.  First 
c   compute geographical latitude and update the longitude.
        glat=deg*atan(1.0067682*(olatc/olats))
        glon=olon
        if(glon.gt.180.) glon=glon-360.
c   Update the origin time, rounding to the nearest millisecond.
        otime=otime+osec+.005
      endif
c
c   Calculate distance to the closest station (delmin) and azimuthal gap.
      nphuse=0
      delmin=360.
c   Loop over phases.
      do j=1,nphass
c   Get station-reading parameters from storage.
        call drdwr(1,j,1,13)
        scode(j)=sta
        if(pflg.eq.'u') then
          delmin=amin1(delmin,delta)
          nphuse=nphuse+1
          azm(nphuse)=deg*azim
        endif
      enddo
      gap=azmgap(nphuse,azm,ndx)
c
c   Count stations.
      call stacnt(nphass,scode,ndx,nstass,nstuse)
c
c   Set the summary quality flag.
      call smqual(nphuse,qflg)
c
c   Calculate errh and errz.
      call sumerr(prax,errh,errz)
c
c   Write the output header.
      write(mout,100,iostat=ios)otime,glat,glon,odep,nstass,nphass,
     1 nstuse,nphuse,int(gap+.5),delmin,(dep.eq.'c')
 100  format(f14.3,1x,f8.4,1x,f9.4,1x,f6.2,4(1x,i4),1x,i3,1x,f6.2,1x,
     1 l1)
c
      write(mout,101,iostat=ios)amin1(setim,999.99),amin1(selat,9999.9),
     1 amin1(selon,9999.9),amin1(sedep,9999.9),amin1(se,999.99),
     2 amin1(errh,9999.9),amin1(errz,9999.9),amin1(avh,9999.9),qflg
 101  format(f6.2,3(1x,f6.1),1x,f6.2,3(1x,f6.1),1x,a1)
c
      write(mout,102,iostat=ios)(amin1(prax(j,3),9999.9),
     1 int(prax(j,1)),int(prax(j,2)),j=1,3)
 102  format(3(f6.1,1x,i3,1x,i3,1x))
c
c   If there were no stations get out.
      if(nphass.le.0) return
c
c   Loop over all phases.
c
      do j=1,nphass
c   Get station-reading parameters from storage.
        call drdwr(1,j,1,13)
c   Write it.
        write(mout,103,iostat=ios)pickid,sta,comp,network,loca,phase,
     1   amin1(amax1(res,-99.9),99.9),delta,int(deg*azim),(pflg.eq.'u')
 103    format(i10,1x,a5,1x,a3,1x,a2,1x,a2,1x,a8,2(1x,f5.1),1x,i3,1x,
     1   l1)
      enddo
      return
      end

      real*4 function azmgap(k,azm,ndx)
c
c   Compute the largest azimuthal gap of a hypocentral solution.  The 
c   station azimuths (in degrees) are stored in azm(k).  Ndx(k) is 
c   scratch storage.
c
      implicit none
      integer*4 k,ndx(*),i
      real*4 azm(*),alst
c
c   Bail if there's no data.
      if(k.le.1) then
        azmgap=360.
        return
      endif
c
c   Sort the data.
      call r4sort(k,azm,ndx)
c
c   Search for the maximum gap between adjacent stations.
      azmgap=0.
      alst=azm(ndx(k))-360.
      do i=1,k
        azmgap=amax1(azmgap,azm(ndx(i))-alst)
        alst=azm(ndx(i))
      enddo
      return
      end

      subroutine stacnt(nphass,scode,ndx,nstass,nstuse)
c
c   Sort station codes scode(nphass) into lexical order and return the 
c   number of stations associated, nstass, and the number of stations 
c   with at least one phase used.
c
      implicit none
      character*5 scode(*)
      character*5 tcode
      logical*4 use
      integer*4 nphass,ndx(*),nstass,nstuse,j
      include 'pickcm.inc'
c
      nstass=0
      nstuse=0
      if(nphass.le.0) return
c
c   Sort the station codes.
      call strsrt(nphass,scode,tcode,ndx)
      tcode=' '
      use=.false.
c
c   Loop over phases.
      do j=1,nphass
c   Get station-reading parameters from storage.
        call drdwr(1,ndx(j),1,13)
        if(sta.ne.tcode) then
c   New code.  Count the last station in nstuse if any phase was used 
c   and count the new station in nstass.
          if(use) nstuse=nstuse+1
          nstass=nstass+1
          tcode=sta
c   Initialize the use flag for the new station.
          if(pflg.eq.'u') then
            use=.true.
          else
            use=.false.
          endif
        else
c   Same station.  See if this phase was used.
          if(pflg.eq.'u') use=.true.
        endif
      enddo
c   Count the last station in nstuse if any phase was used.
      if(use) nstuse=nstuse+1
      return
      end

      subroutine smqual(nphuse,qflg)
c
c   Set the summary quality flag.  Nphuse is the number of phases used. 
c   The quality is based on the number of degrees of freedom and the 
c   equivalent radius of the horizontal projection of the error 
c   ellipsoid, avh.
c
      implicit none
      character*1 qflg
      integer*4 nphuse,nn
      include 'cmndcm.inc'
      include 'statcm.inc'
c
c   Figure the degrees of freedom.
      if(dep.ne.'c') then
        nn=nphuse-3
      else
        nn=nphuse-2
      endif
c
c   Set the quality flag based on avh and the degrees of freedom.
      if(avh.le.8.5.and.nn.gt.2) then
        qflg='A'
      else if(avh.le.16.0.and.nn.gt.1) then
        qflg='B'
      else if(avh.le.60.0.and.nn.gt.0) then
        qflg='C'
      else
        qflg='D'
      endif
c   If the semi-major axis is very large, downgrade the quality.
      if(prax(1,3).gt.300.) qflg='D'
      return
      end

      subroutine sumerr(prax,errh,errz)
c
c   Sumerr converts the Reloc error ellipse, stored in prax(3,3) as 
c   azimuth (deg), plunge or dip (deg), and semi-axis length (km), and 
c   returns maximum horizontal, errh, and vertical, errz, projections of 
c   the axies (km).  Note that all values are appropriate for a 90 th 
c   percentile error ellipsoid (or ellipse for held depth).
c
      implicit none
      real rad
      parameter(rad=1.745329e-2)
      integer*4 j
      real*4 prax(3,3),errh,errz
c
      errh=0.
      errz=0.
      do j=1,3
        errh=amax1(errh,prax(j,3)*cos(rad*prax(j,2)))
        errz=amax1(errz,prax(j,3)*sin(rad*prax(j,2)))
      enddo
      return
      end

      subroutine strsrt(n,key,tmpkey,index)
c
c $$$$$ calls no other routine $$$$$
c
c   Strsrt sorts the n index elements so that key(index(i)), 
c   i = 1, 2, 3, ..., n are in lexical order.  Strsrt is a
c   trivial modification of ACM algorithm 347:  "An efficient
c   algorithm for sorting with minimal storage" by 
c   R. C. Singleton.  Array index is sorted in place in order
c   n*alog2(n) operations.  Coded on 8 march 79 by R. Buland.
c   Modified to handle indexed character data on 21 April 1981
c   by R. Buland.
c
      character*(*) key(n),tmpkey
      dimension il(20),iu(20),index(n)
c   Note:  il and iu implement a stack containing the upper and
c   lower limits of subsequences to be sorted independently.  A
c   depth of k allows for n<=2**(k+1)-1.
      if(n.le.0) return
c   Initialize index to the current order.
      do 14 i=1,n
 14   index(i)=i
      r=.375
      m=1
      i=1
      j=n
c
c   The first section interchanges low element i, middle element ij,
c   and high element j so they are in order.
c
 5    if(i.ge.j) go to 70
 10   k=i
c   Use a floating point modification, r, of Singleton's bisection
c   strategy (suggested by R. Peto in his verification of the
c   algorithm for the ACM).
      if(r.gt..58984375) go to 11
      r=r+.0390625
      go to 12
 11   r=r-.21875
 12   ij=i+(j-i)*r
      if(key(index(i)).le.key(index(ij))) go to 20
      it=index(ij)
      index(ij)=index(i)
      index(i)=it
 20   l=j
      if(key(index(j)).ge.key(index(ij))) go to 39
      it=index(ij)
      index(ij)=index(j)
      index(j)=it
      if(key(index(i)).le.key(index(ij))) go to 39
      it=index(ij)
      index(ij)=index(i)
      index(i)=it
 39   tmpkey=key(index(ij))
      go to 40
c
c   The second section continues this process.  K counts up from i and
c   l down from j.  Each time the k element is bigger than the ij
c   and the l element is less than the ij, then interchange the
c   k and l elements.  This continues until k and l meet.
c
 30   it=index(l)
      index(l)=index(k)
      index(k)=it
 40   l=l-1
      if(key(index(l)).gt.tmpkey) go to 40
 50   k=k+1
      if(key(index(k)).lt.tmpkey) go to 50
      if(k.le.l) go to 30
c
c   The third section considers the intervals i to l and k to j.  The
c   larger interval is saved on the stack (il and iu) and the smaller
c   is remapped into i and j for another shot at section one.
c
      if(l-i.le.j-k) go to 60
      il(m)=i
      iu(m)=l
      i=k
      m=m+1
      go to 80
 60   il(m)=k
      iu(m)=j
      j=l
      m=m+1
      go to 80
c
c   The fourth section pops elements off the stack (into i and j).  If
c   necessary control is transfered back to section one for more
c   interchange sorting.  If not we fall through to section five.  Note
c   that the algorighm exits when the stack is empty.
c
 70   m=m-1
      if(m.eq.0) return
      i=il(m)
      j=iu(m)
 80   if(j-i.ge.11) go to 10
      if(i.eq.1) go to 5
      i=i-1
c
c   The fifth section is the end game.  Final sorting is accomplished
c   (within each subsequence popped off the stack) by rippling out
c   of order elements down to their proper positions.
c
 90   i=i+1
      if(i.eq.j) go to 70
      if(key(index(i)).le.key(index(i+1))) go to 90
      k=i
      kk=k+1
      ib=index(kk)
 100  index(kk)=index(k)
      kk=k
      k=k-1
      if(key(ib).lt.key(index(k))) go to 100
      index(kk)=ib
      go to 90
      end
