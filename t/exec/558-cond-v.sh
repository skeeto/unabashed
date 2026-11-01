t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
a=1 b=
t '[[ -v a ]]'
t '[[ -v b ]]'
t '[[ -v c ]]'
n=a
t '[[ -v $n ]]'
t '[[ -v "n" ]]'
f() { local l; t '[[ -v l ]]'; local m=1; t '[[ -v m ]]'; }
f
t '[[ -v l ]]'
set -- x
t '[[ -v 1 ]]'
t '[[ -v 2 ]]'
t '[[ -v HOME ]]'
t '[[ -v PWD ]]'
declare -i i
t '[[ -v i ]]'
i=0
t '[[ -v i ]]'
export ex
t '[[ -v ex ]]'
t '[[ -v "" ]]'
t '[[ -v 1abc ]]'
