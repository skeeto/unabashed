# personality: msys
# Hand-checked baseline, not recorded from an oracle (M5a step 9); the
# rules themselves are recorded from Git Bash in t/msys/070-076. /etc
# is the install's etc through the mount table. A login shell reads
# /etc/profile and the first of ~/.bash_profile, ~/.bash_login and
# ~/.profile; an interactive one /etc/bash.bashrc then ~/.bashrc (or
# --rcfile's file); a non-interactive one $BASH_ENV. A login shell's
# exit reads ~/.bash_logout, then /etc/bash.bash_logout. A leading
# dash makes a login shell. Named sh, the POSIX rules: /etc/profile and
# ~/.profile, $ENV for an interactive shell.
cd ~
echo 'echo etc-profile "$0" "$1"' >/etc/profile
echo 'echo bash_login' >.bash_login
echo 'echo profile' >.profile
echo 'echo bashrc' >.bashrc
echo 'echo etc-bashrc' >/etc/bash.bashrc
echo 'echo rcfile' >rc.sh
echo 'echo logout' >.bash_logout
echo 'echo etc-logout' >/etc/bash.bash_logout
echo 'echo bash_env' >env.sh
bash -l -c 'echo login; exit 4' zero one
echo "status $?"
bash -i -c 'echo interactive' 2>/dev/null
bash --rcfile rc.sh -i -c 'echo rcfile' 2>/dev/null
bash --norc -i -c 'echo norc' 2>/dev/null
BASH_ENV=env.sh bash -c 'echo after'
bash --noprofile -l -c 'echo noprofile; exit'
(exec -a -bash bash -i -c 'echo dash' 2>/dev/null)
cp C:/msys/usr/bin/bash.exe C:/msys/usr/bin/sh.exe
ENV=env.sh /usr/bin/sh -l -i -c 'echo sh; shopt -po posix' 2>/dev/null
rm .bash_login
bash -l -c 'logout'
logout
echo "status $?"
(unset PS1; bash --norc -i -c 'echo "PS1=$PS1"' 2>/dev/null)
