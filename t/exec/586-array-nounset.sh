set -u
a=(1 2); declare -A h
echo "${a[@]}" "${#a[@]}"; echo "${h[@]}" "${#h[@]}"
echo "${e[@]-def}"
(echo "${e[@]}") ; echo st=$?
(echo "${a[5]}") ; echo st=$?
(echo "${h[q]}") ; echo st=$?
(echo "${#e[@]}") ; echo st=$?
(echo $((a[9]))) ; echo st=$?
set +u
[[ -v a[1] ]] && echo y1; [[ -v a[5] ]] || echo n5; [[ -v a ]] && echo ya; [[ -v h ]] || echo nh
h[k]=1; [[ -v h[k] ]] && echo yk; [[ -v h[@] ]] && echo yall; test -v 'a[0]' && echo t0
[[ -v a[@] ]] && echo aall; [[ -v e[@] ]] || echo noe
b=([3]=x); [[ -v b ]] || echo nob; [[ -v b[-1] ]] && echo neg
set +u
set -u
a=(); declare -A h; declare -a d; s=str; declare u
echo "1${a[@]}"; echo "2${h[@]}"; echo "3${d[@]}"; echo "4${s[@]}"; echo "5${u[@]}"; echo "6${e[@]}"
echo "7${a[*]}"; echo "8${h[*]}"; echo "9${e[*]}"
echo "10${#a[@]}"; echo "11${#h[@]}"; echo "12${#d[@]}"; echo "13${#u[@]}"
echo "14${!a[@]}"; echo "15${!e[@]}"; echo "16${!h[@]}"
echo "17${a[@]:0}"; echo "18${e[@]:0}"; echo "19${h[@]#x}"; echo "20${e[@]#x}"
echo "21${a[0]-x}"; echo end
set +u
set -u
a=(1); s=x
(echo $((a[9]))); echo st=$?
(echo $((s[3]))); echo st=$?
(echo $((e[0]))); echo st=$?
(echo $((e))); echo st=$?
declare -a d; (echo "${#d[@]}"); echo st=$?
declare u; (echo "${#u[@]}"); echo st=$?
(echo "${#e[*]}"; echo same); echo st=$?
(echo "${#a[5]}"; echo same); echo st=$?
