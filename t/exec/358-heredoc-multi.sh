# several here-documents on one command, and on one line
f() { read a; read -u 3 b; echo "a=$a b=$b"; }
f <<ONE 3<<TWO
from one
ONE
from two
TWO
g() { read x; echo "x=$x"; }
g <<A; g <<B
first
A
second
B
read p <<X <<Y
xx
X
yy
Y
echo "p=$p"
