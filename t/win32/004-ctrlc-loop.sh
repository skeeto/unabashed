# conrun: await:ready ctrl-c
# Ctrl-C ends a script busy in a loop, as SIGINT
echo start
: >ready
while :; do :; done
echo notreached
