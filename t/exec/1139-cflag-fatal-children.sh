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
