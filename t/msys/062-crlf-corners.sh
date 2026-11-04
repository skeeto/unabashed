# vos
# More CRs in msys mode: continuations, quoting, heredocs, $'\r'
show() { local s=$1; s=${s//$'\r'/'\r'}; s=${s//$'\n'/'\n'}; echo "[$s] ${#1}"; }
printf 'echo a\\\r\nb\r\n' >c.sh
. ./c.sh
printf "x='p\rq'; echo \${#x}\r\n" >c.sh
. ./c.sh
x=$'a\rb'; show "$x"
eval $'y=a\rb'; show "$y"
eval "$(printf 'z=$'"'"'a\\rb'"'"'\r\n')"; show "$z"
printf 'cat <<"E"\r\nq\rr\r\nE\r\n' >h.sh
show "$(. ./h.sh)"
printf 'cat <<E\r\nq\\\r\nr\r\nE\r\n' >h.sh
show "$(. ./h.sh)"
case $(printf 'z\r') in z) echo plain ;; *) echo cr ;; esac
f() { show "$1"; }
f $(printf 'w\r\n')
f $(printf 'w\r\r\n')
v=$(printf 'echo "[i\r]"\r\n'); eval "$v"
show "$(eval "$(printf 'echo m\r')")"
show "$(echo $'n\r')"
show "${ printf 'o\r\n'; }"
printf 'show "$1"\r\n' >s.sh
. ./s.sh "$(printf 'k\r')"
