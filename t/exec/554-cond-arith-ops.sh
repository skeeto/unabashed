t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
t '[[ 1 -eq 1 ]]'
t '[[ 1+1 -eq 2 ]]'
t '[[ "2 * 3" -eq 6 ]]'
x=3
t '[[ x -eq 3 ]]'
t '[[ $x -eq 3 ]]'
t '[[ x+1 -gt x ]]'
t '[[ 0x10 -eq 16 ]]'
t '[[ 010 -eq 8 ]]'
t '[[ 2#101 -eq 5 ]]'
t '[[ -1 -lt 0 ]]'
t '[[ 5 -le 5 ]]'
t '[[ 5 -ne 5 ]]'
t '[[ 7 -ge 8 ]]'
t '[[ unsetvar -eq 0 ]]'
t '[[ "" -eq 0 ]]'
t '[[ " 4 " -eq 4 ]]'
e='x * 2'
t '[[ e -eq 6 ]]'
i=0
[[ i++ -eq 0 ]]; echo "i=$i"
[[ ++i -eq 2 ]]; echo "i=$i status=$?"
[[ (j=5) -eq 5 ]]; echo "j=$j"
t '[[ 1 -eq 1 && 2 -eq 2 ]]'
t '[[ 3 -lt 10 ]]'
t '[[ 3 < 10 ]]'
