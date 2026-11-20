# A temporary binding before a builtin or function is a new variable,
# exported and nothing more: its value is the text as written, and only
# += computes it from the old variable, with that one's attributes
declare -i X=5
X=1+2 declare -p X
X=1/0 declare -p X; echo "st $?"
X+=1 declare -p X
X+=1+1 printenv X
f() { echo "in f [$X]"; }; X=3*4 f
X=1+2 eval 'X+=1; echo "eval [$X]"'
declare -p X
declare -l L=x
L=ABC declare -p L
L+=ABC declare -p L
declare -u U
U=abc printenv U
declare -a A=(1)
A=x declare -p A
g() { local -i Y; Y=1+2 declare -p Y; }; g
declare -ri R=1
R=2 declare -p R; echo "readonly $?"
(X+=1/0 declare -p X; echo not reached)
(set -o posix; X=1+2 :; declare -p X)
