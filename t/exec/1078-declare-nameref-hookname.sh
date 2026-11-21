# declare, typeset and local through a nameref to a variable that
# exists run the hook of the name they were given, as bash's declare
# (stupidly_hack_special_variables on its own name): the target's hook
# does not run, so assigning LC_ALL or BASH_XTRACEFD that way leaves
# the locale or the trace descriptor as it was, while a nameref named
# LC_ALL runs LC_ALL's hook through itself. export assigns as an
# assignment does, running the target's hook.
exec 2>&1
exec 3>xt
LC_ALL=C; declare -n l=LC_ALL; declare l=C.UTF-8; printf '1 %d %s\n' "'é" "$LC_ALL"
LC_ALL=C; typeset l=C.UTF-8; printf '2 %d\n' "'é"
LC_ALL=C; g() { declare -n m=LC_ALL; local m=C.UTF-8; printf '3 %d %s\n' "'é" "$LC_ALL"; }; g
LC_ALL=C; export l=C.UTF-8; printf '4 %d\n' "'é"
LC_ALL=C; l=C.UTF-8; printf '5 %d\n' "'é"
unset -n l; LC_ALL=C
declare -n LC_ALL=loc; loc=C; declare LC_ALL=C.UTF-8; printf '6 %d %s\n' "'é" "$loc"
unset -n LC_ALL; unset LC_ALL
BASH_XTRACEFD=2; declare -n t=BASH_XTRACEFD; declare t=3
set -x; : seven; set +x
echo "7 $BASH_XTRACEFD [$(cat xt)]"
BASH_XTRACEFD=3; declare t=2
set -x; : eight; set +x
echo "8 [$(cat xt)]"
