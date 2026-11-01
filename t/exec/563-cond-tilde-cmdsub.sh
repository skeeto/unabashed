t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
t '[[ ~ == /home/oracle ]]'
t '[[ /home/oracle == ~ ]]'
t '[[ ~/x == /home/oracle/x ]]'
t '[[ "~" == /home/oracle ]]'
t '[[ $(echo hi) == hi ]]'
t '[[ hi == $(echo h)* ]]'
t '[[ "h*" == "$(echo h)*" ]]'
t '[[ $(echo "*") == x ]]'
t '[[ x == $(echo "*") ]]'
t '[[ x == "$(echo "*")" ]]'
t '[[ $((2*3)) == 6 ]]'
x=hello
t '[[ ${#x} -eq 5 ]]'
t '[[ ${x:-d} == hello ]]'
t '[[ ${nope:-d} == d ]]'
t '[[ ${x%llo} == he ]]'
t '[[ ${x/l/L} == heLlo ]]'
t "[[ \$'a\tb' == a?b ]]"
t "[[ a == \$'\x61' ]]"
