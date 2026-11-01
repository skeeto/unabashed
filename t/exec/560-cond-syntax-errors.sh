t() { eval "$1"; echo "$? <- $1"; }
t '[[ a -eq ]]'
t '[[ ( a ]]'
t '[[ a b ]]'
t '[[ ]]'
t '[[ a == ]]'
t '[[ && a ]]'
t '[[ a && ]]'
t '[[ -n a b ]]'
t '[[ a -foo b ]]'
t '[[ ( ) ]]'
t '[[ a ) ]]'
t '[[ a "==" a ]]'
t '[[ a \== a ]]'
t '[[ "-n" a ]]'
echo end
eval 'test x y'; echo "runtime $?"
eval '
echo two
[[ a b ]]
echo four'; echo "multiline $?"
