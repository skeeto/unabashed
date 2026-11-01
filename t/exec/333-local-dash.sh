hasu() { case $- in *u*) echo "$1: u on";; *) echo "$1: u off";; esac; }
f() { local -; set -u; hasu "in f"; }
f
hasu "after f"
g() { set -u; }
g
hasu "after g"
set +u
h() { local -; set -f; set -o noglob; }
h
case $- in *f*) echo "f still set";; *) echo "f restored";; esac
k() { local - x=1; set -u; hasu "in k"; echo "x=$x"; }
k
hasu "after k"
