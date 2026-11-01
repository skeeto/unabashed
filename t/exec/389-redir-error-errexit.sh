# a failed redirection under set -e ends the shell
echo start
set -e
: >/tmp/ok
echo x </tmp/nosuch || echo "handled $?"
if echo y </tmp/nosuch; then echo yes; else echo "else $?"; fi
echo z </tmp/nosuch
echo notreached
