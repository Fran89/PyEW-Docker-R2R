      real function ellip(phase,edist,edepth,slat,bazim)

c MMW 20071129 merged Hydra and ew version

c IGD 06/01/04 Added dirname common for portability
 
C==========================================================================
C
C    Ellipticity correction for any given phase using
C    Dziewonski & Gilbert representation
C    The ellipticity corrections are found by linear interpolation
C    in terms of values calculated for the ak135 model for a wide
C    range of phases to match the output of the iasp software
C
C    Input Parameters:
C    character
C          phase : a  string specifying the PHASE,   -e.g P, ScP etc.
C                  'phase' should have at least 8 characters length
C
C    real
C          edist  :  epicentral distance to station (in degrees)
C          edepth :  depth of event (km)
C          slat   :  epicentral latitude of source (in degrees)
C          bazim  :  azimuth from source to station (in degrees)
C
C    Output:
C    real
C          ellip  :  time correction for path to allow for ellipticity
C                    (ellip has to be added to the AK135 travel times)
C                     returns ellip=0. for phases unknown to this software)
C
C    Usage:-one call: tcor=ellip(phase,edist,edepth,slat,bazim)
C           to initialize
C          -every next call computes ellip from input parameters
C
C==========================================================================
C
C   B.L.N. Kennett RSES,ANU        May 1995
C   (based on earlier routine by D.J. Brown)
 
C  (modified for processing of large data sets
C   by W. Spakman, Earth Sciences, Utrecht University, June 1996)

c   Modified logical units for compatibility with RayLocator, July 2003.
c   Changed lue to merr and allocated lut with freeunit.
C=========================================================================
 
      common /my_dir/dirname
      character*1001 dirname

      character *(*) phase
 
      real edist,edepth,ecolat,bazim,slat,azim,degrad,
     ^     sc0,sc1,sc2,tcor,
     ^     tau0, a0,b0,h0,d0,e0,f0,g0,
     ^     tau1, a1,b1,h1,d1,e1,f1,g1,
     ^     tau2, a2,b2,h2,d2,e2,f2,g2
      integer Nd,ios,lut
 
c..
      logical INIT,ECHO1,ECHO2
 
c     no screen output if ECHO1=.false.
c     Only put ECHO1=.true. to familiarize yourself
c     Only put ECHO2=.true. to get some additional
c     numbers on output (which only make sense with
c     the code at hand)
 
c..
      parameter(NUMPH=57)
 
c     NUMPH: number of phases supported by the tau-tables.
c     NUMPH should exactly match the number of phases in the
c     tau-tables
c     To add a phase to the software proceed as follows:
c       . increase NUMPH by 1
c       . add the phase code at the end of the phcod data statement
c       . add the proper tau-table entries at the bottom of
c         file 'tau.tables'
c       The only syntax check on the tau.table file is that
c       the order in which the phases appear in the file should be
c       the same as the order of the phases in the phcod data statement
 
      character*8 phdum,phcod(NUMPH)
      integer phnch(NUMPH),np(NUMPH)
 
      parameter(MDEL=50)
      real dpth(6),delta(NUMPH,MDEL),di1(NUMPH),di2(NUMPH)
      real t0(NUMPH,MDEL,6),t1(NUMPH,MDEL,6),t2(NUMPH,MDEL,6)
 
      common /tau1/ phcod
      common /tau2/ phnch,np
      common /tau3/ dpth,delta,di1,di2,t0,t1,t2

	integer minr,mout,merr
	common /unit/ minr,mout,merr
 
      data phcod/
     &"Pup     ","P       ","Pdiff   ","PKPab   ","PKPbc   ","PKPdf   ",
     &"PKiKP   ","pP      ","pPKPab  ","pPKPbc  ","pPKPdf  ","pPKiKP  ",
     &"sP      ","sPKPab  ","sPKPbc  ","sPKPdf  ","sPKiKP  ","PcP     ",
     &"ScP     ","SKPab   ","SKPbc   ","SKPdf   ","SKiKP   ","PKKPab  ",
     &"PKKPbc  ","PKKPdf  ","SKKPab  ","SKKPbc  ","SKKPdf  ","PP      ",
     &"P'P'    ","Sup     ","S       ","Sdiff   ","SKSac   ","SKSdf   ",
     &"pS      ","pSKSac  ","pSKSdf  ","sS      ","sSKSac  ","sSKSdf  ",
     &"ScS     ","PcS     ","PKSab   ","PKSbc   ","PKSdf   ","PKKSab  ",
     &"PKKSbc  ","PKKSdf  ","SKKSac  ","SKKSdf  ","SS      ","S'S'    ",
     &"SP      ","PS      ","PnS     "/
 
c     In addition to the phase names listed above a number of phase aliases
c     are available in the routine phase_alias, e.g. Pn --> P etc
c     The input phase code is first checked against the phcod array
c     and next against the phase aliases.
 
      save INIT,ECHO1,ECHO2,/tau1/,/tau2/,/tau3/,Nd,deldst,degrad
 
c..   fixed
      data dpth   / 0.0, 100.0, 200.0, 300.0, 500.0, 700.0 /
      data degrad / 0.01745329/
      data Nd     / 6 /
      data deldst / 5.0 /
      data INIT   /.true./
 
c..   changeable
      data ECHO1  /.false./
      data ECHO2  /.false./
c     data lut    / 10 /
 
c...
 
      ellip=0.
 
      if(INIT) then
c...   Initialize at first call and return
        INIT=.false.
 
c       check on the length of phase
        l=len(phase)
        if(l.lt.8) then
          write(merr,*)
     >    'character variable `phase` should have at least length 8'
	  call exit(104)
        endif
 
c       initialize arrays
        do i=1,NUMPH
          phnch(i)=lnblk(phcod(i))
          np(i)     =0
          di1(i)    =0.
          di2(i)    =0.
          do j=1,MDEL
            delta(i,j)=0.
            do k=1,6
              t0(i,j,k)=0.
              t1(i,j,k)=0.
              t2(i,j,k)=0.
            enddo
          enddo
        enddo
 
c       Read tau.table from unit 'lut'
 
        ios=0
        if(ECHO1)
     >  write(merr,*) 'ellip: open and read ellipticity tau-table'
 
        call freeunit(lut)

c       Make sure we use dirname if one is provided
        if(dirname(1:2).eq.'. ') then
          open(lut,file='tau.table',form='formatted',
     >      iostat=ios,err=998,status='old')
        else
          open(lut, file=dirname(1:ntrbl(dirname))//'/'//'tau.table',
     >         form='formatted',
     >         iostat=ios,err=998,status='old')
        endif

c       open(lut,file='tau.table',form='formatted',
c    >           iostat=ios,err=998,status='old',readonly,shared)
        rewind(lut)
 
        ip=0
20      continue
          ip=ip+1
          if(ip.gt.NUMPH) then
c           stop reading NUMPH: limit is reached'
            goto 21
          endif
c         phase_code, number_of_epicentral_distance_entries, min_max_dist.
          read(lut,'(a8,i2,2f10.1)',end=21,iostat=ios)
     >         phdum,np(ip),di1(ip),di2(ip)
          if(ios.ne.0) then
            write(merr,*) 'ellip: read error on phase record err= ',ios,
     >                 ' on tau-table'
            call exit(104)
          endif
          if(ECHO1) write(merr,'(''reading: '',i3,1x,a8,1x,i2,2f10.1)')
     >             ip,phdum,np(ip),di1(ip),di2(ip)
          if(phdum(1:8).ne.phcod(ip)) then
            write(merr,*)
     >     'syntax of tau-table does not conform to ',
     >     'syntax of phase data statement'
            call exit(104)
          endif
          do i=1,np(ip)
           read(lut,*,iostat=ios,err=999) delta(ip,i)
           read(lut,*,iostat=ios,err=999) (t0(ip,i,m),m=1,6)
           read(lut,*,iostat=ios,err=999) (t1(ip,i,m),m=1,6)
           read(lut,*,iostat=ios,err=999) (t2(ip,i,m),m=1,6)
          enddo
        go to 20
 
21      continue
 
        if(ECHO1) then
          write(merr,*) 'Number of phases: ',ip-1
          write(merr,*) 'Phase codes     : '
 
c         print out of available phases
          i=NUMPH/6
          do j=1,i
            i1=1+(j-1)*6
            i2=i1-1+6
            write(merr,'(7a9)') (phcod(k),k=i1,i2)
          enddo
          i=i*6+1
          j=NUMPH-i
          if(j.gt.0) then
            write(merr,'(7a9)') (phcod(k),k=i,NUMPH)
          endif
          write(merr,*) ' '
          write(merr,*)
     >       'See also the phase aliases in routine phase_alias'
          write(merr,*) ' '
        endif
c.....  End initialization
        return
      endif
 
c...  set up source dependent constants
      azim   = bazim*degrad
      ecolat = (90.0-slat)*degrad
      call ellref(ecolat,sc0,sc1,sc2)
 
      if(ECHO2) then
        write(merr,*) 'phase,edist,edepth,ecolat,azim'
        write(merr,*)  phase(1:8),edist,edepth,ecolat,azim
        write(merr,*) 'sc0,sc1,sc2: ',sc0,sc1,sc2
      endif
 
c...  select phase
      ip = -1
      nc=min(lnblk(phase),8)
      do 10 i=1,NUMPH
        if(nc.ne.phnch(i)) goto 10
        if (phase(1:nc) .eq. phcod(i)(1:nc)) then
          ip = i
          go to 11
        endif
 10   continue
 11   continue
 
      if(ip.eq.-1) then
c       check phase aliases
        call phase_alias(phase,edist,ip)
      endif
 
      if(ip.gt.0) then
        if(ECHO1)
     >  write(merr,'(''ip:'',i3,'' Selected phase: '',a8,
     >              '' table distance range: '',2f8.1)')
     >              ip,phcod(ip),di1(ip),di2(ip)
      else
        if(ECHO1)  write(merr,
     >    '('' Selected phase '',a8,'' is not available'')')
     >     phase(1:8)
        ellip=0.
        return
      endif
 
c...  distance index
      idist = 1 + int( (edist-di1(ip))/ deldst )
      if(edist.lt.di1(ip)) idist =1
      if(edist.gt.di2(ip)) idist= np(ip)-1
 
c...  depth index
      do 25 j = 1,Nd-1
        if ((dpth(j).le.edepth).and.(dpth(j+1).ge.edepth))then
          jdepth = j
          goto 26
        endif
 25   continue
c   Extrapolate below 700 km (& make behavior deterministic).  Added by 
c   R. Buland on 18 March 2004.
      jdepth=Nd-1
 26   continue
 
      if(ECHO2) write(merr,*) 'idist, jdepth;',idist,jdepth
 
c...  Compute tau-values and ellip
c     need to allow for zero entries (where phase
c     description strongly depth dependent)
 
c tau0
         a0 = t0(ip,idist,jdepth)
         b0 = t0(ip,idist,jdepth+1)
         h0 = t0(ip,idist+1,jdepth+1)
         d0 = t0(ip,idist+1,jdepth)
         e0 = a0 +
     ^       (d0-a0)*(edist-delta(ip,idist))/
     ^       (delta(ip,idist+1)-delta(ip,idist))
         f0 = b0 +
     ^       (h0-b0)*(edist-delta(ip,idist))/
     ^       (delta(ip,idist+1)-delta(ip,idist))
         g0 = e0 +
     ^       (f0-e0)*(edepth-dpth(jdepth))/
     ^       (dpth(jdepth+1)-dpth(jdepth))
         tau0 = g0
c tau1
         a1 = t1(ip,idist,jdepth)
         b1 = t1(ip,idist,jdepth+1)
         h1 = t1(ip,idist+1,jdepth+1)
         d1 = t1(ip,idist+1,jdepth)
         e1 = a1 +
     ^       (d1-a1)*(edist-delta(ip,idist))/
     ^       (delta(ip,idist+1)-delta(ip,idist))
         f1 = b1 +
     ^       (h1-b1)*(edist-delta(ip,idist))/
     ^       (delta(ip,idist+1)-delta(ip,idist))
         g1 = e1 +
     ^       (f1-e1)*(edepth-dpth(jdepth))/
     ^       (dpth(jdepth+1)-dpth(jdepth))
         tau1 = g1
c tau2
         a2 = t2(ip,idist,jdepth)
         b2 = t2(ip,idist,jdepth+1)
         h2 = t2(ip,idist+1,jdepth+1)
         d2 = t2(ip,idist+1,jdepth)
         e2 = a2 +
     ^       (d2-a2)*(edist-delta(ip,idist))/
     ^       (delta(ip,idist+1)-delta(ip,idist))
         f2 = b2 +
     ^       (h2-b2)*(edist-delta(ip,idist))/
     ^       (delta(ip,idist+1)-delta(ip,idist))
         g2 = e2 +
     ^       (f2-e2)*(edepth-dpth(jdepth))/
     ^       (dpth(jdepth+1)-dpth(jdepth))
         tau2 = g2
c
         caz = cos(azim)
         cbz = cos(2.0*azim)
 
         if(ECHO2) then
           write(merr,*) 'tau0,tau1,tau2:',tau0,tau1,tau2
           write(merr,*) 'azim,caz,cbz',azim,caz,cbz
         endif
 
         tcor = sc0*tau0 + sc1*cos(azim)*tau1 + sc2*cos(2.0*azim)*tau2
 
      ellip=tcor
 
      return
 
999   write(merr,*) 'ellip: read error ',ios,' on tau-table'
      call exit(104)
998   write(merr,*) 'ellip: open error ',ios,' on tau-table'
      call exit(104)
      end
 
c--------
 
      subroutine ellref(ecolat,sc0,sc1,sc2)
c...  s3 = sqrt(3.0)/2.0
      real ecolat,s3,sc0,sc1,sc2
 
      data s3 /0.8660254/
 
      sc0 = 0.25*(1.0+3.0*cos(2.0*ecolat))
      sc1 = s3*sin(2.0*ecolat)
      sc2 = s3*sin(ecolat)*sin(ecolat)
      return
      end
 
c--------
 
      integer function lnblk(s)
      character *(*) s
      l = len(s)
      do i=l,1,-1
        if (s(i:i).gt.' ') then
          lnblk=i
          return
        endif
      end do
      lnblk=0
      return
      end
 
c------
 
      subroutine phase_alias(phase,delta,ip)
 
c-    check for alternative phase names
c     input phase, delta
c     output ip (index of phcod)
 
      character*(*) phase
 
      if(phase(1:3).eq.'Pg '.or.phase(1:3).eq.'Pb '.or.
     1 phase(1:3).eq.'Pn ') then
c       phase='P       '
        ip=2
      else if(phase(1:3).eq.'Sg '.or.phase(1:3).eq.'Sb '.or.
     1 phase(1:3).eq.'Sn ') then
c       phase='S       '
        ip=33
      else if(phase(1:4).eq.'pPg '.or.phase(1:4).eq.'pPb '.or.
     1 phase(1:4).eq.'pPn ') then
c       phase='pP      '
        ip=8
      else if(phase(1:4).eq.'pwP ') then
c       phase='pP      '
        ip=8
      else if(phase(1:5).eq.'pwPg '.or.phase(1:5).eq.'pwPb '.or.
     1 phase(1:5).eq.'pwPn ') then
c       phase='pP      '
        ip=8
      else if(phase(1:4).eq.'sPg '.or.phase(1:4).eq.'sPb '.or.
     1 phase(1:4).eq.'sPn ') then
c       phase='sP      '
        ip=13
      else if(phase(1:4).eq.'pSg '.or.phase(1:4).eq.'pSb '.or.
     1 phase(1:4).eq.'pSn ') then
c       phase='pS      '
        ip=37
      else if(phase(1:4).eq.'sSg '.or.phase(1:4).eq.'sSb '.or.
     1 phase(1:4).eq.'sSn ') then
c       phase='sS      '
        ip=40
      else if(phase(1:4).eq.'SPg '.or.phase(1:4).eq.'SPb '.or.
     1 phase(1:4).eq.'SPn ') then
c       phase='SP      '
        ip=55
      else if(phase(1:4).eq.'SgP '.or.phase(1:4).eq.'SbP '.or.
     1 phase(1:4).eq.'SnP ') then
c       phase='SP      '
        ip=55
      else if(phase(1:4).eq.'PSg '.or.phase(1:4).eq.'PSb '.or.
     1 phase(1:4).eq.'PSn ') then
c       phase='PS      '
        ip=56
      else if(phase(1:5).eq.'PgPg '.or.phase(1:5).eq.'PbPb '.or.
     1 phase(1:5).eq.'PnPn ') then
c       phase='PP      '
        ip=30
      else if(phase(1:5).eq.'SgSg '.or.phase(1:5).eq.'SbSb '.or.
     1 phase(1:5).eq.'SnSn ') then
c       phase='SS      '
        ip=53
      else if(phase(1:2).eq.'p ') then
c       phase='Pup     '
        ip=1
      else if(phase(1:2).eq.'s ') then
c       phase='Sup     '
        ip=32
      elseif(phase(1:6).eq."P'P'ab") then
c       phase="P'P'    '
        ip=31
      else if(phase(1:6).eq."P'P'bc") then
c       phase="P'P'    '
        ip=31
      else if(phase(1:6).eq."P'P'df") then
c       phase="P'P'    '
        ip=31
      else if(phase(1:6).eq."S'S'ac") then
c       phase="S'S'    '
        ip=54
      else if(phase(1:6).eq."S'S'df") then
c       phase="S'S'    '
        ip=54
        else if(phase.eq.'Pdif') then
c	  phase='Pdiff'
          ip=3
	else if(phase.eq.'Sdif') then
c	  phase='Sdiff'
	  ip=34
      else if(delta.le.100.0.and.phase.eq.'pPdif   ') then
c       phase='pP      '
        ip=8
      else if(delta.le.100.0.and.phase.eq.'pwPdif  ') then
c       phase='pP      '
        ip=8
      else if(delta.le.100.0.and.phase.eq.'sPdif   ') then
c       phase='sP      '
        ip=13
      else if(delta.le.100.0.and.phase.eq.'pSdif   ') then
c       phase='pS      '
        ip=37
      else if(delta.le.100.0.and.phase.eq.'sSdif   ') then
c       phase='sS      '
        ip=40
      else if(delta.le.165.0.and.phase.eq.'PKPdif  ') then
c       phase='PKPbc '
        ip=5
      else if(delta.le.165.0.and.phase.eq.'pPKPdif ') then
c       phase='pPKPbc '
        ip=10
      else if(delta.le.165.0.and.phase.eq.'sPKPdif ') then
c       phase='sPKPbc
        ip=15
      else
        ip=-1
      endif
      return
      end
