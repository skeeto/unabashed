# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a steps 4, 5).
# Names ignore case, reserved DOS names are the null device in every
# directory, /dev/null is NUL and /dev/tty the console.
echo hi >Foo.TXT
cat foo.txt
[ -f FOO.txt ] && echo found
echo more >>FOO.TXT
cat C:/WORK/foo.TXT
echo x >nul
echo "status $?"
echo y >C:/work/sub/NUL.txt
echo "status $?"
cat <nul
[ -c NUL ] && echo char
[ -e con.log ] && echo con
echo z >/dev/null
[ -c /dev/null ] && echo devnull
[ -e /dev/tty ] || echo no-tty
ls
mkdir Dir
cd dir && pwd
cd ..
echo *
echo [fF]*
