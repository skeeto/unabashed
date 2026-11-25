# The kernel gives a #! script's interpreter the path the shell ran it
# by, which bash passes to execve as it found it: relative stays
# relative (exec's is made whole, without a leading ./).
printf '#!/usr/local/bin/bash\necho "0=$0 src=${BASH_SOURCE[0]}"\n' > f3
chmod +x f3
mkdir d
cp f3 d/f4
./f3
d/f4
bash ./f3
PATH=d:$PATH f4
PATH=$PWD/d:$PATH f4
cd d; ../f3; ./f4 | cat; cd ..
x=$(./f3); echo "$x"
./f3 & wait
(umask 022; ./f3)
exec ./f3
