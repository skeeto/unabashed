# Started in a directory with no name (removed): bash reports it, keeps
# the PWD it inherited, and fails its pwd and its redirections there;
# its programs still run in the directory (the fakes take / for it, as
# getcwd fails). The shell once took / itself: its redirections made
# files in /, and its globs listed /.
top=$PWD
p=$(type -P pwd)
export PWD
mkdir gone && cd gone && rmdir "$top/gone"
bash -c '
echo "PWD ${PWD#$HOME/}"
pwd; echo "pwd $?"
echo x >f || echo "redirection failed"
echo *
"$1"
( "$1" )
"$1" | cat
cd /; pwd; cd "$HOME" && pwd | sed "s|^$HOME\$|home|"
' bash "$p"
echo "status $?"
cd "$top"

# Started in a directory whose name is longer than the system resolves
# (PATH_MAX), reached through symbolic links but not named by $PWD:
# it works as any other, by relative names (but for the fakes, which
# take an absolute name)
seg=$(printf "%0200d" 0)
mkdir l0
for i in $(seq 25); do
    mkdir "l$((i-1))/$seg" && ln -s "l$((i-1))/$seg" "l$i" || exit
done
cd l25 && echo hello >file
PWD=/nonexist bash -c '
echo "PWD $((${#PWD} - ${#HOME}))"
[ "$(pwd)" = "$PWD" ] && echo "pwd same"
read x <file && echo "$x"
echo x >f2 && read y <f2 && echo "$y"
echo *
cat file
' 2>&1
echo "status $?"
