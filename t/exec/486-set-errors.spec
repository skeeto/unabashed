== stdout 98
st=2
st=2
st=2
st=0
args=2 a b
after dash=2 ehBc
args=-x
plus st=0
pq=pq ehBc
plain=x y z
empty=0

== stderr 207
bash: line 1: set: nosuch: invalid option name
bash: line 3: set: nosuch: invalid option name
bash: line 5: set: -Z: invalid option
set: usage: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]

== merged 305
bash: line 1: set: nosuch: invalid option name
st=2
bash: line 3: set: nosuch: invalid option name
st=2
bash: line 5: set: -Z: invalid option
set: usage: set [-abefhkmnptuvxBCEHPT] [-o option-name] [--] [-] [arg ...]
st=2
st=0
args=2 a b
after dash=2 ehBc
args=-x
plus st=0
pq=pq ehBc
plain=x y z
empty=0

== status 0
