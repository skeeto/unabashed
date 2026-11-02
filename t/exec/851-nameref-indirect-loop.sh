declare -n r1=r2 r2=r3 r3=r4 r4=r5 r5=r6 r6=r7 r7=r8 r8=r9 r9=r10 r10=ok; ok=fine
echo "<$r1>"
echo "<${!r1}>"; echo notreached
echo "<${!r2}>"
echo "<${!r3}>" "<${!r4}>"
declare -n a=b b=a
echo "<$a>"
echo "<${!a}>"; echo notreached
echo "<${!a-def}>" "<${!a:+x}>"
f() { declare -n x=x; echo "<${!x}>"; echo notreached; }; f; echo st=$?
g() { local -n y=z z=y; echo "<${!y}>"; }; g; echo st=$?
