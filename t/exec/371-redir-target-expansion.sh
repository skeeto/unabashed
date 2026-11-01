# redirection targets undergo tilde, parameter, command substitution
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
d=/tmp
echo tilde >~/t1
show ~/t1
echo param >$d/p1
show /tmp/p1
echo cmd >$(echo /tmp/c1)
show /tmp/c1
echo spaced >"/tmp/s p"
show "/tmp/s p"
n=2
echo arith >/tmp/a$((n*2))
show /tmp/a4
echo glob >/tmp/gl.txt
echo replaced >/tmp/gl.*
show /tmp/gl.txt
echo noglob >"/tmp/q*"
show "/tmp/q*"
echo twoglob >/tmp/x1; echo >/tmp/x2
echo three >/tmp/x*
echo status $?
