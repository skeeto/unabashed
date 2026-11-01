# &> and &>> send both streams; >&word does too
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
both() { echo out$1; echo err$1 >&2; }
both 1 &>/tmp/f
show /tmp/f
both 2 &>>/tmp/f
show /tmp/f
both 3 &>/tmp/f
show /tmp/f
both 4 >&/tmp/g
show /tmp/g
both 5 >/tmp/h 2>&1
both 6 >>/tmp/h 2>&1
show /tmp/h
both 7 &>/tmp/nodir/x
echo status $?
