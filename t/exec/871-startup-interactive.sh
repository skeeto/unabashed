# Interactive non-login shells read ~/.bashrc, or --rcfile FILE (also
# --init-file), unless --norc; login shells do not. bash -i without a
# terminal complains about job control: those lines are filtered.
mkdir h && HOME=$PWD/h && export HOME
quiet() { "$@" 2>&1 | grep -v -e 'job control' -e 'terminal process group'; }
echo 'echo "bashrc: $0 $# ${-//[^i]/}"' >h/.bashrc
quiet bash -i -c 'echo "cmd: $0 $1"' name arg
quiet bash --norc -i -c 'echo norc'
echo 'echo "rcfile $1"' >rc
quiet bash --rcfile rc -i -c 'echo rcfile-cmd' a b
quiet bash --init-file "$PWD/rc" -i -c 'echo init-file-cmd'
quiet bash --rcfile '~/.bashrc' -i -c 'echo tilde'
quiet bash --rcfile nosuch -i -c 'echo missing'
quiet bash --rcfile rc --norc -i -c 'echo rcfile-norc'
quiet bash -c 'echo not interactive'
echo 'echo profile' >h/.profile
quiet bash -l -i -c 'echo login-interactive'
quiet bash --noprofile -l -i -c 'echo login-noprofile'
bash --rcfile 2>&1 | head -1
echo "status ${PIPESTATUS[0]}"
mkdir dir
quiet bash --rcfile dir -i -c 'echo dir rc'
echo 'echo "line $LINENO"; nosuch' >rc
quiet bash --rcfile rc -i -c 'echo after'
