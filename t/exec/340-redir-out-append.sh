# > creates and truncates, >> appends, read back with a loop
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
echo one >/tmp/f
echo two >/tmp/f
show /tmp/f
echo three >>/tmp/f
echo four 1>>/tmp/f
show /tmp/f
>>/tmp/new
show /tmp/new
echo status $?
printf 'no newline' >/tmp/g
while IFS= read -r l || [ -n "$l" ]; do echo "<$l>"; done </tmp/g
echo a 1>/tmp/h b
show /tmp/h
