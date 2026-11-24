# A script without #! that cannot be read: bash commits to running it as
# shell code, then reports that it cannot read it, with no line number
printf 'echo ran\n' > s1; chmod 111 s1
./s1; echo "st $?"
./s1 | cat; echo "st ${PIPESTATUS[*]}"
./s1 & wait $!; echo "st $?"
mkdir d; printf 'echo ran\n' > d/s2; chmod 111 d/s2
PATH=$PWD/d:$PATH; s2; echo "st $?"
: > s4; chmod 111 s4; ./s4; echo "st $?"
: > s5; chmod 755 s5; ./s5; echo "st $?"
f() { ./s1; echo "in f $?"; }; f
