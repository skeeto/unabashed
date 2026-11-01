== stdout 267
abc +([a-c]) yes
abcd +([a-c]) no
foo.c *.@(c|h) yes
foo.o *.@(c|h) no
foo !(*.*) yes
foo.x !(*.*) no
aaab *(a)b yes
 *(a) yes
 +(a) no
ab @(a|ab) yes
ab ?(a)?(b) yes
ne2
ABC +(a|b|c) yes
Foo.C *.@(c|h) yes
XYZ !(xyz) no
unquoted var
quoted var literal
literal match

== stderr 0

== merged 267
abc +([a-c]) yes
abcd +([a-c]) no
foo.c *.@(c|h) yes
foo.o *.@(c|h) no
foo !(*.*) yes
foo.x !(*.*) no
aaab *(a)b yes
 *(a) yes
 +(a) no
ab @(a|ab) yes
ab ?(a)?(b) yes
ne2
ABC +(a|b|c) yes
Foo.C *.@(c|h) yes
XYZ !(xyz) no
unquoted var
quoted var literal
literal match

== status 0
