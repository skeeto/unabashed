# #!/usr/bin/env finds the interpreter on PATH
printf '#!/usr/bin/env sh\necho "env sh $1"\n' >a
printf '#!/usr/bin/env bash\necho "env bash $1"\n' >b
printf '#!/usr/bin/env cat\nhello\n' >c
printf '#!/usr/bin/env nosuchinterpreter\n' >d
./a 1
./b 2
./c
./d 2>&1 | sed 's/^.*env: //'
echo "status ${PIPESTATUS[0]}"
