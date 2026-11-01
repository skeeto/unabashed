== stdout 83
a=ab#cd
b='#x'
c=a=b
d=x~
e='~x'
f='a b'
g='p=~'
h='q:~r'
i=$'a\001b'
j=$'x\ny'
k=

== stderr 0

== merged 83
a=ab#cd
b='#x'
c=a=b
d=x~
e='~x'
f='a b'
g='p=~'
h='q:~r'
i=$'a\001b'
j=$'x\ny'
k=

== status 0
