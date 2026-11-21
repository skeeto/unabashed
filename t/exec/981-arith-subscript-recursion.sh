# Arithmetic recursing through array subscripts meets bash's recursion
# limit, which spans the subscripts and the expansions in them; the
# error abandons the -c text, so each case runs in a subshell
a[0]='a[a[0]]'
(echo $((a[0])); echo not reached); echo "status $?"
( ((a[0])); echo not reached ); echo "status $?"
(let 'x = a[0]'; echo not reached); echo "status $?"
(x=$((a[0])) && echo not reached); echo "status $?"
b[0]='b[$((b[0]))]'
(echo $((b[0])); echo not reached); echo "status $?"
c[0]='d[0]+1' d[0]='c[c[0]]'
(echo $((c[0])); echo not reached); echo "status $?"
(echo $((a[0])) | cat; echo "pipe ${PIPESTATUS[*]}")
# the limit starts afresh afterward, and recursion within one
# expression is as before
echo $(( 1 + 2 ))
v=w w=v
(echo $((v)); echo not reached); echo "status $?"
f[0]='f[0]+1'
(echo $((f[0])); echo not reached); echo "status $?"
n=1 m=n
echo $(( a[m] + 1 )) $(( g[g[g[g[1]]]] ))
