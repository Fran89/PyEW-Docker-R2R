c--Reads a hypoinverse p delay file & writes an s delay file
c  using the POS value specified
	character sta*8,sou*1

	call iofl
	pos=askr('P to S velocity ratio',1.75)
2	read (2,1000,end=9) sta,p,sou
1000	format (a8,f6.2,1x,a1)
	s=p*pos
	write (3,1000) sta,s,sou
	goto 2
9	stop
	end

