== stdout 71
spaced
trailing
crlf
one two ./d arg
./e arg
status 126
empty
status 0

== stderr 79
test.sh: ./f: /no/such/interpreter: bad interpreter: No such file or directory

== merged 150
spaced
trailing
crlf
one two ./d arg
./e arg
test.sh: ./f: /no/such/interpreter: bad interpreter: No such file or directory
status 126
empty
status 0

== status 0
