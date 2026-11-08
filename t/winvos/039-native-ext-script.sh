# personality: native
# Hand-checked baseline: native mode finds ./s as s.sh, as busybox-w32
# does, and the child shell is given the name with the extension, so
# it can open the script. $0 shows that name.
mkdir w && cd w
printf 'echo "ran $0"\n' >s.sh
printf '#!/bin/sh\necho "bang $0"\n' >b.sh
./s
echo "status $?"
./b
echo "status $?"
