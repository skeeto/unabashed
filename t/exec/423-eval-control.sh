# eval with return, break, continue, exit
f() { eval 'return 3'; echo not-reached; }
f
echo "f: $?"
for i in 1 2 3; do eval 'if [ $i = 2 ]; then break; fi'; echo "i=$i"; done
for i in 1 2 3; do eval '[ $i = 2 ] && continue'; echo "j=$i"; done
g() { eval 'local l=loc'; echo "$l"; }
g
echo "l=${l-unset}"
(eval 'exit 4'; echo no)
echo "sub: $?"
eval 'eval "eval echo triple"'
eval 'echo $LINENO'
eval '
echo line2-$LINENO'
