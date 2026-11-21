# set -o history gives HISTSIZE and HISTFILESIZE their defaults as
# bash's set_if_not: a variable the name finds through namerefs is
# kept, even one declared without a value (HISTFILESIZE then declared
# without one too); otherwise the name itself gets the default as a
# global, never through a nameref, so a nameref's subscript runs
# nothing. (bash binds the new variable beside a global nameref, which
# an unset reveals again; that is not shown here.)
t() {
    echo "== $1"
    bash -c "HISTFILE=/nonexistent/h; cmd() { echo cmd ran >&2; echo 1; }
        $1; set -o history; declare -p HISTSIZE HISTFILESIZE $2" bash 2>&1
}
t 'declare -n HISTFILESIZE="a[\$(cmd)]"' a
t 'declare -n HISTSIZE="b[\$(cmd)]"; b[1]=4' b
t 'declare -n HISTSIZE=zz' zz
t 'declare -n HISTSIZE=zz; zz=7' zz
t 'declare -n HISTSIZE'
t 'declare HISTSIZE'
t 'declare -r HISTSIZE'
t 'HISTSIZE='
t 'f() { local HISTSIZE; set -o history; declare -p HISTSIZE; }; f'
t 'f() { local -n HISTSIZE=q; set -o history; declare -p HISTSIZE q; }; f' q
