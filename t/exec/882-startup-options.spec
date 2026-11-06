== stdout 286
st 2
st 2
X: nosuch: invalid option name
st 2
shopt -s extglob
shopt -u dotglob
fhBc
st 0
extglob             	off
globasciiranges     	on
noexpand_translation	off
ran
shopt -u extglob
shopt -s globasciiranges
shopt -u noexpand_translation
ran
noglob         	off
ran
set +o noglob
ran

== stderr 92
bash: line 0: nosuch: invalid shell option name
bash: line 0: -s: invalid shell option name

== merged 378
bash: line 0: nosuch: invalid shell option name
st 2
bash: line 0: -s: invalid shell option name
st 2
X: nosuch: invalid option name
st 2
shopt -s extglob
shopt -u dotglob
fhBc
st 0
extglob             	off
globasciiranges     	on
noexpand_translation	off
ran
shopt -u extglob
shopt -s globasciiranges
shopt -u noexpand_translation
ran
noglob         	off
ran
set +o noglob
ran

== status 0
