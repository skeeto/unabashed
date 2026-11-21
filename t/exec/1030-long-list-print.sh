# A long list prints as a short one does, for declare -f and for a
# command substitution's reparsed text, and the printed form reads
# back as the same function
n=400
s=$(printf 'i=$((i+1)); j=$((j+1)) && :; ! false || x; %.0s' $(seq $n))
eval "f() {
$s
: & k=\$(printf '%s\n' \$((i+1)); $s echo \$i) <<EOF
here
EOF
echo \"f \$i \$j \$k\"
$s
wait
}"
i=0 j=0; f
declare -f f > f1
head -c 200 f1; echo
tail -c 200 f1
wc -l < f1
grep -o 'k=\$(printf.\{0,80\}' f1
unset -f f
. ./f1
declare -f f > f2
cmp f1 f2 && echo same
i=0 j=0; f | cat
# $BASH_COMMAND is each command in turn
trap 'case $BASH_COMMAND in j=*) c=$((c+1)) ;; esac' DEBUG
c=0; eval "$s"
trap - DEBUG
echo "debug $c"
