# Loops reading from pipes, including streams far larger than a pipe
seq 100000 | while read x; do s=$x; done; echo "s=$s"
seq 100000 | { while read x; do s=$x; done; echo "inside s=$s"; }
printf 'a b\nc d\n' | while read x y; do echo "[$x][$y]"; done
for i in $(seq 10000); do echo "row number $i"; done | { while read l; do last=$l; done; echo "$last"; }
seq 3 | while read n; do seq $n | tr '\n' ' '; echo; done
echo data | { cat | while read l; do echo "got $l"; done; }
seq 5 | { read a; read b; echo "$a $b"; cat; }
shopt -s lastpipe
seq 100000 | while read x; do s=$x; done; echo "lastpipe s=$s"
echo 5 | read v; echo "v=$v"
shopt -u lastpipe
echo 6 | read w; echo "w=$w"
