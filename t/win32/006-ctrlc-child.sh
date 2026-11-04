# conrun: await:ready1 sleep:300 ctrl-c await:ready2 ctrl-c await:ready3 ctrl-c
# The cooperative rule: Ctrl-C while a program runs counts against the
# shell only if the program died of it (or failed). A program under
# 100 ms old is taken to have missed it (still attaching to the
# console), hence the pause before the first.
"$U" -c 'trap "exit 0" INT; : >ready1; while :; do :; done'
echo "handled $?"
trap 'echo caught' INT
"$U" -c ': >ready2; while :; do :; done'
echo "died $?"
trap - INT
"$U" -c 'trap "exit 3" INT; : >ready3; while :; do :; done'
echo notreached
