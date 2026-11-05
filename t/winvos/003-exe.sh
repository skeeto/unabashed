# personality: native
# Hand-checked baseline of today's minimal Windows layer, not recorded
# from an oracle (M5a step 4); steps 5 and 7 change it on purpose.
# No execute bit: an executable extension, #! or a PE image (not
# merely MZ, as busybox-w32 checks the header) makes a file
# executable, and a name without one is found with .exe added, also
# as ./mine (busybox-w32's add_win32_extension), argv[0] getting
# backslashes as spawnveq gives them (checked against busybox sh).
: >plain
: >tool.EXE
: >run.bat
: >x.sh
: >y.com
printf '#!/bin/sh\necho shebang "$@"\n' >shebang
printf 'MZ' >mz
for f in plain tool.EXE run.bat x.sh y.com shebang mz C:/w64/bin/ls.exe; do
    if [ -x "$f" ]; then echo "$f x"; else echo "$f -"; fi
done
cp C:/w64/bin/argv.exe mine.exe
./mine a
./mine.exe b
PATH="$PATH;C:\\work"
mine c
printf 'echo script "$@"\n' >noext
./noext d
./shebang e
./x.sh f
