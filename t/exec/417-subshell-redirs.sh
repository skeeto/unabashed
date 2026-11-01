# Redirections on subshells and command substitution
(echo out; echo err >&2) > /tmp/o 2> /tmp/e
while read l; do echo "o: $l"; done < /tmp/o
while read l; do echo "e: $l"; done < /tmp/e
x=$( (echo to-err >&2; echo to-out) 2>&1 )
echo "[$x]"
y=$(echo e1 >&2)
echo "y=[$y]"
(exec > /tmp/z; echo inside)
echo "parent stdout ok"
while read l; do echo "z: $l"; done < /tmp/z
(read a; read b; echo "$b $a") <<END
first
second
END
