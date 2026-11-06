== stdout 124
one
two
st 0
-rw-------
st 0
-rw-------
9
st 0
lrwxrwxrwx
14
h1
h2
h3
t3
-rw-------
history -w h1; echo "st $?"
st 1
st 0
h

== stderr 0

== merged 124
one
two
st 0
-rw-------
st 0
-rw-------
9
st 0
lrwxrwxrwx
14
h1
h2
h3
t3
-rw-------
history -w h1; echo "st $?"
st 1
st 0
h

== status 0
