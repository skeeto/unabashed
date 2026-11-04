# /dev/null for the shell, MSYS2 children and native children
echo x >/dev/null && echo ok
cat /dev/null; echo "status $?"
[ -c /dev/null ] && echo char
[ -e /dev/null ] && echo exists
[ -e /dev/Null ] || echo case-sensitive
read -r x </dev/null; echo "status $?"
argv /dev/null
argv >/dev/null; echo "status $?"
argv </dev/null >/dev/null 2>/dev/null; echo "status $?"
cygpath -w /dev/null
wc -c </dev/null
