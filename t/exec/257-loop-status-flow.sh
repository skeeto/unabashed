# loop status after break and continue
for i in 1; do false; break; done; echo "for break: $?"
for i in 1 2; do false; continue; done; echo "for continue: $?"
while :; do (exit 3); break; done; echo "while break: $?"
i=0; while [ $i -lt 2 ]; do i=$((i+1)); (exit 4); continue; done; echo "while continue: $?"
until false; do false; break 0; done 2>/dev/null; echo "break 0: $?"
for i in 1 2; do false || break; done; echo "or break: $?"
for i in 1 2; do (exit 6) && break; done; echo "and break: $?"
i=0; while [ $i -lt 2 ]; do i=$((i+1)); (exit 7); done; echo "while last: $?"
for i in 1 2; do (exit $i); done; echo "for last: $?"
for i in 1 2; do continue; done; echo "continue only: $?"
while break; do echo never; done; echo "break in cond: $?"
false; for i in a; do echo "for body sees $?"; done
(exit 3); for i in $(exit 5) a; do echo "for list cmdsub: $?"; done
false; while echo "while cond sees $?"; do break; done
false; until echo "until cond sees $?"; do :; done
false; if echo "if cond sees $?"; then :; fi
false; { echo "group sees $?"; }
false; for ((i=0; i<1; i++)); do echo "arith for body sees $?"; done
false; for ((i=0; $?==0 && i<1; i++)); do echo "arith test saw zero"; done
