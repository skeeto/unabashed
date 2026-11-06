# A background job's own redirections are made for good, as in bash's
# forked child: the descriptor they replace closes at once, so a
# command substitution reading it does not wait for the job
x=$( { sleep 1; echo late >late.txt; } >/dev/null & echo early )
echo "x=$x"
[ -e late.txt ] || echo "not yet"
y=$( ( sleep 1; echo later >later.txt ) >/dev/null 2>&1 & )
echo "y=[$y]"
f() { sleep 1; echo f >f.txt; }
z=$( f >/dev/null & echo z )
echo "z=$z"
[ -e later.txt ] || [ -e f.txt ] || echo "none yet"
sleep 2
cat late.txt later.txt f.txt
# within the job, a redirection is undone as usual
{ { echo inner; } >inner.txt; echo after; } &
wait
cat inner.txt
# a failing one ends the job's command
{ echo never; } <nosuch &
wait $!
echo "status $?"
