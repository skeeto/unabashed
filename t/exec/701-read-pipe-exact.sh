# read takes exactly its line from a pipe a program writes, however it
# gets there, leaving the rest for the next reader
printf 'a\nb b\nc\\nd\ne\n' | { read x; read -r y; read z; cat; echo "$x|$y|$z"; }
printf 'one\ntwo\nthree\n' | { read -n 2 x; read y; read -N 3 z; od -c; echo "$x|$y|$z"; }
printf 'k1:v1:k2:v2\n' | { read -d : a; read -d : b; cat; echo "$a $b"; }
printf 'p q\\n r\ns\n' | { read -a arr; read t; echo "${arr[*]}|$t"; }
printf 'no newline' | { read x; echo "$? $x"; }
printf 'x\n' | { read; read; echo "$? [$REPLY]"; }
