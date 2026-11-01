== stdout 113
status 2
status 1
status 1 ok=[a]
status 1
status 1
status 2
status 2
status 0 x=dashdash
status 1 ro=1
status 1

== stderr 797
bash: line 2: read: -z: invalid option
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
bash: line 3: read: `1bad': not a valid identifier
bash: line 4: read: `bad-name': not a valid identifier
bash: line 5: read: abc: invalid number
bash: line 6: read: -2: invalid number
bash: line 7: read: -d: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
bash: line 8: read: -u: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
bash: line 11: ro: readonly variable
bash: line 12: read: x: invalid timeout specification

== merged 910
bash: line 2: read: -z: invalid option
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
status 2
bash: line 3: read: `1bad': not a valid identifier
status 1
bash: line 4: read: `bad-name': not a valid identifier
status 1 ok=[a]
bash: line 5: read: abc: invalid number
status 1
bash: line 6: read: -2: invalid number
status 1
bash: line 7: read: -d: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
status 2
bash: line 8: read: -u: option requires an argument
read: usage: read [-Eers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
status 2
status 0 x=dashdash
bash: line 11: ro: readonly variable
status 1 ro=1
bash: line 12: read: x: invalid timeout specification
status 1

== status 0
