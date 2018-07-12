c
c   Station-reading parameters
c
      character*1 pflg
      character*2 network,loca
      character*3 comp
      character*5 sta
      character*8 phase
      integer*4 pickid
      real*4 res,delta,azim,dtdd,dtdh,elev,slats,slatc,slons,slonc,
     1trvtim,qual
c
      common /stadat/ res,delta,azim,dtdd,dtdh,elev,slats,slatc,slons,
     1slonc,trvtim,qual,pickid
      common /stadtc/ sta,comp,network,loca,phase,pflg
