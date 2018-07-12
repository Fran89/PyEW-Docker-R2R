c RESPRM.INC:
c
      integer mxbr,mxo
      parameter (mxbr=45,mxo=8)
      character*8 phnm
      integer*4 nbr,ms,me,mo
      real*4 xm,prm,cut
      common/resprm/nbr,phnm(mxbr),ms(mxbr),me(mxbr),xm(mxbr),
     1 cut(mxbr),mo(3,mxbr),prm(mxo,3,mxbr)

