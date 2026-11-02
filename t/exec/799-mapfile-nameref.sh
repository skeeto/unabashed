# mapfile through namerefs
declare -n r=target; mapfile -t r <<< $'a\nb'; declare -p target
declare -n e; mapfile e < /dev/null 2>&1; declare -p e; unset e
declare -n ref='XXX[0]'; mapfile ref <<< $'1\n2' 2>&1; echo $?; declare -p XXX 2>&1
f() { local -n out=$1; mapfile -t out <<< $'x\ny'; }; f result; declare -p result
