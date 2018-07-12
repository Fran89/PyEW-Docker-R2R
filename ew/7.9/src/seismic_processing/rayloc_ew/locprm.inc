c
c   RayLocator wide constants.
c
c   Maxph    Maximum number of phases
c   Maxtt    Maximum number of phase travel-times computed by Trtm
c   Rsc1     Scales one variable rank-sum parameter estimate
c   Rsc2     Scales two variable rank-sum parameter estimate
c   Rsc3     Scales three variable rank-sum parameter estimate
c
      integer maxph,maxtt
      real rsc1,rsc2,rsc3
      parameter(maxph=5000,maxtt=60)
      parameter(rsc1=2.0285161,rsc2=2.6465147,rsc3=3.0834703)
