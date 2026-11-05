# Named sh: a login shell reads /etc/profile and ~/.profile, an
# interactive one the file $ENV names (expanded), and then the shell
# is in POSIX mode; BASH_ENV is ignored. bash --posix reads only $ENV.
mkdir h && HOME=$PWD/h && export HOME
quiet() { "$@" 2>&1 | grep -v -e 'job control' -e 'terminal process group'; }
ln -s /usr/local/bin/bash sh
echo 'echo bash_profile' >h/.bash_profile
echo 'echo "profile ${POSIXLY_CORRECT-unset} $(shopt -o posix)"' >h/.profile
echo 'echo bashrc' >h/.bashrc
echo 'echo "envfile $1 ${POSIXLY_CORRECT-unset}"' >h/envfile
echo 'echo bash_env' >h/bash_env
./sh -c 'echo "plain $0 ${POSIXLY_CORRECT-unset}"; shopt -o posix'
./sh -l -c 'echo "login ${POSIXLY_CORRECT-unset}"'
(exec -a -sh bash -c 'echo "dash-sh $0"')
ENV='$HOME/envfile' quiet ./sh -i -c 'echo "interactive ${POSIXLY_CORRECT-unset}"' n x
ENV='$HOME/envfile' BASH_ENV='$HOME/bash_env' ./sh -c 'echo ignored'
ENV='~/envfile' quiet ./sh -i -c 'echo "tilde env"'
ENV='$HOME/envfile' quiet bash --posix -i -c 'echo "posix interactive"'
ENV='$HOME/envfile' quiet bash --posix -l -c 'echo "posix login"'
ENV='$HOME/envfile' quiet bash --posix -l -i -c 'echo "posix login interactive"'
ENV='$HOME/envfile' quiet ./sh -l -i -c 'echo "sh login interactive"'
ENV='$HOME/envfile' quiet bash -i -c 'echo "bash ignores ENV"'
BASH_ENV='$HOME/bash_env' bash -c 'echo "bash_env read"'
BASH_ENV='~/bash_env' bash -c 'echo "bash_env tilde"'
BASH_ENV='$HOME/bash_env' bash --posix -c 'echo "posix no bash_env"'
BASH_ENV='$HOME/bash_env' bash -l -c 'echo "login and bash_env"'
BASH_ENV='$HOME/bash_env' quiet bash -i -c 'echo "interactive no bash_env"'
POSIXLY_CORRECT=1 ENV='$HOME/envfile' quiet bash -i -c 'echo "POSIXLY_CORRECT interactive"'
POSIXLY_CORRECT=1 bash -l -c 'echo "POSIXLY_CORRECT login"'
ENV='$HOME/envfile' quiet ./sh --norc -i -c 'echo "sh norc"'
ENV='$HOME/envfile' quiet ./sh --noprofile -l -i -c 'echo "sh noprofile"'
