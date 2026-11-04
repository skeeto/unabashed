# vos
# /dev/stdin and /dev/fd/N as inputs, and their existence. (Writing to
# /dev/stdout reopens the output file and truncates it, so not here.)
read -r x </dev/stdin <<<input; echo "$x"
read -r x </dev/fd/0 <<<fd0; echo "$x"
read -r x 3<<<fd3 </dev/fd/3; echo "$x"
read -r x < <(echo procsub); echo "$x"
[ -e /dev/stdin ] && echo stdin
[ -e /dev/stdout ] && echo stdout
[ -e /dev/fd/1 ] && echo fd1
[ -e /dev/fd/9 ] || echo no-fd9
