== stdout 130
! break: 1
! continue: 1
redirected break: 1
redirected continue: 2
assign break: 1 v=unset
and break 1
or continue 3
! return: 3

== stderr 0

== merged 130
! break: 1
! continue: 1
redirected break: 1
redirected continue: 2
assign break: 1 v=unset
and break 1
or continue 3
! return: 3

== status 0
