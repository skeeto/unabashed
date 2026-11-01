t() { "$@"; echo "$? <- $*"; }
t test 1 -eq 1
t test 1 -eq 2
t test 1 -ne 2
t test -3 -lt -2
t test 5 -le 5
t test 6 -gt 5
t test 4 -ge 5
t test 007 -eq 7
t test +3 -eq 3
t test -0 -eq 0
t test " 12 " -eq 12
t test 9223372036854775807 -gt 0
t test -9223372036854775808 -lt 0
t [ 10 -gt 9 ]
t [ 10 '>' 9 ]
t test 1 -eq x
t test x -eq 1
t test "" -eq 0
t test 3x -lt 4
t test 0x10 -eq 16
t test 9223372036854775808 -gt 0
t test 1+1 -eq 2
t test 1 -eq
t test 1.5 -lt 2
