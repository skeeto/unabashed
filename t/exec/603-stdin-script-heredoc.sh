# commands from standard input leave the rest for the commands they run
printf '%s\n' 'read x; echo got $x' 'hello' 'echo end' | bash
printf '%s\n' 'echo before' 'bash -c "read l; echo child read \$l"' 'this line' 'echo after' >in.sh
bash <in.sh
bash <<'X'
read z
zzz line
echo z=$z
cat
more for cat
X
# a redirection of fd 0 changes where commands come from
printf '%s\n' 'exec </dev/null; read y; echo y=$y st=$?' 'echo after' | bash
printf '%s\n' 'echo inner' >g.sh
printf '%s\n' 'exec 0<g.sh; echo again' 'echo not here' >h.sh
bash <h.sh
printf '%s\n' 'exec 3<&0 0</dev/null; echo moved' 'echo after' | bash
printf 'echo noeol' | bash
# an unterminated $( in a here-document fails when it is expanded
cat <<EOF
$(echo hi
EOF
echo st=$?
x=$'no\tOK'; y=notOK
cat <<EOF
1: ${x#$"no	"}
3: O${x#n$"o	"O}
5: ${y#$"not"}
EOF
