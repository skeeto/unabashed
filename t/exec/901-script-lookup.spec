== stdout 173
in script scr a b
/opt/fakes/bin/cat: /opt/fakes/bin/cat: cannot execute binary file
st 126
st 127
in script scr x
st 126
x
st 126
fn
/x is a function
st 127
st 1
st 0
st 1

== stderr 304
bash: nosuchscript: No such file or directory
bash: ./ns: nosuch: bad interpreter: No such file or directory
bash: ./crs: /no/sh^M: bad interpreter: No such file or directory
bash: line 25: /x: No such file or directory
bash: line 27: export: a=b: cannot export
bash: line 30: export: c/d: cannot export

== merged 477
in script scr a b
/opt/fakes/bin/cat: /opt/fakes/bin/cat: cannot execute binary file
st 126
bash: nosuchscript: No such file or directory
st 127
in script scr x
bash: ./ns: nosuch: bad interpreter: No such file or directory
st 126
x
bash: ./crs: /no/sh^M: bad interpreter: No such file or directory
st 126
fn
/x is a function
bash: line 25: /x: No such file or directory
st 127
bash: line 27: export: a=b: cannot export
st 1
st 0
bash: line 30: export: c/d: cannot export
st 1

== status 0
