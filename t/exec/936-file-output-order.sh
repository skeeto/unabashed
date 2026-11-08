# Output to a regular file is seen by whatever looks next: another
# descriptor for the same file, a stat, a program, a reader woken
# through a pipe, a subshell, a background job
exec 3>f
echo a >&3
echo b >>f
echo c >&3
cat f
echo x >g; [ -s g ] && echo nonempty
: >h; { echo one >>h; echo go; } | { read -r; cat h; }
printf '%s\n' 1 2 3 >k; wc -l <k
(echo sub >>k); tail -n 1 k
{ echo bg >>k; } & wait; tail -n 1 k
for i in 1 2 3; do echo "$i" >&3; done; exec 3>&-; tail -n 3 f
exec 4>>m; echo first >&4; read -r line <m; echo "read: $line"
echo partial >&4; ls m >/dev/null; od -c m | head -2
