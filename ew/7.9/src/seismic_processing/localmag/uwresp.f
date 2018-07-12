      subroutine getpzg(sta, chan, date, gain, nump, poles, numz, 
     $     zeros)
c........................................
c........................................

      character*1 band,instrum,orient,unit
      character*10 cmpnt(50)
      character*3 chan
      character*3 helic
      character*4 sta
      character*5 seisid,recsn
      character*5 seisty
      character*5 amptyp,discty,filter,allsta,recty
      character*50 calibf
      complex a(50), b(50)
      complex cdisp, cvel, cacc
      complex poles(50),zeros(50)
      dimension ntest(5)
      double precision amp,sd,a0
      integer date,date1,date2
      integer flag
      integer nump, numz
      logical opn
      real natper,recdr

      common date1,date2,flag,ampdb,natper,damp,efftra
      common /ampl/amp
      common /channel/band,instrum,orient
      common /chrblk/amptyp,discty,filter,seisty,seisid,recty,recsn,unit
      common /comps/cmpnt
      common /deebee/dbfac,attenfac,recdr,recgain
      common /helon/helic
      common /nzp/nzero,npole
      common /poles/a
      common /zeros/b

c...define calibration data input file for respcurve.f

      calibf ='/stor/seis/calib.sta'

      nump = 0
      freq = 1.0

      unit='v'

c.......................................
c....INITIALIZE THE ONE DIMENSIONAL ARRAY ntest(5), 
c....USED TO TURN COMPONENTS "ON" OR "OFF"
c....IN THE CALCULATION (SEE COMMENT REGARDING ntest(5) BELOW)
c.......................................
      do 4 l=1,5
         ntest(l)=1
 4    continue

c...Normally helicorder option is off
      helic='no'

      inquire(unit=2, opened=opn)
      if ( opn .eqv. .false. ) then
         open(2,file=calibf,status='old')
      endif
      rewind(2)


c...................................
c....RETRIEVE CALIBRATION DATA FROM THE 'calib.sta' FILE HERE; IF
c....THE STATION ISN'T LISTED IN THAT FILE, A MESSAGE TO THAT EFFECT
c....WILL APPEAR.
c...................................

      call initia(sta,chan,date)

c..."flag" and "date1" are variables used in 
c...initia(sta,chan,date); flag=0 when
c...the station wasn't found in the data file, 
c...date1 is the earliest date
c...for which calibration data is available for the station.

      if (flag .eq. 0) go to 99
      if (date1 .gt. date) go to 99

c...................................
c....A CALL TO SETUP defines poles, zeros, and amplitude of
c...complex functions used to compute response of seismograph
c...system.vcodis(ntest)
c...................................

      call setup(sta,chan,ntest)

c...Make assignments to pass back from subroutine;           

      gain = real(amp)
c convert amplification so response input is nanometers
      gain = gain / 1.0e9

      do 97 i = 1, nzero
         zeros(i) = b(i)
 97   continue
      numz = nzero
c add some poles to switch to displacement response
      if (unit .eq. 'a') then
         numz = numz + 1
         zeros(numz) = cmplx(0,0)
         numz = numz+1
         zeros(numz) = cmplx(0,0)
      else if (unit .eq. 'v') then
         numz = numz + 1
         zeros(numz) = cmplx(0,0)
      endif

      nump = npole
      do 98 i = 1, npole
         poles(i) = a(i)
 98   continue
 99   return
      
c...
      
      end
c...####################################################################


c........................................
c...subroutines and functions libresp for use by
c...respcurve and response (for use by podblocks
c...RESPCURVE IS A REWRITE OF AN OLDER VERSION. RESPCURVE NOW COMPUTES
C...RESPONSE IN COMPLEX PLANE USING a specification of
c...system in terms of complex  poles and zeros
c...
c....'RESPCURVE' USES CALIBRATION DATA IN THE 'calib.sta' FILE TO

c....COMPUTE THE GROUND MOTION RESPONSE (MAGNIFICATION) OF A 
c....SEISMOGRAPH STATION IN UNITS OF DIGITAL COUNTS/METER OF 
c....DISPLACEMENT OR DIGITAL COUNTS/(METER/SECOND)  OR DIGITAL
c....COUNTS COUNTS/(METER/SECOND**2) DEPENDING ON SPECIFICATION IN
c....-type OPTION.
c........................................

      subroutine initia(sta,chan,date)
c...####################################################################
c.........................................
c......'INITIA(STA,chan,FILE)' GOES INTO THE calib.sta FILE AND
c RETRIEVES ALL THE
c......PERTINENT DATA TO BE USED IN 'MAGNFY' AND 'VCODIS'; STA, ASSIGNED
c......IN THE MAIN PROGRAM, IS THE THREE-CHARACTER STATION NAME.  THIS
c......SUBROUTINE SHOULD BE CALLED IN THE MAIN PROGRAM PRIOR TO USE OF
c......'MAGNFY'.
c.........................................

c...sta is a 3 or 4 character station name
c...Routine gets station parameters from calib.sta.
      character*1 band,instrum,orient,unit
      character*3 stafnd
      character*5 seisty
      character*5 seisid
      character*5 amptyp,discty,filter,recty,recsn
      character*4 sta,st
      character*3 chan, ch
      character*120 line

      real natper
      integer date,date1,date2
      integer flag

      common /channel/band,instrum,orient
      common date1,date2,flag,ampdb,natper,damp,efftra
      common /chrblk/amptyp,discty,filter,seisty,seisid,recty,recsn,unit
      common /deebee/dbfac,attenfac,recdr,recgain
      flag=0
      stafnd='no'
      
c...Start looking for station on appropriate date
 4    continue
      
      read(2,299,end=20) line
 299  format(a)
c...ignore special comment cards (# in col 1)
      if(line(1:1).eq."#") go to 4
      
      read(line,2000)st,band,instrum,orient,date1,date2,
     1     seisid,seisty,natper,damp,efftra,unit,attenfac,amptyp,
     2     ampdb,discty,filter,recty,recgain,recdr,recsn

 2000 format(a4,1x,3a1,1x,i8,1x,i8,1x,a5,1x,a5,1x,
     1     f8.4,1x,f4.2,1x,f6.1,1x,a1,1x,f5.3,1x,a5,1x,f6.2,
     2     1x,a5,1x,a1,1x,a5,1x,
     3     f6.2,1x,f6.2,1x,a5)

      ch(1:1)=band
      ch(2:2)=instrum
      ch(3:3)=orient
      
      if (st .eq. sta .and. ch.eq.chan)then 
         stafnd='yes'

         if (date1 .le. date .and. date2 .ge. date)then
            flag=1
            go to 20
         elseif(date1 .gt. date)then
            write(0,1000)date1,sta
 1000       format('** station calibration data begins ',
     1           i8,' for station ', a4)
            go to 20
         endif
      endif

      go to 4

 20   close (2)
      if (flag .eq. 0) then
	 write (0,1010)sta,chan,date
 1010    format('** station (and channel) ',a4,a3,' not in data',
     1        ' file for date specified.',1x,i8)
      endif
      return
      end

      subroutine setup(sta,chan,ntest)
c............................................
c......SUBROUTINE SETUP ASSIGNS values of poles (complex array "a")
c...and zeros used to characterize
c...the reponse of seismic system. 
c...It also computes normalizing amplitude "amp" and the number of
c...poles and zeros (npole and nzero). Values are returned through
c...labeled common. Response  of seismic system
c...is taken to be of the form
c...
c...F(s) = amp*Z(s)/P(s)
c...
c...where
c...Z(s)= (s-b(1))*(s-b(2))*(s-b(3))*...*(s-b(nzero))
c...P(s)= (s-a(1))*(s-a(2))*(s-a(3))*...*(s-a(npole))
c...
c...s=i*w, i=sqrt(-1), w=2*pi*f, f=Frequency at which response is
c...desired, nzero= number of zeros 
c...npole= number of poles. Poles are put in complex array "a".
c...zeros are put in complex array "b".
c...Computed value "amp" is real.

c...Note function lo1 defines 1-pole low pass butterworth filter
c...Note function lo2 defines 2-pole low pass butterworth filter
c...Note function hi1 defines 1-pole high pass butterworth filter
c...Note function hi2 defines 2-pole high pass butterworth filter
c..................................................
c......ntest(5) IS A ONE DIMENSIONAL ARRAY WHOSE ELEMENTS SPECIFY THE
c......COMPONENT(S) OF THE SEISMOGRAPH SYSTEM FOR WHICH RESPONSE CURVES
c......WILL BE CALCULATED.  THE FIRST THROUGH FIFTH ELEMENTS CORRESPOND
c......TO SEISMOMETER, VCO, DISCRIMINATOR, RECORDER, AND BANDPASS
c......FILTER, RESPECTIVELY.  IF THE RESPONSE OF THE ENTIRE SYSTEM IS
c......DESIRED, THEN ALL FIVE ELEMENTS OF ntest MUST EQUAL 1 (ELEMENTS
c......ARE ASSIGNED IN THE MAIN PROGRAM).  IT IS POSSIBLE TO ISOLATE
c......THE RESPONSE OF ANY ONE OR A COMBINATION OF COMPONENTS BY 
c......LETTING ONLY THE ELEMENTS OF ntest CORRESPONDING TO THE DESIRED
c......COMPONENTS EQUAL 1.  FOR EXAMPLE, TO OBTAIN THE VCO-DISCRIMINATOR
c......RESPONSE ONLY, THE FIVE ELEMENTS OF NTEST would have the
c...values (0 1 1 0 0).

c...Note ntest function will not work if poles and zeros read in from a
c file
c..................................................

c......SETUP SHOULD BE CALLED
c......AFTER INITIA BUT BEFORE FREQUENCY RESPONSE IS CALCULATED
      real natper
      real lo1,lo2
      integer date1
      integer flag
      integer oldpole
      dimension ntest(5)
      double precision amp,am
      complex a(50),b(50),z(50),p(50)

      character*1 unit
      character*3 helic
      character*5 seisty
      character*5 seisid
      character*5 recty,recsn
      character*5 amptyp,discty,filter
      character*10 cmpnt(50)

      character*4 sta
      character*3 chan
      character*8 stachan
      character*100 filenm,atmp

      common /comps/cmpnt
      common /nzp/nzero,npole
      common /poles/a
      common /zeros/b
      common /helon/helic
      common /chrblk/amptyp,discty,filter,seisty,seisid,recty,recsn,unit
      common /deebee/dbfac,attenfac,recdr,recgain
      common date1,date2,flag,ampdb,natper,damp,efftra
      common /ampl/amp


c...Initialize the critical parameters
      nzero=0
      npole=0
      oldpole=0
      amp=1.0

c--------------
c...New section added here Oct 28, 1999 to allow getting parameters from
c...a poles and zeros file instead of procedure in rest of routine.

      if(seisid.eq.'FILE' .or. seisid.eq.' FILE') then
c...Create station-channel name and create directory name of 
c...corresponding poles and zeros file
         call concat(sta,".",atmp)
         call concat(atmp,chan,stachan)

         call concat("/stor/seis/calib.poles_zeros/",stachan,filenm)

         call getpoles(filenm, date1,am,nz,z,np,p,unit,nchek) 

         if(nchek.ne.1) then
            write(0,334) stachan, filenm
 334        format('Calibration parameters for ',a8,' on specified ',
     1           'date not found in file: ',/a75)
            return
         endif

         ntest(1)=1

         amp = am
c...Assume all zeros have zero real and imaginary parts
         nzero=nz
         npole=np
         do 77 i=1,np
            a(i)=p(i)
 77      continue
         do 78 i=1,nz
            b(i)=z(i)
 78      continue
         
         return
      endif
c--------------
      

c...****************** SEISMOMETER ****************************
c...
      if (ntest(1) .eq. 1) then
         
         if(seisty.eq.'GRNSN')then
c...Case of Guralp NSN type seismometer (velocity Response)
c...Combination of 1-pole components: 2 high-pass with
c...corner periods of 200 and 30 sec; and three low-pass
c...with corner frequencies of 267, 75, 30 Hz.
            amp=amp*hi1(0.005)
            amp=amp*hi1(0.03333)
            amp=amp*lo1(267.0)
            amp=amp*lo1(75.0)
            amp=amp*lo1(30.0)

         elseif(seisty.eq.'GXNSN')then
c...TEST Case of Guralp NSN type seismometer (velocity Response)
            amp=amp*hi1(1.0)

         elseif(seisty.eq.'ES-T')then
c...Kinemetrics FBA EpiSensor ES-T Force balance accelerometer
c...Note: this unit is effectively a cascade of two 2-pole
c...low pass filters with corner frequencies of
c...224 Hz (beta .6971) and 561 Hz (beta .9336)

c...10/24/00 modified this section to specify poles and zeros directly
c...There are no zeros
            nzero=0
c...Now specify 4 poles
            a(1) = cmplx( -981.0, 1009.0)
            a(2) = cmplx( -981.0,-1009.0)
            a(3) = cmplx(-3290.0, 1263.0)
            a(4) = cmplx(-3290.0,-1263.0)
            npole=4
            amp=amp*2.46e+13

            if(unit.ne.'a') then
               write(0,2345)
 2345          format('Warning. ES-T unit in calib.sta should be ',
     $              'specified as an accelerometer')
            endif

         else

c...Case of generic seismometer or accelerometer
            ffo=(1.0/natper)

            if(unit.eq.'a') then
c...We assume an accelerometer response, flat to  ground
c...acceleration at low frequency

               amp=amp*lo2(ffo,damp)

            elseif(unit.eq.'v') then

c...We assume generic seismometer with response flat to velocity at
c...high frequency

               amp=amp*hi2(ffo,damp)

            elseif(unit.eq.'d') then

c...We assume generic seismometer with response flat to displacement at
c...high frequency

               amp=amp*hi2(ffo,damp)
            else
               write(0,455) unit
 455           format('ERROR. Routine setup. unit=',a1)
               return

            endif

         endif

 33      amp=amp*efftra

         call addcmp(oldpole,npole,cmpnt,'Seismomter')

      endif
c...
c...******************** Account for Attenuator
c *****************************
c...

c...Now account for the seismometer attenuator if present
c...(assumed to have no frequency dependence)
      amp=amp*attenfac


c...
c...******************** AMPLIFIER/VCO *****************************
c...
c...write(6,5572)ntest(2),amptyp
 5572 format('   Before check of ntest2 ## amptyp is', i5,a5)

      if (ntest(2) .eq. 1) then

c...write(6,5571)amptyp
 5571    format('   ## amptyp is',a5)
         if (amptyp .eq. 'SLU')then
            amp=amp*lo2(18.5,0.8)*lo2(33.9,0.6)*lo2(64.0,0.7)
            amp=amp*hi2(0.0094,1.2)
            amp=amp*10**(ampdb/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif (amptyp .eq. 'McVCO' .or. amptyp .eq. 'McVC2')then
c...Pat McChesney VCO. Assume a 4-pole low filter with 30 Hz corner
c...write(6,5561)
 5561       format('***** We are in the McVCO section !!')
            amp=amp*buttlo(30.0,4)
            amp=amp*10**(ampdb/20.0)
            if(amptyp .eq. 'McVC2') then
c...Some USGS installations (SEP, YEL) use 115V/4.05V
c...See Pat McChesney
               amp=amp*28.4
            else
c...Standard Intrinsic sensitivity of vco is 125Hz/3volt. 
               amp=amp*41.666667
            endif

         elseif (amptyp .eq. 'AS110' .or. amptyp .eq. 'AS111')then
c...Sprengnether AS110 (AS111 is an artificial designation
c...for an AS110 with VCO stage sensitivity set at
c...125Hz/5.0Volt
            amp=amp*lo2(30.0,1.0)
            amp=amp*hi2(0.3,0.8)*hi2(0.3,0.6)
            amp=amp*10**(ampdb/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
c...AS111 uses 125Hz/5volt.
            if(amptyp .eq. 'AS111')then
               amp=amp*25.0
            else
               amp=amp*41.666667
            endif
            
         elseif (amptyp .eq. '42.50' )then
c...Geotech 42.50 Amp VCO
            amp=amp*lo2(25.0,0.8)
            amp=amp*hi2(0.2,0.8)
            amp=amp*10**(ampdb/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif (amptyp .eq. '6202' .or. amptyp .eq. 'MCU')then
            amp=amp*lo2(29.9,0.7)
            amp=amp*hi2(0.09,0.75)
            amp=amp*10**((120.0-ampdb)/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif (amptyp .eq. '6242')then
            amp=amp*lo2(28.0,0.7)*lo2(81.0,0.7)
            amp=amp*hi2(0.05,1.1)
            amp=amp*10**((90.0-ampdb)/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif ((amptyp.eq.'J302').or.(amptyp.eq.'J402').or.
     1           (amptyp.eq.'J402L')) then
            amp=amp*lo2(59.0,1.0)*lo2(64.0,0.8)
            amp=amp*hi2(0.08,1.2)*hi2(0.012,1.6)
            amp=amp*10**((90.0-ampdb)/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif ((amptyp.eq.'J502').or.(amptyp.eq.'J512').or.
     1           (amptyp.eq.'J402L')) then
            amp=amp*hi1(0.1)*hi1(0.1)
            amp=amp*lo1(48.0)*lo1(49.0)
            amp=amp*10**((90.0-ampdb)/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif (amptyp .eq. 'NEV')then
            amp=amp*lo2(13.0,0.8)
            amp=amp*hi2(0.056,1.0)
            amp=amp*10**((86.0-ampdb)/20.0)
c...Intrinsic sensitivity of vco is 125Hz/3volt. 
            amp=amp*41.666667

         elseif (amptyp .eq. 'SWAN')then
c...Synthetic WoodAnderson at Seattle. North-South (SEN).
c...This Section accounts for gain of integrator-amplifier
c...which has 1/f response at freq >>0.16 Hz.
c...There is also a front end high pass filter
            amp=amp*lo1(0.16)
            amp=amp*hi1(0.18)
            amp=amp*9.06*10.0**(ampdb/20.0)

         elseif (amptyp .eq. 'SWAE')then
c...Synthetic WoodAnderson at Seattle. East-West (SEE).
c...This Section accounts for gain of integrator-amplifier
c...which has 1/f response at freq >>0.16 Hz.
c...There is also a front end high pass filter
            amp=amp*lo1(0.16)
            amp=amp*hi1(0.18)
            amp=amp*9.38*10.0**(ampdb/20.0)

         elseif (amptyp .eq. 'reftk' .or. amptyp .eq. 'terra'
     1           .or. amptyp .eq. 'kinem') then
c...Go for generic amplifier where the gain in counts/volt is 
c...10**(value in table)

            amp=amp*10**ampdb

         elseif (amptyp .eq. 'none')then
c...Test
            continue


         else
            write(0,79)amptyp
 79         format(2x,a5,1x,'Unknown AMP or VCO Type ******')
            amp=0.0
         endif

         call addcmp(oldpole,npole,cmpnt,'Amplf(vco)')
      endif
c...
c...******************* DISCRIMINATOR **************************
c...
      if (ntest(3) .eq. 1) then
         if (discty .eq. 'J101')then
            amp=amp*lo2(44.0,1.0)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. '6243')then
            amp=amp*lo2(30.5,0.85)*lo2(53.8,0.65)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

c...Temporaily make the J120 be similar to the J110 until we put in
c...better parameters for the J120
         elseif (discty .eq. 'J110' .or. discty .eq. 'J120')then
            amp=amp*lo2(20.8,0.55)*lo2(26.5,0.65)
            amp=amp*lo2(29.7,0.65)*lo2(34.3,0.65)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. 'NPF')then
            amp=amp*lo2(25.75,1.05)*lo2(64.5,0.7)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. 'SLU')then
            amp=amp*lo2(17.1,0.95)*lo2(25.3,0.65)*lo2(35.0,0.6)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. '6203')then
            amp=amp*lo2(15.8,1.2)*lo2(45.8,0.7)*lo2(44.5,0.65)
            amp=amp*lo2(99.0,0.8)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. 'SPRNG')then
            amp=amp*lo2(15.7,0.8)*lo2(25.8,0.65)*lo2(37.5,0.65)
            amp=amp*lo2(46.5,0.65)
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. 'NEV')then
            amp=amp*0.24805
c...Intrinsic sensitivity of discriminator is 3volt/125 Hz
            amp=amp*0.0240

         elseif (discty .eq. 'SWAN')then
c...Final  Antialias lowpass filter
c...section of NS Synthetic WoodAnderson response
c...(8-pole filter corner freq=33 hz, beta=0.65)
            amp=amp*lo2(33.0,0.65)*lo2(33.0,0.65)
            amp=amp*lo2(33.0,0.65)*lo2(33.0,0.65)

         elseif (discty .eq. 'SWAE')then
c...Final  Antialias lowpass filter
c...section of EW Synthetic WoodAnderson response
c...(8-pole filter corner freq=33 hz, beta=0.65)
            amp=amp*lo2(33.0,0.65)*lo2(33.0,0.65)
            amp=amp*lo2(33.0,0.65)*lo2(33.0,0.65)

         elseif (discty .eq. 'none')then
            continue

         else
            write(0,80)discty
 80         format(2x,a5,1x,'Unknown Discriminator Type ******')
            amp=0.0

         endif

         call addcmp(oldpole,npole,cmpnt,'Discrimter')
      endif
c...
c...****************** RECORDER *********************************
c...
      if (ntest(4) .eq. 1) then


         if(helic.eq.'yes')then

c...Helicorder recorder option
c...Note helicorder gain at freq less than 28 Hz is 0.030 m/volt
c...at 6db atten. dbfac gives a compensating factor 
c...at other attenuator db settings.

            amp=amp*lo2(28.0,0.65)
            amp=amp*0.030*dbfac

         elseif(recty(1:3).eq.'72A')then
            amp=amp*10**recgain

         elseif(recty.eq.'ralph')then
            amp=amp*10**recgain

         elseif(recty.eq.'worm')then
            amp=amp*10**recgain

         elseif(recty.eq.'IDS24' .or. recty.eq.'IDS20' .or.
     1           recty.eq.'ids24' .or. recty.eq.'ids20') then
            amp=amp*10**recgain

         elseif(recty.eq.'none')then
            continue

         elseif(recty.eq.'k2')then
            amp=amp*10**recgain

         else
            write(0,81)recty
 81         format(2x,a5,1x,'Unknown Recorder Type ******')
            amp=0.0

         endif

         call addcmp(oldpole,npole,cmpnt,'Recorder  ')
      endif


c...******************* OPTIONAL ANTIALIAS FILTER ********************
c..........................................
c......IF AN ANTIALIAS FILTER HAS BEEN ADDED TO THE SYSTEM AT THE DIS-
c......CRIMINATOR END, MULTIPLY IN THE EFFECTS OF ITS RESPONSE HERE.
c.........................................

      if(ntest(5) .eq. 1 .and. filter .eq.'y') then
         amp=amp*buttlo(28.5,8)
         call addcmp(oldpole,npole,cmpnt,'Aliasfiltr')
      endif

 99   return
      end


      real function hi1(f0)
c...Defines the pole for a 1-pole butterworth filter (HIGH PASS)
c...The Complex pole is stored in array "a" iIt is
c...placed after any other
c...complex poles already there.
c...Index "npole" is incremented by one
c...The function returns the amplitude 1.0,
c...the value implied in the numerator of the complex representation
c...of the filter

c...Z(s) = s/(s-a1)
c...
c...where s = i*w,  and i=sqrt(-1), w = 2*pi*f, a1 is the pole
c...which is real and equal to -2*pi*f0

      common /nzp/nzero,npole
      complex a(50),b(50)
      common /poles/a
      common /zeros/b

      pi=4.0*atan(1.0)
      w0 = 2.0*pi*f0
      npole=npole+1
      a(npole)=-w0

c...Counter npole was incremented to indicate the number 
c...of complex poles
c...currently contained in complex array "a"
c...Also increment another counter to indicate that this filter
c...has one zero (at s=0) associated with it.

      nzero=nzero+1
      b(nzero)=0.0

      hi1 = 1.0
      return
      end
c.................................................
      real function hi2(f0,beta)
c...Defines the poles for a 2-pole butterworth filter (HIGH PASS)
c...The two Complex poles are stored in array "a" (they are
c...placed after any other
c...complex poles already there.
c...Index "npole" is incremented by two
c...The function returns the amplitude 1.0,
c...the value implied in the numerator of the complex representation
c...of the filter

c...Z(s) = s**2/(s-a1)*(s-a2)
c...
c...where s = i*w,  and i=sqrt(-1), w = 2*pi*f, a1, a2 are the poles.

      common /nzp/nzero,npole
      complex a(50),b(50)
      common /poles/a
      common /zeros/b

      pi=4.0*atan(1.0)
      w0 = 2.0*pi*f0
      call pol2(w0,beta,a(npole+1),a(npole+2))
      
      b(nzero+1)=0.0
      b(nzero+2)=0.0

c...Increment counter to indicate the number of complex poles
c...currently contained in complex array "a"
c...Also increment another counter to indicate that this filter
c...has two zeros (at s=0) associated with it.

      npole=npole+2
      nzero=nzero+2

      hi2 = 1.0
      return
      end
c.....................................................
      real function lo1(f0)
c...Defines the pole for a 1-pole butterworth filter (LOW PASS)
c...The Complex pole is stored in array "a". It is
c...placed after any other
c...complex poles already there.
c...Index "npole" is incremented by one
c...The function returns the amplitude (2*pi*f0),
c...the value implied in the numerator of the complex representation
c...of the filter

c...Z(s) = (2*pi*f0)/(s-a1)
c...
c...where s = i*w,  and i=sqrt(-1), w = 2*pi*f, a1 is the pole
c...which is real and equal to -2*pi*f0

      common /nzp/nzero,npole
      complex a(50),b(50)
      common /poles/a
      common /zeros/b

      pi=4.0*atan(1.0)
      w0 = 2.0*pi*f0
      npole=npole+1
      a(npole)=-w0

c...Counter npole was incremented to indicate the number 
c...of complex poles
c...currently contained in complex array "a"

      lo1 = w0
      return
      end
c.....................................................
      real function lo2(f0,beta)
c...Defines the poles for a 2-pole butterworth filter (LOW PASS)
c...The two Complex poles are stored in array "a" (they are
c...placed after any other
c...complex poles already there.
c...Index "npole" is incremented by two
c...The function returns the amplitude (2*pi*f0)**2 which is the normal
c...value found in the numerator of the complex representation
c...of the filter

c...Z(s) = ((2*pi*f0)**2)/(s-a1)*(s-a2)
c...
c...where s = i*w,  and i=sqrt(-1), w = 2*pi*f, a1, a2 are the poles.

      common /nzp/nzero,npole
      complex a(50),b(50)
      common /poles/a
      common /zeros/b

      pi=4.0*atan(1.0)
      w0 = 2.0*pi*f0
      call pol2(w0,beta,a(npole+1),a(npole+2))

c...Increment counter to indicate the number of complex poles
c...currently contained in complex array "a"
      npole=npole+2

      lo2 = w0**2
      return
      end

      subroutine pol2(wc,beta,a1,a2)
c...returns two complex poles a1, a2
c...of two-pole butterworth filter
c...corresponding to corner freq wc (in radians) and
c...damping value beta (fraction of critical)
c...
c...a1 = -wc*beta + i * wc * sqrt(1-beta**2)
c...a2 = -wc*beta - i * wc * sqrt(1-beta**2)
c...
c...where i = sqrt(-1)

c...NOTE: if beta > 1    a1, a2 are real and unequal
c...= 1    a1, a2 are real and equal
c...< 1    a1, a2 are complex conjugates

      complex a1,a2,ci,rad


c...Define ci=sqrt(-1)

      ci=(0,1)
      rad=1.0-beta**2
      a1=wc*(-beta + ci*csqrt(rad))
      a2=wc*(-beta - ci*csqrt(rad))

      return
      end

      real function buttlo(f0,norder)

c...Defines the poles for a n-order low-pass butterworth filter 
c...The norder Complex poles, all on the left hand side of the
c...complex s plane, are stored in complex array "a" (they are
c...placed after any other
c...complex poles already there.
c...Index "npole" is incremented by n by call to buttlo.
c...The function returns the amplitude (2*pi*f0)**norder 
c...which is the normal
c...value found in the numerator of the complex representation
c...of the filter

c...Z(s) = ((2*pi*f0)**norder)/(s-a1)*(s-a2)*..*(s-anorder)
c...
c...where s = i*w,  and i=sqrt(-1), w = 2*pi*f, a1, a2 .. are the 
c...complex poles.

      common /nzp/nzero,npole
      complex eye,a(50),b(50)
      common /poles/a
      common /zeros/b

      pi=4.0*atan(1.0)
      eye=cmplx(0.0,1.0)
      w0 = 2.0*pi*f0

      if(norder .lt. 0 .or. norder .gt. 10) then
         write(0,1000)norder
 1000    format(i5, 'Order of butterworth filter out of bounds')
      endif

      do 100 k=1,norder
         arg=pi*(0.5 + (2.0*k-1.0)/(2.0*norder) )
         npole=npole+1
         a(npole)= w0*cexp(eye*arg)
 100  continue

c...Now define numerator amplitude factor

      buttlo=w0**norder

      return
      end

c################

      subroutine addcmp(oldpole,npole,cmpnt,label)
c...Produces an array of labels corresponding to computed poles. That is
c...it keeps track of whether poles correpond to seismometer, recorder,
c etc.

      integer npole, oldpole
      character*10 cmpnt(50),label
      if(npole.gt.oldpole)then
         do 10 i=oldpole+1,npole
            cmpnt(i)=label
 10      continue
      elseif(oldpole.lt.0 .or. npole.lt.0 .or. oldpole.gt.npole)then
         write(0,1000)oldpole,npole
 1000    format('ADDCMP: Illegal values for oldpole, npole ',2i5)
         return
      endif

      oldpole=npole
      return
      end

      subroutine argin2(card,maxar,narg,arg,nchs,loc)
c...This routine is a free-format cracker.
c...A revised version of argin.f

c...from character variable "card" containing one to 50 arguments
c...separated by blanks, routine determines the number of arguments,
c..."narg" and stores them in character variable array "arg".

c...MUST INPUT: card and maxar

c...Routine looks for a maximum of maxar arguments.  

c...Routine also returns nchs, the number of characters in card.
c...The loc array returns
c...the index of the first character of each argument in card.
c...ie, if loc(3)=21, the third argument in card 
c...starts in column 21.
c...Make sure arg and loc are dimensioned in calling routine
c...to at least the maximum number of arguments desired.
c...Author T. Qamar 8/18/87 revised 9/11/89

      character*(*) card
      character*(*) arg(1)
      dimension loc(1)
      character*3 new,grabit
      character*1 blnk,tab
      parameter (blnk=' ')
      parameter (tab='	')

c...lenstr determines number of characters in card.
      nchs=lstrng(card)
      new='yes'
      grabit='no'
      narg=0

      if(nchs.eq.0) then
         arg(1)=""
         return
      endif

      do 100 i=1,nchs

c...Blanks  and tabs separate arguments

         if(card(i:i).ne.blnk .and. card(i:i).ne.tab)then

c...go here for non-blank character
            if(new.eq.'yes')then

c...go here for new argument

c...Return if we have already processed enough args.
               if(narg.eq.maxar)go to 5000
               narg=narg+1
               loc(narg)=i
               nchar=1
               new='no'
               grabit='yes'
            else
c...go here if still working on an argument

               nchar=nchar+1
            endif
         else
            new='yes'

            if(grabit.eq.'yes')then
               arg(narg)=card(loc(narg):loc(narg)+nchar-1)
               grabit='no'
            endif
         endif
 100  continue

      if(grabit.eq.'yes')then
         arg(narg)=card(loc(narg):loc(narg)+nchar-1)
         grabit='no'
      endif


 5000 return
      end

      function lstrng(char)
c...Determines the length of char.  char may contain
c...blanks except that any trailing blanks are not included in the
c...computation.
      character*(*) char

      lstrng=0

c...Get dimension of char

      length=len(char)
      if(length.le.0)return

      do 100 n=length,1,-1
         if(char(n:n).ne.' ')then
            lstrng=n
            return
         endif
 100  continue

      return
      end
      subroutine concat(a,b,c)
c...Concatenates character strings a and b to produce c
      character*(*) a,b,c

      c=''
c...get storage length of c
      len_c=len(c)

      na=lnblnk(a)
      nb=lnblnk(b)
      nc=na+nb
      ndif=len_c-nc

      if(na.eq.0) go to 10

      if(na.le.len_c) then
         c(1:na)=a(1:na)
      else
         c(1:len_c)=a(1:len_c)
         write(0,55) a(1:lnblnk(a)),b(1:lnblnk(b))
 55      format('concat: Warning. Not enough space for ',
     1        'concatenation of'/a/a)
         return
      endif
 10   continue

      if(nc.le.len_c) then
         if(nc.eq.na) then
	    return
         else
	    c(na+1:nc)=b(1:nb)
         endif
      else
         c(na+1:len_c)=b(1:len_c-na)
         write(0,55) a(1:lnblnk(a)),b(1:lnblnk(b))
         return
      endif

      return
      end
      subroutine staname (stachan,sta,chan)
c...give a station name of form SHW.EHZ, this routine splits
c...it into components ie sets sta=SHW" and chan="EHZ"
c...If no channel is specified chan is set to EHZ

      character*(*) stachan,sta,chan
      character*20 rest

      sta=""
      chan=""

      n= index(stachan,".")

      if (n.eq.0) then
c...Only station name was specified
         sta(1:len(sta))=stachan
         chan='EHZ'
         return
      elseif (n.eq.1) then
         sta=""
         chan(1:len(chan))=stachan(2:)
         return
      elseif (n.gt.1) then
         sta(1:len(sta))=stachan(1:n-1)
         rest=stachan(n+1:)
         n=index(rest,".")
         if(n.eq.0) then
            chan(1:len(chan))=rest
         elseif (n.eq.1) then
            chan='EHZ'
         else
            chan(1:len(chan)) = rest(1:n-1)
         endif
         return
      endif
      return
      end


      subroutine getpoles(filename, idate,am,nz,z,np,p,unit,nchek)
c...Get amp,zeros and poles  from a file for a particular 
c...date-group in the file
c...
c...returns nchek=1 if valid idate found in data file

      character*(*) filename
      character*100 line
      character*1 unit
      real z1,z2,p1,p2
      complex z(1),p(1)
      double precision am

      character*3 ampf,zerof,polef

      dimension loc (20)
      character*50 arg(20)

      integer idate

      open (unit=7,file=filename,err=990,status='old')

      am=1.0
      nz = 0
      np = 0
      chek=0
      ampf='no'
      zerof='no'
      polef='no'

c...First search for a valid  start date

 5    continue
      read(7,1010,end=5000)line
      if(line(1:5).eq. "start" ) then
         call argin2(line,20,narg,arg,nchs,loc)
         read(arg(2),*)ndate
         if(ndate.eq.idate) then
            nchek=1
            go to 10
         endif
      endif
      go to 5


c...Continue here if we found a section in data with proper date

 10   continue
      read(7,1010,end=5000)line
 1010 format(a)


c...See if this is a comment card or blank card
      if(line(1:1).eq."#" .or. lstrng(line) .eq. 0) go to 10

      call argin2(line,20,narg,arg,nchs,loc)

      if(arg(1)(1:5).eq. "amp_f") then

         read(arg(2),*) am
         ampf='yes'

      elseif(arg(1)(1:5).eq."num_z") then

c...Read in the zeros (real and imaginary parts. 
         read(arg(2),*) nz
         do 20 i=1,nz
            read(7,*)z1,z2
            z(i)=cmplx(z1,z2)
 20      continue
         zerof='yes'

      elseif(arg(1)(1:5).eq."num_p") then

c...Read in the real and imaginary components of poles.
         read(arg(2),*) np
         do 30 i=1,np
            read(7,*)p1,p2
            p(i)=cmplx(p1,p2)
 30      continue
         polef='yes'

      elseif(arg(1)(1:4).eq."unit") then
         unit=arg(2)(1:1)
         if (unit.eq.'V') unit='v'
         if (unit.eq.'D') unit='d'
         if (unit.eq.'A') unit='a'
         
         if(unit.ne.'v'.and. unit.ne.'d'.and.unit.ne.'a')then
            write(0,970)filename,line
            np = 0   ! Tell the caller we had a booboo.
            close(7)
            return
         endif

      elseif(arg(1)(1:4).eq."stat") then
         continue
      elseif(arg(1)(1:4).eq."stop") then
         continue
      elseif(arg(1)(1:3).eq."net") then
         continue
      elseif(arg(1)(1:3).eq."ref") then
         continue
      elseif(arg(1)(1:2).eq."sd") then
         continue
      elseif(arg(1)(1:2).eq."a0") then
         continue



      else
         write(0,970)filename,line
 970     format('ERROR: in file ',a50/,'Unrecognized line:',/a75)
         np = 0  ! tell the caller we had a booboo
         close(7)
         return
      endif

c...See if we have got everything we need yet

      if(ampf.eq.'yes'.and.zerof.eq.'yes'.and.polef.eq.'yes') then
         return
      endif

      go to 10



 5000 continue
      close(7)
      return

 990  write(0,1000)
 1000 format('WARNING. could not open file ',a50)
      return
      end
