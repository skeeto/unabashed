# A temporary binding's value is computed once, in order, as bash's
# assign_in_env: X+=v under -i evaluates its arithmetic once whatever
# the command and its words, each += sees the bindings before it
# (which carry no attributes), a readonly variable is reported before
# its value expands and is neither bound nor exported, and xtrace shows
# the value bound.
exec 2>&1
f() { echo "f $X"; }
declare -i X=5; X+="X=7" eval "echo 1 \$X"; echo "1b $X"
declare -i X=5; X+="X=7" eval 'echo 2 $X'; echo "2b $X"
declare -i X=5; X+="X=7" printenv X; echo "3b $X"
declare -i X=5; X+="X=7" printenv X "$y"; echo "4b $X"
declare -i X=5; X+="X=7" f "$y"; echo "5b $X"
declare -i c=0 Z=0; Z+="c+=1" printenv Z; echo "6 $c"
declare -i c=0 Z=0; Z+="c+=1" A=1 B=$A printenv Z; echo "7 $c"
declare -i c=0 Z=0; Z+="c+=1" command printenv Z "$y"; echo "8 $c"
declare -i c=0 Z=0; Z+="c+=1" nosuchcmd "$y" 2>/dev/null; echo "9 $c"
declare -i c=0 Z=0; Z+="c+=1" Z+="c+=10" printenv Z; echo "10 $c $Z"
declare -ri R=1; declare -i c=0
R+="c+=1" : ; echo "11 $c $?"
c=0; R=$((c+=1)) printenv R; echo "12 $c $?"
c=0; R+="c+=1" X=$c printenv X; echo "13 $c $?"
set -x
c=0; Z=0; Z+="c+=1" : "$y"
R=2 X=3 : "$y"
Y=a; Y+=b printenv Y
set +x
set -o posix
declare -i c=0 Z=0; Z+="c+=1" : "$y"; echo "14 $c $Z"
declare -i c=0 Z=0; Z+="c+=1" W=$Z : ; echo "15 $c $Z $W"
