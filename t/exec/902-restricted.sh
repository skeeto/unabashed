# Restricted shells: rbash by name, -r or set -r. The startup files run
# unrestricted; then PATH SHELL ENV BASH_ENV HISTFILE are readonly, and
# cd, slashes in command names, . with a slash or -p, write
# redirections (even to /dev/null), exec, command -p, hash -p with a
# slash, enable -d or of a disabled builtin, set +r are refused; no
# functions or SHELLOPTS come in, none go out. A script run without #!
# is unrestricted.
(exec -a rbash bash -c 'echo $-; shopt restricted_shell; cd /')
(exec -a -rbash bash -c 'echo $-')
bash -r -c 'echo $-; shopt -u restricted_shell; shopt restricted_shell'
bash -c 'set -r; echo $-; shopt restricted_shell'
echo 'cd /; echo env file ran in $PWD' > envfile
BASH_ENV=envfile bash -r -c 'echo $-; cd /'
bash -r -c 'declare -p PATH SHELL ENV BASH_ENV HISTFILE' | sed 's/=.*//'
bash -r -c 'PATH=/x; echo $?; unset SHELL; echo $?'
bash -r -c '/bin/true; echo $?; echo hi >/dev/null; echo $?; echo ok 2>&1'
bash -r -c 'cat </dev/null; echo st $?; echo x >>out; echo st $?; echo x &>out; echo st $?'
bash -r -c 'echo x >&out; echo st $?; echo x <>out; echo st $?; echo x >|out; echo st $?'
test -e out && echo "out exists"
echo 'echo sourced' > src.sh
bash -r -c '. ./src.sh; echo st $?; . -p . src.sh; echo st $?; . src.sh'
bash -r -c 'source src.sh; echo st $?'
bash -r -c 'exec; echo st $?; exec true; echo st $?'
bash -r -c 'command -p true; echo st $?; command -pv true; echo st $?; command true; echo st $?'
bash -r -c 'hash -p /bin/sh x; echo st $?; hash -p nosuchcmd y; echo st $?; hash -p cat c; hash -t c'
bash -r -c 'BASH_CMDS[q]=/bin/sh; BASH_CMDS[r]=nosuchcmd; BASH_CMDS[s]=cat; hash -t s'
bash -r -c 'enable -n echo; enable echo; echo st $?; enable -d echo; echo st $?'
bash -r -c 'set +r; echo st $?; set +o restricted; echo st $?; echo $-'
bash -r -c 'pushd /; echo st $?; popd; echo st $?'
bash -r -c 'history -w h; echo st $?; history -w ./h; echo st $?'
f() { echo f ran; }
export -f f
bash -c 'type -t f'
bash -r -c 'type -t f; echo st $?'
env SHELLOPTS=noglob bash -c 'echo $-'
env SHELLOPTS=noglob bash -r -c 'echo $-'
bash -r -c 'g() { :; }; export -f g; bash -c "type -t g"; echo st $?'
printf 'cd /; echo script in $PWD $-\n' > noshebang
chmod +x noshebang
PATH=$PWD:$PATH bash -r -c 'noshebang; echo $-'
