# vos
# set -o igncr, Cygwin's option: what it changes beyond the parser, which
# drops every CR in msys mode anyway
show() { local s=$1; s=${s//$'\r'/'\r'}; s=${s//$'\n'/'\n'}; echo "[$s] ${#1}"; }
probe() {
    show "$(printf 'a\r\n')"
    show "$(printf 'a\rb\r\nc\r\n\r\n')"
    show "$(printf 'a\r')"
    show "$(printf 'a\r\r\n')"
    show "`printf 'a\r\nb\r\n'`"
    show "$(<f)"
    read -r line <f; show "$line"
    read line <f; show "$line"
    { read -r x; read -r y z; read w; read -r v; } <f
    show "$x"; show "$y"; show "$z"; show "$w"; show "$v"
    mapfile -t arr <f; show "${arr[0]}"; show "${arr[1]}"
    printf 'x\ry\r\n' | { read -r v; show "$v"; }
    IFS= read -r -d $'\r' v <f; show "$v"
    read -r -n 3 v <f; show "$v"
    read -r -N 3 v <f; show "$v"
    for w in $(printf 'p\r\nq\r\n'); do show "$w"; done
    x=$(printf 'r\r\ns\r\n'); y=(${x}); show "${y[0]}"; show "${y[1]}"
    eval "$(printf 'echo "[e\r]"')"
}
printf 'b\r\nc d\r\ne\\\r\nf\r\ng\rh\r\n' >f
probe
set -o igncr; echo "set $?"
case $SHELLOPTS in *igncr*) echo "in SHELLOPTS" ;; esac
echo "$-"
probe
set +o igncr
case $SHELLOPTS in *igncr*) echo "in SHELLOPTS" ;; esac
