# conrun: 'type:line1\n' await:ready ctrl-c
# A console read gets a typed line. Ctrl-C ends the wait builtin and
# the script; the background program, which ignores Ctrl-C as bash's
# jobs do without job control, runs on (until the sandbox ends it).
read x
echo "x=$x"
"$U" -c 'sleep 20' &
: >ready
wait
echo notreached
