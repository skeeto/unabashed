# here-documents in lists and pipelines on the same line
while read l; do echo "1 $l"; done <<A && echo after-and
one
A
false <<B || echo after-or
ignored
B
while read l; do echo "$l"; done <<C | while read m; do echo "piped $m"; done
c-body
C
if read l <<D; then echo "if $l"; fi
d-body
D
case $(read l <<E
e-body
E
echo "$l") in e-body) echo matched;; esac
