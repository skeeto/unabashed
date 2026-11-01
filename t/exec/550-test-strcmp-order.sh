t() { "$@"; echo "$? <- $*"; }
t test "" '<' a
t test a '<' ""
t test A '<' a
t test Z '<' a
t test a '<' aa
t test ab '<' b
t test 10 '<' 9
t test a '>' A
t test same '<' same
t test same '>' same
t [[ "" < a ]]
t [[ A < a ]]
t [[ 10 < 9 ]]
t [[ a > A ]]
t [[ same < same ]]
t [[ ab > b ]]
x=abc y=abd
t [[ $x < $y ]]
t [[ $x > $y ]]
t [[ ~ < ~x ]]
