# exec <file redirects the shell's input for later commands
printf '%s\n' first second third >/tmp/f
exec </tmp/f
read a
echo "a=$a"
read b
echo "b=$b"
f() { read c; echo "c=$c"; }
f
read d
echo status $? "d=$d"
exec <&-
read e
echo status $?
