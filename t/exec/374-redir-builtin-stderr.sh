# builtin diagnostics follow fd 2 redirections
show() { while IFS= read -r l; do printf '[%s]\n' "$l"; done <"$1"; }
cd /nonexistent 2>/tmp/e
echo status $?
show /tmp/e
shift 5 2>>/tmp/e
echo status $?
show /tmp/e
{ cd /nonexistent; echo inside; } 2>/tmp/e2
show /tmp/e2
nosuchcmd_xyz 2>/tmp/e3
echo status $?
show /tmp/e3
echo x >/tmp/nodir/x 2>/tmp/e4
echo status $?
read l </tmp/e4; echo "e4 status $?"
