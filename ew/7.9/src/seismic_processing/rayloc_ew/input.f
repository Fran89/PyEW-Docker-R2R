      subroutine input(nphase)
c
c $$$$$ calls drdwr and geocen $$$$$
c
c   Input reads the reloc input file and does setup for the beginning
c   of the relocation problem.  Specifically, the initial hypocenter is 
c   set from analyst commands (if needed) and converted to geocentric 
c   coordinates, phase arrival times are converted to travel-times, and 
c   the individual phase flags (pkp, dphs, pflg ) are set.
c
c   Some details on specific parameters
c
c   pkp:    u = use PKP arrivals; ' ' = don't use PKP arrivals (default).
c   dphs:   u = use pP and sP phases in the earthquake location
c   sphs    u = use S phases in the earthquake location
c   pflg:   u = use phase in the location
c   dep:    c = constrain the depth of the event to that read in
c   hld:    c = constrain all hypocentral parameters
c   rres:   r = don't use residuals outside the range [reslim(1),reslim(2)]
c   dres:   r = don't use distances in the range [dellim(1),dellim(2)]
c
c   Original code written by H.M. Benz, Apr. 2002
c   Rewritten to accept EDR style input by S.D. Sheaffer, Feb. 2003
c   Rewritten to accept IOC stype input by R. Buland, July 2003
c
      implicit none
      real qualim
      parameter (qualim=.1)
      logical ecmd(6),use
c
      integer*4 nphase,i,k
c
      real*4 slat,slon,tlat,tlon
c
      real*8 atime
c
      integer*4 min,mout,merr
c
      common /unit/ min,mout,merr
c
      include 'hypocm.inc'
      include 'cmndcm.inc'
      include 'pickcm.inc'
c
c   Initialize the command and status flags.
      data hld,dep,pkp,dphs,sphs,rres,dres/11*' '/,ng/' '/
c
c	stdin = 5
c	stdout= 6
c
c   Read event level information.
      read(min,1001) otime,glat,glon,odep,ecmd,reslim
1001  format(f14.3,f9.4,f10.4,f7.2,6l2,2f6.2)
c   Translate into Reloc variables.
      call geocen(glat,glon,olat,olats,olatc,olon,olons,olonc)
      if(ecmd(1)) hld='c'
      if(ecmd(2)) dep='c'
      if(ecmd(3)) pkp='u'
      if(ecmd(4)) dphs='u'
      if(ecmd(5)) sphs='u'
      if(ecmd(6)) rres='r'
c
      read(min,1002) (ecmd(i),(dellim(k,i),k=1,2),i=1,5)
1002  format(l1,2f6.1,4(l2,2f6.1))
c
      do i=1,5
	  if(ecmd(i)) dres(i)='r'
      enddo
c If the depth is constrained, don't use depth phases.
      if( dep .eq. 'c' ) dphs = ' '
c
      nphase = 0
500   read(min,1005,end=900) pickid,sta,comp,network,loca,slat,slon,
     1elev,qual,phase,atime,use
1005  format(i10,1x,a5,1x,a3,1x,a2,1x,a2,f9.4,f10.4,f6.2,f5.2,1x,
     1a8,f15.3,l2)
c   Translate into Reloc variables.
      call geocen(slat,slon,tlat,slats,slatc,tlon,slons,slonc)
c
c   Reset to generic phase codes so they will be used and reidentified.
c       if(phase.eq.'Pg'.or.phase.eq.'Pn'.or.phase.eq.'Pb'.or.
c       1	phase.eq.'P*') phase='P'
c       if(phase.eq.'Sg'.or.phase.eq.'Sn'.or.phase.eq.'Sb'.or.
c       1	phase.eq.'S*') phase='S'
c
c Check to see if you should use the phase in the earthquake location
      if(use) then
        if( phase(1:1) .ne. 'p' .and. phase(1:1) .ne. 'P' .and.
     1     phase(1:1) .ne. 'S') then
		use=.false.
	  else
	    if((phase(1:2) .eq. 'pP' .or. phase(1:2) .eq. 'pS') .and. 
     1	       dphs .ne. 'u') use=.false.
	    if((phase(1:2) .eq. 'PK' .or. phase(1:4) .eq. 'pPKP' 
     1	        .or. phase(1:2) .eq. 'P''') .and. pkp .ne. 'u') 
     2	      use=.false.
	    if( phase(1:1) .eq. 'S' .and. sphs .ne. 'u') use=.false.
	  endif
	  if(qual.gt.qualim) use=.false.
	endif
c   Set the use flag.
	if(use) then
	  pflg='u'
	else
	  pflg=' '
	endif
c
c   Calculate the travel time for the phase.
	trvtim=atime-otime
	nphase = nphase + 1
	call drdwr(2,nphase,1,13)
	go to 500
c
c   Done.
900	return
      end
