# vos
# cygpath's other options: -W, -S, -f FILE and -f -, -l and -s on
# plain names, combined flags, statuses. The Windows directory's case
# depends on the machine, so it is folded.
lc() { local s; s=$("$@"); echo "${s,,}"; }
lc cygpath -W; lc cygpath -wW; lc cygpath -mW
lc cygpath -S; lc cygpath -wS; lc cygpath -mS
lc cygpath -u -W
printf '%s\n' 'C:\a' /tmp 'D:/b c' '' x >list
cygpath -f list; echo "status $?"
cygpath -m -f list
cygpath -w -f - <list
printf 'C:\\a\r\n/tmp\r\n' | cygpath -f -
printf '/c/a:/tmp\n' | cygpath -wp -f -
cygpath -l -w /c/Windows; cygpath -s -m /c/Windows
cygpath -am -- sub; cygpath -m -- -x
cygpath -wa .; mkdir a; cygpath -ma ./a/../b
cygpath -a 'C:\Windows'
cygpath -w 'C:\a' /c/b; echo "status $?"
cygpath ''; echo "status $?"
cygpath -w ''; echo "status $?"
cygpath -u 'C:\a' ''; echo "status $?"
cygpath >/dev/null 2>&1; echo "no args $?"
cygpath -q /x >/dev/null 2>&1; echo "bad option $?"
cygpath -w -u /c >/dev/null 2>&1; echo "two forms $?"
cygpath -f missing >/dev/null 2>&1; echo "missing file $?"
cygpath -W x >/dev/null 2>&1; echo "-W with a name $?"
cygpath -p 'C:\a;D:\b'
cygpath -mp '/c/a;/d/b'
cygpath -wp /c/a:
