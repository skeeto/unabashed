# Login shells read /etc/profile, then the first of ~/.bash_profile,
# ~/.bash_login and ~/.profile; -l, --login and a $0 starting with -
# make one, --noprofile skips the files
mkdir h && HOME=$PWD/h && export HOME
echo 'echo "profile: $0 $# ${x-unset}"' >h/.profile
bash -l -c 'echo "cmd: $0 $1"; shopt login_shell; echo "$PATH"' name arg
echo 'echo bash_login' >h/.bash_login
bash --login -c 'echo cmd2'
echo 'echo "bash_profile $x"; x=set' >h/.bash_profile
x=env bash -l -c 'echo "cmd3 $x"'
bash --noprofile -l -c 'echo noprofile2'
bash -c 'echo nonlogin; shopt login_shell'
(exec -a -bash bash -c 'echo "dash: $0"; shopt login_shell')
(exec -l bash -c 'echo "exec -l: $0"')
(exec -a -bash bash -i -c 'echo "dash-i: $0"') 2>&1 | grep -v -e 'job control' -e 'terminal process group'
bash -lc 'echo lc'
echo 'echo "script: $0 $1"' >s.sh
bash -l s.sh one
echo "status $?"
bash -c 'shopt -s login_shell; echo "set $?"; shopt -u login_shell; echo "unset $?"'
bash -l -c 'shopt -u login_shell; echo "unset $?"; shopt login_shell'
rm h/.bash_profile
echo 'echo "login $-"; return 3; echo never' >h/.bash_login
bash -l -c 'echo "after $?"'
echo 'echo "exiting"; exit 4' >h/.bash_login
bash -l -c 'echo never'
echo "status $?"
echo 'echo "bad syntax"; if' >h/.bash_login
bash -l -c 'echo "after syntax $?"'
rm h/.bash_login
mkdir h/.bash_profile
bash -l -c 'echo "dir profile"'
