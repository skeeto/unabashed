# mapfile -C and -c: the callback gets the index and the quoted line
mapfile -C 'echo cb' -c 1 a <<< $'x\ny'; declare -p a
mapfile -t -C 'echo cb' -c 1 a <<< $'x\ny'; declare -p a
f() { echo "[$*]"; return 1; }; mapfile -C f -c 2 a <<< $'1\n2\n3\n4\n5'; echo $?; declare -p a
mapfile -t -C 'printf "<%s>\n"' -c 1 a <<< "it's \$HOME"
mapfile -C 'f(' -c 1 a <<< $'x' 2>&1; echo $?
printf '%s\n' {1..20} > nums
mapfile -n 5 -c 1 -C "echo foo" array < nums
mapfile -n 5 -c 1 -C "echo foo" array < /dev/null
g() { local x; read x; echo "callback $1 read $x"; }
mapfile -C g -c 1 a < nums; declare -p a
printf '%s\n' {1..6} | { mapfile -t -C g -c 2 a; declare -p a; }
printf "abc\0def\0ghi\0jkl\0" | { mapfile -C echo -c 1 -d '' A; echo "${A[@]}"; }
