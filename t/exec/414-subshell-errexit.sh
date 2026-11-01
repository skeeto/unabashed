# set -e inside a subshell exits only the subshell
(set -e; echo one; false; echo not-printed)
echo "after subshell: $?"
(set -e; false || echo or-ok; echo continues)
echo "st=$?"
(set -e; if false; then :; fi; echo if-ok)
echo "st=$?"
(set -e; ! true; echo bang-ok)
echo "st=$?"
(set -e; (exit 4); echo not-reached)
echo "nested exit: $?"
(set -e; x=$(false); echo not-reached2)
echo "assign: $?"
case $- in *e*) echo "parent has e";; *) echo "parent no e";; esac
