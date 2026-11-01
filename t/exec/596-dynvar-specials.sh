# BASHPID changes in subshells and substitutions, $$ does not
p=$BASHPID
[ "$p" = "$$" ] && echo main same
[ "$(echo $BASHPID)" != "$p" ] && echo comsub differs
( [ "$BASHPID" != "$p" ] && echo subshell differs )
[ "$(echo $$)" = "$$" ] && echo dollar same
# BASH_ARGV0 sets $0, and keeps it once unset
BASH_ARGV0=hello; echo "$0 $BASH_ARGV0"
f() { BASH_ARGV0=$1; }; f infunc; echo $0
( BASH_ARGV0=sub; echo $0 ); echo $0
unset BASH_ARGV0; echo "$0 [$BASH_ARGV0]"; BASH_ARGV0=q; echo $0
# error messages keep the shell's name
(( 1/0 ))
# BASH_COMMAND is the simple command being run
echo "A:$BASH_COMMAND"
for x in "$BASH_COMMAND"; do echo "F:$x"; done
x=$BASH_COMMAND; echo "X:$x"
z=$(echo "$BASH_COMMAND"); echo "Z:$z"
BASH_COMMAND=foo; echo "S:$BASH_COMMAND"
g() { :; }; g a b; echo "R:$BASH_COMMAND"
! echo "N:$BASH_COMMAND" >/dev/null 2>&1; echo "$?"
# GROUPS ignores assignments
GROUPS[0]=-1; echo $? ${GROUPS[0]}
GROUPS=5; echo $? ${GROUPS[0]}
GROUPS=(1 2); echo $? ${#GROUPS[@]} ${GROUPS[0]}
declare GROUPS=3; echo $? ${GROUPS[0]}
unset GROUPS; GROUPS=1; echo ${GROUPS[0]}
FUNCNAME=42; echo $? "[$FUNCNAME]"
# SECONDS follows sleep
s=$SECONDS; sleep 2; (( SECONDS - s >= 2 )) && echo slept
SECONDS=100; (( SECONDS >= 100 && SECONDS < 102 )) && echo seconds set
