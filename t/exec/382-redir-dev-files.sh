# /dev/null, /dev/stdout and /dev/stderr
echo gone >/dev/null
echo status $?
read x </dev/null; echo "status $? x=[$x]"
echo to-stderr >/dev/stderr
echo to-stdout >/dev/stdout
{ echo e1 >/dev/stderr; } 2>/dev/null
echo after
{ echo o1 >/dev/stdout; } >/dev/null
echo after2
{ echo swap >/dev/stdout; } 1>&2
