      function ntrbl(ia)
c
c $$$$$ calls no other routine $$$$$
c
c   Ntrbl returns the position of the last non-blank character stored
c   in character variable ia.  If ia is completely blank, then ntrbl
c   returns one so that ia(1:ntrbl(ia)) will always be a legal substring
c   with as many trailing blanks trimmed as possible.
c
      character*(*) ia
      ln=len(ia)
      do 1 i=ln,1,-1
      if(ichar(ia(i:i)).gt.32) go to 2
 1    continue
      i=1
 2    ntrbl=i
      return
      end
