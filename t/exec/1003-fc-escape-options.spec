== stdout 329
one
n=1+
history        	on
verbose        	off
    4  set -o | grep -E '^(history|verbose)'
    5  history 2
edited
echo edited
n=1+
line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
history        	on
verbose        	off
   10  set -o | grep -E '^(history|verbose)'
   11  ls tmp
   12  history 3
st 0

== stderr 10
echo n=1+

== merged 339
one
echo n=1+
n=1+
history        	on
verbose        	off
    4  set -o | grep -E '^(history|verbose)'
    5  history 2
edited
echo edited
n=1+
line 2: 1+: arithmetic syntax error: operand expected (error token is "+")
history        	on
verbose        	off
   10  set -o | grep -E '^(history|verbose)'
   11  ls tmp
   12  history 3
st 0

== status 0
