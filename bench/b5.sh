# pattern substitution and removal on long strings, in the C locale and
# in UTF-8 with one non-ASCII character at the end
shopt -s extglob
s=
for ((i=0; i<6000; i++)); do s+="ab:cd$i "; done
t=${s:0:4000}
for loc in C C.UTF-8; do
    LC_ALL=$loc
    [[ $loc == C ]] || { s+=é; t+=é; }
    x=${s//ab/x}; echo ${#x}
    x=${s//[0-9]/}; echo ${#x}
    x=${t//a*b/x}; echo ${#x}
    x=${t//+([0-9]) /_}; echo ${#x}
    for ((i=0; i<300; i++)); do y=${s#*:}; done; echo ${#y}
    for ((i=0; i<300; i++)); do y=${s%% *}; done; echo ${#y}
    n=0
    for ((i=0; i<20000; i++)); do c=${s:i:1}; [[ $c == a ]] && ((n++)); done
    echo $n
done
