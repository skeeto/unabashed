# conrun: await:ready1 ctrl-c await:ready2 ctrl-c
# Ctrl-C right after a program starts, perhaps before it (or busybox
# behind the sleep alias) attached to the console, acts as if it had
# it: a handled one does not end the shell; a missed one is ended
# at once.
"$U" -c 'trap "exit 0" INT; : >ready1; sleep 1'
echo "handled $?"
trap 'echo "trapped at $SECONDS"; exit 3' INT
SECONDS=0
: >ready2
sleep 5
echo "after $?"
