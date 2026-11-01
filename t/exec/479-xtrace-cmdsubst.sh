set -x
echo $(echo hi)
x=$(echo $(echo deep))
echo "$(echo a; echo b)"
y=`echo bq`
echo $(echo $(echo $(echo three)))
z=$( (echo sub) )
(echo paren)
( (echo paren2) )
f() { echo infunc; }
echo $(f)
eval 'echo ev'
eval 'echo $(echo evsub)'
