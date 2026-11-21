# An error that abandons a file sourced inside a redirection's ${ cmd; }
# leaves the shell reading its own input again: later diagnostics name
# the shell's input and line, and xtrace's depth is the top level's
declare -i n
cat </dev/null > ${ . /dev/stdin <<<'n=1+'; }
echo "$(echo ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ)" > /dev/null
"f"() { :; }
echo ${y!!}
echo "[$BASH_SOURCE]"
set -x
: traced
