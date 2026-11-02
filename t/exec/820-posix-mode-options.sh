# set -o posix turns on shopt options and POSIXLY_CORRECT; leaving
# restores defaults, or what a temporary POSIXLY_CORRECT saved
exec 2>&1
shopt expand_aliases inherit_errexit shift_verbose sourcepath
set -o posix
echo "${POSIXLY_CORRECT-unset}"
shopt expand_aliases inherit_errexit shift_verbose
set +o posix
echo "${POSIXLY_CORRECT-unset}"
shopt expand_aliases inherit_errexit shift_verbose
POSIXLY_CORRECT=1
shopt -o posix
unset POSIXLY_CORRECT
shopt -o posix
shopt -s expand_aliases
POSIXLY_CORRECT=y true
shopt -o posix; shopt expand_aliases shift_verbose
f() { local -; set -o posix; shopt -o posix; echo "${POSIXLY_CORRECT-unset}"; }
f
shopt -o posix; echo "${POSIXLY_CORRECT-unset}"
# aliases in $( ) and ${ } are expanded once in POSIX mode
alias let='let --'
set -o posix
let '1 == 1'; echo $?
: $(let '1 == 1'); echo $?
: ${ let '1 == 1'; }; echo $?
set +o posix
shopt -s expand_aliases
: $(let '1 == 1'); echo $?
# set -e in a funsub: off unless inherit_errexit
set -e
echo "${ echo in; false; echo after false; }"
set +e
