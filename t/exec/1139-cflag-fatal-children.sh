# A fatal error ends bash -c with 127, and so a child forked for a
# simple command (a stage, a job), which goes back to the same place;
# a child with a catch of its own ends with 1 (issue #17)

bash -c ': | ${X?}'; echo "st $?"
bash -c ': | ${X?}; echo "ps ${PIPESTATUS[*]}"'
bash -c '${X?} | :; echo "ps ${PIPESTATUS[*]}"'
bash -c ': | ${X?} | :; echo "ps ${PIPESTATUS[*]}"'
bash -c ': | echo ${X?}; echo "ps ${PIPESTATUS[*]}"'
bash -c '${X?} & wait $!; echo "bg $?"'
bash -c 'x=${X?} & wait $!; echo "bg $?"'
bash -c 'set -u; : | echo $Y; echo "ps ${PIPESTATUS[*]}"'
bash -c 'set -u; f() { : $1; }; : | f; echo "ps ${PIPESTATUS[*]}"'
bash -c 'f() { trap "echo exit \$?" EXIT; ${X?}; }; : | f; echo "ps ${PIPESTATUS[*]}"'
bash -c 'eval ": | \${X?}"; echo "eval $?"'
bash -c 'set -o posix; readonly r; : | r=1; echo "ps ${PIPESTATUS[*]}"'

# but 1 in a subshell, a comsub, a compound command, a coproc, or a
# builtin run in the child, as it has a catch of its own
bash -c '(${X?}); echo "sub $?"'
bash -c 'y=$(${X?}); echo "comsub $?"'
bash -c '{ ${X?}; } & wait $!; echo "group $?"'
bash -c ': | (: | ${X?}; echo "in ${PIPESTATUS[*]}"); echo "ps ${PIPESTATUS[*]}"'
bash -c 'coproc ${X?}; wait $!; echo "coproc $?"'
bash -c ': | eval "\${X?}"; echo "ps ${PIPESTATUS[*]}"'
bash -c ': | command eval "\${X?}"; echo "ps ${PIPESTATUS[*]}"'
bash -c 'eval "\${X?}" & wait $!; echo "bg $?"'
bash -c ': | eval ": | \${X?}; echo in \${PIPESTATUS[*]}"; echo "ps ${PIPESTATUS[*]}"'

# A script's are 1, though sourced in -c they are -c's
printf '%s\n' ': | ${X?}; echo "ps ${PIPESTATUS[*]}"' '${X?} & wait $!; echo "bg $?"' >f17.sh
bash f17.sh
bash -c '. ./f17.sh'

# The builtin's catch also ends the child with 1 on set -e, where a
# subshell keeps its status
bash -c ': | eval "set -e; (exit 3); echo no"; echo "ps ${PIPESTATUS[*]}"'
bash -c ': | eval "trap '"'"'echo exit \$?'"'"' EXIT; set -e; (exit 3)"; echo "ps ${PIPESTATUS[*]}"'
bash -c 'eval "set -e; (exit 3)" & wait $!; echo "bg $?"'
bash -c ': | eval "set -e; f() { (exit 3); echo no; }; : | f"; echo "ps ${PIPESTATUS[*]}"'
bash -c ': | eval "set -e; { (exit 3); echo no; } | :; echo \"in \${PIPESTATUS[*]}\""; echo "ps ${PIPESTATUS[*]}"'
bash -c ': | eval "set -e; x=\$( (exit 3); echo no ); echo \"cs \$?\""; echo "ps ${PIPESTATUS[*]}"'
bash -c 'f() { set -e; (exit 3); echo no; }; : | f; echo "ps ${PIPESTATUS[*]}"'

# In POSIX mode a forked child exits with export's and readonly's own
# status for an invalid name, EX_UTILERROR (or EX_BADASSIGN)
bash -c 'set -o posix; : | export "a b=1"; echo "ps ${PIPESTATUS[*]}"'
bash -c 'set -o posix; : | readonly "a b"; echo "ps ${PIPESTATUS[*]}"'
bash -c 'set -o posix; : | export "a[1]=2"; echo "ps ${PIPESTATUS[*]}"'
bash -c 'set -o posix; : | unset -v "a b"; echo "ps ${PIPESTATUS[*]}"'

# and eval's or source's syntax error jumps, even from a condition,
# though not through command: to the builtin's catch, 1
bash -c 'set -o posix; : | eval "a b("; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
bash -c 'set -o posix; : | eval "a b(" || echo "or $?"; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
bash -c 'set -o posix; : | eval "eval \"a b(\""; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
bash -c 'set -o posix; : | command eval "a b("; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
bash -c 'set -o posix; f() { eval "a b("; }; : | f; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
bash -c 'set -o posix; : | if eval "a b("; then :; fi; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
bash -c 'set -o posix; if eval "a b("; then :; else echo else; fi; echo after' 2>/dev/null; echo "st $?"
bash -c 'set -o posix; echo "a b(" >s.sh; if . ./s.sh; then :; else echo else; fi; echo after' 2>/dev/null; echo "st $?"
bash -c ': | eval "a b("; echo "ps ${PIPESTATUS[*]}"' 2>/dev/null
