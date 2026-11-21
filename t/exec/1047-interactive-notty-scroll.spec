== stdout 287
$ ^M<pqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
$ echo 2
2
$ exit
---
abcdefghijklm$ ^M<m$ echo one   ^H^H^H
one
abcdefghijklm$ exit
---
abcdefghijklmno$ ^M<o$ echo one     ^H^H^H^H^H
one
abcdefghijklmno$ ^M<o$ echo two     ^H^H^H^H^H
two
abcdefghijklmno$ exit
---
$ echo three
three
$ exit

== stderr 0

== merged 287
$ ^M<pqrstuvwxyz
abcdefghijklmnopqrstuvwxyz
$ echo 2
2
$ exit
---
abcdefghijklm$ ^M<m$ echo one   ^H^H^H
one
abcdefghijklm$ exit
---
abcdefghijklmno$ ^M<o$ echo one     ^H^H^H^H^H
one
abcdefghijklmno$ ^M<o$ echo two     ^H^H^H^H^H
two
abcdefghijklmno$ exit
---
$ echo three
three
$ exit

== status 0
