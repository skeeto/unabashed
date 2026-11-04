# #!/bin/sh and #!/bin/bash scripts get their name and arguments
printf '#!/bin/sh\necho "[$0]" "$#" "$@"\n' >s
printf '#!/bin/bash\necho "[$0]" "$#" "$@"\n' >b
printf '#!/usr/bin/bash\necho "[$0]" "$1"\n' >ub
./s a 'b c'
./b x
./ub y
sh ./s z
mkdir d && cp s d/s
d/s
PATH=$PWD/d:$PATH s 1 2
