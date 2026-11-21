# declare -n on a variable with a hook (LC_ALL, POSIXLY_CORRECT, ...)
# runs the hook after the nameref attribute is set, so it reads what the
# nameref names, not the nameref's own text; declare without a value runs
# the hooks too, and an unset variable is no setting
exec 2>&1
declare -n LC_ALL=foo; echo 1; unset -n LC_ALL
foo=bar; declare -n LC_ALL=foo; echo 2; unset -n LC_ALL
declare -n POSIXLY_CORRECT='a[$(echo ran >&2)0]'; shopt -po posix; unset -n POSIXLY_CORRECT
POSIXLY_CORRECT=y; declare -n POSIXLY_CORRECT; shopt -po posix; unset -n POSIXLY_CORRECT
GLOBIGNORE=x; shopt -p dotglob; declare -n GLOBIGNORE; shopt -p dotglob; unset -n GLOBIGNORE
f() { local -n POSIXLY_CORRECT=zz; shopt -po posix; local -n LC_ALL=foo; echo 3; }
POSIXLY_CORRECT=1; f; shopt -po posix; unset POSIXLY_CORRECT
declare -n r=POSIXLY_CORRECT; r=1; shopt -po posix; unset r; shopt -po posix; unset -n r
declare -n r=LC_ALL; r=qq; echo 4; unset r; unset -n r
declare -n IGNOREEOF=n; n=3; set -o | grep ignoreeof; declare -n IGNOREEOF; set -o | grep ignoreeof
unset -n IGNOREEOF
LC_ALL=foo2; declare LC_ALL; export LC_ALL; readonly LC_ALL; echo 5
(declare POSIXLY_CORRECT; shopt -po posix; declare -a POSIXLY_CORRECT; shopt -po posix)
(declare -i IGNOREEOF; set -o | grep ignoreeof)
g() { local POSIXLY_CORRECT; shopt -po posix; }; POSIXLY_CORRECT=1; g; shopt -po posix
