# empty subscripts in arithmetic are reported and skipped, not fatal;
# let under assoc_expand_once evaluates subscripts as they are
declare -a a
(( a[]=1 )); echo "1 $? ${a[@]}"
(( a[""]=2 )); echo "2 $? ${a[@]}"
let 'a[]=3'; echo "3 $? ${a[@]}"
let 'a[""]=4'; echo "4 $? ${a[@]}"
echo "5 $(( a[""]=5 )) ${a[@]}"
echo "6 $(( a[]=6 )) ${a[@]}"
(( b[""]=7 )); echo "7 $? ${b[@]-unset}"
x=""; (( a[$x]=8 )); echo "8 $? ${a[@]}"
(( a[""]++ )); echo "9 $? ${a[@]}"
(( a[""] )); echo "10 $? ${a[@]}"
shopt -s assoc_expand_once
let 'a[""]=11'; echo "11 $? ${a[@]}"
let 'a[" "]=12'; echo "12 $? ${a[@]}"
let 'a[1+1]=13'; echo "13 $? ${a[@]}"
i=0; let 'a[i]=14'; echo "14 $? ${a[@]}"
let 'a[$i]=15'; echo "15 $? ${a[@]}"
(( a[" "]=16 )); echo "16 $? ${a[@]}"
