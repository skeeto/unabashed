# Errors that end a POSIX-mode shell, as bash: a special builtin's
# usage error, invalid name or readonly assignment (not under command,
# nor where set -e is ignored), its redirection error (even there), an
# assignment error before it, a bad for or select variable, an
# expansion error, an eval syntax error. An assignment error before
# any other command abandons that command.
exec 2>&1
bash -o posix -c 'set -o nosuch; echo after'; echo "st $?"
bash -o posix -c 'unset -q; echo after'; echo "st $?"
bash -o posix -c 'export 1a 2b; echo after'; echo "st $?"
bash -o posix -c 'command export 1a 2b; echo after $?'; echo "st $?"
bash -o posix -c 'unset -v 1a 2b; echo after'; echo "st $?"
bash -o posix -c 'readonly r; unset r; echo after'; echo "st $?"
bash -o posix -c 'readonly r; export r=1; echo after'; echo "st $?"
bash -o posix -c 'f() { :; }; readonly -f f; unset -f f; echo after'; echo "st $?"
bash -o posix -c 'export -f nosuch; echo after $?'; echo "st $?"
bash -o posix -c 'shift 9; echo after $?'; echo "st $?"
bash -o posix -c 'shift x; echo after'; echo "st $?"
bash -o posix -c 'return; echo after'; echo "st $?"
bash -o posix -c 'f() { return x; }; f; echo after'; echo "st $?"
bash -o posix -c 'set -o nosuch || echo or; echo after $?'; echo "st $?"
bash -o posix -c 'set -o nosuch && echo and; echo after $?'; echo "st $?"
bash -o posix -c 'if set -o nosuch; then :; fi; echo after $?'; echo "st $?"
bash -o posix -c 'f() { set -o nosuch; }; f || echo or; echo after $?'; echo "st $?"
bash -o posix -c '(set -o nosuch; echo in); echo after $?'; echo "st $?"
bash -o posix -c 'x=$(set -o nosuch; echo in); echo "after $? [$x]"'; echo "st $?"
bash -o posix -c 'eval "set -o nosuch"; echo after'; echo "st $?"
bash -o posix -c 'eval "("; echo after'; echo "st $?"
bash -o posix -c ': </nonexist || echo or; echo after'; echo "st $?"
bash -o posix -c 'echo </nonexist; echo after $?'; echo "st $?"
bash -o posix -c 'readonly r; r=1; echo after'; echo "st $?"
bash -o posix -c 'readonly r; r=1 :; echo after'; echo "st $?"
bash -o posix -c 'readonly r; r=1 true; echo after'; echo "st $?"
bash -o posix -c 'readonly r
r=1 true
echo next $?
r=1 cat </dev/null
echo next $?
f() { :; }
r=1 f
echo next $?'; echo "st $?"
bash -o posix -c 'for a-b in 1; do :; done; echo after'; echo "st $?"
bash -o posix -c 'readonly r; for r in 1; do :; done; echo after'; echo "st $?"
bash -o posix -c 'readonly r; select r in 1; do :; done <<<1; echo after'; echo "st $?"
bash -o posix -c 'echo ${x!y}
echo after'; echo "st $?"
bash -o posix -c 'set -o nosuch
echo after'; echo "st $?"
bash -c 'set -o nosuch; echo after $?'; echo "st $?"
bash -c 'readonly r; r=1 true; echo after $?'; echo "st $?"
bash -c 'for a-b in 1; do :; done; echo after $?'; echo "st $?"
