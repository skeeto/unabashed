== stdout 239
[9223372036854775807] st=0 a 2
[99999999999999999999] st=1 ? 4
[-9223372036854775809] st=0 a 2
[2147483649] st=0 a 2
[4294967297] st=0 a 2
[4294967298] st=0 b 3
[4294967299] st=0 c 4
[0] st=0 a 2
[-1] st=0 a 2
st=1
st=1
st=0 abc
st=1
st=1

== stderr 241
bash: line 3: read: 2147483648: invalid number
bash: line 4: read: 4294967297: invalid number
bash: line 6: read: 4294967296: invalid file descriptor specification
bash: line 7: read: 2147483647: invalid file descriptor: Bad file descriptor

== merged 480
[9223372036854775807] st=0 a 2
[99999999999999999999] st=1 ? 4
[-9223372036854775809] st=0 a 2
[2147483649] st=0 a 2
[4294967297] st=0 a 2
[4294967298] st=0 b 3
[4294967299] st=0 c 4
[0] st=0 a 2
[-1] st=0 a 2
bash: line 3: read: 2147483648: invalid number
st=1
bash: line 4: read: 4294967297: invalid number
st=1
st=0 abc
bash: line 6: read: 4294967296: invalid file descriptor specification
st=1
bash: line 7: read: 2147483647: invalid file descriptor: Bad file descriptor
st=1

== status 0
