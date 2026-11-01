r() { return $1; }
r 0; echo $?
r 7; echo $?
r 255; echo $?
r 256; echo $?
r 257; echo $?
r -1; echo $?
last() { false; return; }
last; echo "bare return after false: $?"
last2() { true; return; }
last2; echo "bare return after true: $?"
implicit() { false; }
implicit; echo "implicit: $?"
empty_body() { :; }
empty_body; echo "colon: $?"
r abc; echo "abc: $?"
r 1 2; echo "two args: $?"
early() { echo before; return 3; echo after; }
early; echo "early: $?"
inloop() { for i in 1 2 3; do if [ $i = 2 ]; then return $i; fi; echo "i=$i"; done; }
inloop; echo "inloop: $?"
