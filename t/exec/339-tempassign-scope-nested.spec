== stdout 164
outer v=tmp
inner v=inner-tmp
outer again v=tmp
top v=global
inner v=tmp2
loc v=loc
after setv v=global
u=[tmp4]
u=[unset]
local w=[tmpw]
w=changed
w after=[unset]

== stderr 0

== merged 164
outer v=tmp
inner v=inner-tmp
outer again v=tmp
top v=global
inner v=tmp2
loc v=loc
after setv v=global
u=[tmp4]
u=[unset]
local w=[tmpw]
w=changed
w after=[unset]

== status 0
