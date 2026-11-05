# personality: msys
# Hand-checked baseline, not recorded from an oracle (M5a step 7). The
# rules are Git Bash's (t/msys 042-047): #!/bin/sh and #!/usr/bin/env
# bash run this shell, other interpreters go through the mount table
# then PATH, the line ends at CR, files without #! run in-process, and
# a missing interpreter is a "bad interpreter" error (status 126).
printf '#!/bin/sh\necho sh "$@"\n' >a
printf '#!/usr/bin/env bash\r\necho env-bash "$@"\n' >b
printf '#!/usr/bin/echo one  two\n' >c
printf '#!./c\n' >d
printf 'echo plain "$@"\n' >e
printf '#!/no/such/interpreter\n' >f
printf '#!/usr/bin/env fake\n' >g
chmod +x a b c d e f g
./a 1
./b 2
./c 3
./d 4
./e 5
./f; echo "status $?"
./g; echo "status $?"
