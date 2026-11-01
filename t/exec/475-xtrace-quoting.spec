== stdout 172
* ? [a] a*b
#a a#b ~ a~ ~/x
a=b =b x= ! a!b
$x `x` a\b "q"
a;b a|b a&b (a) {a} a,b < >
a	b nl
x  esc del
é é
-- -n - + % ^ @ a:b a/b a.b
 lead trail  in ner
\ a'b'c '

== stderr 343
+ echo '*' '?' '[a]' 'a*b'
+ echo '#a' a#b '~' a~ '~/x'
+ echo a=b =b x= '!' 'a!b'
+ echo '$x' '`x`' 'a\b' '"q"'
+ echo 'a;b' 'a|b' 'a&b' '(a)' '{a}' a,b '<' '>'
+ echo $'a\tb' $'nl\nx' $'\001' $'esc\E' $'del\177'
+ echo $'\303\251' $'\303\251'
+ echo -- -n - + % '^' @ a:b a/b a.b
+ echo ' lead' 'trail ' 'in ner'
+ echo '\' 'a'\''b'\''c' \'

== merged 515
+ echo '*' '?' '[a]' 'a*b'
* ? [a] a*b
+ echo '#a' a#b '~' a~ '~/x'
#a a#b ~ a~ ~/x
+ echo a=b =b x= '!' 'a!b'
a=b =b x= ! a!b
+ echo '$x' '`x`' 'a\b' '"q"'
$x `x` a\b "q"
+ echo 'a;b' 'a|b' 'a&b' '(a)' '{a}' a,b '<' '>'
a;b a|b a&b (a) {a} a,b < >
+ echo $'a\tb' $'nl\nx' $'\001' $'esc\E' $'del\177'
a	b nl
x  esc del
+ echo $'\303\251' $'\303\251'
é é
+ echo -- -n - + % '^' @ a:b a/b a.b
-- -n - + % ^ @ a:b a/b a.b
+ echo ' lead' 'trail ' 'in ner'
 lead trail  in ner
+ echo '\' 'a'\''b'\''c' \'
\ a'b'c '

== status 0
