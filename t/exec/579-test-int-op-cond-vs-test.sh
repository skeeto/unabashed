t() { if eval "$1"; then echo "T: $1"; else echo "F: $1 ($?)"; fi; }
v=7
t 'test v -eq 7'
t '[[ v -eq 7 ]]'
t 'test 0x7 -eq 7'
t '[[ 0x7 -eq 7 ]]'
t 'test 08 -eq 8'
t '[[ 10#08 -eq 8 ]]'
t 'test " -3 " -lt 0'
t '[[ " -3 " -lt 0 ]]'
t '[[ $v -eq v ]]'
t '[[ v == 7 ]]'
t '[[ "v" -eq "7" ]]'
