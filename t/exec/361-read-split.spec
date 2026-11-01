== stdout 148
a=one b=two c=three four five
a=[only] b=[] c=[]
a=[lead and trail]
REPLY=[   reply keeps   spaces   ]
a=x b=y	z
a=[x] b=[y   z]
status 0 a=[] b=[]

== stderr 0

== merged 148
a=one b=two c=three four five
a=[only] b=[] c=[]
a=[lead and trail]
REPLY=[   reply keeps   spaces   ]
a=x b=y	z
a=[x] b=[y   z]
status 0 a=[] b=[]

== status 0
