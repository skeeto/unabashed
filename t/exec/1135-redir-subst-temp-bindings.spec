== stdout 155
:1
:2
global:1
global:2
x=global:3 FOO=global
env bar
FOO=new
baz
eval bar
g in
h in
aglobal.out bglobal.out cglobal.out dnew.out ebar.out fin.out hin.out

== stderr 4
foo

== merged 159
foo
:1
:2
global:1
global:2
x=global:3 FOO=global
env bar
FOO=new
baz
eval bar
g in
h in
aglobal.out bglobal.out cglobal.out dnew.out ebar.out fin.out hin.out

== status 0
