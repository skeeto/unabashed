# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 6).
# busybox-w32's CR rules (notes/research/busybox.md §3): parser input
# loses the CR of each CR LF pair, lone CRs stay; command substitution
# strips trailing CR LF; an unquoted one splitting on newlines takes CR
# LF as the newline; read takes CR LF, and backslash CR LF, as the end
# of a line. No igncr or completion_strip_exe here: bash 5.3's lists.
show() { local s=$1; s=${s//$'\r'/'\r'}; s=${s//$'\n'/'\n'}; echo "[$s] ${#1}"; }
printf 'x=a\r\necho "[$x]" ${#x}\r\ny=p\rq; echo ${#y}\r\n' >c.sh
printf 'cat <<E\r\nh\r\nE\r\necho a\\\r\nb\r\n' >>c.sh
. ./c.sh
eval "$(printf 'z=v\r\nshow "$z"\r\n')"
eval "$(printf 'show "w\r"')"
show "$(printf 'a\r\n')"
show "$(printf 'a\rb\r\nc\r\n\r\n')"
show "$(printf 'a\r')"
show "$(printf 'a\r\r\n')"
for w in $(printf 'p\r\nq\r\n'); do show "$w"; done
v=$(printf 'r\r\ns\r\n'); show "$v"
show "$(printf 'r\r\ns\r\n')"
IFS=' '; for w in $(printf 't\r\nu\r\n'); do show "$w"; done; unset IFS
printf 'b\r\nc d\r\ne\\\r\nf\r\ng\rh\r\n' >f
{ read -r x; read -r y z; read w; read -r v; } <f
show "$x"; show "$y"; show "$z"; show "$w"; show "$v"
read -r -d '' all <f; show "$all"
read -r -N 3 v <f; show "$v"
mapfile -t arr <f; show "${arr[0]}"
printf 'x\r\n' | { read -r v; show "$v"; }
show "$(<f)"
set -o igncr; echo "igncr $?"
shopt completion_strip_exe; echo "strip $?"
case $SHELLOPTS in *igncr*) echo listed ;; *) echo unlisted ;; esac
set -o | grep -c igncr
