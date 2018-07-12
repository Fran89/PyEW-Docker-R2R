c	to create the P and S travel-time tables read by
c	usnsn_loc2trig.
c	From Ray Buland's routines, and from Harley's sample program. 
c	Alex Dec30 99
c
        parameter(max=60)
        logical prnt(3)
        integer n
        real tt(max), dtdd(max), dtdh(max), dddp(max)
        real usrc(2)
        character*8 phlst(10), phcd(max)
	character*1 phchar(8)
	character*8  phoct, Pname, Sname
	equivalence(phchar(1),phoct)
	character*40 modnam
	data in/1/,modnam/'./Iasp91/iasp91'/
	character*40 TblFil
c
c 	Setup the call to compute the theoretical traveltimes
c	*****************************************************
        prnt(1) = .false.
        prnt(2) = .false.
        prnt(3) = .false.
        phlst(1) = 'ALL       '
        call tabin(in,modnam)
	call brnset(1,phlst,prnt)

c	Get table values from user
c	**************************
        write(6,*)'Enter max source depth in km'
        read(5,*)depthMax
	write(6,*)'Enter depth increment'
	read(5,*)dxdepth
	ndepth = depthMax/dxdepth + 1
	write(6,*)'Enter max source distance (degrees)'
	read(5,*)degMax
 	write(6,*)'Enter distance increment'
	read(5,*)dxdeg
	ndeg = degMax/dxdeg + 1
 	write(6,*)'Enter fixed duration past P in minutes. 0 => use S arrival'
	read(5,*)xfixedTime
	write(6,*)'Enter file name to write table to'
	read(5,*)TblFil
	open(8,file=TblFil,status='new',form='formatted')
c
c	write the table parameters for usnsn_loc2trig
c	*********************************************
	write(8,504) depthMax,degMax
504	format('# max depth:',f8.2,'   max degrees:',f8.2)
	write(8,505)
505	format('# each paragraph is all depths for a given distance')
	write(8,500)ndepth
500	format('Nz  ',I4,'   # number of depth values')
	write(8,501)ndeg
501	format('Nd  ',I4,'   # number of degree values')
	write(8,502)dxdepth
502	format('Ddepth  ',F7.2,'    # depth increment')
	write(8,503)dxdeg
503	format('Ddist  ',F7.2,'    # degree increment')


c	Produce P-arrival table
c	***********************
	write(8,*)'Pphase'
C	Loop over distance
	do 100 ideg=1,ndeg
	xdeg=(ideg-1)*dxdeg
C	loop over depth
	do 101 idepth=1,ndepth
	xdepth=(idepth-1)*dxdepth
	depth=xdepth
	call depset(depth,usrc)
	call trtm(xdeg,max,n,tt,dtdd,dtdh,dddp,phcd)

C	loop over all phases for this d,z
	Ptime=999999
	do 90 i=1,n
c	write(6,811)phcd(i),tt(i)
c811	format(A10,f7.2)
	phoct=phcd(i)
C	if it starts with 'P', and its earlier than what we've got, use it
	if(phchar(1).ne.'P') go to 90
	if(tt(i).ge.Ptime) go to 90
	Ptime=tt(i)
	Pname=phcd(i) 
90	continue

	write(8,810)Ptime,xdeg,xdepth,Pname
810	format(f7.2,5X,'# deg:',f5.1,2X,'z:',f5.1,3X,A10)
c	end of loop over depth
101	continue
	write(8,*)' '
c	end of loop over distance
100     continue
	write(8,*)' '

c	Produce the S-arrival table
c	***************************
	write(8,*)'Sphase'
C	Loop over distance
	do 200 ideg=1,ndeg
	xdeg=(ideg-1)*dxdeg
C	loop over depth
	do 201 idepth=1,ndepth
	xdepth=(idepth-1)*dxdepth
	depth=xdepth

	call depset(depth,usrc)
	call trtm(xdeg,max,n,tt,dtdd,dtdh,dddp,phcd)

C	find S arrival
C	loop over all phases for this d,z
	Stime=0
	Ptime=999999
	do 95 i=1,n
	phoct=phcd(i)
C	take the largest of S, Sg, or Sn
	if(phcd(i).ne.'S       ') go to 91
	if(tt(i).le.Stime) go to 91
	Sname=phcd(i)
	Stime=tt(i)
91	continue

	if(phcd(i).ne.'Sg      ') go to 92
	if(tt(i).le.Stime) go to 92
	Sname=phcd(i)
	Stime=tt(i)
92	continue

	if(phcd(i).ne.'Sn      ') go to 93
	if(tt(i).le.Stime) go to 93
	Sname=phcd(i)
	Stime=tt(i)
93	continue

C	Now find P again, as above
	if(phchar(1).ne.'P') go to 95
	if(tt(i).ge.Ptime) go to 95
	Ptime=tt(i)

95	continue

c	Harley has spoken: If we didn't find any of the above
c	phases, use P+20minutes.
	if(Stime.eq.0) Sname='P+20min'
	if(Stime.eq.0) Stime=Ptime+20*60

c	See if we should use fixed time past P
	if(xfixedTime.eq.0) go to 202
	Sname='P+fixed'
	Stime=Ptime +xfixedTime*60
202	continue

	write(8,710)Stime,xdeg,xdepth,Sname
710	format(f7.2,5X,'# deg:',f5.1,2X,'z:',f5.1,3X,A10)
201	continue
	write(8,*)' '
200     continue

	stop
	end
