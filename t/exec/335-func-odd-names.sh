function a.b { echo "a.b"; }
function c-d { echo "c-d"; }
e:f() { echo "e:f"; }
function 9x { echo "9x"; }
g_h() { echo "g_h"; }
a.b; c-d; e:f; 9x; g_h
declare -F
declare -f c-d
unset -f a.b
a.b
echo "status $?"
