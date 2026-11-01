== stdout 114
[hello $name $(echo no) `no` $((1+2))]
[back\slash \]
[next]
{dq $name}
<part $name>
(spaced $name)
|mixed $name|

== stderr 0

== merged 114
[hello $name $(echo no) `no` $((1+2))]
[back\slash \]
[next]
{dq $name}
<part $name>
(spaced $name)
|mixed $name|

== status 0
