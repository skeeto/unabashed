alias ll='ls -l'
type ll; type -t ll; command -v ll
shopt -s expand_aliases
type ll; type -t ll; command -v ll; command -V ll
type -f ll
shopt -u expand_aliases
f() { echo f; }
type -t f while builtin cat
type -a echo
echo() { builtin echo fn; }
type -a echo; type -t echo
unset -f echo
break() { echo inside $FUNCNAME; }
type -t break; type break | head -1
set -o posix
type -t break; type break; command -V break; type -a break | head -2
break; echo $?
set +o posix
break; echo $?
unset -f break
mkdir -p a b; printf 'echo A\n' > a/x; printf 'echo B\n' > b/x
chmod 655 a/x; chmod 755 b/x
PATH=$PWD/a:$PWD/b:$PATH
x; type -p x | sed "s|$PWD|PWD|"
type -a x | sed "s|$PWD|PWD|"
