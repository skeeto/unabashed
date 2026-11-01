# here-strings: expansion without splitting or globbing, newline added
x='a   b  *'
read -r l <<<"$x"; echo "[$l]"
IFS= read -r l <<<$x; echo "[$l]"
IFS= read -r l <<<"  lead"; echo "[$l]"
read -r l <<<''; echo "status $? [$l]"
IFS= read -r l <<<~; echo "[$l]"
IFS= read -r l <<<$(echo one; echo two); echo "[$l]"
while read -r l; do echo "<$l>"; done <<<"$(printf 'p\nq')"
read -r a b <<< "1 2 3"; echo "a=$a b=$b"
IFS= read -r l <<< 'single $x'; echo "[$l]"
IFS= read -r l <<< $((6*7)); echo "[$l]"
cnt=0; while read -r l; do cnt=$((cnt+1)); done <<<"x"; echo "cnt=$cnt"
