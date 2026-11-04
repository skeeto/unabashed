# exec options and failures
shopt -s execfail
exec nothere; echo st=$?
mkdir dir; exec dir; echo st=$?
exec ./dir; echo st=$?
exec >out 2>.; echo restored
cat out; echo
exec 3>&1 4>three; echo kept >&4; cat three
bash -c 'exec -a renamed bash -c "echo \$0"'
bash -c 'exec -c printenv HOME; echo no'; echo st=$?
shopt -u execfail
bash -c 'exec dir; echo not reached'; echo st=$?
# command_not_found_handle runs in a subshell
command_not_found_handle() { echo "handler: $# $*"; x=1; return 3; }
x=0; nothere a b; echo $? $x
unset -f command_not_found_handle
nothere; echo $?
# each pipeline stage starts with the same $?
false; true | echo $?
true; (false) | echo $?
( echo sub: $BASH_SUBSHELL ) | cat
{ echo group: $BASH_SUBSHELL; } | cat
# scripts that are directories or binaries
printf '\177ELF\0\0\0\n' > bin; chmod +x bin
bash ./bin; echo st=$?
bash dir; echo st=$?
. ./bin; echo st=$?
echo 'echo sourced' > env.sh
BASH_ENV=env.sh bash -c 'echo body'
BASH_ENV='$HOME/env.sh' bash -c 'echo expanded'
# BASH_ENV expands as in double quotes, then a leading tilde; a missing
# file is ignored
mkdir envdir
BASH_ENV='"env.sh"' bash -c 'echo dquote'
BASH_ENV="'env.sh'" bash -c 'echo squote'
BASH_ENV='>(:)' bash -c 'echo procsub'
BASH_ENV='env*' bash -c 'echo glob'
BASH_ENV='\env.sh' bash -c 'echo backslash'
BASH_ENV='$(echo env.sh)' bash -c 'echo cmdsub'
BASH_ENV='${u:-"env.sh"}' bash -c 'echo param'
BASH_ENV='${u}' bash -c 'echo empty'
BASH_ENV='~/env.sh' bash -c 'echo tilde'
BASH_ENV='x:~/env.sh' bash -c 'echo notilde'
BASH_ENV='~' bash -c 'echo home'
BASH_ENV=envdir bash -c 'echo dir'; echo st=$?
BASH_ENV=nope.sh bash -c 'echo missing'; echo st=$?
