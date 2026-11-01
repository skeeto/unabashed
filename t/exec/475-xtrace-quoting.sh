set -x
echo '*' '?' '[a]' 'a*b'
echo '#a' 'a#b' '~' 'a~' '~/x'
echo 'a=b' '=b' 'x=' '!' 'a!b'
echo '$x' '`x`' 'a\b' '"q"'
echo 'a;b' 'a|b' 'a&b' '(a)' '{a}' 'a,b' '<' '>'
echo $'a\tb' $'nl\nx' $'\001' $'esc\033' $'del\177'
echo $'\303\251' 'é'
echo -- -n '-' '+' '%' '^' '@' 'a:b' 'a/b' 'a.b'
echo ' lead' 'trail ' 'in ner'
echo "\\" "a'b'c" "'"
