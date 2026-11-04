# personality: msys
# Hand-checked baseline, not recorded from an oracle (M5a step 6). The
# CR rules themselves are Git Bash's (t/msys 048-050, 061, 062): every
# CR leaves parser input (-c text, eval, source, comsub bodies, traps),
# command substitution strips trailing CR LF, igncr also drops CRs from
# comsub output and read. Here: those rules under the msys persona, the
# option lists, the hidden msyspath switch hiding Cygwin's options and
# cygpath, and the shell copied as cygpath.exe.
show() { local s=$1; s=${s//$'\r'/'\r'}; s=${s//$'\n'/'\n'}; echo "[$s] ${#1}"; }
eval "$(printf 'x=a\rb\r\nshow "$x"\r\n')"
trap "$(printf 'show "t\r"\r\n')" EXIT
show "$(printf 'c\r\n\r\n')"
show "$(printf 'c\r')"
show "$( show "$(printf 'n\r')" )"
printf 'f() { show "f\r"; }\r\n' >fn.sh; . ./fn.sh; f
printf 'q\r\n' >q; read -r v <q; show "$v"
set -o igncr; read -r v <q; show "$v"; show "$(printf 'i\rj\r')"
set -o | grep igncr; echo "$SHELLOPTS"
shopt -p completion_strip_exe; shopt -s completion_strip_exe; echo "$BASHOPTS"
compgen -A setopt ig; compgen -A shopt compl; compgen -b cyg
type -t cygpath; builtin cygpath -m /usr
shopt -u msyspath
set -o | grep -c igncr; compgen -A shopt compl; echo "$BASHOPTS" "$SHELLOPTS"
show "$(printf 'k\rl\r\n')"
type -t cygpath || echo "no cygpath"
set +o igncr; echo "status $?"
shopt -s msyspath
set -o | grep igncr
set +o igncr
cygpath -w /usr/bin; cygpath -u 'C:\msys\etc'
cygpath --windows --absolute x; cygpath --mixed --file=- <<'E'
/tmp/a
C:\b
E
cygpath -h | head -1
cygpath -k 2>&1; echo "status $?"
cygpath --path --unix 'C:\a;C:\msys\usr\bin'
cp C:/msys/usr/bin/bash.exe C:/msys/usr/bin/cygpath.exe
cygpath.exe -w /usr/bin /c/x; echo "status $?"
/usr/bin/cygpath.exe -u ''; echo "status $?"
