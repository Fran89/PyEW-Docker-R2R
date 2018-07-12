# ForcePub  
# Specifies forced publication times.
# ForcePub  <seconds since event declared> <Delay Time> <Minimum Num Phases>
#      <seconds>     seconds since event was detected/declared by glass.
#      <Delay Time>  seconds to delay the forced publication of the event
#      <Min Num Ph>  Minimum number of phases in the event, required for this
#                    rule to apply.
# So the following rule:
#ForcePub   1200     0   20
#  would say:
#   If an Event has atleast 20 phases, then at 20 minutes after declaration
#   immediately(0) (re)publish it.
# Multiple entries should be listed in increasing time order (1200, 2400, 3600, ..)
#ForcePub   1200     0   20
ForcePub   1200     0   20
ForcePub   2400     0   20
ForcePub   3600     0   20
ForcePub   4800     0   20


# ChangePub  
# Specifies change based publication times.
# ChangePub  <Min num Ch> <Interval Time> <Delay Time> 
#      <Min num ch>     Minimum number of changes required for this rule to apply.
#      <Interval Time>  Minimum number of seconds between the last publication, and
#                       the next publication based on this rule.
#      <Delay Time>     Delay time between when the rule applies, and when the publication
#                       will be done.
# So the following rule:
#ChangePub     1   600   10  #  10 minutes
#  would say:
#   If an Event has atleast 1 change, then publish it at the later of 10 seconds from now,
#   or 600 seconds after it was last published.
ChangePub     1   600   10  #  10 minutes
ChangePub    10   240   10  #   4 
ChangePub    20   120   10  #   2
ChangePub    40     0    0  # NOW


# MinNumPhases
# Controls the minimum number of phases required to publish an event.
# default is 10
MinNumPhases 10


# OldestEventToPublish
# Controls the oldest event (how many days ago) to publish.
# Events older than X days will not be published, even if 
# detected by glass.
# Set to 0.0 to exclude NO events based on time.
OldestEventToPublish 0.0
