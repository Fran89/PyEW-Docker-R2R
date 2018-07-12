      subroutine distaz(the,phe,ths,phs,ns,dist,az,baz,xdeg,nerr)
*=====================================================================
* PURPOSE:  To compute the distance and azimuth between locations.
*=====================================================================
* INPUT ARGUMENTS:
*    THE:     Event latitude in decimal degrees, North positive. [r]
*    PHE:     Event longitude, East positive. [r]
*    THS:     Array of station latitudes. [r]
*    PHS:     Array of station longitudes. [r]
*    NS:      Length of THS and PHS. [i]
*=====================================================================
* OUTPUT ARGUMENTS:
*    DIST:    Array of epicentral distances in km. [r]
*    AZ:      Array of azimuths in degrees. [r]
*    BAZ:     Array of back azimuths. [r]
*    XDEG:    Array of great circle arc lengths. [r]
*    NERR:    Error flag:
*             =    0   No error.
*             = 0904   Calculation failed internal consistency checks.
*=====================================================================
* MODULE/LEVEL:  DFM/4
*=====================================================================
* GLOBAL INPUT:
*    MACH:
*=====================================================================
* SUBROUTINES CALLED:
*    SACLIB:  SETMSG, APCMSG
*=====================================================================
* LOCAL VARIABLES:
*=====================================================================
* KNOWN ERRORS:
* - Problem with equation for distance. See discussion below.
*=====================================================================

      include 'mach'
      dimension ths(ns), phs(ns)
      dimension dist(ns), az(ns), baz(ns), xdeg(ns)

* PROCEDURE:

* - Calculations are based upon the reference spheroid of 1968 and
*   are defined by the major radius (RAD) and the flattening (FL).

      data rad/6378.160/,fl/0.00335293/
      data twopideg/360./
      data c00,c01,c02,c03/1.,0.25,-4.6875e-02,1.953125e-02/
      data c21,c22,c23/-0.125,3.125e-02,-1.46484375e-02/
      data c42,c43/-3.90625e-03,2.9296875e-03/
      data degtokm/111.3199/

* - Initialize.

      nerr=0
      ec2=2.*fl-fl*fl
      onemec2=1.-ec2
      eps=1.+ec2/onemec2

* - Check which output items are required.

      laz=.true.
      if(az(1).lt.0.)laz=.false.
      lbaz=.true.
      if(baz(1).lt.0.)lbaz=.false.
      ldist=.true.
      if(dist(1).lt.0.)ldist=.false.
      lxdeg=.true.
      if(xdeg(1).lt.0.)lxdeg=.false.

* - Convert event location to radians.
*   (Equations are unstable for latidudes of exactly 0 degrees.)

      temp=the
      if(temp.eq.0.)temp=1.0e-08
      therad=torad*temp
      pherad=torad*phe

* - Must convert from geographic to geocentric coordinates in order
*   to use the spherical trig equations.  This requires a latitude
*   correction given by: 1-EC2=1-2*FL+FL*FL

      thg=atan(onemec2*tan(therad))
      d=sin(pherad)
      e=-cos(pherad)
      f=-cos(thg)
      c=sin(thg)
      a= f*e
      b=-f*d
      g=-c*e
      h=c*d

* - Loop on stations:

      do 5000 i=1,ns

* -- Convert to radians.
        temp=ths(i)
        if(temp.eq.0.)temp=1.0e-08
        thsrad=torad*temp
        phsrad=torad*phs(i)

* -- Calculate some trig constants.
        thg=atan(onemec2*tan(thsrad))
        d1=sin(phsrad)
        e1=-cos(phsrad)
        f1=-cos(thg)
        c1=sin(thg)
        a1=f1*e1
        b1=-f1*d1
        g1=-c1*e1
        h1=c1*d1
        sc=a*a1+b*b1+c*c1

* - Spherical trig relationships used to compute angles.

        if(lxdeg)then
          sd=0.5*sqrt(((a-a1)**2+(b-b1)**2+(c-c1)**2)*((a+a1)**2
     #       +(b+b1)**2+(c+c1)**2))
          xdeg(i)=atan2(sd,sc)*todeg
          if(xdeg(i).lt.0.)xdeg(i)=xdeg(i)+twopideg
        endif
        if(laz)then
          ss = ((a1-d)**2+(b1-e)**2+c1**2-2.)
          sc = ((a1-g)**2+(b1-h)**2+(c1-f)**2-2.)
          az(i)=atan2(ss,sc)*todeg
          if(az(i).lt.0.)az(i)=az(i)+twopideg
        endif
        if(lbaz)then
          ss=((a-d1)**2+(b-e1)**2+c**2-2.)
          sc=((a-g1)**2+(b-h1)**2+(c-f1)**2-2.)
          baz(i)=atan2(ss,sc)*todeg
          if(baz(i).lt.0.)baz(i)=baz(i)+twopideg
        endif

* - Now compute the distance between the two points using Rudoe's
*   formula given in GEODESY, section 2.15(b).
*   (There is some numerical problem with the following formulae.
*   If the station is in the southern hemisphere and the event in
*   in the northern, these equations give the longer, not the
*   shorter distance between the two locations.  Since the equations
*   are fairly messy, the simplist solution is to reverse the
*   meanings of the two locations for this case.)
        if(ldist)then
          if(thsrad.gt.0.)then
            t1=thsrad
            p1=phsrad
            t2=therad
            p2=pherad
          else
            t1=therad
            p1=pherad
            t2=thsrad
            p2=phsrad
          endif
          el=ec2/onemec2
          e1=1.+el
          costhi=cos(t1)
          costhk=cos(t2)
          sinthi=sin(t1)
          sinthk=sin(t2)
          tanthi=sinthi/costhi
          tanthk=sinthk/costhk
          al=tanthi/(e1*tanthk)+
     #       ec2*sqrt((e1+tanthi**2)/(e1+tanthk**2))
          dl=p1-p2
          a12top=sin(dl)
          a12bot=(al-cos(dl))*sinthk
          a12=atan2(a12top,a12bot)
          cosa12=cos(a12)
          sina12=sin(a12)
          e1=el*((costhk*cosa12)**2+sinthk**2)
          e2=e1*e1
          e3=e1*e2
          c0=c00+c01*e1+c02*e2+c03*e3
          c2=c21*e1+c22*e2+c23*e3
          c4=c42*e2+c43*e3
          v1=rad/sqrt(1.-ec2*sinthk**2)
          v2=rad/sqrt(1.-ec2*sinthi**2)
          z1=v1*(1.-ec2)*sinthk
          z2=v2*(1.-ec2)*sinthi
          x2=v2*costhi*cos(dl)
          y2=v2*costhi*sin(dl)
          e1p1=e1+1.
          sqrte1p1=sqrt(e1p1)
          u1bot=sqrte1p1*cosa12
          u1=atan2(tanthk,u1bot)
          u2top=v1*sinthk+e1p1*(z2-z1)
          u2bot=sqrte1p1*(x2*cosa12-y2*sinthk*sina12)
          u2=atan2(u2top,u2bot)
          b0=v1*sqrt(1.+el*(costhk*cosa12)**2)/e1p1
          du=u2 -u1
          pdist=b0*(c2*(sin(2.*u2)-sin(2.*u1))+
     #       c4*(sin(4.*u2)-sin(4.*u1)))
          dist(i)=abs(b0*c0*du+pdist)
          if(lxdeg .and. (abs(dist(i)-degtokm*xdeg(i))).gt.100.)then
            nerr=0904
c	    write(6,*)'ERROR in computing distance and azimuth'
c           call setmsg('ERROR',nerr)
c           call apimsg(i)
          endif
        endif
 5000   continue

 8888 return

*=====================================================================
* MODIFICATION HISTORY:
*    830603:  Fixed bug with negative station latiudes.
*    810000:  Original version.
*=====================================================================

      end
	subroutine delaz(elats,elatc,elons,elonc,slats,slatc,slons,slonc,
     *  delta,azim)
      common  /circle/  pi,rad,deg,radius
      if(slats.ne.0.) go to 1
c
c     south pole station
c
      delta=(pi-acos(elatc))*deg
      azim=pi
      return
    1 cosdel=elats*slats*(slonc*elonc+slons*elons)+elatc*slatc
      tm1=slats*(slons*elonc-slonc*elons)
      tm2=elats*slatc-elatc*slats*(slonc*elonc+slons*elons)
      sindel=sqrt(tm1*tm1+tm2*tm2)
      if(sindel.ne.0..or.cosdel.ne.0.) go to 2
      delta=0.
      go to 3
 2    delta=atan2(sindel,cosdel)*deg
 3    if(tm1.ne.0..or.tm2.ne.0.) go to 4
      azim=0.
      go to 5
 4    azim=atan2(tm1,tm2)
 5    if(azim.lt.0.) azim=2.*pi+azim
      return
      end
	subroutine geocen (glat,glon,elat,elats,elatc,elon,elons,elonc)
      common / circle / pi,rad,deg,radius
      elat=90.0-deg*atan (0.9932773*(sin (glat*rad)/cos (glat*rad)))
      elats=sin (elat*rad)
      elatc=cos (elat*rad)
      elon=glon
      if(glon.lt.0.0) elon=360.0+glon
      elons=sin (elon*rad)
      elonc=cos (elon*rad)
      return
      end
