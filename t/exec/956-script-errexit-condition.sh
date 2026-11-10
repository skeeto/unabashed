# A script without #! runs as a new shell: a condition around it does
# not suspend its own set -e
cd "$(mktemp -d)"
printf 'set -e\nfalse\necho no\n' >s
chmod +x s
./s && echo yes; echo "a $?"
if ./s; then echo yes; fi; echo "b $?"
! ./s; echo "c $?"
./s || echo "d $?"
