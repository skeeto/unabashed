# associative key-value lists, quoted compound declarations, bad subscripts
foo="1 2" bar="3 4 5"
declare -A v=( $foo 3 )
declare -p v
declare -A w=( $foo $bar '*' star "" )
echo "st=$?"
declare -A d
d=( '"' dquote '`' bquote "'" squote '\' bslash )
d+=( '$' dol @ at * star \{ lbrace \} rbrace ? quest )
declare -p d
echo ${d[@]@K}
declare -a e[10]=test
declare -a e[10]='(test)'
declare -p e
unset a; declare -a a; declare a[2]=foo; declare a[1]='(var)'; declare -p a
unset a; declare a[1]='(var)'; declare -p a
declare -a f; declare f='(1 2 3)'; declare f[1]='(4 5 6)'; declare -p f
declare -A m=([x]=1)
echo "1 ${m[$unset]} st=$?"
b=(1 2)
echo "3 ${b[-5]} st=$?"
echo ${!m[$unset]}
( echo "${#b[-5]}"; echo not reached )
declare -A A; declare -u A; A=()
echo ${A[@]:=foo}; declare -p A
