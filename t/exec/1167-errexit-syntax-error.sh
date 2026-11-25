# Under set -e, as bash's parser_error, a syntax error ends the shell
# after the first line of its message, with status 2 and the EXIT
# trap; but not where set -e is ignored for eval (a condition, or a
# command substitution without inherit_errexit)
( set -e; eval ')'; echo not reached ); echo "1: $?"
( set -e; trap 'echo "trap $?"' EXIT; eval '[[ a'; echo not reached ); echo "2: $?"
( set -e; if eval ')'; then :; fi; eval ')' || :; echo "3: $?" )
( set -e; x=$(eval ')'; echo "4: $?"); echo "$x" )
( set -e; shopt -s inherit_errexit; x=$(eval ')'; echo no); echo "5: $? $x" )
( set -e; source /dev/stdin <<<'('; echo not reached ); echo "6: $?"
bash -ec ': ; )'; echo "7: $?"
bash -ec 'cat <<<# || echo no'; echo "8: $?"
