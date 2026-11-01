# Producers that never stop on their own end when the reader goes away
yes | head -3
yes abc def | head -n 2
yes | { read a; read b; echo "$a$b"; }
yes | head -1; echo "st=$?"
{ yes; echo "after yes: $?" >&2; } | head -1
seq 100000 | head -2
seq 200000 | tail -1
while :; do echo loop; done | head -2
while :; do echo loop; done | { read a; echo "got $a"; }
f() { while :; do echo "f out"; done; }
f | head -1
echo x | yes | head -1
