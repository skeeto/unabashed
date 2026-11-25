# A command of assignments alone, one of which fails, is abandoned
# before its redirections, as bash abandons it in its expansion: no
# file is made, no {var} assigned. Assignments before a command word
# fail without stopping it.
cd "$(mktemp -d)"
arr[]=f* {fd}>d1
echo "status $? fd=${fd-unset}"
arr[]=x >d2
a=1 b[]=2 c=3 >d3
echo "status $? a=$a c=${c-unset}"
readonly r
r=1 >d4
r=1 a2=2 >d5
echo "status $? a2=${a2-unset}"
x=$(echo sub >&2) e[]=1 >d6
echo "status $?"
r=1 : >d7
echo "status $?"
echo files: *
set -o posix
e[]=1 {fd}>d8
echo not reached
