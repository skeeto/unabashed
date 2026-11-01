# order matters: 2>&1 >file versus >file 2>&1
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
both() { echo out; echo err >&2; }
both >/tmp/a 2>&1
echo "-- a"; show /tmp/a
both 2>&1 >/tmp/b
echo "-- b"; show /tmp/b
both 2>/tmp/c 1>&2
echo "-- c"; show /tmp/c
both 3>&1 1>&2 2>&3 3>&- 2>/tmp/d
echo "-- d"; show /tmp/d
