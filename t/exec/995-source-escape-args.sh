# A sourced file's arguments replace the positional parameters only
# while it runs: an error that abandons the file puts the caller's back,
# as a return does, at the top level and in a function; exit runs the
# EXIT trap while the file's are still in place
printf 'declare -i x\nx=1+\necho not reached\n' > g.sh
cat > s.sh <<'EOF'
set -- orig1 orig2
. ./g.sh AAAAAAAAAAAAAAAAAAAA BBBBBBBBBBBBBBBBBBB
echo "after: $# <$1> <$2>"
echo "$@" more
f() { . ./g.sh in f; echo "f: $#: $*"; }
f x y
echo "top: $*"
. ./g.sh
echo "no args: $*"
EOF
bash s.sh; echo "st $?"
echo 'exit 3' > x.sh
set -- top1 top2
( trap 'echo "trap: [$*] [${BASH_SOURCE[*]}]"' EXIT; . ./x.sh a b ); echo "st $?"
