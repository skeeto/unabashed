# personality: msys
# Hand-checked baseline, not recorded from an oracle (M5a step 7).
# Children of an msys shell. MSYS2 programs get Cygwin quoting and no
# conversion, their environment's conv_envvars in Windows form (which
# they import back; the vos programs print them as given), .exe
# completed as the runtime does. Native ones get the argument and
# environment heuristics, MSYS2_ARG_CONV_EXCL and MSYS2_ENV_CONV_EXCL
# honoured, SYSTEMROOT and WINDIR added. Batch files go through cmd
# with converted, BatBadBut-quoted arguments.
/usr/bin/echo 'a b' '*' '' 'q"q' '{x,y}' '@f' '~' /c/x --d=/tmp 'a\b'
argv 'a b' '*' /c/x --d=/tmp a:/b
MSYS2_ARG_CONV_EXCL='--d=' argv --d=/tmp /tmp
export FOO=/c/x BAR=/tmp SHELL=/usr/bin/bash
unset SYSTEMROOT WINDIR
ARGV_SHOW='FOO BAR SHELL SYSTEMROOT WINDIR' argv
MSYS2_ENV_CONV_EXCL=FOO ARGV_SHOW='FOO BAR' argv
/usr/bin/env | /usr/bin/grep -E '^(FOO|BAR|SHELL)='
printf '@echo off\r\necho [%%1] [%%2] [%%3]\r\n' >t.bat
./t.bat /c/x 'a b' '%PATH%'
echo "status $?"
