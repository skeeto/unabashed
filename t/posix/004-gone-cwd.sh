# A working directory removed or renamed from under the shell: its
# programs still start in the directory itself, as bash's forked
# children inherit it, never where the shell started. The fake pwd's
# getcwd fails in a removed directory, so it says /. The shell's own
# redirections fail there, and its builtin pwd keeps the name.
top=$PWD
p=$(type -P pwd)
mkdir gone && cd gone && rmdir "$top/gone"
"$p"
( "$p" )
"$p" | cat
"$p" & wait
( exec "$p" )
echo "comsub $("$p")"
( cd /tmp; "$p"; cd "$top"; "$p" | sed "s|^$top\$|top|" )
"$p"
pwd
echo x >f || echo "redirection failed"
cd "$top"
mkdir old && cd old && mv "$top/old" "$top/new"
"$p" | sed "s|^$top/||"
pwd | sed "s|^$top/||"
