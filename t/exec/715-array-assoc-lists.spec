== stdout 502
declare -A v=(["1 2"]="3" )
st=0
declare -A d=(["?"]="quest" ["*"]="star" ["'"]="squote" ["\$"]="dol" ["\""]="dquote" ["\\"]="bslash" ["@"]="at" ["}"]="rbrace" ["{"]="lbrace" ["\`"]="bquote" )
"?" "quest" "*" "star" "'" "squote" "\$" "dol" "\"" "dquote" "\\" "bslash" "@" "at" "}" "rbrace" "{" "lbrace" "\`" "bquote"
declare -a e=([0]="test")
declare -a a=([1]="(var)" [2]="foo")
declare -a a=([1]="(var)")
declare -a f=([0]="1" [1]="(4 5 6)" [2]="3")
1  st=0
3  st=0

FOO
declare -Au A=(["@"]="FOO" )

== stderr 273
bash: line 5: '': bad array subscript
bash: line 16: warning: a[1]=(var): quoted compound array assignment deprecated
bash: line 19: m: bad array subscript
bash: line 21: b: bad array subscript
bash: line 22: m: bad array subscript
bash: line 23: [-5]: bad array subscript

== merged 775
declare -A v=(["1 2"]="3" )
bash: line 5: '': bad array subscript
st=0
declare -A d=(["?"]="quest" ["*"]="star" ["'"]="squote" ["\$"]="dol" ["\""]="dquote" ["\\"]="bslash" ["@"]="at" ["}"]="rbrace" ["{"]="lbrace" ["\`"]="bquote" )
"?" "quest" "*" "star" "'" "squote" "\$" "dol" "\"" "dquote" "\\" "bslash" "@" "at" "}" "rbrace" "{" "lbrace" "\`" "bquote"
declare -a e=([0]="test")
declare -a a=([1]="(var)" [2]="foo")
bash: line 16: warning: a[1]=(var): quoted compound array assignment deprecated
declare -a a=([1]="(var)")
declare -a f=([0]="1" [1]="(4 5 6)" [2]="3")
bash: line 19: m: bad array subscript
1  st=0
bash: line 21: b: bad array subscript
3  st=0
bash: line 22: m: bad array subscript

bash: line 23: [-5]: bad array subscript
FOO
declare -Au A=(["@"]="FOO" )

== status 0
