== stdout 93
0022
st 0
st 0
0777
st 1
st 1
st 0
0027
st 1
u=rwx,g=rx,o=
umask 0027
umask -S u=rwx,g=rx,o=

== stderr 180
bash: line 7: umask: 17777: octal number out of range
bash: line 8: umask: 66666666666666666666666666: octal number out of range
bash: line 11: umask: 8: octal number out of range

== merged 273
0022
st 0
st 0
0777
bash: line 7: umask: 17777: octal number out of range
st 1
bash: line 8: umask: 66666666666666666666666666: octal number out of range
st 1
st 0
0027
bash: line 11: umask: 8: octal number out of range
st 1
u=rwx,g=rx,o=
umask 0027
umask -S u=rwx,g=rx,o=

== status 0
