# Running files by the noacl rule: a script without #! runs in a
# subshell, a non-executable file fails, PATH search skips it
printf '#!/bin/sh\necho "shebang $#"\n' >withbang
printf 'echo "plain $#"\n' >plain
printf ':\necho "colon $#"\n' >colon
printf 'print(1)\n' >p.py
./withbang a
./colon a b
./plain; echo "status $?"
./p.py; echo "status $?"
mkdir bin
printf 'echo onpath\n' >bin/tool
printf '#!/bin/sh\necho onpath2\n' >bin/tool2
PATH=$PWD/bin:$PATH
tool; echo "status $?"
tool2
