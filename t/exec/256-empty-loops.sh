# loops that never run leave status 0
false; while false; do echo never; done; echo "while: $?"
false; until true; do echo never; done; echo "until: $?"
false; for i in; do echo never; done; echo "for empty: $?"
set --
false; for i; do echo never; done; echo "for no args: $?"
false; for ((i=0; i<0; i++)); do echo never; done; echo "arith for: $?"
false; for ((;0;)); do echo never; done; echo "arith for 0: $?"
e=
false; for i in $e $e; do echo never; done; echo "for empty expansion: $?"
false; while (exit 3); do echo never; done; echo "while exit 3: $?"
false; until (exit 0); do echo never; done; echo "until exit 0: $?"
