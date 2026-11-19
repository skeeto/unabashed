# A command with no words whose redirections touch fd 0, or name a
# {var}, runs them in bash's child: the status is theirs alone, not the
# last command substitution's, and the {var} is not assigned
$(false) <&2; echo "dup $?"
$(false >&2) 0</dev/null; echo "file $?"
`false` <<<x; echo "herestring $?"
$(false) 0>&-; echo "close $?"
$(false) 0>/dev/null; echo "output $?"
x=$(exit 3) $(exit 4) </dev/null; echo "assign $? [$x]"
$(false) >/dev/null; echo "other fd $?"
$(false) 3</dev/null; echo "fd 3 $?"
$(false) </nonexistent; echo "failed $?"
$(true) {fd}>/dev/null; echo "var $? [$fd]"
{fd}>/dev/null; echo "alone [$fd]"
echo hi >&10; echo "fd 10 $?"
(readonly fd=5; {fd}>/dev/null; echo "readonly $?")
$(false) 0</dev/null || echo or
f() { $(return 4) </dev/null; echo "function $?"; }; f
(set -e; $(false) </dev/null; echo survived)
(set -e; $(false) >/dev/null; echo not reached); echo "errexit $?"
(trap 'echo ERR' ERR; $(false) </dev/null; $(false) >/dev/null)
$(false) </dev/null & wait $!; echo "async $?"
