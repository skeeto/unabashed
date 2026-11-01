== stdout 305
extglob=1
nullglob=1
both=0
mixed=1
nullglob            	on
dotglob             	on
failglob            	off
st=1
shopt -s nullglob
shopt -u failglob
st=1
nullglob            	off
st=1
st=1
q st=1
plain st=1
su st=1
badflag st=2
o st=0 ehBc
qo=1
o bad st=1
shopt -u inherit_errexit
shopt -u shift_verbose

== stderr 387
bash: line 12: shopt: nosuchopt: invalid shell option name
bash: line 13: shopt: nosuchopt: invalid shell option name
bash: line 14: shopt: nosuchopt: invalid shell option name
bash: line 15: shopt: cannot set and unset shell options simultaneously
bash: line 16: shopt: -x: invalid option
shopt: usage: shopt [-pqsu] [-o] [optname ...]
bash: line 20: shopt: nosuch: invalid option name

== merged 692
extglob=1
nullglob=1
both=0
mixed=1
nullglob            	on
dotglob             	on
failglob            	off
st=1
shopt -s nullglob
shopt -u failglob
st=1
nullglob            	off
st=1
bash: line 12: shopt: nosuchopt: invalid shell option name
st=1
bash: line 13: shopt: nosuchopt: invalid shell option name
q st=1
bash: line 14: shopt: nosuchopt: invalid shell option name
plain st=1
bash: line 15: shopt: cannot set and unset shell options simultaneously
su st=1
bash: line 16: shopt: -x: invalid option
shopt: usage: shopt [-pqsu] [-o] [optname ...]
badflag st=2
o st=0 ehBc
qo=1
bash: line 20: shopt: nosuch: invalid option name
o bad st=1
shopt -u inherit_errexit
shopt -u shift_verbose

== status 1
