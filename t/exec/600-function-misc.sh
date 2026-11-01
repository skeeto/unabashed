# exported functions reach child shells
zf() { echo this is zf; }
f2() { echo a; { echo b; } >&2; } 2>&1
export -f zf f2
bash -c 'type -t zf; zf; f2; declare -F | grep -c zf'
env | grep -c '^BASH_FUNC_zf%%=() {'
export -nf zf; bash -c 'type -t zf || echo gone'
# type does not show attributes
type f2
# names printed without the function keyword unless assignment-like
function a=2 { :; }
function 11111 { :; }
declare -f a=2 11111
set -o posix; declare -f 11111; set +o posix
# posix special builtins: before functions, and not definable
set -o posix
v=0; k2() { v=20 :; echo in $v; }; v=30 k2; echo out $v
(
break() { :; }
echo after
); echo st=$?
set +o posix
# definition errors cite where the parser is, or the line in a function
'a b'() { :; }
if true; then
 'a b'() { :; }
 :
fi
g() {
  :
  'c d'() { :; }
}
g
# compgen lists names
h1() { :; }; h0() { :; }
compgen -A function h; echo $?
compgen -A function zzz; echo $?
compgen -W 'c b ab' a; compgen -P '<' -S '>' -W 'x y'
compgen -X 'b*' -W 'a b bc c'; compgen -X '!b*' -W 'a b bc c'
compgen -k | head -3; compgen -b | head -3
compgen -v BASH_VERS; compgen -A nope; echo $?
