	program RayLocator
c
c   The main program gets the command line argument and opens input,
c   output, and error files.  It then drives reading the input file,
c   relocating the event, and writing the output file.
c
	implicit none
c
	character*8 phlst(2)
	character*1000 argv
c
	logical*4 prnt(3)
c
	integer*4 ntrbl,ln,nphase,nexit
	integer*4 iargc,nargs
c	integer stdin, stdout
c
	integer*4 min,mout,merr
c
	common /unit/ min,mout,merr
c
	real*4 pi,rad,deg,radius
c
	common /circle/ pi,rad,deg,radius
c
	include 'hypocm.inc'
c
	integer*4 miter1,miter2,miter3
	real*4 tol1,tol2,tol3,hmax1,hmax2,hmax3,tol,hmax,zmin,zmax,
	1	zshal,zdeep,rchi,roff,utol
c
	common /itcntl/ miter1,miter2,miter3,tol1,tol2,tol3,
	1	hmax1,hmax2,hmax3,tol,hmax,zmin,zmax,
	2	zshal,zdeep,rchi,roff,utol
c
c IGD 06/01/04 Increased static array to allow for pathname
	character*400 modnam

	character*1000 tokenName
	character*1001 dirname

	common /my_dir/dirname

	integer*4 mlu
c
	common /vmodel/mlu,modnam
c
c Define values carried around in the common block circle
c
	data pi/3.1415926536/, radius/6371.0/
c
c Set the robust regression values
c
	data miter1,miter2,miter3,tol1,tol2,tol3,hmax1,hmax2,hmax3,zmin,
     1 zmax,zshal,zdeep,rchi,utol/10,10,50,3.,3.,2.,500.,300.,30.,0.,
     2 800.,33.,600.,.45,20./
c
c Hard wire the model.
c
	data modnam,phlst,prnt/'ak135','ALL',' ',3*.false./
c
c	stdin = 5
c	stdout= 6
c
c Define the values needed to convert between radians and degrees that are
c   carried around in the common block circle
c
	rad = pi / 180.0
	deg = 180.0 / pi
c
c   Set the randomizer limit.
c
	roff=.58984375+(rchi-.375)
c
c   Get the command line.
	nargs = iargc()
	if( nargs .lt. 2 ) go to 999
	call getarg(1,argv)
        if(argv.ne.'-i') go to 999
	call getarg(2,argv)
	ln=ntrbl(argv)
	tokenName=argv

	if (nargs.eq.4) then	
		call getarg(3,argv)
	        if(argv.ne.'-d') go to 999
		call getarg(4,argv)
		dirname=argv
	else
		dirname='.'
	endif


c
c Initialize the reference earth model
c
	call freeunit(mlu)
	call tabin(mlu,dirname(1:ntrbl(dirname))//'/'//modnam)
	call brnset(1,phlst,prnt)
c
	call freeunit(merr)
	open(merr,file=dirname(1:ntrbl(dirname))//'/'//'RayLocError'//tokenName(1:ln)//'.txt',
	1 access='sequential',form='formatted',status='new')
c
c Read in the starting hypocenter, analyst commands, and phase data
c
	call freeunit(min)
	open(min,file=dirname(1:ntrbl(dirname))//'/'//'RayLocInput'//tokenName(1:ln)//'.txt',
	1	access='sequential',form='formatted',status='old')
	rewind(min)
c
	call input(nphase)
	close(min)
c
c Locate the earthquake
c
	call hypo(nphase)
c
c Calculate the adjustment to the hypocenter parameters and magnitudes,
c  do the final statistics, and write it out.
c
	call freeunit(mout)
	open(mout,file=dirname(1:ntrbl(dirname))//'/'//'RayLocOutput'//tokenName(1:ln)//'.txt',
	1	access='sequential',form='formatted',status='new')
	call output(nphase)
c
	close(mout)
c   Set the exit flag.
	if(ng.eq.' ') then
	  nexit=0
	else if(ng.eq.'o') then
	  nexit=1
	else if(ng.eq.'q') then
	  nexit=2
	else if(ng.eq.'u') then
	  nexit=3
	else if(ng.eq.'s') then
	  nexit=4
	else
	  write(merr,*)'Unknown status flag (ng = ',ng,').'
	  nexit=5
	endif
	close(merr)
	call exit(nexit)
c
c   Command line error.
999	print *,'Command line USAGE:'
	print *,'RayLocator -i Origin_ID -d dirName'
	call exit(103)
	end
