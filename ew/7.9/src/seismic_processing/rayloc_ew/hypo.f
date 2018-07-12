      subroutine hypo(nphase)
c
c $$$$$ calls nclose, nstep, setup, stats
c
c   Hypo drives the Gauss-Newton iteration process to determine a
c   refined hypocenter.  In the process, data may be eliminated and
c   depth may be constrained in several ways.  In the close out phase,
c   mb and Ms magnitudes and pP-P depths are computed.  Nphase is the
c   number of associated phases.  Rewritten on 25 July 1983 by
c   R. Buland from a routine by E. R. Engdahl.
c
	include 'locprm.inc'
      character*1 loop
c     character*1 savng
c	integer stdout, stdin
      dimension hyprm(8)
c
c   Station sort scratch area.
      character*5 scode
	integer*4 ndx
	common /statmp/ndx(maxph),scode(maxph)
c   Hypocentral parameters.
      include 'hypocm.inc'
c   Event level analyst commands.
	include 'cmndcm.inc'
c   Back up hypocenter.
      common /hypfix/ hypbck(8)
c   Iteration control parameters.
      common /itcntl/ miter1,miter2,miter3,tol1,tol2,tol3,hmax1,hmax2,
     1 hmax3,tol,hmax,zmin,zmax,zshal,zdeep,rchi,roff,utol
c   Scores.
      equivalence (osec,hyprm)
c
c	stdin = 5
c	stdout= 6
c
c   Establish the number of equations to be solved.
c
      ifl=1
      ihold=0
c
c Count the number of constraints.  The only one that works
c  is constraining the depth of the earthquake
c
      if(dep.eq.'c') ihold = 1
      if(hld.eq.'c') ihold = 4
c
c   Initialize the location proceedure.
c
      call setup(nphase,ihold,navsum,av,chisq)
c   Count stations.
      call stacnt(nphase,scode,ndx,nstass,nstuse)
c
c   Trap three station solutions.  This code doesn't make any sense for 
c   robust locator.
c     if(navsum.ne.2.or.ihold.ne.0) go to 30
c     odep=33.0
c     dep='c'
c     ihold=1
c
c   Go close out the calculation if all parameters are held or there is
c   insufficient data.
c
c30   if(ihold.gt.1.or.3-ihold.gt.navsum) go to 3
 30   if(ihold.gt.1.or.nstuse.lt.3) go to 3
c
c   Initial iteration.
c
 29   tol=tol3
      hmax=hmax2
      kfl=1
      lfl=1
c   Start by holding the current depth fixed.  This stablized the search
c   for a free depth and provides a backup in case depth goes out of
c   bounds.
      loop='h'
      if(ihold.eq.0) loop='f'
 6    do 4 iter=1,miter3
c   Make a Gauss-Newton step.  If depth is held by nstep, restart the
c   iteration if this is a new depth.  If it is the same depth used in
c   the previous fixed depth iteration, we are done.
      if(nstep(nphase,ihold,loop,iter,navsum,av,chisq,deld)) 6,3,8
c   Bail out on convergence, a "singular matrix", or an "unstable
c   solution".
 8    if(deld.le.tol.or.ng.ne.' ') go to 3
 4    continue
c
c   Compute all output parameters not needed in the iteration itself.
c   If ifl = 1, the solution comes from the last Gauss-Newton step.  In
c   this case, the normal matrix will need to be constructed and
c   decomposed one last time.  If ifl = 2, either the normal matrix has
c   already been decomposed or no normal matrix is needed.
c
 3    call nclose(nphase,ihold,navsum,nstuse,ifl,ierr)
 1001	continue
      return
      end
