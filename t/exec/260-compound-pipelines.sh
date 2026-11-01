# compound commands as pipeline stages
for i in 1 2 3; do echo $i; done | while read n; do echo "sq $((n*n))"; done
if true; then echo yes; fi | { read a; echo "piped if: $a"; }
case b in b) echo bee;; esac | while read w; do echo "case: $w"; done
x=0
echo 5 | while read x; do echo "inner x=$x"; done
echo "outer x=$x"
for i in 1; do (exit 3); done | true; echo "pipe status: $?"
true | for i in 1; do (exit 4); done; echo "pipe last: $?"
{ echo a; echo b; } | { while read l; do echo "<$l>"; done; }
