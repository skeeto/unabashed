# pathname expansion over a directory of 50000 names: everything, a
# suffix, directories only, extglob, and matching the list with [[ ]]
shopt -s extglob
d=${TMPDIR:-/tmp}/unabashed-b11
if [ ! -d "$d" ]; then
    mkdir -p "$d.new" && cd "$d.new" || exit 1
    for ((i=0; i<50000; i++)); do
        case $((i % 5)) in
        0) : >f$i.c ;;
        1) : >f$i.h ;;
        2) : >f$i.o ;;
        3) mkdir d$i ;;
        4) : >.x$i ;;
        esac
    done
    cd / && mv "$d.new" "$d"
fi
cd "$d" || exit 1
for r in 1 2 3; do
    a=(*); echo ${#a[@]}
    a=(*.c); echo ${#a[@]}
    a=(*/); echo ${#a[@]}
    a=(f*[0-9][0-9].[ch]); echo ${#a[@]}
    a=(!(*.o)); echo ${#a[@]}
    a=(*); n=0
    for f in "${a[@]}"; do [[ $f == *.@(c|h) ]] && ((n++)); done
    echo $n
done
