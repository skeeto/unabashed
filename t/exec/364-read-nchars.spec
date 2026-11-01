== stdout 136
status 0 x=abc
status 0 x=ab
x=[a]
x=[a
b]
status 1 x=[ab
]
status 0 x=[]
a=[x] b=[y]
a=[x y ] b=[]
x=[  ab]
x=[ab]
x=[ab]
x=[\a]
x=123

== stderr 0

== merged 136
status 0 x=abc
status 0 x=ab
x=[a]
x=[a
b]
status 1 x=[ab
]
status 0 x=[]
a=[x] b=[y]
a=[x y ] b=[]
x=[  ab]
x=[ab]
x=[ab]
x=[\a]
x=123

== status 0
