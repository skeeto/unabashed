simple() { echo hi; }
multi() {
    local x=1
    if [ "$x" = 1 ]; then
        echo one
    fi
    for i in a b; do echo $i; done
}
function kw { echo kw; }
sub() ( cd /tmp; echo "$PWD" )
redir() { echo out; } > /dev/null
pipe() { echo a | while read l; do echo "$l"; done && true || false; }
declare -f simple
declare -f multi kw
declare -f sub redir
typeset -f pipe
