x=global
callee() { echo "callee sees: $x"; x=changed-by-callee; }
caller() { local x=caller-local; callee; echo "caller after: $x"; }
caller
echo "global after: $x"
setter() { y=from-setter; }
wrap() { local y=wrapped; setter; echo "wrap: $y"; }
y=gy
wrap
echo "y: $y"
newvar() { z=created; }
holder() { local z; newvar; echo "holder z: $z"; }
holder
echo "z: [${z-unset}]"
