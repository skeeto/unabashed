false
f() { echo body; }
echo "def status: $?"
false
function g { :; }
echo "def kw status: $?"
st() { return 3; }
st && echo yes || echo "no: $?"
! st; echo "negated: $?"
if st; then echo then; else echo "else $?"; fi
while st; do echo loop; done; echo "while: $?"
pipe() { echo x | st; }
pipe; echo "pipe: $?"
cmd() { st; }
x=$(cmd); echo "cmdsub: $?"
