# A script without #! starts as a new shell: a BASH_COMPAT it cannot
# take, from its environment, warns as it starts, under the old $0 and
# with no line. A binding for the command itself (NAME=value ./f2) is
# dropped there once more, under the script's name, so warns twice.
printf 'echo "in f2 $BASH_COMPAT"\n' >f2
chmod +x f2
export BASH_COMPAT=x
./f2
( ./f2 )
bash -c ./f2 zz
export BASH_COMPAT=4.0
./f2
BASH_COMPAT=y ./f2
echo "after $BASH_COMPAT"
unset BASH_COMPAT
BASH_COMPAT=y BASH_ARGV0=q ./f2
