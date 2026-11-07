== stdout 1588
declare -r a
declare -r b
declare -Ar c=([k]="v" )
declare -ar d=([0]="1" [1]="2")
declare -r e="1"
declare -ar f=([0]="1")
declare -Ar g
declare -ar h
declare -Ar c=([k]="v" )
declare -Ar g
st=0
declare -Ar i
declare -Ar j
declare -x k
declare -ar a=([0]="scalar")
declare -Ar b=([0]="scalar" )
declare -Ar c=([1]="2" )
declare -ar e=([0]="x")
bash: line 25: f: cannot convert indexed to associative array
bash: line 34: f: cannot convert indexed to associative array
bash: line 35: f: cannot convert indexed to associative array
b2 1
bash: line 37: g: cannot convert indexed to associative array
c2 1
bash: line 39: readonly: h: cannot convert indexed to associative array
d 1
bash: line 40: x: cannot convert indexed to associative array
bash: line 40: local: x: cannot convert indexed to associative array
e 1
bash: line 41: k: cannot convert associative to indexed array
f2 1
bash: line 43: r: cannot convert indexed to associative array
g2 1 unset
bash: line 45: f: cannot convert indexed to associative array
a2 1
bash: line 47: f: cannot convert indexed to associative array
b2 1
c 0
c2 0
bash: line 50: fn2: f: cannot convert indexed to associative array
bash: line 50: declare: f: cannot convert indexed to associative array
d 1
d2 0
bash: line 51: y: cannot convert indexed to associative array
bash: line 51: declare: y: cannot convert indexed to associative array
e 1
e2 0
bash: line 52: z: cannot convert associative to indexed array
bash: line 52: local: z: cannot convert associative to indexed array
f 1
f2 0
bash: line 53: f: cannot convert indexed to associative array

== stderr 0

== merged 1588
declare -r a
declare -r b
declare -Ar c=([k]="v" )
declare -ar d=([0]="1" [1]="2")
declare -r e="1"
declare -ar f=([0]="1")
declare -Ar g
declare -ar h
declare -Ar c=([k]="v" )
declare -Ar g
st=0
declare -Ar i
declare -Ar j
declare -x k
declare -ar a=([0]="scalar")
declare -Ar b=([0]="scalar" )
declare -Ar c=([1]="2" )
declare -ar e=([0]="x")
bash: line 25: f: cannot convert indexed to associative array
bash: line 34: f: cannot convert indexed to associative array
bash: line 35: f: cannot convert indexed to associative array
b2 1
bash: line 37: g: cannot convert indexed to associative array
c2 1
bash: line 39: readonly: h: cannot convert indexed to associative array
d 1
bash: line 40: x: cannot convert indexed to associative array
bash: line 40: local: x: cannot convert indexed to associative array
e 1
bash: line 41: k: cannot convert associative to indexed array
f2 1
bash: line 43: r: cannot convert indexed to associative array
g2 1 unset
bash: line 45: f: cannot convert indexed to associative array
a2 1
bash: line 47: f: cannot convert indexed to associative array
b2 1
c 0
c2 0
bash: line 50: fn2: f: cannot convert indexed to associative array
bash: line 50: declare: f: cannot convert indexed to associative array
d 1
d2 0
bash: line 51: y: cannot convert indexed to associative array
bash: line 51: declare: y: cannot convert indexed to associative array
e 1
e2 0
bash: line 52: z: cannot convert associative to indexed array
bash: line 52: local: z: cannot convert associative to indexed array
f 1
f2 0
bash: line 53: f: cannot convert indexed to associative array

== status 1
