# A procsub or coprocess started in ${ ...; } that is still running
# when the substitution ends does not hold it up, and what it writes to
# the substitution's output afterwards is lost
x=${ exec 7> >(cat); echo a; }
echo "[$x]"
exec 7>&-
x=${ exec 7> >(read l; echo "got $l"); echo b; }
echo "[$x]"
echo hi >&7
exec 7>&-
wait $!
x=${ exec 2>&1; exec 7> >(read l; echo "err $l" >&2); echo c; }
echo "[$x]"
echo hi >&7
exec 7>&-
wait $!
exec 2>&1
x=${ exec 2>&1; coproc { read l; echo "r $l" >&2; }; echo d; }
echo "[$x]"
echo hi >&${COPROC[1]}
wait
exec 2>&1
f() {
    local y=${ exec 7> >(read l; echo "f $l"); echo e; }
    echo "[$y]"
    echo x >&7
    exec 7>&-
    wait $!
}
f
for i in 1 2 3; do
    x=${ exec 7> >(read l; echo "loop $l"); echo $i; }
    echo hi$i >&7
    exec 7>&-
    y=${ echo new$i; }
    echo "[$x] [$y]"
done
wait
echo end
