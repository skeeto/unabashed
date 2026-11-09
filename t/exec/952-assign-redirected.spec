== stdout 285
target=val gref=val
x=2
arr=a Z c
newvar=created
newvar=again up=again
bash: line 14: ro: readonly variable
status 1
bash: line 16: ro: readonly variable
status 1 ro=1
in v=loc
out v=glob
gv=viaref
shadow gv=viaref
global gv=viaref
end=deep
num=5
num=9
s=12
acc=123
gv=viaref
status 0

== stderr 0

== merged 285
target=val gref=val
x=2
arr=a Z c
newvar=created
newvar=again up=again
bash: line 14: ro: readonly variable
status 1
bash: line 16: ro: readonly variable
status 1 ro=1
in v=loc
out v=glob
gv=viaref
shadow gv=viaref
global gv=viaref
end=deep
num=5
num=9
s=12
acc=123
gv=viaref
status 0

== status 0
