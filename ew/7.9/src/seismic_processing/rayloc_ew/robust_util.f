      subroutine adder(navsum,ihold)
c
c $$$$$ calls drdwr $$$$$
c
c   Adder constructs minus the gradient of the dispersion function and
c   also the mean of the travel-time derivitives. Navsum is the number
c   of data to be used and ihold is the number of constraints.  Ihold
c   will be 0 for a hypocentral step and 1 for an epicentral step.
c   Programmed on 10 December 1983 by R. Buland.
c
      include 'locprm.inc'
      double precision a,b,c,x,s,avdr
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Storage for the normal equations.
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
c   Scores.
      common /sccom/ scgen(maxph)
      data eps/1e-5/
c
      m1=3-ihold
c   Zero out array storage.
      do 1 k=1,m1
      b(k)=0d0
 1    avdr(k)=0d0
c   Loop over the stations accumulating the sums.
      do 2 i=1,navsum
c   Pop one of the sorted residuals.
      ierr=lpop(i,j,res)
c   Access station parameter storage.
      call drdwr(1,j,1,5)
c   C is the contribution to the normal equations.
      c(1)=deg*cos(azim)*dtdd
      c(2)=-deg*sin(azim)*dtdd
      c(3)=dtdh
c   Update the sums.
      do 3 k=1,m1
 3    b(k)=b(k)+scgen(i)*c(k)
      c(2)=c(2)*olats
c     print *,'c',j,(c(k),k=1,3)
      do 6 k=1,m1
 6    avdr(k)=avdr(k)+c(k)
 2    continue
      b(1)=b(1)/radius
      b(2)=b(2)/radius
c   Compute the mean.
      fac=0.
      do 4 k=1,m1
 4    fac=fac+b(k)*b(k)
      cn=1./navsum
      fac=1./sqrt(amax1(fac,eps))
      do 5 k=1,m1
      b(k)=fac*b(k)
 5    avdr(k)=cn*avdr(k)
      return
      end

      subroutine cholin(ndim,n,a,jstage,ifbad)
c
c $$$$$ calls only library routines $$$$$
c
c   Routine for the inversion and factorization of a positive definite,
c   symmetric matrix based on an Algol routine given by Wilkinson and
c   Reinsch, Linear Algebra, p16-17.  Cholin may be used to recover the
c   factor L in the Cholesky factorization A = L*L-trans, or the inverse
c   of L, or the inverse of A found by Cholesky.  Storage has been
c   transposed from the Algol original for the convenience of Fortran
c   users.  As a result, it is convienent to supply L-trans or inverse
c   L-trans when L or inverse L respectively have been called for.  A
c   must be dimension at least A(ndim,n+1).  On entry, the input matrix
c   must occupy the first n columns, or if desired, just the lower left
c   triangle.  On exit, the upper triangle of the array comprised of
c   columns 2,3...n+1 of A contains the desired result.  Jstage
c   indicates the desired calculation:  1 to determine L-trans, 2 to
c   determine inverse L-trans, 0 or 3 to determine inverse A.  If jstage
c   is -2 or -3, then inverse L-trans or inverse A respectively is
c   determined assuming that L-trans has already been found in a
c   previous call to cholsl.  Note that the lower triangle of the
c   orignal A matrix is not destroyed.  Ifbad is an error flag which
c   will be set to 0 if the calculation has been sucessful or to 1 if
c   the matrix is not positive definite.  Translated from Algol by
c   R. L. Parker.
c
      implicit double precision (a-h,o-z)
      dimension a(ndim,1)
c
c   Stage 1:  form factors (L-trans is stored above the diagonal).
c
      ifbad=0
      istage=iabs(jstage)
      if (jstage.lt.0) go to 1550
      do 1500 i=1,n
      i1=i + 1
      do 1400 j=i,n
      j1=j + 1
      x=a(j,i)
      if (i.eq.1) go to 1200
      do 1100 kback=2,i
      k=i-kback+1
 1100 x=x - a(k,j1)*a(k,i1)
 1200 if (i.ne.j) go to 1300
      if (x.le.0d0) go to 4000
      y=1d0/dsqrt(x)
      a(i,i1)=y
      go to 1400
 1300 a(i,j1)=x*y
 1400 continue
 1500 continue
      if (istage.ne.1) go to 2000
 1550 do 1600 i=1,n
 1600 a(i,i+1)=1d0/a(i,i+1)
      if (istage.eq.1) return
c
c   Stage 2:  construct inverse L-trans.
c
 2000 do 2600 i=1,n
      i1=i+1
      if (i1.gt.n) go to 2600
      do 2500 j=i1,n
      z=0d0
      j1=j + 1
      j2=j - 1
      do 2200 kback=i,j2
      k=j2-kback+i
 2200 z=z - a(k,j1)*a(i,k+1)
      a(i,j1)=z*a(j,j1)
 2500 continue
 2600 continue
      if (istage.eq.2) return
c
c   Stage 3:  construct the inverse of A above the diagonal.
c
      do 3500 i=1,n
      do 3500 j=i,n
      z=0d0
      n1=n + 1
      j1=j + 1
      do 3200 k=j1,n1
 3200 z=z + a(j,k)*a(i,k)
      a(i,j+1)=z
 3500 continue
      return
c   Error exit.
 4000 ifbad=1
      return
      end

      subroutine cholsl(ndim,n,a,x,b,istage)
c
c $$$$$ calls no other routine $$$$$
c
c   Routine for solving the n x n linear system A*x = b by back-
c   substitution assuming that A(ndim,n+1) has already been factored by
c   calling cholin with jstage = 1.  See cholin for details on the
c   organization of A.  The right-hand-side vector, b(n), must be given
c   and the unknown vector, x(n), will be determined.  Istage is the
c   mode of the solution:  1 to not complete the back-substitution,
c   returning x = inverse(L-trans)*b; otherwiser complete the solution
c   as described above.  Translated from Algol by R. L. Parker.
c
      implicit double precision (a-h,o-z)
      dimension a(ndim,1),x(n),b(n)
c
      do 2500 i=1,n
      z=b(i)
      if (i.eq.1) go to 2500
      do 2200 kk=2,i
      k=i-kk+1
 2200 z=z - a(k,i+1)*x(k)
 2500 x(i)=z/a(i,i+1)
      if (istage.eq.1) return
c   Complete the back-substitution.
      do 2700 ii=1,n
      i=n-ii+1
      z=x(i)
      i1=i+1
      if (i.eq.n) go to 2700
      do 2600 k=i1,n
 2600 z= z - a(i,k+1)*x(k)
 2700 x(i)=z/a(i,i+1)
      return
      end

      subroutine drdwr(irw,irec,i1,i2)
c
c $$$$$ calls no other routine $$$$$
c
c   Drdwr manages station parameter storage for reloc.  Up to max
c   stations will be stored in main memory.  Additional stations will
c   be stored on disc.  The storage media is transparant to reloc and
c   the parameter max need be set only in drdwr.  If irw = 1, a read
c   into commons /stadat/ and /stadtc/ will be done from station record
c   number irec.  Only elements i1,i1+1,...,i2 of common /stadat/ will
c   be copied.  If irw = 2, a write from the commons will be done to
c   record irec.  The partial transfers are simply to save time on
c   transfers to and from main memory.  Transfers to and from disc are
c   necessarily always complete.  Note that stations should have record
c   numbers:  irec = 1,2,3,...,nsta, where nsta is the number of
c   stations.  Records 1 - max will be in memory and records (max+1) -
c   nsta will be disc records 1 - (nsta-max).  Rewritten by R. Buland on
c   19 July 1983 from a similar routine by E. R. Engdahl.
c
      include 'locprm.inc'
	character*22 flgs,cstor
c   Logical unit numbers.
      common /unit/ min,mout,merr
c   Communication with outside world (station parameter data).
      common /stadat/ sdat(13)
      common /stadtc/ flgs
c   Main memory storage.
      common /bnstor/ rstor(22,maxph)
      common /chstor/ cstor(maxph)
c	integer stdout, stdin
c
c	stdout = 6
c	stdin  = 5
c
c   Branch on operation type.
      if (irw.ne.1) go to 1
c
c   Do a read operation.
c
      if(irec.gt.maxph) go to 2
c   Transfer from memory.
      do 3 i=i1,i2
      sdat(i)=rstor(i,irec)
 3	continue
      flgs=cstor(irec)
      return
c
c   Do a write operation.
c
 1    if(irec.gt.maxph) go to 2
c   Transfer to memory.
      do 5 i=i1,i2
      rstor(i,irec)=sdat(i)
5	continue
      cstor(irec)=flgs
      return
c   Write to error report.
2	write(merr,*)'Maximun stations exceeded!! '
	write(merr,*)'irec= ',irec,' > maxph = ',maxph
	call exit(101)
      end

      function dsprsn(n,a)
c
c $$$$$ calls scgen $$$$$
c
c   Given n residuals, a(1) <= a(2) <= ... <= a(n), dsprsn returns the
c   dispersion of the residuals as defined by Jaeckel,L.A.,1972,
c   "Estimating regression coefficients by minimizing the dispersion of
c   the residuals":AMS,v43,p1449-58.  Scgen is the score generating
c   function which must be nondecreasing and the sum of the scores must
c   be zero.  Programmed on 10 December 1983 by R. Buland.
c
      include 'locprm.inc'
      dimension a(1)
      common/sccom/scgen(maxph)
c
      d=0.
      do 1 i=1,n
 1    d=d+scgen(i)*a(i)
      dsprsn=d
      return
      end

      subroutine errelp(n,m,se)
c
c $$$$$ calls f10 and jacobi $$$$$
c
c   Errelp computes the azimuth, plunge, and semi-length of the
c   principal axes of the three dimensional (two if depth is held)
c   spatial error ellipsoid at a 90% confidence level.  Asymptotically, 
c   the rank-sum estimator produces parameters (i.e., the hypocenter) 
c   that are chi-squared distributed.  An empirical model of 
c   travel-time residuals has been used to compute the constants.  N is 
c   the number of data used, m is the number of degrees of freedom 
c   (including origin time), and se is the standard deviation of the 
c   residuals.  Programmed on 28 October 1983 by R. Buland.
c
      include 'locprm.inc'
      double precision a,b,c,x,s,ev(3,3),emax,ce,tol,tol2,avdr
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Storage for the normal equations.
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c   Principal axies.
      include 'statcm.inc'
      data tol,tol2/1d-15,2d-15/
c
c   Initialize the principal axies.
      do 1 j=1,3
      do 1 i=1,3
 1    prax(i,j)=0.
      avh=0.
c   If all parameters are held, we are done.
      if(m.lt.3) return
c   M1 is the number of degrees of freedom of the spatial projection of
c   the correlation matrix.
      m1=m-1
c   Decompose the correlation matrix.
      call jacobi(m1,3,3,a,b,ev,c,x)
      cn=rsc3
      if(m1.lt.3) cn=rsc2
c
c   Loop over the eigenvalues.
c
      do 2 i=1,m1
c   Sort the eigenvalues into order of decreasing size.
      emax=-1d0
      do 3 j=1,m1
      if(b(j).le.emax) go to 3
      k=j
      emax=b(j)
 3    continue
c   Find the azimuth, plunge, and standard deviation for each axis.
      ce=1d0
      if(m1.gt.2) ce=dsign(1d0,ev(3,k))
      if(dabs(ev(1,k))+dabs(ev(2,k)).gt.tol2) prax(i,1)=deg*datan2(ce*
     1 ev(2,k),-ce*ev(1,k))
      if(prax(i,1).lt.0.) prax(i,1)=prax(i,1)+360.
      if((dabs(ev(3,k)).le.tol).and.(prax(i,1).gt.180.)) prax(i,1)=
     1 prax(i,1)-180.
      if(m1.gt.2) prax(i,2)=deg*dasin(dmin1(ce*ev(3,k),1d0))
      prax(i,3)=cn*dsqrt(dmax1(b(k),0d0))
c   Guarantee that this axis is not found again.
 2    b(k)=-1d0
c
c   Compute avh.
      if(m.le.3) then
c   If depth was held, we already have all the information we need.
        avh=rsc1*sqrt(prax(1,3)*prax(2,3))/cn
      else
c   If depth was free, we need to re-decompose the latitude-longitude
c   correlation sub-matrix.
        call jacobi(2,3,3,a,b,ev,c,x)
        avh=rsc1*dsqrt(dsqrt(dmax1(b(1)*b(2),0d0)))
      endif
      return
      end

      function estlin(navsum,ihold,xx)
c
c $$$$$ calls drdwr $$$$$
c
c   Estlin returns a linear estimate of the dispersion function at a
c   position z(k)=y(k)+xx*x(k), where y(k) is the current position.
c   Programmed on 12 December 1983 by R. Buland.
c
      double precision a,b,c,x,s,avdr
      dimension dr(3)
c
c   Logical units numbers
c
      common /unit/ min,mout,merr
c
c   Storage for the normal equations.
c
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
c
      m1=3-ihold
      n=0
c   Zero out array storage.
      do 1 k=1,m1
 1    dr(k)=xx*x(k)
c     print *,'dr ',dr
c   Loop over the stations accumulating the sums.
      do 2 i=1,navsum
c   Pop one of the sorted residuals.
      ierr=lpop(i,j,res)
c   Access station parameter storage.
      call drdwr(1,j,1,5)
c     print *,'f2 ',j,-cos(azim)*dtdd,sin(azim)*olats*dtdd,-dtdh
      r0=res-(dr(1)*cos(azim)-dr(2)*sin(azim)*olats)*dtdd-dr(3)*dtdh
c     print *,'res ',j,res,r0
      res=r0
 2    ierr=lpush(n,j,res)
      ierr=lsrt(navsum,av,sd,chisq,'TT')
      estlin=chisq
      return
      end

      subroutine jacobi(n,l1,l2,a,d,v,b,z)
c
c $$$$$ calls only library routines $$$$$
c
c   Jacobi finds all eigenvalues and eigenvectors of the n by n real
c   symmetric matrix a by Jacobi's method.  The unordered eigenvalues
c   are returned in real n vector d and the corresponding eigenvectors
c   are in the respective columns of n by n real matrix v.  B and z are
c   scratch arrays.  The arrays must be dimensioned at least a(l,n),
c   d(n), v(n,n), b(n), and z(n).  Note that the inner dimension of
c   array a is l.  Taken from Linear Algebra, Volume II, by Wilkinson
c   and Reinsch, 1971, Springer-Verlag.  The algorithm is published
c   (in algol) by H. Rutishauser on pages 202-211.  Converted to Fortran
c   by R. Buland, 30 October 1978.
c
      implicit double precision (a-h,o-z)
      integer p,q,p1,p2,q1,q2
      double precision a(l1,n),d(n),v(l2,n),b(n),z(n)
      n1=n-1
      nn=n*n
c   Initialize array storage.
      do 1 p=1,n
      do 2 q=1,n
 2    v(p,q)=0d0
      v(p,p)=1d0
      b(p)=a(p,p)
      d(p)=b(p)
 1    z(p)=0d0
c
c   Make up to 50 passes rotating each off diagonal element.
c
      do 3 i=1,50
      sm=0d0
      do 4 p=1,n1
      p1=p+1
      do 4 q=p1,n
 4    sm=sm+dabs(a(p,q))
c   Exit if all off diagonal elements have underflowed.
      if(sm.eq.0d0) go to 13
      tresh=0d0
      if(i.lt.4) tresh=.2d0*sm/nn
c
c   Loop over each off diagonal element.
c
      do 5 p=1,n1
      p1=p+1
      p2=p-1
      do 5 q=p1,n
      q1=q+1
      q2=q-1
      g=1d2*dabs(a(p,q))
c   Skip this element if it has already underflowed.
      if((i.le.4).or.(dabs(d(p))+g.ne.dabs(d(p))).or.
     1 (dabs(d(q))+g.ne.dabs(d(q)))) go to 6
      a(p,q)=0d0
      go to 5
 6    if(dabs(a(p,q)).le.tresh) go to 5
c   Compute the rotation.
      h=d(q)-d(p)
      if(dabs(h)+g.eq.dabs(h)) go to 7
      theta=.5d0*h/a(p,q)
      t=1d0/(dabs(theta)+dsqrt(1d0+theta*theta))
      if(theta.lt.0d0) t=-t
      go to 14
 7    t=a(p,q)/h
 14   c=1d0/dsqrt(1d0+t*t)
      s=t*c
      tau=s/(1d0+c)
c   Rotate the diagonal.
      h=t*a(p,q)
      z(p)=z(p)-h
      z(q)=z(q)+h
      d(p)=d(p)-h
      d(q)=d(q)+h
      a(p,q)=0d0
c   Rotate the off diagonal.  Note that only the upper diagonal elements
c   are touched.  This allows the recovery of matrix a later.
      if(p2.lt.1) go to 15
      do 8 j=1,p2
      g=a(j,p)
      h=a(j,q)
      a(j,p)=g-s*(h+g*tau)
 8    a(j,q)=h+s*(g-h*tau)
 15   if(q2.lt.p1) go to 16
      do 9 j=p1,q2
      g=a(p,j)
      h=a(j,q)
      a(p,j)=g-s*(h+g*tau)
 9    a(j,q)=h+s*(g-h*tau)
 16   if(n.lt.q1) go to 17
      do 10 j=q1,n
      g=a(p,j)
      h=a(q,j)
      a(p,j)=g-s*(h+g*tau)
 10   a(q,j)=h+s*(g-h*tau)
c   Rotate the eigenvector matrix.
 17   do 11 j=1,n
      g=v(j,p)
      h=v(j,q)
      v(j,p)=g-s*(h+g*tau)
 11   v(j,q)=h+s*(g-h*tau)
 5    continue
c   Reset the temporary storage for the next rotation pass.
      do 3 p=1,n
      d(p)=b(p)+z(p)
      b(p)=d(p)
 3    z(p)=0d0
c
c   All finished.  Prepare for exiting by reconstructing the upper
c   triangle of a from the untouched lower triangle.
c
 13   do 12 p=1,n1
      p1=p+1
      do 12 q=p1,n
 12   a(p,q)=a(q,p)
      return
      end

      function lsrt(l,av,sd,chisq,tag)
c
c   Function Lsrt sorts the l data in array del2 (reordering ind2
c   accordingly) and computes the series median, av, spread, sd, and
c   dispersion, chisq.  Tag is a two character ID printed with errors 
c   to help identify the source of the problem.
c
c   At most, min2 data may be considered.
      include 'locprm.inc'
      parameter (min2=maxph)
      character*2 tag
      character*1 pflg
      character*20 alpha
	common /unit/ min, mout, merr
c   Elimination scratch storage.
      common /elimsc/ ind2(min2),del2(min2)
c   Station flags.
      common /stadtc/ alpha,pflg
c
c   Assume success.
      lsrt=1
c   Check the number of data.
      if(l-1)16,17,18
c   No data.
 16   av=0.
      lsrt=-1
      return
c   One data.
 17   av=del2(1)
      return
c   More than one datum.  Issue an error message if necessary.
 18   if(mfl.gt.0) write(merr,100)mfl,tag
 100  format(/1x,'Reloc:  scratch storage full -',i4,1x,a,'''s ',
     1 'residualed.'/)
c   Sort all data in the scratch storage.
      call magsrt(l,del2,ind2)
c   Compute the median, spread, and dispersion.
      av=xmed(l,del2,sd)
      chisq=dsprsn(l,del2)
      return
c
c   Entry lpush increments storage pointer n and saves index, ndex, and
c   datum, dat, in arrays ind2 and del2.  If successful, +1 is returned.  
c   If no more storage is available, -1 is returned, pflg is set to 
c   residual, and a the error count is bumped.
c
      entry lpush(n,ndex,dat)
c   Assume success.
      lpush=1
c   On the first datum, initialize the error count.
      if(n.le.0) mfl=0
c   See if elimination scratch storage is full.
      if(n.ge.min2) go to 13
c   Push the datum onto the stack.
      n=n+1
      ind2(n)=ndex
      del2(n)=dat
      return
c   Scratch storage is full.  Set the error flag.
 13   lpush=-1
c   Change the bad datum flag from 'u' to ' '.
      pflg=' '
c   Up the error count.
      mfl=mfl+1
      return
c
c   Entry lpop returns the n th saved values of index, ndex, and datum,
c   dat, from arrays ind2 and del2.  If successful, +1 is returned.  If
c   n is beyond array storage, -1 is returned.
c
      entry lpop(n,ndex,dat)
c   Assume success.
      lpop=1
c   See if n is beyond array storage.
      if(n.ge.min2) go to 15
c   Pop the datum from the stack.
      ndex=ind2(n)
      dat=del2(n)
      return
c   N is too big.  Set the error flag.
 15   lpop=-1
      return
      end

      subroutine lintry(navsum,ihold,chisq,deld,dmin,dmax)
c
c $$$$$ calls drdwr $$$$$
c
c   Lintry uses a Taylor series linearization of travel time to find a
c   minimum in the dispersion function in the direction of the travel-
c   time gradient.  Programmed on 12 December 1983 by R. Buland.
c
      double precision a,b,c,x,s,avdr
c   Storage for the normal equations.
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c
      m1=3-ihold
      del2=deld
      x0=0.
      d0=chisq
      x1=deld
      d1=estlin(navsum,ihold,x1)
      if(d0.lt.d1) go to 1
      x2=2.*deld
      d2=estlin(navsum,ihold,x2)
 3    if(d1.lt.d2) go to 2
      x0=x1
      d0=d1
      x1=x2
      d1=d2
      if(x1.ge.dmax) go to 13
      del2=2.*del2
      x2=amax1(x1+del2,dmax)
      d2=estlin(navsum,ihold,x2)
      go to 3
 1    x2=x1
      d2=d1
      x1=.5*(x0+x2)
      d1=estlin(navsum,ihold,x1)
      if(x1.le.dmin) go to 14
      if(d0.lt.d1) go to 1
 2    xx=.5*(x0+x1)
      if((x2-x0)/x1.le..15.or.x2-x0.le.dmin) go to 13
      dd=estlin(navsum,ihold,xx)
      if(dd.lt.d1) go to 4
      x0=xx
      d0=dd
      xx=.5*(x1+x2)
      dd=estlin(navsum,ihold,xx)
      if(dd.lt.d1) go to 5
      x2=xx
      d2=dd
      go to 2
 4    x2=x1
      d2=d1
      x1=xx
      d1=dd
      go to 2
 5    x0=x1
      d0=d1
      x1=xx
      d1=dd
      go to 2
 14   if(d1.lt.d0) go to 13
      x1=0.
      d1=chisq
 13   do 6 i=1,m1
 6    x(i)=x1*x(i)
      deld=x1
      return
      end

      subroutine magsrt(n,rkey,ndex)
c
c $$$$$ calls no other routine $$$$$
c
c   Magsrt sorts the n elements of array rkey so that rkey(i),
c   i = 1, 2, 3, ..., n are in asending order.  Auxillary integer*4
c   array ndex is sorted in parallel.  Magsrt is a trivial modification
c   of ACM algorithm 347:  "An efficient algorithm for sorting with
c   minimal storage" by R. C. Singleton.  Array rkey (and ndex) is
c   sorted in place in order n*alog2(n) operations.  Coded on
c   8 March 1979 by R. Buland.  Modified to handle real*4 data on
c   27 September 1983 by R. Buland.
c
      dimension rkey(1),ndex(1),il(10),iu(10)
c   Note:  il and iu implement a stack containing the upper and
c   lower limits of subsequences to be sorted independently.  A
c   depth of k allows for n<=2**(k+1)-1.
      if(n.le.1) return
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
      if(rkey(i).le.rkey(ij)) go to 20
      rit=rkey(ij)
      rkey(ij)=rkey(i)
      rkey(i)=rit
      it=ndex(ij)
      ndex(ij)=ndex(i)
      ndex(i)=it
 20   l=j
      if(rkey(j).ge.rkey(ij)) go to 39
      rit=rkey(ij)
      rkey(ij)=rkey(j)
      rkey(j)=rit
      it=ndex(ij)
      ndex(ij)=ndex(j)
      ndex(j)=it
      if(rkey(i).le.rkey(ij)) go to 39
      rit=rkey(ij)
      rkey(ij)=rkey(i)
      rkey(i)=rit
      it=ndex(ij)
      ndex(ij)=ndex(i)
      ndex(i)=it
 39   tmpkey=rkey(ij)
      go to 40
c
c   The second section continues this process.  K counts up from i and
c   l down from j.  Each time the k element is bigger than the ij
c   and the l element is less than the ij, then interchange the
c   k and l elements.  This continues until k and l meet.
c
 30   rit=rkey(l)
      rkey(l)=rkey(k)
      rkey(k)=rit
      it=ndex(l)
      ndex(l)=ndex(k)
      ndex(k)=it
 40   l=l-1
      if(rkey(l).gt.tmpkey) go to 40
 50   k=k+1
      if(rkey(k).lt.tmpkey) go to 50
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
      if(rkey(i).le.rkey(i+1)) go to 90
      k=i
      kk=k+1
      rib=rkey(kk)
      ib=ndex(kk)
 100  rkey(kk)=rkey(k)
      ndex(kk)=ndex(k)
      kk=k
      k=k-1
      if(rib.lt.rkey(k)) go to 100
      rkey(kk)=rib
      ndex(kk)=ib
      go to 90
      end

      subroutine nclose(nphase,ihold,navsum,nstuse,ifl,ierr)
c
c $$$$$ calls ellip, delaz, drdwr, lest, lpush,
c             normeq $$$$$
c
c   Nclose performs all calculations for output which are not needed for
c   the Gauss-Newton iteration itself.  This includes setting remaining
c   free flags to use, computing distance-azimuth-residuals for
c   residualed first arrivals, re-computing pP-P depths, and computing
c   hypocentral parameter standard deviations.
c   Nphase is the number of associated data, ihold is the number of
c   constraint equations, navsum is the number of usable data, and ifl
c   is a matrix decomposition flag.  If ifl is 1, the solution will be
c   presumed to have changed since the last matrix decomposition.  If
c   ifl is 2, the previous decomposition will be used.  If all goes
c   well, nclose will return +1.
c
      include 'locprm.inc'
	logical phasid,log
c	integer stdin, stdout
      double precision a,b,c,x,s,avdr
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Logical units.
      common /unit/ min,mout,merr
c   Storage for the normal equations.
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
c
c	stdin = 5
c	stdout= 6
c
c   Assume success.
c
      ierr=1
c
c   If all hypocentral parameters are held, there is no need to
c   decompose the matrix as all standard deviations must be zero anyway.
c   If the matrix was singular, there is nothing to decompose.
c
      if(ihold.eq.4.or.ng.eq.'b') go to 2
      m=4-ihold
c
c   If there are insufficient equations for a solution, set a flag and
c   skip the matrix decomposition.
c
      if(m-1.le.navsum.and.nstuse.ge.3) go to 1
      ng='s'
      go to 2
c
c   Compute the correlation matrix (A**-1) from the matrix
c   decomposition
c
1     ierr=normeq(navsum,ihold)
c
c   Loop over all station-readings.
c
2     do 3 j = 1, nphase
c
c   Fetch data from station parameter storage.
c
      call drdwr(1,j,1,13)
c
c   Skip stations with bad stations or which have been noassed.
c
      if( pflg .eq. 'u' ) go to 3
c
c   Compute distance and azimuth.
c
      call delaz(olats,olatc,olons,olonc,slats,slatc,slons,slonc,
     1           delta,azim)
c
      log=phasid()
c
c   All done.  Push data back out to station parameter storage.
c
      call drdwr(2,j,1,13)
3     continue
c
c   Clean up event level data.
c
c   Compute hypocentral parameter standard deviations.  If depth is
c   held, all parameters are held, or the solution is bad, zero
c   appropriate standard deviations.
c
      sedep=0.
      if(ihold.eq.4.or.ng.eq.'s') se=0.
      if(ihold.ne.4.and.ng.ne.'s'.and.ng.ne.'b') go to 10
      selat=0.
      selon=0.
      setim=0.
      call errelp(navsum,0,se)
      return
c   Convert the units of the correlation matrix to km**2.
 10   rs=radius-odep
      rl=rs*olats
      m1=3-ihold
      do 11 k=1,m1
      a(1,k)=rs*a(1,k)
      a(k,1)=rs*a(k,1)
      a(2,k)=rl*a(2,k)
 11   a(k,2)=rl*a(k,2)
c   Otherwise, compute standard deviations from the diagonal elements of
c   the correlation matrix.
      setim=rsc1*se
      selat=rsc1*dsqrt(dmax1(a(1,1),0d0))
      selon=rsc1*dsqrt(dmax1(a(2,2),0d0))
      if(ihold.eq.0) sedep=rsc1*dsqrt(dmax1(a(3,3),0d0))
c     print *,'setim selat selon sedep',setim,selat,selon,sedep
      call errelp(navsum,m,se)
      return
c   Flag a problem with the pP-P depth.
 13   ierr=-1
      return
      end

      function normeq(navsum,ihold)
c
c $$$$$ calls cholin, cholsl, and drdwr $$$$$
c
c   Normeq constructs and solves the normal equations to determine the
c   next hypocentral step.  Nphases are processed.  All usable
c   (not residualed or noassed) data are used.  M is the number of
c   equations solved.  It will be 4 for a hypocentral step and 3 for an
c   epicentral step.  If all goes well normeq returns 1.  Normeq returns
c   -1 if the matrix is singular.  If ifl is 1, the system of equations
c   is decomposed and  solved.  If ifl is 2, the correlation matrix is
c   computed assuming that the matrix has already been decomposed.  If
c   ifl is 3, the system of equations is decomposed, but not solved.
c   Rewritten on 21 July 1983 by R. Buland from a fragment of a routine
c   by E. R. Engdahl.
c
      double precision a,b,c,x,s,avdr,eps,fac
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Storage for the normal equations.
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
      data eps,fac0/1d-5,31.06341/
c
c   Assume success.
      normeq=1
      jfl=1
      m1=3-ihold
c   Zero out array storage.
      do 1 k=1,m1
      do 1 l=k,m1
 1    a(l,k)=0d0
c   Loop over the stations accumulating the normal matrix and data
c   vector.
      do 2 i=1,navsum
c   Pop one of the sorted residuals.
      ierr=lpop(i,j,res)
c   Access station parameter storage.
      call drdwr(1,j,1,5)
      if(pflg.ne.'u') go to 2
c   C is the contribution to the normal equations.
      c(1)=deg*cos(azim)*dtdd-avdr(1)
      c(2)=-deg*sin(azim)*dtdd*olats-avdr(2)
      c(3)=dtdh-avdr(3)
c     print *,'normeq',j,avdr,c
c   Update the normal equations.
      do 3 k=1,m1
      do 3 l=k,m1
 3    a(l,k)=a(l,k)+c(k)*c(l)
 2    continue
c     print 500,((a(l,k),k=1,m1),l=1,m1)
c500  format(/(1x,1p3e15.4))
c
c   Solve the system.
c
c   Weight the system of equations by diagonal matrix S to improve
c   conditioning.  Note that here as above, only the lower triangle of
c   the normal matrix is needed by the Cholesky routine used below.
      do 5 k=1,m1
 5    s(k)=1d0/dmax1(dsqrt(a(k,k)),eps)
      do 6 k=1,m1
      do 6 l=k,m1
 6    a(l,k)=s(l)*s(k)*a(l,k)
c   Compute the Cholesky decomposition of the normal matrix.  The lower
c   triangle of array A is unchanged.  The upper triangle of A is
c   replaced by the Cholesky factor (offset by one column so everything
c   will fit).
 7    call cholin(3,m1,a,1,ierr)
c   Branch if there is no error.
      if(ierr.eq.0) go to 8
c   Bail out if the fixup has already failed.
      if(jfl.le.0) go to 13
      jfl=-1
c   Add eps times the identity matrix to the normal matrix to improve
c   conditioning and try again.
      do 9 k=1,m1
 9    a(k,k)=a(k,k)+eps
      go to 7
c
c   Transform the Cholesky factor in the upper triangle of A into the
c   inverse of the normal matrix (the covariance matrix) in place.
 8    call cholin(3,m1,a,-3,ierr)
c   Remove the weighting from the covariance matrix, shift it over to
c   replace the original a matrix, and reconstruct the lower triangle.
      do 11 k=1,m1
      do 11 l=k,m1
      a(k,l)=s(k)*s(l)*a(k,l+1)
 11   a(l,k)=a(k,l)
      return
c   Flag a singular solution.
 13   normeq=-1
      return
      end

      function nstep(nsta,ihold,loop,iter,navsum,av,chisq,deld)
c
c $$$$$ calls normeq and stats $$$$$
c
c   Assuming that travel-time residuals and derivitives with respect to
c   distance and depth have already been computed, nstep finds the next
c   Gauss-Newton step, updates the hypocenter, and assesses the goodness
c   of fit of the new solution.  In the assessment phase, travel-time
c   residuals and derivitives for the new solution are computed.  Nsta
c   is the number of associated stations, ihold is the number of
c   constraint equations, loop is a character flag indicating the
c   iteration stage, iter is the current Gauss-Newton iteration number,
c   navsum is the number of usable arrivals, av is the average travel-
c   time residual, chisq is the sum of the squares of the residuals, and
c   deld is the length of the spatial step made in kilometers.  Nsta,
c   loop and iter0 are not changed.  Navsum is assumed to be valid on
c   entry, but is recomputed internally.  Av, chisq, and deld are
c   computed each time by nstep.  If nstep returns -1, depth was
c   constrained by nstep and ihold has been modified accordingly.
c   Otherwise, nstep returns +1.  Rewritten on 25 July 1983 by R. Buland
c   from a fragment of a routine by E. R. Engdahl.
c
      character*1 loop
      double precision a,b,c,x,s,avdr
      dimension hyprm(8),hypsav(4)
c   Logical units on input and output files
      common /unit/ min, mout, merr
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Storage for the normal equations.
      common /matrix/ a(3,4),b(3),c(4),x(3),s(3),avdr(3)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Commands.
      include 'cmndcm.inc'
c   Back up hypocenter.
      common /hypfix/ hypbck(8)
c   Iteration control parameters.
      common /itcntl/ miter1,miter2,miter3,tol1,tol2,tol3,hmax1,hmax2,
     1 hmax3,tol,hmax,zmin,zmax,zshal,zdeep,rchi,roff,utol
      equivalence (osec,hyprm)
      data almost,eps,savz,addz,nexp/1.1,1e-4,-1.,0.,1/
c
c   Assume success.
      nstep=1
c   Find number of equations.
      m=4-ihold
      m1=m-1
c   If there aren't enough equations, bail out.
      if(m1.gt.navsum) return
c   Initialize output parameters.
      delh=0.
      delz=0.
      if(iter.le.1) cn=50.
c   Addz accounts for depth changes imposed outside nstep in the tag
c   line printed below.
      if(savz.ge.0.) addz=odep-savz
c   The damping factor, roff, is fiddled each time nstep is called.
c   This "randomization" avoids excessive iteration in certain
c   pathlogical cases.
      if(rchi.gt.roff) go to 33
      rchi=rchi+.0390625
      go to 34
 33   rchi=rchi-.21875
c
c   Use steepest descents to find the minimum of the piecewise linear 
c   rank-sum penelty function.
c
c   Set the base hypocenter.
 34   do 3 i=1,m
 3    hypsav(i)=hyprm(i)
      cn=amax1(cn,2.*tol)
      x(1)=b(1)/(rad*radius)
      x(2)=b(2)/(rad*radius*olats)
      x(3)=b(3)
      call lintry(navsum,ihold,chisq,cn,tol,hmax)
c     print *,'x',(x(i),i=1,m1)
c   Set the horizontal and vertical steps (in km).
      delh=sqrt(amax1((sngl(x(1))*rad*radius)**2+
     1 (sngl(x(2))*rad*radius*olats)**2,0.))
      if(ihold.eq.0) delz=x(3)
      savdel=sqrt(amax1(delh*delh+delz*delz,0.))
c     print *,'del',delh,delz,savdel
c
c   Damp the step on maximum horizontal distance.
c
      if(delh.le.hmax) go to 8
      rat=hmax/delh
      do 9 i=1,m1
 9    x(i)=rat*x(i)
      delh=hmax
c
c   Update the depth.
c
 8    if(ihold.ne.0) go to 4
c
c   Trap airquakes.
      if(odep+x(3).ge.zmin) go to 10
      if(abs(odep-zmin).le.eps) go to 27
c   If we aren't yet at zmin, damp the step to just reach zmin.
      rat=(zmin-odep)/x(3)
      do 18 i=1,m1
 18   x(i)=rat*x(i)
      delh=rat*delh
c   If the damped step would be interpreted as convergence, constrain
c   depth.
      if(sqrt(amax1(delh*delh+sngl(x(3))*sngl(x(3)),0.))-tol) 20,20,11
c   If we are already at zmin, move along the z = zmin surface.
 27   if(delh.le.tol) go to 20
      x(3)=0d0
      go to 11
c
c   Trap lower mantle quakes.
 10   if(odep+x(3).le.zmax) go to 11
      if(abs(zmax-odep).le.eps) go to 28
c   If we aren't yet at zmax, damp step to just get there.
      rat=(zmax-odep)/x(3)
      do 19 i=1,m1
 19   x(i)=rat*x(i)
      delh=rat*delh
c   If the damped step would be interpreted as convergence, constrain
c   depth.
      if(sqrt(amax1(delh*delh+sngl(x(3))*sngl(x(3)),0.))-tol) 21,21,11
c   If we are already at zmax, move along the z = zmax surface.
 28   if(delh.le.tol) go to 21
      x(3)=0d0
 11   delz=x(3)
      go to 4
c
c   On a bad depth, reset it to be held at either zshal or zdeep as
c   apropriate and set flags.
c
 20   rat=zshal
      go to 22
 21   rat=zdeep
 22   if(rat.ne.hypbck(4)) go to 29
c   If we have already converged at the held depth, reset the hypocenter
c   to the known solution.
      delh=sqrt(amax1(((hypbck(2)-hypsav(2))*rad*radius)**2+((hypbck(3)-
     1 hypsav(3))*rad*radius*olats)**2,0.))
      delz=hypbck(4)-hypsav(4)
      do 30 i=1,8
 30   hyprm(i)=hypbck(i)
      nstep=0
      go to 31
c   Otherwise, use the current epicenter, reset depth to the desired
c   value, and find an approximately compatible origin time.
 29   do 24 i=1,m
 24   hyprm(i)=hypsav(i)
      delh=0.
      delz=rat-odep
      osec=osec+.1*delz
      odep=rat
      nstep=-1
c   Set the depth held flags.
 31   ihold=1
      m=3
      m1=2
      hmax=hmax1
      dep ='c'
c   Go recompute residuals and derivitives for the next iteration.
      go to 1
c
c   Update the hypocentral parameters.
c
 4    do 12 i=1,m1
 12   hyprm(i+1)=hypsav(i+1)+x(i)
      odep=amin1(amax1(odep,zmin),zmax)
c   Guarantee legal co-latitude and longitude.
      if(olat.ge.0..and.olat.le.180.) go to 15
      olon=olon+180.
      if(olat.lt.0.) olat=abs(olat)
      if(olat.gt.180.) olat=360.-olat
 15   if(olon.lt.0.) olon=olon+360.
 17   if(olon.le.360.) go to 16
      olon=olon-360.
      go to 17
c   Calculate epicentral sines and cosines.
 16   olats=sin(olat*rad)
      olatc=cos(olat*rad)
      olons=sin(olon*rad)
      olonc=cos(olon*rad)
c
c   Recompute travel-time residuals, derivitives, and chi**2.
c
 1    call stats(nsta,ihold,navsum,av,chi2)
      hyprm(1)=hyprm(1)+av
c   Deld is the convergence parameter.
      deld=sqrt(delh*delh+delz*delz)
c
c   If chi**2 has decreased or damping has been unsuccessful, get out.
c
      if(chi2.lt.chisq.or.nstep.le.0.or.ng.ne.' ') go to 6
c   If we can't damp the step any more, bail out.
      if(rchi*deld.le.tol) go to 25
c   If chi**2 has increased, damp the step length by factor rchi.
      do 7 i=1,m1
 7    x(i)=rchi*x(i)
      delh=rchi*delh
      delz=rchi*delz
      go to 4
c   Trap an unstable solution on a clipped depth.
 25   if(abs(odep-zmin).le..01.and.ihold.eq.0) go to 20
      if(abs(odep-zmax).le..01.and.ihold.eq.0) go to 21
c   Flag an unstable solution.  Go around one more time to recover the
c   starting point which can't be improved.
      ng='u'
      if(savdel.le.utol) ng='q'
      if(chi2.le.almost*chisq.and.deld.le.tol) ng='o'
c   Zero the step and go around again to recompute the original
c   residuals.
      do 26 i=1,m1
 26   x(i)=0d0
      delh=0.
      delz=0.
      go to 4
c
c   Compute the event standard deviation.
 6    chisq=chi2
      rms=0.0
      if(navsum.gt.0) rms=chisq/navsum
c
c   Write a summary line to the standard output.
      rlat=90.-olat
      rlon=olon
      if(rlon.gt.180.) rlon=rlon-360.
      delz=delz+addz
      savz=odep
      write(merr,100,iostat=ios)loop,iter,navsum,rlat,rlon,odep,
     1 delh,delz,rms,ng,dep
 100  format('nstep: ',a1,i3,i5,f9.3,f10.3,f7.1,' delh=',f6.1,
     1       ' delz=',f7.1,' rms=',f10.4,1x,'ng= ',a1,'dep= ',a1)
 13   return
      end

      subroutine scopt(n,sc)
c
c   Generate scores for n data.  Note that the scores don't depend on 
c   the data value, only on it's position.
c
      dimension sc(1),p(29),s(29)
      data p/0.,.1375,.1625,.1875,.2125,.2375,.2625,.2875,.3125,.3375,
     1 .3625,.3875,.4125,.4375,.4625,.4875,.5125,.5375,.5625,.5875,
     2 .6125,.6375,.6625,.6875,.7125,.7375,.7625,.7875,1./
      data s/0.0775,0.0775,0.1546,0.5328,0.8679,1.1714,1.4542,1.7266,
     1 1.9987,2.2802,2.5803,2.9068,3.2657,3.6603,4.0912,4.5554,5.0470,
     2 5.5572,6.0754,6.5906,7.0919,7.5702,8.0194,8.4365,8.8223,9.1812,
     3 9.5207,9.5974,9.5974/
c
      cn=1./(n+1)
      k1=1
      k=2
      av=0.
      do 1 i=1,n
      x=cn*i
 3    if(x.le.p(k)) go to 2
      k1=k
      k=k+1
      go to 3
 2    sc(i)=(x-p(k1))*(s(k)-s(k1))/(p(k)-p(k1))+s(k1)
 1    av=av+sc(i)
      av=av/n
      do 4 i=1,n
 4    sc(i)=sc(i)-av
      return
      end

      subroutine setup(nphase,ihold,navsum,av,chisq)
c
c $$$$$ calls ellip, delaz, drdwr $$$$$
c
c   Setup initializes the location process.  This involves calculating
c   distance and azimuth, travel-time and derivitives, setting the phase
c   flag, and implementing the pkp commands.  nphase is the number of
c   associated phases, ihold is the
c   number of constraints on the solution, navsum is the number of
c   usable first arrivals, av is the average travel-time residual, and
c   chisq is the sum of the squared residuals.  Navsum, av,and chisq are
c   computed by setup.  Rewritten on 22 July 1983 by R. Buland from a
c   fragment of a routine by E. R. Engdahl.
c
      include 'locprm.inc'
      logical phasid
c  integer stdin, stdout
      common /unit/ min, mout, merr
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Event level analyst commands.
      include 'cmndcm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
c   Scores.
      common /sccom/ scgen(maxph)
c
c	stdout = 6
c	stdin = 5
c
c   Initialize counter
c
      navsum=0
c
c   Loop over all station-readings.
c
      jj = 0
      do 2 j=1,nphase
c
c   Fetch data from station parameter storage.
c
      call drdwr(1,j,6,11)
c
c   Skip stations which can't be used in the location.
c
      if( pflg .ne. 'u' ) go to 2
c
c   Compute distance, azimuth and traveltime of phase.
c
      call delaz(olats,olatc,olons,olonc,slats,slatc,slons,slonc,
     1           delta,azim)
c
c   Apply the DRES commands.
      do k=1,5
        if(dres(k).eq.'r'.and.delta.ge.dellim(1,k).and.delta.le.
     1   dellim(2,k)) then
          pflg=' '
          go to 8
        endif
      enddo
c
c   ID the phases and compute residuals.
      if(phasid()) then
c
c   Apply the RRES command.
         if(rres.eq.'r'.and.(res.lt.reslim(1).or.res.gt.reslim(2))) then
                  pflg=' '
                  go to 8
         endif
c   Push the residual for statistical purposes.
           ierr=lpush(navsum,j,res)
      endif
c
c   Restore data to station parameter storage.
c
 8    call drdwr(2,j,1,5)
c      call drdwr(2,j,22,22)
 2    continue
c
      call scopt(navsum,scgen)
c
c   Compute all statistical parameters.
c
      ierr=lsrt(navsum,av,se,chisq,'TT')
      call adder(navsum,ihold)
c       write(merr,*)'setup: ',' nphase= ',nphase,' navsum= ',navsum
      return
      end

      subroutine stats(nphase,ihold,navsum,av,chisq)
c
c $$$$$ calls ellip, delaz, drdwr $$$$$
c
c   Stats computes travel-time residuals and derivitives for the current
c   solution.  Nphase is the number of associated stations, ihold is the
c   number of constraint equations, navsum is the number of usable
c   arrivals, av is the average travel-time residual, and chisq is the
c   sum of the squared residuals.  Nphase and ihold are not changed.
c   Navsum, av, and chisq are computed by stats.  Rewritten on
c   5 August 1983 by R. Buland from a fragment of a routine by
c   E. R. Engdahl.
c
      include 'locprm.inc'
	logical phasid
c	integer stdout,stdin
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Logical units.
      common /unit/ min,mout,merr
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
c
c	stdout = 6
c	stdin = 5
c
c   Initialize statistical counters.
c
      navsum=0
c
c   Loop over all station-readings.
      do 1 j=1,nphase
c   Fetch data from station parameter storage.
      call drdwr(1,j,6,11)
c   Don't bother with bad stations or noassed or residualed readings.
      if( pflg .ne. 'u' ) go to 1
c   Get the new distance and azimuth.
      call delaz(olats,olatc,olons,olonc,slats,slatc,slons,slonc,
     1           delta,azim)
c
c   ID the phase and compute the residual.
      if(phasid()) then
c
c   Update the statistical sums.
          ierr=lpush(navsum,j,res)
      endif
c
c   Save computed data in station parameter storage.
c
      call drdwr(2,j,1,5)
1     continue
c
c   Compute the average travel-time residual and chi**2.
c
      ierr=lsrt(navsum,av,se,chisq,'TT')
      call adder(navsum,ihold)
      return
      end

      function stirf(u,t0,t1,t2)
c
c   Compute an asymptotic approximation to the Stirling function.
c
      stirf=t1+0.5*u*(t2-t0)+0.5*(u**2)*(t2-2.0*t1+t0)
      return
      end
 
      function xmed(n,a,sd)
c
c $$$$$ calls only library routines $$$$$
c
c   Given the n numbers, a(1) <= a(2) <= ... <= a(n), xmed returns
c   robust estimates of center and spread of the data about the center.
c   The array a is not altered.  Programmed on 9 December 1983 by
c   R. Buland.
c
      dimension a(1)
      data xno,xne/1.482580,.7412898/
c   See if there is enough data to work with.
      if(n-1)13,12,14
c   No, return.
 13   xmed=0.
      sd=0.
      return
c   One datum is a special case.
 12   xmed=a(1)
      sd=0.
      return
c   N > 1, the median is a robust estimate of center.
 14   m=n/2
      md=mod(n,2)
      if(md.le.0) xmed=.5*(a(m)+a(m+1))
      if(md.gt.0) xmed=a(m+1)
c   Find the smallest positive value.
      do 1 i=1,n
      if(a(i)-xmed.ge.0.) go to 2
 1    continue
      i=n
c   Find the smallest absolute value.
 2    if(i.gt.1.and.abs(a(i-1)-xmed).lt.abs(a(i)-xmed)) i=i-1
      k=i
      j=1
      if(j.ge.m) go to 15
c   Implicitly order the series in order of increasing absolute value by
c   sucessively moving i up or k down until the m th term is found.
 6    if(k.le.1) go to 3
      if(i.ge.n) go to 4
      if(abs(a(i+1)-xmed).gt.abs(a(k-1)-xmed)) go to 5
c   Move i up.
      i=i+1
      j=j+1
      if(j.lt.m) go to 6
c   The m th term is found, mark it.
 15   l1=i
      go to 8
c   Move k down.
 5    k=k-1
      j=j+1
      if(j.lt.m) go to 6
c   The m th term is found, mark it.
      l1=k
c   Find and mark the m+1 th term.
 8    if(k.le.1) go to 9
      if(i.ge.n) go to 10
      if(abs(a(i+1)-xmed)-abs(a(k-1)-xmed))9,9,10
 9    l2=i+1
      go to 11
 10   l2=k-1
      go to 11
c   We have run out of negative terms, the m th term must be up.
 3    l1=i+m-j
      l2=l1+1
      go to 11
c   We have run out of positive terms, the m th term must be down.
 4    l1=k-(m-j)
      l2=l1-1
c   The spread is the normalized median of the absolute series.
 11   if(md.le.0) sd=xne*(abs(a(l1)-xmed)+abs(a(l2)-xmed))
      if(md.gt.0) sd=xno*abs(a(l2)-xmed)
      return
      end

c   Withers modified declaration of phasid function to prefent f90
c   from thinking its main.  (gotta love .for)
c     logical function phasid
      logical function phasid()

c
c   Re-identify the current phase and return its travel-time residual.
c
      implicit none
	real depmin,cn
	parameter(depmin=1.,cn=5.)
c   Miscellaneous constants.
      include 'locprm.inc'
      character*8 phcd1(maxtt), tmpcod
	character*20 prmmod
	logical first
	integer*4 n,i,j,ntrbl,mprm,ln
c	integer*4 min,mout,merr
      real*4 zs,tt1(maxtt),dtdd1(maxtt),dtdh1(maxtt),dddp1(maxtt),
     1 ellip,elcor,pi,rad,deg,radius,tcor,ecor,zs0,usrc(2),
     2 travt(maxtt),win(maxtt),spd,amp
c   Physical constants.
      common /circle/ pi,rad,deg,radius
c   Logical units.
c     common /unit/ min,mout,merr
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Station-reading parameters.
      include 'pickcm.inc'
	data first,zs0/.true.,-1./
c
c   The first time through initialize statistical parameters and the 
c   ellipticity routine.
	if(first) then
		first=.false.
		call freeunit(mprm)
		call getprm(mprm,prmmod)
		close(mprm)
		tcor=ellip('P       ',100.0,zs,glat,45.0)
c		print *,'Phasid: ellip set'
	endif
c
c   Initialize depth for the calculation of the traveltimes and partial
c      derivatives
	zs = amax1(odep,depmin)
	if(zs.ne.zs0) then
		zs0=zs
		call depset(zs,usrc)
c		print *,'Phasid: depset - zs glat =',zs,glat
	endif
c
c   Compute travel times of candidate phases.
c	print *,'Phasid: scode depth delta azim elev = ',sta,zs,delta,
c	1 azim*deg,elev
      call trtm(delta,maxtt,n,tt1,dtdd1,dtdh1,dddp1,phcd1)
c
c   Compute and apply travel-time corrections.
      do i = 1, n
c              print *,'Trtm: phcd tt dtdd dtdh = ',phcd1(i),tt1(i),dtdd1(i),
c     1          dtdh1(i)
                ecor=elcor(phcd1(i),elev,dtdd1(i))
                if((phcd1(i).eq.'Pg'.or.phcd1(i).eq.'Pb'.or.
     1             phcd1(i).eq.'Pn'.or.
     2             phcd1(i).eq.'P').and.dtdh1(i).gt.0.) then
                    tcor=ellip('Pup',delta,zs,glat,azim*deg)
c                   print *,'Pup'
                else if((phcd1(i).eq.'Sg'.or.phcd1(i).eq.'Sb'.or.
     1             phcd1(i).eq.'Sn'.or.phcd1(i).eq.'S').and.
     2             dtdh1(i).gt.0.) then
                    tcor=ellip('Sup',delta,zs,glat,azim*deg)
c                   print *,'Sup'
                else
                    tcor=ellip(phcd1(i),delta,zs,glat,azim*deg)
                endif
                travt(i)=tt1(i)+(tcor+ecor)+osec
c   Base the association window on half the spread.
          call getstt(phcd1(i),delta,dtdd1(i),spd,amp)
          win(i)=cn*spd
c		print *,'Phasid: phase tcor ecor travt win = ',
c	1	 phcd1(i),tcor,ecor,travt(i),win(i)
        enddo
c	pause
c
c   Identify the phase using the Chicxulub algorithm.
c
c   Handle a generic P (phase class 'p').
      if(phase.eq.'P') then
        i=0
        res=1e30
        do j=1,n
          if(phcd1(j).eq.'Pg'.or.phcd1(j).eq.'Pb'.or.phcd1(j).eq.
     1       'Pn'.or.phcd1(j).eq.'P'.or.phcd1(j).eq.'Pdif'.or.
     2       phcd1(j)(1:3).eq.'PKP'.or.phcd1(j).eq.'PKiKP') then
            if(abs(trvtim-travt(j)).lt.res) then
              i=j
              res=abs(trvtim-travt(j))
            endif
          endif
        enddo
c   Handle a generic S (phase class 's').
      else if(phase.eq.'S') then
        i=0
        res=1e30
        do j=1,n
          if(phcd1(j).eq.'Sg'.or.phcd1(j).eq.'Sb'.or.phcd1(j).eq.
     1       'Sn'.or.phcd1(j).eq.'S') then
            if(abs(trvtim-travt(j)).lt.res) then
              i=j
              res=abs(trvtim-travt(j))
            endif
          endif
        enddo
c   Handle a generic PKP (phase class 'k').
      else if(phase.eq.'PKP') then
        i=0
        res=1e30
        do j=1,n
          if(phcd1(j)(1:3).eq.'PKP'.or.phcd1(j).eq.'PKiKP') then
            if(abs(trvtim-travt(j)).lt.res) then
              i=j
              res=abs(trvtim-travt(j))
            endif
          endif
        enddo
c   Handle a generic PP (phase class '2').
      else if(phase.eq.'PP') then
        i=0
        res=1e30
        do j=1,n
          if(phcd1(j).eq.'PgPg'.or.phcd1(j).eq.'PbPb'.or.phcd1(j).eq.
     1     'PnPn'.or.phcd1(j).eq.'PP') then
            if(abs(trvtim-travt(j)).lt.res) then
              i=j
              res=abs(trvtim-travt(j))
            endif
          endif
        enddo
c   Handle a generic core phase (phase class 'c').
      else if(phase.eq.'pPKP'.or.phase.eq.'sPKP'.or.phase.eq.'SKP'.or.
     1 phase.eq.'PKKP'.or.phase.eq.'P''P'''.or.phase.eq.'SKS') then
        ln=ntrbl(phase)
        i=0
        res=1e30
        do j=1,n
          if(phcd1(j)(1:ln).eq.phase) then
            if(abs(trvtim-travt(j)).lt.res) then
              i=j
              res=abs(trvtim-travt(j))
            endif
          endif
        enddo
c   Handle an unidentified phase (phase class ' ').
      else if(phase.eq.' ') then
        i=0
        res=1e30
        do j=1,n
c         if(abs(trvtim-travt(j)).lt.10.) print *,'phase res win = ',
c    1     phcd1(j),trvtim-travt(j),win(j),delta
          if(abs(trvtim-travt(j)).le.win(j)) then
            if(abs(trvtim-travt(j)).lt.res) then
              i=j
              res=abs(trvtim-travt(j))
            endif
          endif
        enddo
c       if(i.gt.0) print *,'Select ',phcd1(i)
c   Otherwise, look for an exact match (phase class 'e').
      else
        do i=1,n
          if(phase.eq.phcd1(i)) go to 1
        enddo
        i=0
      endif
c
c   Take care of special cases.
      if(i.eq.0.and.phase.ne.' ') then
c   If an explicit P crustal phase doesn't exist, let it float.
        if(phase.eq.'Pg'.or.phase.eq.'Pb'.or.phase.eq.'Pn') then
          i=0
          res=1e30
          do j=1,n
            if(phcd1(j).eq.'Pg'.or.phcd1(j).eq.'Pb'.or.phcd1(j).eq.
     1       'Pn') then
              if(abs(trvtim-travt(j)).lt.res.and.
     1           abs(trvtim-travt(j)).lt.win(j)) then
                i=j
                res=abs(trvtim-travt(j))
              endif
            endif
          enddo
c   If an explicit S crustal phase doesn't exist, let it float.
        else if(phase.eq.'Sg'.or.phase.eq.'Sb'.or.phase.eq.'Sn') then
          i=0
          res=1e30
          do j=1,n
            if(phcd1(j).eq.'Sg'.or.phcd1(j).eq.'Sb'.or.phcd1(j).eq.
     1       'Sn') then
              if(abs(trvtim-travt(j)).lt.res.and.
     1           abs(trvtim-travt(j)).lt.win(j)) then
                i=j
                res=abs(trvtim-travt(j))
              endif
            endif
          enddo
c   Allow natural extensions of P and PKP phases.
        else
          tmpcod=' '
          if(phase.eq.'Pg') then
            tmpcod='Pb'
          else if(phase.eq.'Pn') then
            tmpcod='P'
          else if(phase.eq.'Sg') then
            tmpcod='Sb'
          else if(phase.eq.'Sn') then
            tmpcod='S'
          else if(phase.eq.'PKPdf') then
            tmpcod='PKiKP'
          else if(phase.eq.'PKiKP') then
            tmpcod='PKPdf'
          else if(phase.eq.'PKPbc') then
            tmpcod='PKPdif'
          else if(phase.eq.'PKPdif') then
            tmpcod='PKPbc'
          else if(phase.eq.'Pdif') then
            tmpcod='P'
          endif
          if(tmpcod.ne.' ') then
            do i=1,n
              if(tmpcod.eq.phcd1(i)) go to 1
            enddo
          endif
        endif
      endif
c
c   Got it.  Compute the residual.
c
1	if(i.gt.0) then
c		print *,'Phasid: phase phcd res = ',phase,phcd1(i),
c	1	 trvtim-travt(i)
		phase=phcd1(i)
		res=trvtim-travt(i)
		dtdd=dtdd1(i)
		dtdh=dtdh1(i)
		phasid=.true.
	else
c
c Phase not found for that distance and depth
c
		pflg = ' '
		res=0.
		phasid=.false.
	endif
	return
	end

	real*4 function elcor(phase,elev,dtdd)
c
c   Return the elevation correction.
c
	implicit none
      real vp,vs,kmpd
      parameter(vp=5.80,vs=3.46,kmpd=111.19493)
	character*(*) phase
	integer*4 ntrbl,j
	real*4 elev,dtdd
c
c   Figure out the arriving wave type.
	do j=ntrbl(phase),1,-1
		if(phase(j:j).eq.'P') then
c   This phase arrives as a P.  Do the correction using Vp.
			elcor=(elev/vp)*sqrt(abs(1.-
     1		 amin1((vp*dtdd/kmpd)**2,1.)))
c			print *,'Elcor: P corr = ',phase,elcor
			return
		endif
		if(phase(j:j).eq.'S') then
c   This phase arrives as an S.  Do the correction using Vs.
			elcor=(elev/vs)*sqrt(abs(1.-
     1		 amin1((vs*dtdd/kmpd)**2,1.)))
c			print *,'Elcor: S corr = ',phase,elcor
			return
		endif
	enddo
      return
	end

      subroutine getprm(nprm,modnam)
c
c   Read the parameter file.
c
      implicit none
      character*(*) modnam
      character*20 flnm
      integer*4 nprm,i,j,k,ntrbl
      include 'resprm.inc'
      data flnm/'res_ac.prm'/
      character*1001 dirname
      common /my_dir/dirname
c
c    Withers modified to be as other open statements in rayloc_ew
c     open(nprm,file=flnm,access='sequential',form='formatted',
c    1 status='old',readonly,shared)
c
c    Note that this param file is new to this version of rayloc_ew
c    (as of 20071205).  It should live in the same place as the model
c    (e.g. ak135.*).  Withers
c
c       Make sure we use dirname if one is provided
      if(dirname(1:2).eq.'. ') then
        open(nprm,file=flnm,access='sequential',form='formatted',
     >       status='old')
      else
        open(nprm,file=dirname(1:ntrbl(dirname))//'/'//flnm,
     >       access='sequential',form='formatted',status='old')
      endif

      read(nprm,101)modnam
 101  format(1x,a)
c     print *,'Getprm:  modnam = ',modnam
c
      do i=1,mxbr
         read(nprm,102,end=1)phnm(i),ms(i),me(i),xm(i),cut(i)
 102     format(1x,a8,2i8,f8.1,f8.0)
c        write(*,102)phnm(i),ms(i),me(i),xm(i),cut(i)
         do k=1,3
            read(nprm,103)mo(k,i),(prm(j,k,i),j=1,mo(k,i))
 103        format(6x,i5,1p4e15.7:/(11x,1p4e15.7))
c           write(*,103)mo(k,i),(prm(j,k,i),j=1,mo(k,i))
c	      pause
         enddo
      enddo
      i=mxbr+1
c
 1    nbr=i-1
      if(nbr.lt.mxbr) phnm(nbr+1)=' '
c     print *,'Getprm:  modnam nbr = ',modnam(1:ntrbl(modnam)),nbr
      close(nprm)
      return
      end

      subroutine getstt(phcd,delta,dtdd,spd,amp)
c
c   Given a phase code, phcd, source-receiver distance, delta (in deg), 
c   and dt/d(delta), dtdd (in s/deg), Getstt returns the observed 
c   spread, spd (sec), and relative amplitude, amp of the phase at that 
c   distance.  Note that the spread is the robust equivalent of one 
c   standard deviation.  If statistics aren't available, spd and amp are 
c   set to zero.
c
      implicit none
      character*(*) phcd
      integer*4 j,iupcor,margin
      real*4 delta,dtdd,spd,amp,xcor,tcor,x0,evlply
      include 'resprm.inc'
c
c   Find the desired phase in the statistics common.
      do j=1,nbr
        if(phcd.eq.phnm(j)) then
c   Figure the correction to surface focus.
          if(iupcor(phcd(1:1),dtdd,xcor,tcor).le.0) then
c           print *,'Iupcor failed on phase ',phcd
            xcor=0.
          endif
c         print *,'Getstt:  phcd delta xcor = ',phcd,delta,xcor
c   Correct the phase to surface focus.
          if(phcd(1:1).eq.'p'.or.phcd(1:1).eq.'s') then
c   For depth phases, the deeper, the closer.
            x0=delta-xcor
          else
c   For other phases, the deeper, the farther.
            x0=delta+xcor
          endif
c   If statistics are defined, do the interpolation.
          if(phnm(j).ne.phnm(min0(j+1,mxbr)).or.j.ge.nbr) then
            margin=15
          else
            margin=0
          endif
          if(x0.ge.ms(j)-16.and.x0.le.me(j)+margin) then
            x0=x0-xm(j)
            spd=amin1(amax1(evlply(x0,mo(3,j),prm(1,3,j)),.7),10.)
            amp=amax1(evlply(x0,mo(1,j),prm(1,1,j)),0.)
c           print *,'Getstt:  x0 spd amp =',x0,spd,amp,ms(j),me(j)
c           pause
            return
          endif
        endif
      enddo
c   If we don't find anything, return zero.
      spd=0.
      amp=0.
      return
      end

      function evlply(x,m,b)
c
c   Evlply evaluates the polynomial.
c
      dimension b(1)
c
      xx=1.
      evlply=b(1)
      do j=2,m
         xx=xx*x
         evlply=evlply+xx*b(j)
      enddo
      return
      end
