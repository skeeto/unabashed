a() { echo "a: $*"; }
function b { echo "b: $*"; }
function c() { echo "c: $*"; }
d() ( echo "d in subshell: $1" )
e() if [ "$1" = y ]; then echo e-yes; else echo e-no; fi
f() for i in 1 2; do echo "f $i"; done
g() [[ $1 == x ]]
function h
{
    echo h-body
}
a 1 2
b x y z
c
d q
e y; e n
f
g x; echo "g x: $?"
g z; echo "g z: $?"
h
a() { echo "a redefined"; }
a
