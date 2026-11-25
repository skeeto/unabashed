# The redirections of (( )), [[ ]] and an arithmetic for cite the
# command's own line, as bash's messages do while it runs; those of
# other compound commands cite where the parser finished or the
# enclosing command's line
for ((i=0; i<1; i++)); do
    :
done <nope
for \
((i=0; i<1; i++)) { :
} <nope 2>&1 >&$LINENO
for i in 1; do
    for ((i=0;
      i<1; i++)) { :
    } <nope
    ((1 +
      1)) <nope
    [[ 1 &&
       1 ]] <nope
    { :
    } <nope
done
: <<E; ((1)) <nope; [[ 1 ]] <nope
x
E
{ :
} <nope
while false; do :
done <nope
echo "end $?"
