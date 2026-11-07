# source: a file setting its own positional parameters keeps them;
# POSIX mode's missing files (no cwd fallback, fatal unless `command`
# ran it); the name a file found through -p goes by
printf 'set -- m n o p\n' > setpos.sub
printf 'echo "args: $*"\n' > showpos.sub
printf 'echo "in $BASH_SOURCE"\n' > cwdf
set -- a b c
. ./showpos.sub x y; echo "$*"
. ./setpos.sub x y z; echo "$*"
set -- a b c
f() { . ./setpos.sub q; echo "f: $*"; }; f 1 2; echo "$*"
g() { . ./setpos.sub; echo "g: $*"; }; g 1 2; echo "$*"
( PATH=/nonexist; . cwdf; . -p /nonexist cwdf; echo "status $?" )
source -p /nonexist:. cwdf
bash -c '. notthere; echo two' bash
bash -o posix -c '. notthere; echo two' bash
bash -o posix -c 'command . notthere; echo one' bash
bash -o posix -c 'command command . notthere; echo one' bash
bash -c 'set -o posix; . cwdf; echo eight' bash
bash -c 'set -o posix; . ./cwdf; echo nine' bash
bash -c 'set -o posix; . -p /nonexist cwdf; echo ten' bash
bash -c 'set -o posix; . /nonexist/x; echo eleven' bash
bash -c 'set -o posix; command . /nonexist/x; echo twelve' bash
bash -c '. /nonexist/x; echo thirteen' bash
